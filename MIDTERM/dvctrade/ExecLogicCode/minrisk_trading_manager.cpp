/*
 * spread_trading_manager.cpp
 *
 *  Created on: 12-May-2014
 *      Author: archit
 */

#include "dvctrade/ExecLogic/minrisk_trading_manager.hpp"
#define FAT_FINGER_FACTOR 5

namespace HFSAT {

MinRiskTradingManager::MinRiskTradingManager(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             MultBasePNL* _mult_base_pnl_, std::string _common_paramfilename,
                                             bool _livetrading_, std::string base_stir)
    : TradingManager(_dbglogger_, _watch_),
      mult_base_pnl_(_mult_base_pnl_),
      livetrading_(_livetrading_),
      common_paramfilename_(_common_paramfilename),
      base_stir_(base_stir),
      outright_spread_map_() {
  LoadCommonParams();
  InitialiseStructure();

  mult_base_pnl_->AddListener(this);
  if (base_stir_.compare("VX") != 0) {
    spread_def_ = -1;
  }
  for (auto i = 0u; i < all_instruments.size(); i++) {
    stm_listeners_.push_back(NULL);
    idx_to_pos_map_.push_back(0);
    min_risk_pos.push_back(0);
    beta_pos.push_back(0);
    lrdb_vec_.push_back(OfflineReturnsLRDB::GetUniqueInstance(_dbglogger_, _watch_, all_instruments[i]));
  }
  for (int i = 0; i < num_outrights_; i++) {
    outright_pos.push_back(0);
  }
  InitialiseBetaValues();
  secid_to_idx_map_.resize(HFSAT::SecurityNameIndexer::GetUniqueInstance().NumSecurityId(), -1);
  for (auto i = 0u; i < HFSAT::SecurityNameIndexer::GetUniqueInstance().NumSecurityId(); i++) {
    int sec_idx = GetSecurityIndex(HFSAT::SecurityNameIndexer::GetUniqueInstance().GetShortcodeFromId(i));
    if (sec_idx >= 0) {
      secid_to_idx_map_[i] = sec_idx;
    }
  }
}

void MinRiskTradingManager::InitialiseStructure() {
  std::ifstream shortcode_list_file;
  std::ostringstream filepath;
  filepath << structure_file_;
  shortcode_list_file.open(filepath.str(), std::ifstream::in);
  if (shortcode_list_file.is_open()) {
    const int kL1AvgBufferLen = 1024;
    char readline_buffer_[kL1AvgBufferLen];
    bzero(readline_buffer_, kL1AvgBufferLen);
    shortcode_list_file.getline(readline_buffer_, kL1AvgBufferLen);
    while (shortcode_list_file.good()) {
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      std::string t_shc_ = tokens_[0];
      all_instruments.push_back(t_shc_);
      shortcode_secidx_map_[t_shc_] = all_instruments.size() - 1;
      bzero(readline_buffer_, kL1AvgBufferLen);
      shortcode_list_file.getline(readline_buffer_, kL1AvgBufferLen);
    }
    for (auto i = 0u; i < all_instruments.size(); i++) {
      std::string t_shc_ = all_instruments[i];
      if (t_shc_.substr(0, 3) == "SP_") {
        std::size_t t_occurence_ = t_shc_.find("_", 3);
        std::string t_security1_ =
            base_stir_ + "_" + t_shc_.substr(3 + base_stir_.size(), t_occurence_ - 3 - base_stir_.size());
        std::string t_security2_ =
            base_stir_ + "_" +
            t_shc_.substr(t_occurence_ + 1 + base_stir_.size(), t_shc_.size() - t_occurence_ - base_stir_.size() - 1);
        if (atoi(t_shc_.substr(3 + base_stir_.size(), t_occurence_ - 3 - base_stir_.size()).c_str()) >=
            atoi(t_shc_.substr(t_occurence_ + 1 + base_stir_.size(),
                               t_shc_.size() - t_occurence_ - base_stir_.size() - 1)
                     .c_str())) {
          ExitVerbose(kStirExitError, "incorrect shortcode in config file");
        }
        spread_outright_map_.push_back({shortcode_secidx_map_[t_security1_], shortcode_secidx_map_[t_security2_]});
        outright_spread_map_[shortcode_secidx_map_[t_security1_] * 100 + shortcode_secidx_map_[t_security2_]] = i;
      } else {
        num_outrights_++;
        spread_outright_map_.push_back({});
      }
    }
  }
}
void MinRiskTradingManager::UpdateOutrightRisk(const unsigned int sec_idx, int pos_change) {
  double new_outright_risk = outright_risk + pos_change * (stdevratio[sec_idx]);
  UpdateOpenUnrealizedPNL(new_outright_risk, outright_risk);
  outright_risk = new_outright_risk;
  mult_base_pnl_->UpdateTotalRisk(outright_risk);
}

inline void MinRiskTradingManager::GetFlat() { getting_flat_ = true; }

inline void MinRiskTradingManager::ResumeAfterGetFlat() { getting_flat_ = false; }

inline double MinRiskTradingManager::total_pnl() { return total_pnl_; }

inline void MinRiskTradingManager::OnControlUpdate(const ControlMessage& _control_message_, const char* symbol,
                                                   const int trader_id) {
  switch (_control_message_.message_code_) {
    case kControlMessageCodeGetflat: {
      if (!getting_flat_) {
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME << "getflat_due_to_external_getflat_ " << trader_id << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
        GetFlat();
        for (auto i = 0u; i < stm_listeners_.size(); i++) {
          if (stm_listeners_[i] != NULL) {
            stm_listeners_[i]->OnControlUpdate(_control_message_, symbol, trader_id);
          }
        }
      }
    } break;
    case kControlMessageCodeStartTrading: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "StartTrading Called " << trader_id << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
      ResumeAfterGetFlat();

      for (auto i = 0u; i < stm_listeners_.size(); i++) {
        if (stm_listeners_[i] != NULL) {
          stm_listeners_[i]->OnControlUpdate(_control_message_, symbol, trader_id);
        }
      }

    } break;

    case kControlMessageCodeDumpPositions: {
      DBGLOG_TIME_CLASS << "DumpPositions" << DBGLOG_ENDL_FLUSH;
      if (livetrading_) {
        DBGLOG_DUMP;
      }
      DumpPositions();
    } break;

    case kControlMessageCodeFreezeTrading:
    case kControlMessageCodeUnFreezeTrading:
    case kControlMessageCodeCancelAllFreezeTrading:
    case kControlMessageCodeSetTradeSizes:
    case kControlMessageCodeSetUnitTradeSize:
    case kControlMessageCodeSetMaxUnitRatio:
    case kControlMessageCodeSetMaxPosition:
    case kControlMessageCodeSetWorstCaseUnitRatio:
    case kControlMessageCodeAddPosition:
    case kControlMessageCodeSetWorstCasePosition:
    case kControlMessageCodeDisableImprove:
    case kControlMessageCodeEnableImprove:
    case kControlMessageCodeDisableAggressive:
    case kControlMessageCodeEnableAggressive:
    case kControlMessageCodeShowParams:
    case kControlMessageCodeCleanSumSizeMaps:
    case kControlMessageCodeSetEcoSeverity:
    case kControlMessageCodeForceIndicatorReady:
    case kControlMessageCodeForceAllIndicatorReady:
    case kControlMessageDisableSelfOrderCheck:
    case kControlMessageEnableSelfOrderCheck:
    case kControlMessageDumpNonSelfSMV:
    case kControlMessageCodeEnableAggCooloff:
    case kControlMessageCodeDisableAggCooloff:
    case kControlMessageCodeEnableNonStandardCheck:
    case kControlMessageCodeDisableNonStandardCheck:
    case kControlMessageCodeSetMaxIntSpreadToPlace:
    case kControlMessageCodeSetMaxIntLevelDiffToPlace:
    case kControlMessageCodeSetExplicitMaxLongPosition:
    case kControlMessageCodeSetExplicitWorstLongPosition: {
      for (auto i = 0u; i < stm_listeners_.size(); i++) {
        if (strcmp(_control_message_.strval_1_, all_instruments[i].c_str()) == 0) {
          DBGLOG_TIME_CLASS << "User Message for shortcode " << all_instruments[i] << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
          if (stm_listeners_[i] != NULL) stm_listeners_[i]->OnControlUpdate(_control_message_, symbol, trader_id);
        }
      }
    } break;
    case kControlMessageCodeShowIndicators:
    case kControlMessageDisableMarketManager:
    case kControlMessageEnableMarketManager:
    case kControlMessageCodeEnableLogging:
    case kControlMessageCodeDisableLogging:
    case kControlMessageCodeShowOrders:
    case kControlMessageCodeEnableZeroLoggingMode:
    case kControlMessageCodeDisableZeroLoggingMode:
    case kControlMessageCodeSetStartTime:
    case kControlMessageCodeSetEndTime: {
      for (auto i = 0u; i < stm_listeners_.size(); i++) {
        if (stm_listeners_[i] != NULL) stm_listeners_[i]->OnControlUpdate(_control_message_, symbol, trader_id);
      }
    } break;

    case kControlMessageCodeSetMaxGlobalRisk: {
      for (auto i = 0u; stm_listeners_.size(); i++) {
        if (stm_listeners_[i] != NULL) stm_listeners_[i]->OnControlUpdate(_control_message_, symbol, trader_id);
      }
    }

    case kControlMessageCodeSetMaxLoss: {
      if (_control_message_.intval_1_ > max_loss_ / FAT_FINGER_FACTOR &&
          _control_message_.intval_1_ < max_loss_ * FAT_FINGER_FACTOR) {
        max_loss_ = _control_message_.intval_1_;
      }

      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "SetMaxLoss " << trader_id << " called for abs_max_loss_ = " << _control_message_.intval_1_
                          << " and MaxLoss set to " << max_loss_ << DBGLOG_ENDL_FLUSH;

        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
    } break;

    case kControlMessageCodeSetOpenTradeLoss: {
      if (_control_message_.intval_1_ > max_opentrade_loss_ / FAT_FINGER_FACTOR &&
          _control_message_.intval_1_ < max_opentrade_loss_ * FAT_FINGER_FACTOR) {
        max_opentrade_loss_ = _control_message_.intval_1_;
      }

      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "SetOpenTradeLoss " << trader_id
                          << "UpdatePNL called for abs_opentrade_loss_ = " << _control_message_.intval_1_
                          << " and OpenTradeLoss set to " << max_opentrade_loss_ << DBGLOG_ENDL_FLUSH;

        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
    } break;

    case kControlMessageCodeSetBreakMsecsOpenTradeLoss: {
      if (_control_message_.intval_1_ > 0) {
        break_msecs_on_max_opentrade_loss_ = _control_message_.intval_1_;
        break_msecs_on_max_opentrade_loss_ = std::max(1 * 60 * 1000, break_msecs_on_max_opentrade_loss_);
      }
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "SetBreakMsecsOpenTradeLoss " << trader_id
                          << " called for break_msecs_on_max_opentrade_loss_ = " << _control_message_.intval_1_
                          << " and BreakMsecsOpenTradeLoss set to " << break_msecs_on_max_opentrade_loss_
                          << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
      }
    } break;

