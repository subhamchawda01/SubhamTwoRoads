// =====================================================================================
//
//       Filename:  structured_general_trading.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/05/2014 08:18:18 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include "dvctrade/ExecLogic/structured_general_trading.hpp"

namespace HFSAT {
StructuredGeneralTrading::StructuredGeneralTrading(
    DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
    SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
    MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
    const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int t_runtime_id_,
    const std::vector<std::string> _this_model_source_shortcode_vec_, std::string _strategy_name_,
    CurveTradingManager* _trading_manager_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_, false),

      PriceBasedAggressiveTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_,
                                  _livetrading_, _p_strategy_param_sender_socket_, t_economic_events_manager_,
                                  t_trading_start_utc_mfm_, t_trading_end_utc_mfm_, t_runtime_id_,
                                  _this_model_source_shortcode_vec_),

      DirectionalAggressiveTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_,
                                   _livetrading_, _p_strategy_param_sender_socket_, t_economic_events_manager_,
                                   t_trading_start_utc_mfm_, t_trading_end_utc_mfm_, t_runtime_id_,
                                   _this_model_source_shortcode_vec_),

      PriceBasedVolTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                           _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                           t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_),

      PriceBasedAggressiveProRataTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_,
                                         _livetrading_, _p_strategy_param_sender_socket_, t_economic_events_manager_,
                                         t_trading_start_utc_mfm_, t_trading_end_utc_mfm_, t_runtime_id_,
                                         _this_model_source_shortcode_vec_, false),
      execlogic_to_use_(1),
      position_to_close_(0),
      getting_flat_(false),
      disallow_position_in_bid_(false),
      disallow_position_in_ask_(false),
      trading_manager_(_trading_manager_),
      trade_varset_logic_from_structured_(false)

{
  is_structured_general_strategy_ = true;
  // this gets  reset in PBVol so setting it back
  should_increase_thresholds_in_volatile_times_ = true;

  trading_manager_->AddListener(security_id_, this);
  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    param_set_vec_[i].max_global_risk_ =
        (int)(trading_manager_->MaxGlobalRisk() * param_set_vec_[i].max_global_risk_ratio_);
  }
  param_set_.max_global_risk_ =
      param_set_vec_[param_index_to_use_].max_global_risk_;  // needs to be updated as it is a seperate object

  dep_market_view_.ComputeMktPrice();
  dep_market_view_.ComputeMidPrice();

  BuildTradeVarSets();
  TradeVarSetLogic(my_risk_);
}

/**
 *
 * @param _strategy_name_
 */
void StructuredGeneralTrading::SetExecLogic(const std::string& _strategy_name_) {
  if (_strategy_name_.compare("PriceBasedAggressiveTrading") == 0) {
    execlogic_to_use_ = 0;
  } else if (_strategy_name_.compare("DirectionalAggressiveTrading") == 0) {
    execlogic_to_use_ = 1;
  } else if (_strategy_name_.compare("PriceBasedVolTrading") == 0) {
    execlogic_to_use_ = 2;
  } else if (_strategy_name_.compare("PriceBasedAggressiveProRataTrading") == 0) {
    execlogic_to_use_ = 3;
    CheckForExit();
  }

  DBGLOG_TIME_CLASS_FUNC << " ExecLogic: " << execlogic_to_use_ << DBGLOG_ENDL_FLUSH;
}

/**
 *
 * @param num_pages_to_add_
 */
