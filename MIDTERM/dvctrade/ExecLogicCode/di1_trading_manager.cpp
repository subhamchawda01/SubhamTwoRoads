/*
  \file dvctrade/ExecLogicCode/LFI_trading_manager.cpp

  \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
  Address:
  Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
 */

#include "dvctrade/ExecLogic/di1_trading_manager.hpp"

#include <algorithm>
#include <string>

namespace HFSAT {
DI1TradingManager::DI1TradingManager(DebugLogger &_dbglogger_, const Watch &_watch_,
                                     std::vector<std::string> _structure_shortcode_vec_, const bool _livetrading_,
                                     std::string _common_paramfilename_, MultBasePNL *_mult_base_pnl_,
                                     unsigned int _trading_start_utc_mfm_, unsigned int _trading_end_utc_mfm_,
                                     std::string _base_stir_)
    : TradingManager(_dbglogger_, _watch_),
      secid_to_idx_map_(),
      sec_id_to_sumvars_(),
      stddev_(),
      risk_ratio_vec_(),
      book_bias_(),
      positions_to_close_(),
      sec_id_to_zero_crossing_(),
      last_sign_(),
      positions_(),
      common_paramfilename_(_common_paramfilename_),
      di1_listeners_(),
      pnl_samples_(),
      shortcode_vec_(_structure_shortcode_vec_),
      pnl_sampling_timestamps_(),
      sample_index_(0),
      livetrading_(_livetrading_),
      trading_start_utc_mfm_(_trading_start_utc_mfm_),
      trading_end_utc_mfm_(_trading_end_utc_mfm_),
      sec_name_indexer_(&SecurityNameIndexer::GetUniqueInstance()),
      is_liquid_shc_(),
      use_min_portfolio_risk_shc_(),
      mkt_tilt_source_shc_(),
      mkt_tilt_thresh_(),
      last_bid_di_mkt_tilt_cancel_msecs_(),
      last_ask_di_mkt_tilt_cancel_msecs_(),
      common_paramset_(nullptr),
      mult_base_pnl_(_mult_base_pnl_),
      slack_manager_(new SlackManager(BASETRADE)),
      outright_risk_(0),
      max_sum_outright_risk_(0),
      max_allowed_global_risk_(0),
      num_shortcodes_trading_(0),
      exposed_pnl_(0),
      locked_pnl_(0),
      total_pnl_(0),
      max_opentrade_loss_(-100000),
      max_loss_(-100000),
      break_msecs_on_max_opentrade_loss_(900000),
      max_global_risk_(500),
      last_max_opentrade_loss_hit_msecs_(0),
      read_max_opentrade_loss_(false),
      read_break_msecs_on_max_opentrade_loss_(false),
      read_max_loss_(false),
      read_max_global_risk_(false),
      getting_flat_due_to_opentrade_loss_(false),
      base_stir_(_base_stir_),
      mfm_(0),
      last_compute_getflat_mfm_(0),
      cancel_due_to_tilt_(false),
      outright_risk_check_(false) {
  //

  mfm_ = 0;

  auto &sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  secid_to_idx_map_.resize(sec_name_indexer.NumSecurityId(), -1);
  sec_id_to_sumvars_.resize(sec_name_indexer.NumSecurityId(), 0);
  sec_id_to_zero_crossing_.resize(sec_name_indexer.NumSecurityId(), 0);
  last_sign_.resize(sec_name_indexer.NumSecurityId(), 0);
  book_bias_.resize(sec_name_indexer.NumSecurityId(), 0);
  positions_to_close_.resize(sec_name_indexer.NumSecurityId(), 0);

  // Read the structure file for the given portfolio shortcode
  ReadStructure();

  // Currently we are directly parsing the file, ideally we would want to load through paramset
  LoadCommonParams();

  // To compute the cumulative pnls
  mult_base_pnl_->AddListener(this);
}

/**
 *
 */
void DI1TradingManager::Initialize(string shortcode_) {
  int sampling_interval_msecs_ = HFSAT::ExecLogicUtils::GetSamplingIntervalForPnlSeries(shortcode_);

  int t_sampling_start_utc_mfm_ = MathUtils::GetFlooredMultipleOf(trading_start_utc_mfm_, sampling_interval_msecs_);
  int t_sampling_end_utc_mfm_ = MathUtils::GetCeilMultipleOf(
      trading_end_utc_mfm_ + 60000, sampling_interval_msecs_);  // Adding 1 min to incorporate getflat also

  for (int sampling_mfm_ = t_sampling_start_utc_mfm_ + sampling_interval_msecs_;
       sampling_mfm_ <= t_sampling_end_utc_mfm_; sampling_mfm_ += sampling_interval_msecs_) {
    pnl_sampling_timestamps_.push_back(sampling_mfm_);
  }
}

void DI1TradingManager::ReadStructure() {
  auto &sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();

  positions_.clear();
  di1_listeners_.clear();
  stddev_.resize(shortcode_vec_.size());
  // shortcode_to_exec_shc_vec_.resize(shortcode_vec_.size()); // Exec
  risk_ratio_vec_.resize(shortcode_vec_.size());  // to store the risk projection ratio
  is_liquid_shc_.resize(shortcode_vec_.size());
  use_min_portfolio_risk_shc_.resize(shortcode_vec_.size());
  mkt_tilt_source_shc_.resize(shortcode_vec_.size());
  mkt_tilt_thresh_.resize(shortcode_vec_.size());
  is_di_mkt_tilt_buy_cancelled_.resize(shortcode_vec_.size());
  is_di_mkt_tilt_sell_cancelled_.resize(shortcode_vec_.size());
  last_bid_di_mkt_tilt_cancel_msecs_.resize(shortcode_vec_.size());
  last_ask_di_mkt_tilt_cancel_msecs_.resize(shortcode_vec_.size());
  for (auto i = 0u; i < shortcode_vec_.size(); i++) {
    is_di_mkt_tilt_buy_cancelled_[i] = false;
    is_di_mkt_tilt_sell_cancelled_[i] = false;
    Initialize(shortcode_vec_[i]);
    stddev_[i] = SampleDataUtil::GetAvgForPeriod(shortcode_vec_[i], watch_.YYYYMMDD(), 5, trading_start_utc_mfm_,
                                                 trading_end_utc_mfm_, "TrendStdev", true);
    // using last 5 day avg price to compute dv01
    // TODO: take the num_lookback_days as param
    auto t_avg_price_ = SampleDataUtil::GetAvgForPeriod(shortcode_vec_[i], watch_.YYYYMMDD(), 5, trading_start_utc_mfm_,
                                                        trading_end_utc_mfm_, "AvgPrice300", true);
    auto t_dv01_ = HFSAT::CurveUtils::dv01(shortcode_vec_[i], watch_.YYYYMMDD(), t_avg_price_);
    risk_ratio_vec_[i] = t_dv01_ * stddev_[i];

    if (stddev_[i] == 0) {
      std::stringstream st;
      st << "Exiting because the stdev of shortcode " + shortcode_vec_[i] << " is zero";

      ExitVerbose(kExitErrorCodeGeneral, st.str().c_str());
    }
    positions_.push_back(0);
    di1_listeners_.push_back(nullptr);
    secid_to_idx_map_[sec_name_indexer.GetIdFromString(shortcode_vec_[i])] = i;
  }

  // Normalizing risk ratio vec
  for (int i = shortcode_vec_.size() - 1; i >= 0; i--) {
    risk_ratio_vec_[i] /= risk_ratio_vec_[0];
  }
}

void DI1TradingManager::LoadCommonParams() {
  auto this_shortcode = base_stir_;
  if (shortcode_vec_.size() > 0) {
    this_shortcode = shortcode_vec_[0];
  }

  common_paramset_ = new ParamSet(common_paramfilename_, watch_.YYYYMMDD(), this_shortcode);

  max_opentrade_loss_ = common_paramset_->max_opentrade_loss_;
  read_max_opentrade_loss_ = common_paramset_->read_max_opentrade_loss_;
  max_loss_ = common_paramset_->max_loss_;
  read_max_loss_ = common_paramset_->read_max_loss_;
  max_global_risk_ = common_paramset_->max_global_risk_;
  read_max_global_risk_ = common_paramset_->read_max_global_risk_;
  break_msecs_on_max_opentrade_loss_ = common_paramset_->break_msecs_on_max_opentrade_loss_;
  read_break_msecs_on_max_opentrade_loss_ = common_paramset_->read_break_msecs_on_max_opentrade_loss_;
  use_combined_getflat_ = common_paramset_->combined_get_flat_model != std::string("NoModel");
  common_paramset_->min_model_scale_fact_ = std::max(std::min(common_paramset_->min_model_scale_fact_, 5.0), 0.0);

  if (!(read_max_loss_ && read_max_opentrade_loss_ && read_break_msecs_on_max_opentrade_loss_ &&
        read_max_global_risk_)) {
    ExitVerbose(kStirExitError, "Insufficient common parameters provided");
  }
}

/**
 * In getflat mode, compute how much positions to close for each expiries
 * @param t_security_id return for current security
 * @param force_compute Recompute the getflat positions irrespective of when we last computed
 *                      Used whenever we receive execution in some security
 */

int DI1TradingManager::ComputeGetFlatPositions(int t_security_id, bool force_compute) {
  if (!use_combined_getflat_) {
    // In case we are not using combined getflat, just return the original value
    return positions_[secid_to_idx_map_[t_security_id]];
  }

  // TODO: add some min risk position.
  if (std::abs(outright_risk_) < 1) {
    return 0;
  }

  if (force_compute || watch_.msecs_from_midnight() - last_compute_getflat_mfm_ > 1000) {
    double sum_bias = 0.0;
    for (auto id = 0u; id < di1_listeners_.size(); id++) {
      auto smv = di1_listeners_[id]->smv();
      double bias = 0.0;

      if ((int)outright_risk_ < 0) {
        // we are short, compute the book bias from bid side
        bias = smv->mkt_size_weighted_price() - smv->bestbid_price();
      } else if ((int)outright_risk_ > 0) {
        // we are long, compute the book bias from ask side
        bias = smv->bestask_price() - smv->mkt_size_weighted_price();
      } else {
        // Don't do anything, we are already flat (almost)
      }

      book_bias_[smv->security_id()] = bias;
      sum_bias += bias;
    }

    if (sum_bias * 1000 == 0) sum_bias = 1;

    for (auto id = 0u; id < di1_listeners_.size(); id++) {
      auto smv = di1_listeners_[id]->smv();
      auto sec_id = smv->security_id();
      // Compute what's fraction for
      double this_fraction = book_bias_[sec_id] / sum_bias;
      positions_to_close_[sec_id] = (int)(this_fraction * outright_risk_) / risk_ratio_vec_[id];
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " Shortcode: " << smv->shortcode() << " Px: " << smv->mkt_size_weighted_price()
                                    << " mp: " << smv->mid_price() << " [" << smv->bestbid_size() << " "
                                    << smv->bestbid_price() << " * " << smv->bestask_price() << " "
                                    << smv->bestask_size() << "]"
                                    << " book_bias: " << book_bias_[sec_id] << " fraction: " << this_fraction
                                    << " pos_to_close: " << positions_to_close_[sec_id] << " or: " << outright_risk_
                                    << " pos: " << positions_[id] << " risk_ratio: " << risk_ratio_vec_[id]
                                    << DBGLOG_ENDL_FLUSH;
      }
    }

    last_compute_getflat_mfm_ = watch_.msecs_from_midnight();
  }
  return positions_to_close_[t_security_id];
}