    default:
      break;
  }
}

inline void MinRiskTradingManager::UpdatePNL(int _total_pnl_) {
  total_pnl_ = _total_pnl_;
  open_unrealized_pnl_ = total_pnl_ - realized_pnl_;
}

inline void MinRiskTradingManager::LoadCommonParams() {
  std::ifstream paramfile_;
  paramfile_.open(common_paramfilename_.c_str(), std::ifstream::in);
  if (paramfile_.is_open()) {
    const int kParamFileLineBufferLen = 1024;
    char readline_buffer_[kParamFileLineBufferLen];
    bzero(readline_buffer_, kParamFileLineBufferLen);

    while (paramfile_.good()) {
      bzero(readline_buffer_, kParamFileLineBufferLen);
      paramfile_.getline(readline_buffer_, kParamFileLineBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kParamFileLineBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() < 1) continue;

      // look at the second token and depending on string fill in the appropriate variable from the third token
      // example :
      // PARAMVALUE WORST_CASE_POSITION 60 # comments ...
      // PARAMVALUE MAX_POSITION 30 # comments ...
      // PARAMVALUE UNIT_TRADE_SIZE 5 # comments ...
      if ((strcmp(tokens_[0], "PARAMVALUE") == 0) && (tokens_.size() >= 3)) {
        if (strcmp(tokens_[1], "MAX_OPENTRADE_LOSS") == 0) {
          max_opentrade_loss_ = atoi(tokens_[2]);
          read_max_opentrade_loss_ = true;
        } else if (strcmp(tokens_[1], "MAX_LOSS") == 0) {
          max_loss_ = atoi(tokens_[2]);
          read_max_loss_ = true;
        } else if (strcmp(tokens_[1], "BREAK_MSECS_ON_OPENTRADE_LOSS") == 0) {
          break_msecs_on_max_opentrade_loss_ = (int)(std::max(60 * 1000.0, atof(tokens_[2])));
          read_break_msecs_on_max_opentrade_loss_ = true;
        } else if (strcmp(tokens_[1], "STRUCTURE_FILE") == 0) {
          structure_file_ = tokens_[2];
          has_read_structure_file_ = true;
        } else if (strcmp(tokens_[1], "FORWARD_VISIBILITY") == 0) {
          forward_visibility_ = (atoi(tokens_[2]) != 0);
        }
      }
    }
    paramfile_.close();
  } else {
    ExitVerbose(kStirExitError, "could not open common paramfile");
  }

  if (!read_max_loss_ || !read_max_opentrade_loss_ || !read_break_msecs_on_max_opentrade_loss_ ||
      !has_read_structure_file_) {
    ExitVerbose(kStirExitError, "common paramfile incomplete");
  }
}