void StructuredGeneralTrading::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if (trading_manager_->MaxLossReached()) {
    getflat_due_to_max_loss_ = true;
    DBGLOG_TIME_CLASS_FUNC << " getflat_due_to_max_loss: " << dep_market_view_.shortcode()
                           << " TotalVal: " << trading_manager_->total_pnl() << DBGLOG_ENDL_FLUSH;
  } else {
    getflat_due_to_max_loss_ = false;
  }

  if (!getflat_due_to_max_opentrade_loss_ && trading_manager_->MaxOpentradeLossReached()) {
    getflat_due_to_max_opentrade_loss_ = true;
    DBGLOG_TIME_CLASS_FUNC << " getflat_due_to_max_opentrade_loss: " << dep_market_view_.shortcode()
                           << " TotalVal: " << trading_manager_->exposed_pnl() << DBGLOG_ENDL_FLUSH;
  } else if (getflat_due_to_max_opentrade_loss_ && !trading_manager_->MaxOpentradeLossReached()) {
    getflat_due_to_max_opentrade_loss_ = false;
  }

  BaseTrading::OnTimePeriodUpdate(num_pages_to_add_);

  if (should_be_getting_flat_) {
    position_to_close_ = trading_manager_->ComputeGetFlatPositions(dep_market_view_.security_id(), false);
    getting_flat_ = true;
  } else {
    if (getting_flat_) {
      getting_flat_ = false;
      DBGLOG_TIME_CLASS_FUNC_LINE << " resume_trading " << dep_market_view_.shortcode() << " "
                                  << trading_manager_->CombinedRisk() << DBGLOG_ENDL_FLUSH;
    }
  }
}

void StructuredGeneralTrading::OnMarketUpdate(const unsigned int _security_id_,
                                              const MarketUpdateInfo& _market_update_info_) {
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << " secid: " << _security_id_ << DBGLOG_ENDL_FLUSH;
  }

  trading_manager_->ResetCancelFlag();
  BaseTrading::OnMarketUpdate(_security_id_, _market_update_info_);
}
/**
 * We want query to not take any position in the given direction. Following are the steps taken for that.
 * a) Cancel all orders in that side
 * b) Disable further placing orders in that direction
 *
 * @param buysell
 */
void StructuredGeneralTrading::DisAllowOneSideTrade(TradeType_t buysell) {
  if (buysell == kTradeTypeBuy) {
    order_manager_.CancelAllBidOrders();
    disallow_position_in_bid_ = true;
  } else if (buysell == kTradeTypeSell) {
    order_manager_.CancelAllAskOrders();
    disallow_position_in_ask_ = true;
  } else if (buysell == kTradeTypeNoInfo) {
    order_manager_.CancelAllBidOrders();
    order_manager_.CancelAllAskOrders();
    disallow_position_in_bid_ = true;
    disallow_position_in_ask_ = true;
  }
  TradeVarSetLogic(my_risk_);
}

/**
 * Allow trades for given side. ( This doesn't restrict trades for other side)
 * if kTradeTypeNoInfo is provided then it allows trades for both sides
 * @param buysell
 */
void StructuredGeneralTrading::AllowTradesForSide(TradeType_t buysell) {
  bool changed = false;
  if (buysell == kTradeTypeBuy && disallow_position_in_bid_) {
    disallow_position_in_bid_ = false;
    changed = true;
  } else if (buysell == kTradeTypeSell && disallow_position_in_ask_) {
    disallow_position_in_ask_ = false;
    changed = true;

  } else if (buysell == kTradeTypeNoInfo) {
    if (disallow_position_in_bid_) {
      disallow_position_in_bid_ = false;
      changed = true;
    }
    if (disallow_position_in_ask_) {
      disallow_position_in_ask_ = false;
      changed = true;
    }
  }
  if (changed) TradeVarSetLogic(my_risk_);
}

/**
 * Overriding it here to handle allow/disallow trades
 * @param t_position
 */