void DI1TradingManager::PrintStatus() {
  DBGLOG_TIME_CLASS_FUNC << " exposed_pnl: " << exposed_pnl_ << " locked_pnl: " << locked_pnl_
                         << " total_pnl: " << total_pnl_ << DBGLOG_ENDL_FLUSH;
  /// for each lfi listener, print the stats
  for (auto di1_listener : di1_listeners_) {
    PrintStatus(di1_listener->smv()->security_id(), false);
  }
  dbglogger_ << DBGLOG_ENDL_FLUSH;
}

void DI1TradingManager::PrintStatus(unsigned int t_security_id, bool dump_now) {
  DBGLOG_TIME_CLASS_FUNC << sec_name_indexer_->GetShortcodeFromId(t_security_id)
                         << " s_vars: " << sec_id_to_sumvars_[t_security_id] << " outright_risk: " << outright_risk_
                         << " \n";

  if (dump_now) {
    dbglogger_ << DBGLOG_ENDL_FLUSH;
  }
}

/*
void DI1TradingManager::OnExec(const int t_new_position_, const int _exec_quantity_, const TradeType_t _buysell_,
                               const double _price_, const int r_int_price_, const int _security_id_) {
  unsigned t_idx_ = secid_to_idx_map_[_security_id_];

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " DI1 " << t_new_position_ << DBGLOG_ENDL_FLUSH;
  }
  if (shortcode_to_exec_shc_vec_[t_idx_].size() != 0) {
    if (_buysell_ == kTradeTypeBuy) {
      for (auto i = 0u; i < shortcode_to_exec_shc_vec_[t_idx_].size(); i++) {
        int idx = shortcode_to_exec_shc_vec_[t_idx_][i];
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << idx << " " << t_idx_ << " dishcs " << di1_listeners_[idx]->smv()->shortcode()
                                      << " " << di1_listeners_[t_idx_]->smv()->shortcode() << DBGLOG_ENDL_FLUSH;
        }
        if ((di1_listeners_[t_idx_]->smv()->mkt_size_weighted_price() - di1_listeners_[t_idx_]->smv()->bestbid_price() <
                 common_paramset_->ors_exec_threshold_ * di1_listeners_[t_idx_]->smv()->min_price_increment() ||
             r_int_price_ < di1_listeners_[t_idx_]->smv()->bestbid_int_price()) &&
            (di1_listeners_[idx]->smv()->mkt_size_weighted_price() - di1_listeners_[idx]->smv()->bestbid_price() <
             common_paramset_->ors_exec_threshold_ * di1_listeners_[idx]->smv()->min_price_increment()) &&
            (outright_risk_ > common_paramset_->outright_frac_cancel * max_allowed_global_risk_)) {
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC_LINE << idx << " " << t_idx_ << " check for bid cancel "
                                        << di1_listeners_[idx]->smv()->shortcode() << " "
                                        << di1_listeners_[t_idx_]->smv()->shortcode() << DBGLOG_ENDL_FLUSH;
          }
          di1_listeners_[idx]->set_zero_bid_di_trade();
        }
      }
    } else if (_buysell_ == kTradeTypeSell) {
      for (auto i = 0u; i < shortcode_to_exec_shc_vec_[t_idx_].size(); i++) {
        int idx = shortcode_to_exec_shc_vec_[t_idx_][i];
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << idx << " " << t_idx_ << " dishcs " << di1_listeners_[idx]->smv()->shortcode()
                                      << " " << di1_listeners_[t_idx_]->smv()->shortcode() << DBGLOG_ENDL_FLUSH;
        }
        if ((di1_listeners_[t_idx_]->smv()->bestask_price() - di1_listeners_[t_idx_]->smv()->mkt_size_weighted_price() <
                 common_paramset_->ors_exec_threshold_ * di1_listeners_[idx]->smv()->min_price_increment() ||
             r_int_price_ > di1_listeners_[t_idx_]->smv()->bestask_int_price()) &&
            (di1_listeners_[t_idx_]->smv()->bestask_price() - di1_listeners_[idx]->smv()->mkt_size_weighted_price() <
             common_paramset_->ors_exec_threshold_ * di1_listeners_[idx]->smv()->min_price_increment()) &&
            (outright_risk_ < -1 * common_paramset_->outright_frac_cancel * max_allowed_global_risk_)) {
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC_LINE << idx << " " << t_idx_ << " check for ask cancel "
                                        << di1_listeners_[idx]->smv()->shortcode() << " "
                                        << di1_listeners_[t_idx_]->smv()->shortcode() << DBGLOG_ENDL_FLUSH;
          }
          di1_listeners_[idx]->set_zero_ask_di_trade();
        }
      }
    }
  }
}
*/
void DI1TradingManager::AddListener(unsigned _security_id_, CurveTradingManagerListener *p_listener) {
  unsigned t_idx_ = secid_to_idx_map_[_security_id_];
  di1_listeners_[t_idx_] = p_listener;

  // Compute the outright risk from here
  max_sum_outright_risk_ += (p_listener->paramset()->max_position_) * risk_ratio_vec_[t_idx_];

  // Adding liquidity flag so that
  // we don't projected overall position on
  // illiquid shortcodes

  is_liquid_shc_[t_idx_] = p_listener->paramset()->is_liquid_;

  use_min_portfolio_risk_shc_[t_idx_] = p_listener->paramset()->use_min_portfolio_risk_;
  if (p_listener->paramset()->read_cancel_on_market_tilt_source_) {
    mkt_tilt_source_shc_[t_idx_] = ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(
        p_listener->paramset()->cancel_on_market_tilt_source_);
    mkt_tilt_source_shc_[t_idx_]->ComputeMktPrice();
    mkt_tilt_thresh_[t_idx_] =
        p_listener->paramset()->cancel_on_market_tilt_thresh_ * p_listener->smv()->min_price_increment();
  }
  /*
    std::vector<int> temp_shc_indx_vec_;
    for (auto idx = 0u; idx < p_listener->paramset()->shcs_to_react_on_exec_.size(); idx++) {
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << idx << " shc_to_add " << p_listener->paramset()->shcs_to_react_on_exec_[idx]
                                    << " size " << p_listener->paramset()->shcs_to_react_on_exec_.size()
                                    << DBGLOG_ENDL_FLUSH;
      }
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " DI1X " << p_listener->paramset()->shcs_to_react_on_exec_[idx]
                                    << DBGLOG_ENDL_FLUSH;
      }
      int shc_idx = sec_name_indexer_->GetIdFromString(p_listener->paramset()->shcs_to_react_on_exec_[idx]);
      shortcode_to_exec_shc_vec_[shc_idx].push_back(t_idx_);
    }
  */
  num_shortcodes_trading_++;

  if (num_shortcodes_trading_ == (int)shortcode_vec_.size()) {
    // All shortcode listners done
    // Compute the allowed risk
    max_allowed_global_risk_ = common_paramset_->max_global_risk_ratio_ * max_sum_outright_risk_;
  }

  DBGLOG_TIME_CLASS_FUNC << " AddingListener: " << p_listener->smv()->shortcode()
                         << " MaxRisk: " << max_sum_outright_risk_ << " AllowedRisk: " << max_allowed_global_risk_
                         << DBGLOG_ENDL_FLUSH;
}