inline void MinRiskTradingManager::OnTradePrint(const unsigned int _security_id_,
                                                const TradePrintInfo& _trade_print_info_,
                                                const MarketUpdateInfo& cr_market_update_info_) {}

inline void MinRiskTradingManager::UpdateOpenUnrealizedPNL(double last_pc1_risk_, double current_pc1_risk_) {
  if (current_pc1_risk_ * last_pc1_risk_ < 0) {
    realized_pnl_ += open_unrealized_pnl_;
    open_unrealized_pnl_ = 0.0;
  }
}

bool MinRiskTradingManager::IsHittingMaxLoss() {
  if (total_pnl_ < -max_loss_) {
    return true;
  } else {
    return false;
  }
}

bool MinRiskTradingManager::IsHittingOpentradeLoss() {
  if (open_unrealized_pnl_ < -max_opentrade_loss_) {
    last_max_opentrade_loss_hit_msecs_ = watch_.msecs_from_midnight();
    return true;
  } else if (watch_.msecs_from_midnight() - last_max_opentrade_loss_hit_msecs_ < break_msecs_on_max_opentrade_loss_) {
    return true;
  } else {
    return false;
  }
}

void MinRiskTradingManager::InitialiseBetaValues() {
  for (auto i = 0u; i < all_instruments.size(); i++) {
    for (unsigned int j = 0; j < all_instruments.size(); j++) {
      if (i != j) {
        offilnebeta[i][j] = (lrdb_vec_[i].GetLRCoeff(all_instruments[i], all_instruments[j])).lr_coeff_;
      } else {
        offilnebeta[i][j] = 1;
      }
    }
  }
  for (auto i = 0u; i < all_instruments.size(); i++) {
    stdevratio.push_back(sqrt(offilnebeta[i][0] / offilnebeta[0][i]));
  }
}