void StructuredGeneralTrading::TradeVarSetLogic(int t_position) {
  if (trade_varset_logic_from_structured_) {
    // calls base_trading TradeVarSet without calling the trading logic
    current_position_tradevarset_map_index_ = P2TV_zero_idx_;
    // only when param_set_.max_position_ > MAX_POS_MAP_SIZE
    if (map_pos_increment_ > 1) {
      if (t_position > 0) {
        current_position_tradevarset_map_index_ += std::min(MAX_POS_MAP_SIZE, (abs(t_position) / map_pos_increment_));
        current_tradevarset_ = position_tradevarset_map_[current_position_tradevarset_map_index_];
        if ((current_tradevarset_.l1bid_trade_size_ + t_position) > param_set_.max_position_) {
          current_tradevarset_.l1bid_trade_size_ = MathUtils::GetFlooredMultipleOf(
              std::max(0, param_set_.max_position_ - t_position), dep_market_view_.min_order_size());
        }
      } else {
        current_position_tradevarset_map_index_ -= std::min(MAX_POS_MAP_SIZE, (abs(t_position) / map_pos_increment_));
        current_tradevarset_ = position_tradevarset_map_[current_position_tradevarset_map_index_];
        if ((current_tradevarset_.l1ask_trade_size_ - t_position) > param_set_.max_position_) {
          current_tradevarset_.l1ask_trade_size_ = MathUtils::GetFlooredMultipleOf(
              std::max(0, param_set_.max_position_ + t_position), dep_market_view_.min_order_size());
        }
      }
    } else {
      if (t_position > 0) {
        current_position_tradevarset_map_index_ += std::min(MAX_POS_MAP_SIZE, abs(t_position));
      } else {
        current_position_tradevarset_map_index_ -= std::min(MAX_POS_MAP_SIZE, abs(t_position));
      }
      current_tradevarset_ = position_tradevarset_map_[current_position_tradevarset_map_index_];
    }

    if (param_set_.read_high_uts_factor_) {
      double buy_uts_factor_ = exec_logic_indicators_helper_->buy_uts_factor();
      double sell_uts_factor_ = exec_logic_indicators_helper_->sell_uts_factor();

      if (buy_uts_factor_ < FAT_FINGER_FACTOR) {
        current_tradevarset_.l1bid_trade_size_ *= buy_uts_factor_;
        current_tradevarset_.l1bid_trade_size_ =
            std::min(current_tradevarset_.l1bid_trade_size_, param_set_.max_position_ - t_position);
      }

      if (sell_uts_factor_ < FAT_FINGER_FACTOR) {
        current_tradevarset_.l1ask_trade_size_ *= sell_uts_factor_;
        current_tradevarset_.l1ask_trade_size_ =
            std::min(current_tradevarset_.l1ask_trade_size_, param_set_.max_position_ + t_position);
      }
    }

    if (param_set_.read_max_global_position_) {
      if ((my_global_position_ >= param_set_.max_global_position_) && (t_position >= 0)) {
        // All queries are together too long
        current_tradevarset_.l1bid_trade_size_ = 0;
        int canceled_bid_size_ = order_manager_.CancelBidsAtIntPrice(best_nonself_bid_int_price_);

        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " GlobalMaxPosition = " << param_set_.max_global_position_
                                 << " my_global_position_ =  " << my_global_position_
                                 << " canceled_bid_size_ = " << canceled_bid_size_ << " @ "
                                 << best_nonself_bid_int_price_ << DBGLOG_ENDL_FLUSH;
        }
      } else {
        if ((my_global_position_ <= -param_set_.max_global_position_) && (t_position <= 0)) {
          // All queries are together too short
          current_tradevarset_.l1ask_trade_size_ = 0;
          int canceled_ask_size_ = order_manager_.CancelAsksAtIntPrice(best_nonself_ask_int_price_);

          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << " GlobalMaxPosition = " << param_set_.max_global_position_
                                   << " my_global_position_ =  " << my_global_position_
                                   << " canceled_ask_size_ = " << canceled_ask_size_ << " @ "
                                   << best_nonself_ask_int_price_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }

    if (param_set_.read_max_security_position_) {
      if (current_risk_mapped_to_product_position_ >=
          param_set_.max_security_position_) {  //  too long in same underlying
        current_tradevarset_.l1bid_trade_size_ = 0;
      } else {
        if (current_risk_mapped_to_product_position_ <=
            -param_set_.max_security_position_) {  //  too short in same underlying
          current_tradevarset_.l1ask_trade_size_ = 0;
        }
      }
    }

    // This is meant to decrease MUR based on volumes
    if (param_set_.read_scale_max_pos_) {
      if ((current_tradevarset_.l1bid_trade_size_ + t_position > volume_adj_max_pos_) && (t_position >= 0)) {
        // Query is too long
        current_tradevarset_.l1bid_trade_size_ = std::max(0, volume_adj_max_pos_ - t_position);
      } else {
        if ((current_tradevarset_.l1ask_trade_size_ - t_position > volume_adj_max_pos_) && (t_position <= 0)) {
          // Query is too short
          current_tradevarset_.l1ask_trade_size_ = std::max(0, volume_adj_max_pos_ + t_position);
        }
      }
    }
    ModifyThresholdsAsPerVolatility();
    ModifyThresholdsAsPerModelStdev();
    ModifyThresholdsAsPerPreGetFlat();
    ModifyThresholdsAsPerMktConditions();

    ProcessRegimeChange();

    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " newpos: " << t_position << " gpos: " << my_global_position_ << " stddpfac "
                             << stdev_scaled_capped_in_ticks_ << " mapidx " << current_position_tradevarset_map_index_
                             << ' ' << ToString(current_tradevarset_).c_str() << DBGLOG_ENDL_FLUSH;
    }

    if (current_tradevarset_.l1bid_trade_size_ < param_set_.min_allowed_unit_trade_size_) {
      current_tradevarset_.l1bid_trade_size_ = 0;
    }

    if (current_tradevarset_.l1ask_trade_size_ < param_set_.min_allowed_unit_trade_size_) {
      current_tradevarset_.l1ask_trade_size_ = 0;
    }

    // no need to have multiple order check, because we are anyway modifying it before usage
    current_bid_tradevarset_ = current_tradevarset_;
    current_bid_keep_tradevarset_ = current_tradevarset_;
    current_ask_tradevarset_ = current_tradevarset_;
    current_ask_keep_tradevarset_ = current_tradevarset_;
    /*
      if (is_ready_ && !external_freeze_trading_ && !should_be_getting_flat_ && dep_market_view_.is_ready() &&
          (throttle_manager_->allowed_through_throttle(watch_.msecs_from_midnight()))) {
        TradingLogic();
      }
      */
  }
  if (disallow_position_in_bid_) {
    current_tradevarset_.l1bid_trade_size_ = 0;
  }

  if (disallow_position_in_ask_) {
    current_tradevarset_.l1ask_trade_size_ = 0;
  }
  if (param_set_.not_to_trade_ && param_set_.is_liquid_) {
    if (my_risk_ > 0) {
      current_tradevarset_.l1bid_trade_size_ = 0;
      current_tradevarset_.l1ask_trade_size_ = std::min(int(std::abs(my_risk_) / 5) * 5, param_set_.unit_trade_size_);
    } else {
      current_tradevarset_.l1bid_trade_size_ = std::min(int(std::abs(my_risk_) / 5) * 5, param_set_.unit_trade_size_);
      current_tradevarset_.l1ask_trade_size_ = 0;
    }
  }
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " current_tradevarset_.l1bid_trade_size " << current_tradevarset_.l1bid_trade_size_
                                << " current_tradevarset_.l1bask_trade_size " << current_tradevarset_.l1ask_trade_size_
                                << DBGLOG_ENDL_FLUSH;
  }
}