void DI1TradingManager::UpdateOutrightRisk(unsigned _security_id_, int new_position_) {
  unsigned t_idx_ = secid_to_idx_map_[_security_id_];
  double t_outright_risk_ = outright_risk_ + risk_ratio_vec_[t_idx_] * (new_position_ - positions_[t_idx_]);

  /*
  DBGLOG_TIME_CLASS_FUNC << " id: " << _security_id_
                         << " name: " << sec_name_indexer_->GetShortcodeFromId(_security_id_)
                         << " np: " << new_position_ << " or_new: " << t_outright_risk_ << " or_old: " <<
  outright_risk_
                         << " " << exposed_pnl_ << DBGLOG_ENDL_FLUSH;
                         */

  UpdateExposedPNL(outright_risk_, t_outright_risk_);
  outright_risk_ = t_outright_risk_;

  ProcessCombinedGetFlat();

  mult_base_pnl_->UpdateTotalRisk(outright_risk_);
}

/**
 * Try getting flat on one/multiple securities based on conditions
 * a) MaxPosition
 */
void DI1TradingManager::ProcessCombinedGetFlat() {
  if (std::abs(outright_risk_) > max_allowed_global_risk_) {
    // if current risk is more than allowed risk,
    // getflat on all securities which have same sign
    outright_risk_check_ = true;
    // TODO: Think of making it better
    for (auto idx = 0u; idx < di1_listeners_.size(); idx++) {
      if (positions_[idx] * outright_risk_ > 0) {
        // If this security is part of creating max_position
        di1_listeners_[idx]->set_getflat_due_to_non_standard_market_conditions(true);
        DBGLOG_TIME_CLASS_FUNC << " getflat_due_to_risk_ "
                               << " total_risk: " << outright_risk_ << " allowed: " << max_allowed_global_risk_ << " "
                               << di1_listeners_[idx]->smv()->shortcode() << " "
                               << di1_listeners_[idx]->smv()->secname() << DBGLOG_ENDL_FLUSH;
      } else {
        // Either position is zero or in opposite sign.
        // We would want to make sure that we cancel all orders which can be harmful
        if (outright_risk_ < 0) {
          di1_listeners_[idx]->DisAllowOneSideTrade(kTradeTypeSell);
        } else if (outright_risk_ > 0) {
          di1_listeners_[idx]->DisAllowOneSideTrade(kTradeTypeBuy);
        }
        // Keep flags in a way that we are not placing any further orders in that contract
      }
    }

  } else {
    // resume trading for all securities
    if (outright_risk_check_) {
      for (auto di1_listener : di1_listeners_) {
        if (di1_listener->IsDisallowed()) {
          di1_listener->AllowTradesForSide(kTradeTypeNoInfo);
        }
        if (di1_listener->getflat_due_to_non_standard_market_conditions()) {
          di1_listener->set_getflat_due_to_non_standard_market_conditions(false);
          di1_listener->Refresh();
          DBGLOG_TIME_CLASS_FUNC << " resume_normal_trading_"
                                 << " total_risk: " << outright_risk_ << " allowed: " << max_allowed_global_risk_ << " "
                                 << di1_listener->smv()->shortcode() << " " << di1_listener->smv()->secname()
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
      outright_risk_check_ = false;
    }
  }
}

void DI1TradingManager::UpdateExposedPNL(double _outright_risk_, double t_outright_risk_) {
  if (_outright_risk_ * t_outright_risk_ < 0 || int(t_outright_risk_) == 0) {
    locked_pnl_ += exposed_pnl_;
    exposed_pnl_ = 0;
  }
}

void DI1TradingManager::UpdatePNL(int _total_pnl_) {
  total_pnl_ = _total_pnl_;
  exposed_pnl_ = total_pnl_ - locked_pnl_;
}

// Removed Spread function
// void DI1TradingManager::UpdateRiskPosition() {
/*void DI1TradingManager::UpdateRiskFactor() {
  for (unsigned i = 0; i < di1_listeners_.size(); i++) {
    auto smv_ = di1_listeners_[i]->smv();
    auto dv01_vec_ = HFSAT::CurveUtils::dv01(smv_->shortcode(), watch_.YYYYMMDD(),
                                                smv_->mkt_size_weighted_price());
    risk_vec_[i] = dv01_vec_ * stddev_[i];
  }
}*/

void DI1TradingManager::UpdateRiskPosition() {
  for (unsigned i = 0; i < di1_listeners_.size(); i++) {
    auto inflated_risk = positions_[i];

    if (is_liquid_shc_[i] || common_paramset_->project_to_illiquid_) {
      // auto non_self_poj_pos_ = outright_risk_ - positions_[i] * risk_ratio_vec_[i];
      inflated_risk = positions_[i] * common_paramset_->self_pos_projection_factor_ +
                      (1 - common_paramset_->self_pos_projection_factor_) * outright_risk_ / risk_ratio_vec_[i];
      if (use_min_portfolio_risk_shc_[i]) {
        if (positions_[i] > 0 && inflated_risk > 0) {
          inflated_risk = std::min(inflated_risk, positions_[i]);
        } else if (positions_[i] < 0 && inflated_risk < 0) {
          inflated_risk = std::max(inflated_risk, positions_[i]);
        } else {
          inflated_risk = positions_[i];
        }
      }
    }

    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      PrintStatus(di1_listeners_[i]->smv()->security_id(), false);

      DBGLOG_TIME_CLASS_FUNC << " name: " << shortcode_vec_[i] << " self_pos: " << positions_[i]
                             << " overall risk: " << outright_risk_ / risk_ratio_vec_[i]
                             << " inflated_risk: " << inflated_risk << DBGLOG_ENDL_FLUSH;
    }
    // TODO: have inflated risk and min risk different
    di1_listeners_[i]->UpdateRiskPosition((int)inflated_risk, (int)inflated_risk, is_di_mkt_tilt_buy_cancelled_[i],
                                          is_di_mkt_tilt_sell_cancelled_[i]);
  }
}

void DI1TradingManager::OnPositionUpdate(int _new_position_, int _position_diff_, unsigned _security_id_) {
  if (secid_to_idx_map_[_security_id_] < 0) {
    std::stringstream st;
    st << "Unknown security with id " << _security_id_ << " pos: " << _new_position_ << "Aborting\n";
    ExitVerbose(kExitErrorCodeZeroValue, st.str().c_str());
  }

  unsigned t_idx_ = secid_to_idx_map_[_security_id_];
  UpdateOutrightRisk(_security_id_, _new_position_);

  int _actual_position_diff_ = (_new_position_ - positions_[t_idx_]);
  positions_[t_idx_] = _new_position_;

  if (_actual_position_diff_ > 0) {
    UpdateCancelFlag(kTradeTypeBuy, t_idx_);
  } else if (_actual_position_diff_ < 0) {
    UpdateCancelFlag(kTradeTypeSell, t_idx_);
  }
  UpdateRiskPosition();
}

double DI1TradingManager::RecomputeSignal(double current_sumvars, int t_security_id) { return current_sumvars; }

void DI1TradingManager::OnControlUpdate(const ControlMessage &_control_message_, const char *symbol,
                                        const int trader_id) {
  switch (_control_message_.message_code_) {
    case kControlMessageCodeGetflat: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << "getflat_due_to_external_getflat_ " << trader_id << DBGLOG_ENDL_FLUSH;
      }

      for (auto di1_listener : di1_listeners_) {
        if ((strcmp(_control_message_.strval_1_, "") == 0) ||
            strcmp(_control_message_.strval_1_, di1_listener->smv()->shortcode().c_str()) == 0) {
          di1_listener->OnControlUpdate(_control_message_, symbol, trader_id);
        }
      }

    } break;
    case kControlMessageCodeStartTrading: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "StartTrading Called " << trader_id << DBGLOG_ENDL_FLUSH;
      }

      for (auto di1_listener : di1_listeners_) di1_listener->OnControlUpdate(_control_message_, symbol, trader_id);

    } break;

    case kControlMessageCodeSetMaxGlobalRisk:
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
      for (auto i = 0u; i < di1_listeners_.size(); i++) {
        if (strcmp(_control_message_.strval_1_, shortcode_vec_[i].c_str()) == 0) {
          DBGLOG_TIME_CLASS << "User Message for shortcode " << shortcode_vec_[i] << DBGLOG_ENDL_FLUSH;
          di1_listeners_[i]->OnControlUpdate(_control_message_, symbol, trader_id);
        }
      }
    } break;

    case kControlMessageCodeShowIndicators:
    case kControlMessageCodeEnableLogging:
    case kControlMessageCodeDisableLogging:
    case kControlMessageCodeShowOrders: {
      PrintStatus();
      for (auto di1_listener : di1_listeners_) {
        di1_listener->OnControlUpdate(_control_message_, symbol, trader_id);
      }
    } break;
    case kControlMessageDisableMarketManager:
    case kControlMessageEnableMarketManager:
    case kControlMessageCodeEnableZeroLoggingMode:
    case kControlMessageCodeDisableZeroLoggingMode:
    case kControlMessageCodeSetStartTime:
    case kControlMessageCodeSetEndTime: {
      for (auto di1_listener : di1_listeners_) {
        di1_listener->OnControlUpdate(_control_message_, symbol, trader_id);
      }
    } break;

    case kControlMessageCodeSetMaxLoss: {
      if (strcmp(_control_message_.strval_1_, "") == 0) {
        if (_control_message_.intval_1_ > max_loss_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < max_loss_ * FAT_FINGER_FACTOR) {
          max_loss_ = _control_message_.intval_1_;
        }
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetMaxLoss " << trader_id
                            << " called for abs_max_loss_ = " << _control_message_.intval_1_ << " and MaxLoss set to "
                            << max_loss_ << DBGLOG_ENDL_FLUSH;
        }
      } else {
        for (auto di1_listener : di1_listeners_) {
          if (strcmp(di1_listener->smv()->shortcode().c_str(), _control_message_.strval_1_) == 0) {
            di1_listener->OnControlUpdate(_control_message_, symbol, trader_id);
          }
        }
      }
    } break;
    case kControlMessageCodeSetOpenTradeLoss: {
      if (strcmp(_control_message_.strval_1_, "") == 0) {
        if (_control_message_.intval_1_ > max_opentrade_loss_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < max_opentrade_loss_ * FAT_FINGER_FACTOR) {
          max_opentrade_loss_ = _control_message_.intval_1_;
        }

        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetOpenTradeLoss " << trader_id
                            << " called for abs_opentrade_loss_ = " << _control_message_.intval_1_
                            << " and OpenTradeLoss set to " << max_opentrade_loss_ << DBGLOG_ENDL_FLUSH;
        }
      } else {
        for (auto di1_listener : di1_listeners_) {
          if (strcmp(di1_listener->smv()->shortcode().c_str(), _control_message_.strval_1_) == 0) {
            di1_listener->OnControlUpdate(_control_message_, symbol, trader_id);
          }
        }
      }
    } break;

    case kControlMessageCodeSetBreakMsecsOpenTradeLoss: {
      if (strcmp(_control_message_.strval_1_, "") == 0) {
        if (_control_message_.intval_1_ > 0) {
          break_msecs_on_max_opentrade_loss_ = _control_message_.intval_1_;
          break_msecs_on_max_opentrade_loss_ = std::max(1 * 60 * 1000, break_msecs_on_max_opentrade_loss_);
        }
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetBreakMsecsOpenTradeLoss " << trader_id
                            << " called for break_msecs_on_max_opentrade_loss_ = " << _control_message_.intval_1_
                            << " and BreakMsecsOpenTradeLoss set to " << break_msecs_on_max_opentrade_loss_
                            << DBGLOG_ENDL_FLUSH;
        }
      } else {
        for (auto di1_listener : di1_listeners_) {
          if (strcmp(di1_listener->smv()->shortcode().c_str(), _control_message_.strval_1_) == 0) {
            di1_listener->OnControlUpdate(_control_message_, symbol, trader_id);
          }
        }
      }
    } break;

    default:
      break;
  }
}