void MinRiskTradingManager::UpdateBetaPosition(const unsigned int sec_id, int _new_position_) {
  std::vector<int> outright_idx = GetOutrightsFromSpread(secid_to_idx_map_[sec_id]);
  if (outright_idx.size() > 1) {
    int pos_old[2] = {outright_pos[outright_idx[0]], outright_pos[outright_idx[1]]};
    UpdateOutrightPosition(secid_to_idx_map_[sec_id], _new_position_);
    AttributePositionsInSpreads();
    for (int j = 0; j < 2; j++) {
      int pos_change = outright_pos[outright_idx[j]] - pos_old[j];
      for (int i = 0; i < num_outrights_; i++) {
        if (stm_listeners_[i] != NULL) {
          if ((forward_visibility_ && (outright_idx[j] <= i)) || (!forward_visibility_ && (outright_idx[j] >= i))) {
            beta_pos[i] = offilnebeta[outright_idx[j]][i] * (pos_change) + beta_pos[i];
            //		stm_listeners_[i]->OnPositionUpdate(  1.0 * beta_pos[i] + 0.0 * idx_to_pos_map_[i] );
          }
        }
      }
    }
    for (int i = 0; i < num_outrights_; i++) {
      if (stm_listeners_[i] != NULL) {
        if ((forward_visibility_ && (outright_idx[0] <= i || outright_idx[1] <= i)) ||
            (!forward_visibility_ && (outright_idx[0] >= i || outright_idx[1] >= i))) {
          stm_listeners_[i]->OnPositionUpdate(1.0 * beta_pos[i] + 0.0 * idx_to_pos_map_[i], outright_pos[i]);
        }
      }
    }
  } else {
    int outright_idx = secid_to_idx_map_[sec_id];
    int pos_old = outright_pos[outright_idx];
    UpdateOutrightPosition(secid_to_idx_map_[sec_id], _new_position_);
    AttributePositionsInSpreads();
    int pos_change = outright_pos[outright_idx] - pos_old;
    for (int i = 0; i < num_outrights_; i++) {
      if (stm_listeners_[i] != NULL) {
        if ((forward_visibility_ && outright_idx <= i) || (!forward_visibility_ && outright_idx >= i)) {
          beta_pos[i] = offilnebeta[outright_idx][i] * (pos_change) + beta_pos[i];
          stm_listeners_[i]->OnPositionUpdate(1.0 * beta_pos[i] + 0.0 * idx_to_pos_map_[i], outright_pos[i]);
        }
      }
    }
  }
}