/**
 *
 * @param _strategy_name_
 * @return
 */
bool StructuredGeneralTrading::IsRegimeStrategy(std::string _strategy_name_) {
  if (_strategy_name_.find('-') != std::string::npos) {
    return true;
  }
  return false;
}

void StructuredGeneralTrading::TradingLogic() {
  // Compute the reduced mean sumvars for that pack
  auto new_sumvars = trading_manager_->RecomputeSignal(targetbias_numbers_, dep_market_view_.security_id());

  // recompute the target price with new sumvars
  target_price_ = target_price_ + (new_sumvars - targetbias_numbers_);

  // update the targetbias numbers, though it won't be useful unless we are using DAT
  targetbias_numbers_ = new_sumvars;

  /*
 target_price_ =
     dep_market_view_.mid_price() +
     (target_price_ - targetbias_numbers_ - dep_market_view_.mid_price()) * param_set_.sumvars_scaling_factor_ +
     new_sumvars;
 targetbias_numbers_ = new_sumvars;
*/

  switch (execlogic_to_use_) {
    case 0: {
      PriceBasedAggressiveTrading::TradingLogic();
    } break;
    case 1: {
      DirectionalAggressiveTrading::TradingLogic();
    } break;
    case 2: {
      PriceBasedVolTrading::TradingLogic();
    } break;
    case 3: {
      PriceBasedAggressiveProRataTrading::TradingLogic();
    } break;
    default: { break; }
  }
}