void DI1TradingManager::OnTimePeriodUpdate(const int num_pages_to_add) {
  double current_unrealized_pnl_ = 0;
  for (unsigned int i = 0; i < di1_listeners_.size(); i++) {
    auto &t_order_manager_ = di1_listeners_[i]->order_manager();
    current_unrealized_pnl_ += t_order_manager_.base_pnl().total_pnl();
  }

  if (!livetrading_) {
    // update pnl_samples vec for SIM
    if (sample_index_ < pnl_sampling_timestamps_.size() &&
        watch_.msecs_from_midnight() >= pnl_sampling_timestamps_[sample_index_]) {
      pnl_samples_.push_back((int)current_unrealized_pnl_);
      sample_index_++;
    }
  }
}

std::string DI1TradingManager::SavePositionsAndCheck() {
  SendOutstandingPositionMail();
  std::ostringstream t_oss_;
  t_oss_ << "/spare/local/tradeinfo/StructureInfo/logs/DI1_structured_position." << watch_.YYYYMMDD();

  std::ofstream position_output_file_(t_oss_.str().c_str(), std::ofstream::out);

  DBGLOG_TIME_CLASS_FUNC_LINE << "Saving EOD positions in file " << t_oss_.str() << DBGLOG_ENDL_FLUSH;

  for (auto i = 0u; i < positions_.size(); i++) {
    position_output_file_ << shortcode_vec_[i] << " Position: " << positions_[i] << " Outright Risk: " << outright_risk_
                          << "\n";
  }
  position_output_file_.close();
  std::stringstream output_;

  std::ifstream t_avg_file_read_;

  std::string t_avg_file_name_ = "/spare/local/tradeinfo/StructureInfo/logs/DI1_structured_average";

  double average_ = 0.0;
  double number_of_days_ = 1;
  t_avg_file_read_.open(t_avg_file_name_.c_str(), std::ifstream::in);
  if (t_avg_file_read_.is_open()) {
    const int kLineBufferLen = 1024;
    char readline_buffer_[kLineBufferLen];
    bzero(readline_buffer_, kLineBufferLen);
    while (t_avg_file_read_.good()) {
      bzero(readline_buffer_, kLineBufferLen);
      t_avg_file_read_.getline(readline_buffer_, kLineBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kLineBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() >= 2) {
        average_ = atof(tokens_[0]);
        number_of_days_ = atof(tokens_[1]);
      }
    }
  }
  t_avg_file_read_.close();

  average_ = (total_pnl_ + number_of_days_ * average_) / (number_of_days_ + 1);
  number_of_days_++;
  output_ << "Average PNL so far : " << average_ << "\n";
  std::ofstream t_avg_file_write_(t_avg_file_name_.c_str());
  t_avg_file_write_ << average_ << " " << number_of_days_ << "\n";
  t_avg_file_write_.close();

  return output_.str();
}