bool MinRiskTradingManager::AddStmListener(MinRiskTradingManagerListener* _listener_) {
  std::string shrt_code = _listener_->GetShortCode();
  if (GetSecurityIndex(shrt_code) > -1) {
    is_trading_spreads = is_trading_spreads || ((GetOutrightsFromSpread(GetSecurityIndex(shrt_code))).size() > 0);
    stm_listeners_[GetSecurityIndex(shrt_code)] = _listener_;
    return true;
  }
  return false;
}

void MinRiskTradingManager::UpdateOutrightPosition(int sec_idx, int _new_position_) {
  std::vector<int> outrights = GetOutrightsFromSpread(sec_idx);
  if (outrights.size() > 1) {
    int pos_change = _new_position_ - idx_to_pos_map_[sec_idx];
    idx_to_pos_map_[sec_idx] = _new_position_;
    outright_pos[outrights[0]] = outright_pos[outrights[0]] - spread_def_ * pos_change;
    outright_pos[outrights[1]] = outright_pos[outrights[1]] + spread_def_ * pos_change;
    UpdateOutrightRisk(outrights[0], -1 * spread_def_ * pos_change);
    UpdateOutrightRisk(outrights[1], spread_def_ * pos_change);
  }
  if (outrights.size() == 0) {
    int pos_change = _new_position_ - idx_to_pos_map_[sec_idx];
    idx_to_pos_map_[sec_idx] = _new_position_;
    outright_pos[sec_idx] = outright_pos[sec_idx] + pos_change;
    UpdateOutrightRisk(sec_idx, pos_change);
  }
}