void StructuredGeneralTrading::PrintFullStatus() {
  switch (execlogic_to_use_) {
    case 0: {
      PriceBasedAggressiveTrading::PrintFullStatus();
    } break;
    case 1: {
      DirectionalAggressiveTrading::PrintFullStatus();
    } break;
    case 2: {
      PriceBasedVolTrading::PrintFullStatus();
    } break;
    case 3: {
      PriceBasedAggressiveProRataTrading::PrintFullStatus();
    } break;
    default: { break; }
  }
}

void StructuredGeneralTrading::CallPlaceCancelNonBestLevels() {
  switch (execlogic_to_use_) {
    case 2: {
      PriceBasedVolTrading::CallPlaceCancelNonBestLevels();
    } break;
    default: {
      BaseTrading::CallPlaceCancelNonBestLevels();
      break;
    }
  }
}

/**
 *
 */
void StructuredGeneralTrading::PlaceCancelNonBestLevels() {
  switch (execlogic_to_use_) {
    case 2: {
      PriceBasedVolTrading::PlaceCancelNonBestLevels();
    } break;
    case 3: {
      PriceBasedAggressiveProRataTrading::PlaceCancelNonBestLevels();
    } break;
    default: {
      BaseTrading::PlaceCancelNonBestLevels();
      break;
    }
  }
}

void StructuredGeneralTrading::OnPositionChange(int t_new_position_, int position_diff_,
                                                const unsigned int _security_id_) {
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " t_new_position_" << DBGLOG_ENDL_FLUSH;
  }
  BaseTrading::OnPositionChange(t_new_position_, position_diff_, _security_id_);
  trading_manager_->OnPositionUpdate(t_new_position_, position_diff_, _security_id_);
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " t_new_position_ 1" << DBGLOG_ENDL_FLUSH;
  }

  // if we are in getflat mode then recompute the getflat positions
  if (should_be_getting_flat_) {
    position_to_close_ = trading_manager_->ComputeGetFlatPositions(dep_market_view_.security_id(), true);
    getting_flat_ = true;
  } else {
    if (getting_flat_) {
      getting_flat_ = false;
      DBGLOG_TIME_CLASS_FUNC_LINE << " resume_trading " << dep_market_view_.shortcode() << " "
                                  << trading_manager_->CombinedRisk() << DBGLOG_ENDL_FLUSH;
    }
  }
}

/**
 *
 * @param t_new_position_
 * @param _exec_quantity_
 * @param _buysell_
 * @param _price_
 * @param r_int_price_
 * @param _security_id_
 */
void StructuredGeneralTrading::OnExec(const int t_new_position_, const int _exec_quantity_, const TradeType_t _buysell_,
                                      const double _price_, const int r_int_price_, const int _security_id_) {
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " t_new_position_" << DBGLOG_ENDL_FLUSH;
  }
  // trading_manager_->OnExec(t_new_position_, _exec_quantity_, _buysell_, _price_, r_int_price_, _security_id_);
  switch (execlogic_to_use_) {
    case 2: {
      PriceBasedVolTrading::OnExec(t_new_position_, _exec_quantity_, _buysell_, _price_, r_int_price_, _security_id_);
    } break;
    default: {
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " t_new_position_" << DBGLOG_ENDL_FLUSH;
      }
      BaseTrading::OnExec(t_new_position_, _exec_quantity_, _buysell_, _price_, r_int_price_, _security_id_);
      break;
    }
  }
}

/**
 *
 * @param _buysell_
 * @param _price_
 * @param r_int_price_
 * @param _security_id_
 */
void StructuredGeneralTrading::OnCancelReject(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                                              const int _security_id_) {
  switch (execlogic_to_use_) {
    case 2: {
      PriceBasedVolTrading::OnCancelReject(_buysell_, _price_, r_int_price_, _security_id_);
    } break;
    default: {
      BaseTrading::OnCancelReject(_buysell_, _price_, r_int_price_, _security_id_);
      break;
    }
  }
}
}