void DI1TradingManager::SendOutstandingPositionMail() {
  if (positions_.size() <= 0 || di1_listeners_.size() <= 0) return;

  std::stringstream st;
  bool positions_exist = false;
  st << " Strategy Exiting with positions: ";
  for (auto id = 0u; id < positions_.size(); id++) {
    if (positions_[id] != 0) {
      positions_exist = true;
      st << di1_listeners_[id]->smv()->shortcode() << " " << di1_listeners_[id]->smv()->secname()
         << " Pos: " << positions_[id];
    }
  }

  st << "\nCombinedRisk: " << outright_risk_ << " Equivalent in " << di1_listeners_[0]->smv()->shortcode() << ": "
     << outright_risk_ / stddev_[0] << " \n";

  if (positions_exist) {
    slack_manager_->sendNotification(st.str());
  }
}

void DI1TradingManager::ReportResults(HFSAT::BulkFileWriter &_trades_writer_) {
  int t_total_volume_ = 0;
  int t_supporting_orders_filled_ = 0;
  int t_bestlevel_orders_filled_ = 0;
  int t_aggressive_orders_filled_ = 0;
  int t_improve_orders_filled_ = 0;

  // int t_total_pnl_ = di1_listeners_[0]->order_manager().base_pnl().mult_total_pnl();
  int t_total_pnl_ = total_pnl_;

  std::stringstream st;
  int runtime_id = 0;
  st << "PNLSPLIT  ";

  if (di1_listeners_.size() > 0) runtime_id = di1_listeners_[0]->runtime_id();

  st << runtime_id << " ";
  for (auto i = 0u; i < di1_listeners_.size(); i++) {
    t_total_pnl_ = di1_listeners_[i]->order_manager().base_pnl().mult_total_pnl();
    auto &t_order_manager_ = di1_listeners_[i]->order_manager();
    t_total_volume_ += t_order_manager_.trade_volume();
    t_supporting_orders_filled_ += t_order_manager_.SupportingOrderFilledPercent() * t_order_manager_.trade_volume();
    t_bestlevel_orders_filled_ += t_order_manager_.BestLevelOrderFilledPercent() * t_order_manager_.trade_volume();
    t_improve_orders_filled_ += t_order_manager_.ImproveOrderFilledPercent() * t_order_manager_.trade_volume();
    t_aggressive_orders_filled_ += t_order_manager_.AggressiveOrderFilledPercent() * t_order_manager_.trade_volume();
    std::cerr << di1_listeners_[i]->smv()->shortcode() << " " << t_order_manager_.base_pnl().total_pnl() << " "
              << t_order_manager_.trade_volume() << std::endl;

    st << di1_listeners_[i]->smv()->shortcode() << " " << t_order_manager_.base_pnl().total_pnl() << " "
       << t_order_manager_.trade_volume() << " " << (int)t_order_manager_.SupportingOrderFilledPercent() << " "
       << (int)t_order_manager_.BestLevelOrderFilledPercent() << " " << t_order_manager_.ImproveOrderFilledPercent()
       << " " << t_order_manager_.AggressiveOrderFilledPercent() << " ";
  }

  st << "\n";
  _trades_writer_ << st.str();

  st.clear();
  st.str(std::string());

  int num_messages_ = 0;
  for (unsigned i = 0; i < di1_listeners_.size(); i++) {
    auto &t_order_manager_ = di1_listeners_[i]->order_manager();
    num_messages_ +=
        (t_order_manager_.SendOrderCount() + t_order_manager_.CxlOrderCount() + t_order_manager_.ModifyOrderCount());

    st << "EOD_MSG_COUNT: " << di1_listeners_[i]->runtime_id() << " " << di1_listeners_[i]->smv()->secname() << " "
       << (t_order_manager_.SendOrderCount() + t_order_manager_.CxlOrderCount() + t_order_manager_.ModifyOrderCount())
       << "\n";
  }

  st << "EOD_MSG_COUNT: " << runtime_id << " TOTAL " << num_messages_ << "\n";
  st << "SIMRESULT " << runtime_id << " " << total_pnl_ << " " << t_total_volume_ << " " << t_supporting_orders_filled_
     << " " << t_bestlevel_orders_filled_ << " " << t_aggressive_orders_filled_ << " " << t_improve_orders_filled_
     << "\n";
  st << "PNLSAMPLES " << runtime_id << " ";
  if (pnl_samples_.size() > 0) {
    for (auto i = 0u; i < pnl_samples_.size(); i++) {
      st << pnl_sampling_timestamps_[i] << " " << pnl_samples_[i] << " ";
    }
    st << "\n";
  } else {
    st << trading_end_utc_mfm_ << " " << total_pnl_ << "\n";
  }

  std::vector<int> t_positions_;
  st << "PNL and Position at close ";
  int total_pnl_at_close_ = 0;
  int ourtight_at_close_ = 0;
  // TODO:pnl computation logic
  for (auto i = 0u; i < di1_listeners_.size(); i++) {
    const SmartOrderManager &t_order_manager_ = di1_listeners_[i]->order_manager();

    st << " "
       << HFSAT::SecurityNameIndexer::GetUniqueInstance().GetShortcodeFromId(di1_listeners_[i]->smv()->security_id())
       << " " << runtime_id << " " << di1_listeners_[i]->PnlAtClose() << " " << di1_listeners_[i]->PositionAtClose();
    total_pnl_at_close_ += di1_listeners_[i]->PnlAtClose();
    ourtight_at_close_ += risk_ratio_vec_[i] * di1_listeners_[i]->PositionAtClose();
    DBGLOG_TIME_CLASS_FUNC_LINE << "Number of trades in "
                                << HFSAT::SecurityNameIndexer::GetUniqueInstance().GetShortcodeFromId(
                                       di1_listeners_[i]->smv()->security_id()) << " : "
                                << t_order_manager_.trade_volume() << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME_CLASS_FUNC_LINE << "Closing position of "
                                << HFSAT::SecurityNameIndexer::GetUniqueInstance().GetShortcodeFromId(
                                       di1_listeners_[i]->smv()->security_id()) << " : " << positions_[i]
                                << DBGLOG_ENDL_FLUSH;
  }
  st << " TotalPnl " << total_pnl_at_close_ << " Outright " << ourtight_at_close_ << " finalPnl " << t_total_pnl_
     << "\n";
  _trades_writer_ << st.str();
  if (t_total_volume_ > 0) {
    t_supporting_orders_filled_ /= t_total_volume_;
    t_bestlevel_orders_filled_ /= t_total_volume_;
    t_improve_orders_filled_ /= t_total_volume_;
    t_aggressive_orders_filled_ /= t_total_volume_;
  }

  printf("SIMRESULT %d %d %d %d %d %d\n", t_total_pnl_, (t_total_volume_), t_supporting_orders_filled_,
         t_bestlevel_orders_filled_, t_aggressive_orders_filled_, t_improve_orders_filled_);
}
}