void MinRiskTradingManager::AttributePositionsInSpreads() {
  if (is_trading_spreads) {
    for (unsigned int i = num_outrights_; i < all_instruments.size(); i++) {
      if (stm_listeners_[i] != NULL) {
        min_risk_pos[i] = 0;
      }
      std::vector<int> outright_idx = GetOutrightsFromSpread(i);
      if (outright_idx.size() > 1) {
        int pos_n = outright_pos[outright_idx[0]];
        int pos_p = outright_pos[outright_idx[1]];
        int matched_pos = (pos_p > 0) ? std::max(0, std::min(spread_def_ * pos_p, -1 * spread_def_ * pos_n))
                                      : std::min(0, std::max(spread_def_ * pos_p, -1 * spread_def_ * pos_n));
        min_risk_pos[i] = matched_pos;
      }
    }

    for (unsigned int i = num_outrights_; i < all_instruments.size(); i++) {
      if (stm_listeners_[i] != NULL) {
        stm_listeners_[i]->OnPositionUpdate(min_risk_pos[i], min_risk_pos[i]);
      }
    }
  }
}

void MinRiskTradingManager::ReportResults(HFSAT::BulkFileWriter& trades_writer_) {
  int t_total_volume_ = 0;
  int t_supporting_orders_filled_ = 0;
  int t_bestlevel_orders_filled_ = 0;
  int t_aggressive_orders_filled_ = 0;
  int t_improve_orders_filled_ = 0;
  int t_total_pnl_ = 0;
  // t_total_pnl_ = total_pnl_;
  for (auto i = 0u; i < stm_listeners_.size(); i++) {
    if (stm_listeners_[i] != NULL) {
      t_total_pnl_ = stm_listeners_[i]->order_manager().base_pnl().mult_total_pnl();
      const SmartOrderManager& t_order_manager_ = stm_listeners_[i]->order_manager();
      // printf( "SIMRESULT-%s %.2f %d\n", stm_listeners_[i]->GetShortCode().c_str(),
      // t_order_manager_.base_pnl().total_pnl(), t_order_manager_.trade_volume());
      t_total_volume_ += t_order_manager_.trade_volume();
      t_supporting_orders_filled_ += t_order_manager_.SupportingOrderFilledPercent() * t_order_manager_.trade_volume();
      t_bestlevel_orders_filled_ += t_order_manager_.BestLevelOrderFilledPercent() * t_order_manager_.trade_volume();
      t_improve_orders_filled_ += t_order_manager_.ImproveOrderFilledPercent() * t_order_manager_.trade_volume();
      t_aggressive_orders_filled_ += t_order_manager_.AggressiveOrderFilledPercent() * t_order_manager_.trade_volume();
    }
  }

  if (t_total_volume_ > 0) {
    t_supporting_orders_filled_ /= t_total_volume_;
    t_bestlevel_orders_filled_ /= t_total_volume_;
    t_improve_orders_filled_ /= t_total_volume_;
    t_aggressive_orders_filled_ /= t_total_volume_;
  }

  printf("SIMRESULT %d %d %d %d %d %d\n", t_total_pnl_, (t_total_volume_), t_supporting_orders_filled_,
         t_bestlevel_orders_filled_, t_aggressive_orders_filled_, t_improve_orders_filled_);
}
int MinRiskTradingManager::GetSecurityIndex(std::string shrt_code) {
  if (shortcode_secidx_map_.find(shrt_code) == shortcode_secidx_map_.end()) {
    return -1;
  } else
    return shortcode_secidx_map_[shrt_code];
}

std::vector<int> MinRiskTradingManager::GetOutrightsFromSpread(int sec_idx) {
  if (spread_outright_map_.size() < (unsigned int)sec_idx + 1) {
    return {};
  } else
    return spread_outright_map_[sec_idx];
}
int MinRiskTradingManager::GetSpreadFromOutrights(int outright0, int outright1) {
  if (!(outright_spread_map_.find(outright0 * 100 + outright1) == outright_spread_map_.end())) {
    return outright_spread_map_[outright0 * 100 + outright1];
  } else if (!(outright_spread_map_.find(outright1 * 100 + outright0) == outright_spread_map_.end())) {
    return outright_spread_map_[outright1 * 100 + outright0];
  } else {
    return -1;
  }
}

} /* namespace HFSAT */
