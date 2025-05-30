// =====================================================================================
//
//       Filename:  indicator_helper.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  12/21/2015 04:34:27 PM
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

#include "dvctrade/ExecLogic/ExecLogicHelper/indicator_helper.hpp"

namespace HFSAT {
IndicatorHelper::IndicatorHelper(DebugLogger& t_dbglogger, const Watch& r_watch, InstrumentInfo* product,
                                 SecurityMarketView* smv, SmartOrderManager* order_manager, int trading_start_mfm,
                                 int trading_end_mfm, int runtime_id, bool livetrading)
    : dbglogger_(t_dbglogger),
      watch_(r_watch),
      product_(product),
      smv_(smv),
      order_manager_(order_manager),
      listener_(nullptr),
      trading_start_utc_mfm_(trading_start_mfm),
      trading_end_utc_mfm_(trading_end_mfm),
      runtime_id_(runtime_id),
      livetrading_(livetrading) {
  ///

  indices_used_so_far_ = 0;

  computing_l1_bias_ = false;
  computing_l1_order_bias_ = false;
  computing_high_uts_factor_ = false;
  computing_longevity_ = false;
  computing_positioning_ = false;
  computing_bigtrades_ = false;
  computing_l1_trade_volume_bias_ = false;

  // l1_size/l1_order bias variables
  l1_size_indicator_ = nullptr;
  l1_order_indicator_ = nullptr;

  l1_size_indicator_ = nullptr;
  l1_order_indicator_ = nullptr;
  avg_l1_size_ = 0;
  l1_avg_size_upper_bound_ = 0;
  l1_avg_size_lower_bound_ = 0;
  l1_size_upper_bound_ = 0;
  l1_size_lower_bound_ = 0;
  l1_size_upper_percentile_ = 0;
  l1_size_lower_percentile_ = 0;
  use_l1_size_static_value_ = false;
  l1_norm_factor_ = 0;
  l1_factor_ = 0;
  l1_bias_ = 0;
  trade_bias_ = 0;
  cancel_bias_ = 0;
  factor_trade_bias_ = 0.0;
  implied_price_ = 0.0;
  factor_cancel_bias_ = 0;
  avg_l1_order_ = 0;
  l1_avg_order_upper_bound_ = 0;
  l1_avg_order_lower_bound_ = 0;
  l1_order_upper_bound_ = 0;
  l1_order_lower_bound_ = 0;
  l1_order_upper_percentile_ = 0;
  l1_order_lower_percentile_ = 0;
  use_l1_order_static_value_ = false;
  l1_order_norm_factor_ = 0;
  l1_order_factor_ = 0;
  l1_order_bias_ = 0;

  /// high UTS factor variables

  bid_size_indicator_ = nullptr;
  ask_size_indicator_ = nullptr;
  last_buy_uts_in_high_uts_ = 0;
  last_sell_uts_in_high_uts_ = 0;
  l1_size_upper_bound_high_uts_ = 0.0;
  l1_size_lower_bound_high_uts_ = 0.0;
  l1_norm_factor_high_uts_ = 0.0;
  buy_uts_factor_ = 0.0;
  sell_uts_factor_ = 0.0;
  updated_factor_this_round_ = false;

  /// longevity thresholds
  stdev_calculator_onoff_ = nullptr;
  longevity_index_ = -1;
  longevity_version_ = 0;
  longevity_thresh_ = 0;
  longevity_min_stdev_ratio_ = 0;
  offline_stdev_ = 0;
  online_stdev_ = 0;
  bestbid_queue_hysterisis_ = 0;
  bestask_queue_hysterisis_ = 0;

  /// Positioning
  long_positioning_bias_ = 0.0;
  short_positioning_bias_ = 0.0;

  /// stdev trigger for nnbl
  stdev_calculator_suppress_non_best_indc_ = NULL;
  stdev_calculator_suppress_non_best_index_ = -1;
  stdev_suppress_non_best_level_threshold_ = 0;
  stdev_nnbl_switch_ = true;

  /// BigTrades
  l1_trade_volume_bias_indicator_ = nullptr;
  moving_average_buy_indc_ = nullptr;
  moving_average_sell_indc_ = nullptr;
  moving_average_buy_place_indc_ = nullptr;
  moving_average_sell_place_indc_ = nullptr;
  moving_average_buy_aggress_indc_ = nullptr;
  moving_average_sell_aggress_indc_ = nullptr;
  l1_trade_volume_bias_index_ = -1;
  moving_avg_buy_index_ = -1;
  moving_avg_sell_index_ = -1;
  moving_avg_buy_place_index_ = -1;
  moving_avg_sell_place_index_ = -1;
  moving_avg_buy_aggress_index_ = -1;
  moving_avg_sell_aggress_index_ = -1;

  l1_bid_ask_size_flow_cancel_indc_ = nullptr;
  l1_bid_ask_size_flow_cancel_index_ = -1;
  l1_bid_ask_size_flow_cancel_ = 0;
  l1_bid_ask_size_flow_cancel_thresh_ = 0;
  l1_bid_ask_size_flow_cancel_keep_thresh_diff_ = 0;
  l1_bid_ask_size_flow_keep_thresh = 0;
  l1_bias_cancel_thresh_ = 0;
  min_size_to_flow_based_cancel_ = 0;
  l1_bid_ask_size_flow_cancel_buy_ = false;
  l1_bid_ask_size_flow_cancel_sell_ = false;

  last_bigtrades_ask_cancel_msecs_ = 0;
  last_bigtrades_bid_cancel_msecs_ = 0;
  last_bigtrades_ask_place_msecs_ = 0;
  last_bigtrades_bid_place_msecs_ = 0;
  last_bigtrades_ask_aggress_msecs_ = 0;
  last_bigtrades_bid_aggress_msecs_ = 0;

  l1_trade_volume_bias_ = 0;
  hist_avg_l1_size_ = 0;
  l1_trade_volume_bias_threshold_ = 0.2;
  moving_avg_buy_ = 0;
  moving_avg_sell_ = 0;
  moving_avg_buy_place_ = 0;
  moving_avg_sell_place_ = 0;
  moving_avg_buy_aggress_ = 0;
  moving_avg_sell_aggress_ = 0;
  bigtrades_cancel_threshold_ = 100;
  bigtrades_place_threshold_ = 100;
  bigtrades_aggress_threshold_ = 100;
  bigtrades_place_cooloff_interval_ = 0;
  bigtrades_aggress_cooloff_interval_ = 0;

  /// Regime
  regime_indicator_ = nullptr;
  regime_indicator_index_ = -1;
  regime_index_ = -1;
  called_on_regime_update_ = false;

  /// GetFlat Regime
  regime_indicator_vec_.clear();
  trading_vec_.clear();
  regime_indicator_string_vec_.clear();
  regimes_to_trade_.clear();
  getflat_regime_indicator_start_index_ = -1;
  getflat_regime_indicator_end_index_ = -1;
  getflat_due_to_regime_indicator_ = false;

  trade_bias_indicator_start_index_ = -1;
  trade_bias_indicator_end_index_ = -1;

  cancel_bias_indicator_start_index_ = -1;
  cancel_bias_indicator_end_index_ = -1;

  implied_mkt_indicator_start_index_ = -1;
  implied_mkt_indicator_end_index_ = -1;
  /// Moving Average Spread
  moving_average_spread_index_ = -1;  //  indicator index start from 0 so set it to -1 for disabling
  moving_average_spread_value_ = 1;   // Min possible spread is 1
  moving_average_spread_ = nullptr;

  /// Moving stdev
  moving_stdev_index_ = -1;
  moving_stdev_val_ = smv_->min_price_increment() * 0.01;
  use_sqrt_stdev_ = false;
  stdev_fact_ticks_ = 0;
  stdev_cap_ = 1;
  slow_stdev_calculator_ = nullptr;

  // Trade and Cancel Bias

  trade_bias_indicator_string_ = "";
  cancel_bias_indicator_string_ = "";

  // Source based exec change
  src_trend_indicator_ = nullptr;
  src_trend_index_ = -1;
  src_stdev_indicator_ = nullptr;
  src_stdev_index_ = -1;
  src_trend_thresh_ = 1000;
  src_trend_keep_thresh_ = 1000;
  src_stdev_thresh_ = 1000;
  src_trend_val_ = 0;
  src_stdev_val_ = 0;
  avoid_short_market_ = false;
  avoid_short_market_aggressively_ = false;
  avoid_long_market_ = false;
  avoid_long_market_aggressively_ = false;
  sd_l1bias_ = nullptr;
  offline_l1bias_stdev_ = 0;
  l1bias_weight_cap_ = 1;
  // Please be careful while changing the granularity of this
  watch_.subscribe_first_FifteenMinutesPeriod(this);
}

IndicatorHelper::~IndicatorHelper() {}

void IndicatorHelper::NotifyIndicatorHelperListener() {
  if (listener_) {
    listener_->ProcessIndicatorHelperUpdate();
  }
}

void IndicatorHelper::ProcessOnTimePeriodUpdate(int num_pages_to_add, ParamSet* paramset, int position) {
  if (computing_l1_bias_) {
    avg_l1_size_ = l1_size_indicator_->GetL1Factor();
    double t_l1_factor_ = 0;
    if (avg_l1_size_ > l1_size_upper_bound_) {
      t_l1_factor_ = 1.0;
    } else if (avg_l1_size_ < l1_size_lower_bound_) {
      t_l1_factor_ = 0.0;
    } else {
      t_l1_factor_ = (avg_l1_size_ - l1_size_lower_bound_) * l1_norm_factor_;
    }
    l1_factor_ = t_l1_factor_;
    l1_bias_ = l1_factor_ * paramset->max_min_diff_;
  }
  // both l1_size as well as l1_order can b added simultaneously
  if (computing_l1_order_bias_) {
    avg_l1_order_ = l1_order_indicator_->GetL1Factor();
    double t_l1_factor_ = 0;
    avg_l1_order_ = l1_order_indicator_->GetL1Factor();
    if (avg_l1_order_ > l1_order_upper_bound_) {
      t_l1_factor_ = 1.0;
    } else if (avg_l1_order_ < l1_order_lower_bound_) {
      t_l1_factor_ = 0.0;
    } else {
      t_l1_factor_ = (avg_l1_order_ - l1_order_lower_bound_) * l1_order_norm_factor_;
    }

    l1_order_factor_ = t_l1_factor_;
    l1_order_bias_ = l1_order_factor_ * paramset->max_min_diff_order_;
  }

  if (computing_high_uts_factor_) {
    double avg_bid_l1_size_high_uts_ = bid_size_indicator_->GetL1Factor();
    double avg_ask_l1_size_high_uts_ = ask_size_indicator_->GetL1Factor();
    double t_bid_l1_factor_ =
        std::min(paramset->max_high_uts_l1_factor_,
                 std::max(0.0, (avg_bid_l1_size_high_uts_ - l1_size_lower_bound_high_uts_) * l1_norm_factor_high_uts_));
    double t_ask_l1_factor_ =
        std::min(paramset->max_high_uts_l1_factor_,
                 std::max(0.0, (avg_ask_l1_size_high_uts_ - l1_size_lower_bound_high_uts_) * l1_norm_factor_high_uts_));
    buy_uts_factor_ = paramset->high_uts_factor_ * t_bid_l1_factor_;
    sell_uts_factor_ = paramset->high_uts_factor_ * t_ask_l1_factor_;

    updated_factor_this_round_ = false;
    if (last_buy_uts_in_high_uts_ != (int)paramset->unit_trade_size_ * buy_uts_factor_ ||
        last_sell_uts_in_high_uts_ != (int)paramset->unit_trade_size_ * sell_uts_factor_) {
      updated_factor_this_round_ = true;
      last_buy_uts_in_high_uts_ = paramset->unit_trade_size_ * buy_uts_factor_;
      last_sell_uts_in_high_uts_ = paramset->unit_trade_size_ * sell_uts_factor_;
    }

    if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
      DBGLOG_TIME_CLASS << "SetUnitTradeSize "
                        << " bid_l1_factor: " << buy_uts_factor_ << " sell_l1_size: " << avg_ask_l1_size_high_uts_
                        << " ask_l1_factor: " << sell_uts_factor_ << ", max_position: " << paramset->max_position_
                        << DBGLOG_ENDL_FLUSH;
    }
  }

  if (computing_positioning_) {
    double t_positioning_ = paramset->positioning_indicator_->GetPositioning();
    long_positioning_bias_ = 0.0;
    short_positioning_bias_ = 0.0;

    if (t_positioning_ > paramset->positioning_threshold_ && position > 0) {
      long_positioning_bias_ =
          abs(position / paramset->unit_trade_size_) * fabs(t_positioning_) * paramset->positioning_thresh_decrease_;
    } else if (t_positioning_ < -paramset->positioning_threshold_ && t_positioning_ < 0) {
      short_positioning_bias_ =
          abs(position / paramset->unit_trade_size_) * fabs(t_positioning_) * paramset->positioning_thresh_decrease_;
    }
  }
}

void IndicatorHelper::RecomputeHysterisis() {
  // TODO : perhaps we should make place_in_line effect bestbid_queue_hysterisis_ smarter
  //        for instance when short term volatility in the market is very high
  //        then being high in the queue should count for less.
  //        In other words the probability of market reversal within movement of 1 tick is very important
  if (longevity_thresh_ > 0) {
    bestbid_queue_hysterisis_ = 0.0;
    bestask_queue_hysterisis_ = 0.0;

    double stdev_scaled_ = std::max(longevity_min_stdev_ratio_, online_stdev_ / offline_stdev_);

    const BaseOrder* top_selfbid_order_ =
        order_manager_->GetBottomBidOrderAtIntPx(product_->best_nonself_bid_int_price_);
    if (top_selfbid_order_ != nullptr) {
      double queue_size_factor_ =
          2.00 * ((double)(top_selfbid_order_->queue_size_behind()) /
                  (double)(product_->best_nonself_bid_size_ + product_->best_nonself_ask_size_));
      // double queue_size_factor_ = ( (double) ( top_selfbid_order_->queue_size_behind() ) / avg_l1_size_ ) ;
      if (longevity_version_ == 1) {
        bestbid_queue_hysterisis_ = (std::min(1.00, queue_size_factor_) * longevity_thresh_) / stdev_scaled_;
      } else {
        bestbid_queue_hysterisis_ = (std::min(1.00, queue_size_factor_) * longevity_thresh_);
      }
    }

    const BaseOrder* top_selfask_order_ =
        order_manager_->GetBottomAskOrderAtIntPx(product_->best_nonself_ask_int_price_);

    if (top_selfask_order_ != nullptr) {
      double queue_size_factor_ =
          2.00 * ((double)(top_selfask_order_->queue_size_behind()) /
                  (double)(product_->best_nonself_bid_size_ + product_->best_nonself_ask_size_));
      // double queue_size_factor_ = ( (double) ( top_selfask_order_->queue_size_behind() ) / avg_l1_size_ ) ;
      if (longevity_version_ == 1) {
        bestask_queue_hysterisis_ = (std::min(1.00, queue_size_factor_) * longevity_thresh_) / stdev_scaled_;
      } else {
        bestask_queue_hysterisis_ = (std::min(1.00, queue_size_factor_) * longevity_thresh_);
      }
    }
  }
}

/**
 *
 * @param indicator_index
 * @param new_value
 */
void IndicatorHelper::OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value) {
  if ((int)indicator_index == longevity_index_) {
    // Longevity Indicator
    double stdev_maxcap_ = offline_stdev_ * 10;
    double stdev_mincap_ = offline_stdev_ * 0.2;
    double stdev_online_old_ = online_stdev_;
    online_stdev_ = std::min(stdev_maxcap_, std::max(stdev_mincap_, new_value));
    if (std::abs(online_stdev_ - stdev_online_old_) > 0.01 * stdev_online_old_) {
      RecomputeHysterisis();
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " stdev_online_: " << online_stdev_ << DBGLOG_ENDL_FLUSH;
      }
    }
  } else if ((int)indicator_index == l1_trade_volume_bias_index_) {
    // BigTradeVolumeVersion Indicator
    if ((new_value / hist_avg_l1_size_) > l1_trade_volume_bias_threshold_) {
      order_manager_->CancelAsksEqAboveIntPrice(product_->best_nonself_ask_int_price_);
      l1_trade_volume_bias_ = 1;
    } else if ((new_value / hist_avg_l1_size_) < -l1_trade_volume_bias_threshold_) {
      order_manager_->CancelBidsEqAboveIntPrice(product_->best_nonself_bid_int_price_);
      l1_trade_volume_bias_ = -1;
    } else {
      l1_trade_volume_bias_ = 0;
    }
  } else if ((int)indicator_index == moving_avg_buy_index_) {
    // BigTrades Indicator
    moving_avg_buy_ = new_value;
    if (moving_avg_buy_ > bigtrades_cancel_threshold_) {
      order_manager_->CancelAsksEqAboveIntPrice(product_->best_nonself_ask_int_price_);
      last_bigtrades_ask_cancel_msecs_ = watch_.msecs_from_midnight();
    }
  } else if ((int)indicator_index == moving_avg_sell_index_) {
    // BigTrades Indicator
    moving_avg_sell_ = new_value;
    if (moving_avg_sell_ > bigtrades_cancel_threshold_) {
      order_manager_->CancelBidsEqAboveIntPrice(product_->best_nonself_bid_int_price_);
      last_bigtrades_bid_cancel_msecs_ = watch_.msecs_from_midnight();
    }
  } else if ((int)indicator_index == moving_avg_buy_place_index_) {
    // BigTrades Place Indicator
    moving_avg_buy_place_ = new_value;

    // When the threshold is reached it will store the time, if at that moment sell was also active it will reset its
    // value
    if (moving_avg_buy_place_ > bigtrades_place_threshold_) {
      last_bigtrades_bid_place_msecs_ = watch_.msecs_from_midnight();
      if ((watch_.msecs_from_midnight() - last_bigtrades_ask_place_msecs_ < bigtrades_place_cooloff_interval_)) {
        last_bigtrades_ask_place_msecs_ = 0;
      }
    }
  } else if ((int)indicator_index == moving_avg_sell_place_index_) {
    // BigTrades Place Indicator
    moving_avg_sell_place_ = new_value;

    // When the threshold is reached it will store the time, if at that moment buy was also active it will reset its
    // value
    if (moving_avg_sell_place_ > bigtrades_place_threshold_) {
      last_bigtrades_ask_place_msecs_ = watch_.msecs_from_midnight();
      if (watch_.msecs_from_midnight() - last_bigtrades_bid_place_msecs_ < bigtrades_place_cooloff_interval_) {
        last_bigtrades_bid_place_msecs_ = 0;
      }
    }
  } else if ((int)indicator_index == moving_avg_buy_aggress_index_) {
    // BigTrades Aggress Indicator
    moving_avg_buy_aggress_ = new_value;

    // When the threshold is reached it will store the time, if at that moment sell was also active it will reset its
    // value
    if (moving_avg_buy_aggress_ > bigtrades_aggress_threshold_) {
      last_bigtrades_bid_aggress_msecs_ = watch_.msecs_from_midnight();
      if (watch_.msecs_from_midnight() - last_bigtrades_ask_aggress_msecs_ < bigtrades_aggress_cooloff_interval_) {
        last_bigtrades_ask_aggress_msecs_ = 0;
      }
    }
  } else if ((int)indicator_index == moving_avg_sell_aggress_index_) {
    // BigTrades Aggress Indicator
    moving_avg_sell_aggress_ = new_value;

    // When the threshold is reached it will store the time, if at that moment buy was also active it will reset its
    // value
    if (moving_avg_sell_aggress_ > bigtrades_aggress_threshold_)
      last_bigtrades_ask_aggress_msecs_ = watch_.msecs_from_midnight();
    if (watch_.msecs_from_midnight() - last_bigtrades_bid_aggress_msecs_ < bigtrades_aggress_cooloff_interval_) {
      last_bigtrades_bid_aggress_msecs_ = 0;
    }
  } else if ((int)indicator_index == l1_bid_ask_size_flow_cancel_index_) {
    l1_bid_ask_size_flow_cancel_ = new_value;
    double bid_px_ = smv_->GetL1BidPrice();
    double ask_px_ = smv_->GetL1AskPrice();
    double mkt_px_ = smv_->mkt_size_weighted_price();
    int bid_sz_ = smv_->GetL1BidSize();
    int ask_sz_ = smv_->GetL1AskSize();

    if (l1_bid_ask_size_flow_cancel_ < -l1_bid_ask_size_flow_cancel_thresh_ &&
        mkt_px_ - bid_px_ < l1_bias_cancel_thresh_ &&
        (bid_sz_ <= min_size_to_flow_based_cancel_ || min_size_to_flow_based_cancel_ <= -1)) {
      l1_bid_ask_size_flow_cancel_buy_ = true;

      order_manager_->CancelBidsEqAboveIntPrice(product_->best_nonself_bid_int_price_);

    } else if (l1_bid_ask_size_flow_cancel_ > -(l1_bid_ask_size_flow_keep_thresh)) {
      l1_bid_ask_size_flow_cancel_buy_ = false;
    }

    if (l1_bid_ask_size_flow_cancel_ > l1_bid_ask_size_flow_cancel_thresh_ &&
        ask_px_ - mkt_px_ < l1_bias_cancel_thresh_ &&
        (ask_sz_ <= min_size_to_flow_based_cancel_ || min_size_to_flow_based_cancel_ <= -1)) {
      l1_bid_ask_size_flow_cancel_sell_ = true;
      order_manager_->CancelAsksEqAboveIntPrice(product_->best_nonself_ask_int_price_);
    } else if (l1_bid_ask_size_flow_cancel_ < (l1_bid_ask_size_flow_keep_thresh)) {
      l1_bid_ask_size_flow_cancel_sell_ = false;
    }
  }

  else if ((int)indicator_index == regime_indicator_index_) {
    // Param  Switch Indicator
    regime_index_ = new_value - 1;
    if (regime_index_ >= 0) {
      called_on_regime_update_ = true;
      NotifyIndicatorHelperListener();
      called_on_regime_update_ = false;
    }
  } else if ((int)indicator_index >= getflat_regime_indicator_start_index_ &&
             (int)indicator_index <= getflat_regime_indicator_end_index_) {
    int this_idx = (int)indicator_index - getflat_regime_indicator_start_index_;
    bool t_trading_ = false;
    for (unsigned reg_idx = 0; reg_idx < regimes_to_trade_[this_idx].size(); reg_idx++) {
      if (new_value == regimes_to_trade_[this_idx][reg_idx]) {
        t_trading_ = true;
        // Because the regimes in indicator are exclusive
        break;
      }
    }

    trading_vec_[this_idx] = t_trading_;
    t_trading_ = false;
    for (unsigned id = 0; id < trading_vec_.size(); id++) {
      // if any of these say trade, then trade
      t_trading_ = t_trading_ || trading_vec_[id];
    }

    if (!t_trading_) {
      // If any of the regime indicators say not to trade, then don't
      if (!getflat_due_to_regime_indicator_) {
        getflat_due_to_regime_indicator_ = true;
        DBGLOG_TIME_CLASS_FUNC << " getflat_due_to_regime, last-updated: " << regime_indicator_string_vec_[this_idx]
                               << " RegimeVal: " << new_value << " NYC: " << watch_.NYTimeString()
                               << " UTC: " << watch_.UTCTimeString() << " IND: " << watch_.IndTimeString()
                               << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
    } else if (t_trading_) {
      if (getflat_due_to_regime_indicator_) {
        DBGLOG_TIME_CLASS_FUNC << " resume_normal_trading_, last-updated: " << regime_indicator_string_vec_[this_idx]
                               << " RegimeVal: " << new_value << " NYC: " << watch_.NYTimeString()
                               << " UTC: " << watch_.UTCTimeString() << " IND: " << watch_.IndTimeString()
                               << " Strategy: " << runtime_id_ << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
      getflat_due_to_regime_indicator_ = false;
    }
  } else if ((int)indicator_index >= trade_bias_indicator_start_index_ &&
             (int)indicator_index <= trade_bias_indicator_end_index_) {
    trade_bias_ = new_value * factor_trade_bias_;
    if (trade_bias_indicator_string_ == cancel_bias_indicator_string_) {
      cancel_bias_ = new_value * factor_cancel_bias_;
    }
  } else if ((int)indicator_index >= cancel_bias_indicator_start_index_ &&
             (int)indicator_index <= cancel_bias_indicator_end_index_) {
    cancel_bias_ = new_value * factor_cancel_bias_;
    if (trade_bias_indicator_string_ == cancel_bias_indicator_string_) {
      trade_bias_ = new_value * factor_cancel_bias_;
    }
  } else if ((int)indicator_index >= implied_mkt_indicator_start_index_ &&
             (int)indicator_index <= implied_mkt_indicator_end_index_) {
    implied_price_ = new_value;
  } else if ((int)indicator_index == stdev_calculator_suppress_non_best_index_) {
    if (new_value > stdev_suppress_non_best_level_threshold_) {
      stdev_nnbl_switch_ = false;
    } else {
      stdev_nnbl_switch_ = true;
    }
  } else if ((int)indicator_index == moving_average_spread_index_) {
    moving_average_spread_value_ = new_value;
  } else if ((int)indicator_index == moving_stdev_index_) {
    double t_new_stdev_val = new_value;
    if (use_sqrt_stdev_) {
      // to slowdown the impact of stdev on threshholds
      t_new_stdev_val = smv_->min_price_increment() * (sqrt(new_value / smv_->min_price_increment()));
    }

    moving_stdev_val_ = std::min(stdev_fact_ticks_ * t_new_stdev_val, stdev_cap_);
    moving_stdev_val_ *= smv_->min_price_increment();
  } else if ((int)indicator_index == src_trend_index_) {
    src_trend_val_ = new_value;
    set_mkt_condition_flags();
  } else if ((int)indicator_index == src_stdev_index_) {
    src_stdev_val_ = new_value;
    set_mkt_condition_flags();
  }
}

/**
 *
 * @param indicator_index
 * @param new_value_decrease
 * @param new_value_nochange
 * @param new_value_increase
 */
void IndicatorHelper::OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value_decrease,
                                        const double& new_value_nochange, const double& new_value_increase) {}

/**
 *
 * @param paramset
 */

void IndicatorHelper::ComputeL1TradeVolumeBias(ParamSet* paramset) {
  // It will not kill the query but will just not use the param in case sample data is not found.
  hist_avg_l1_size_ =
      HFSAT::SampleDataUtil::GetAvgForPeriod(smv_->shortcode(), watch_.YYYYMMDD(), 60, trading_start_utc_mfm_,
                                             trading_end_utc_mfm_, std::string("L1SZ"), false);
  if (hist_avg_l1_size_ > 0) {
    l1_trade_volume_bias_indicator_ =
        BigTradeVolumeVersion::GetUniqueInstance(dbglogger_, watch_, *smv_, (paramset->bigtrades_window_msecs_));
    l1_trade_volume_bias_index_ = indices_used_so_far_;
    l1_trade_volume_bias_indicator_->add_indicator_listener(l1_trade_volume_bias_index_, this, 1.00);
    indices_used_so_far_++;

    if (paramset->read_l1_trade_volume_bias_) {
      l1_trade_volume_bias_threshold_ = paramset->l1_trade_volume_bias_;
    }
  }
}

void IndicatorHelper::ComputeL1SizeBias(ParamSet* paramset) {
  int t_fractional_seconds_ = HFSAT::ExecLogicUtils::GetDurationForAvgL1Size(smv_->shortcode());
  l1_size_indicator_ = L1SizeTrend::GetUniqueInstance(
      dbglogger_, watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(smv_->shortcode())),
      t_fractional_seconds_, kPriceTypeMktSizeWPrice);
  if (l1_size_indicator_ == nullptr) {
    ExitVerbose(kExitErrorCodeGeneral, "Could not create l1_size indicator in IndicatorHelper\n");
  }

  HFSAT::ExecLogicUtils::GetL1SizeBound(smv_->shortcode(), l1_size_lower_bound_, l1_size_upper_bound_, watch_,
                                        trading_start_utc_mfm_, trading_end_utc_mfm_, dbglogger_,
                                        l1_size_lower_percentile_, l1_size_upper_percentile_,
                                        use_l1_size_static_value_);
  l1_avg_size_lower_bound_ = l1_size_lower_bound_;
  l1_avg_size_upper_bound_ = l1_size_upper_bound_;

  if (l1_avg_size_lower_bound_ == 0 || l1_avg_size_upper_bound_ == 0) {
    ExitVerbose(kSampleDataError, "l1_avg_size_bound_");
  }

  if (l1_size_upper_bound_ - l1_size_lower_bound_ >= 1.0) {
    l1_norm_factor_ = 1.0 / (l1_size_upper_bound_ - l1_size_lower_bound_);
  } else {
    l1_norm_factor_ = 0;
  }
  computing_l1_bias_ = true;
  paramset->max_min_diff_ *= smv_->min_price_increment();
}

void IndicatorHelper::ComputeL1OrderBias(ParamSet* paramset) {
  int t_fractional_seconds_ = HFSAT::ExecLogicUtils::GetDurationForAvgL1Size(smv_->shortcode());

  l1_order_indicator_ = L1OrderTrend::GetUniqueInstance(
      dbglogger_, watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(smv_->shortcode())),
      t_fractional_seconds_, kPriceTypeMktSizeWPrice);
  if (l1_order_indicator_ == nullptr) {
    ExitVerbose(kExitErrorCodeGeneral, "Could not create l1_order indicator in IndicatorHelper\n");
  }

  HFSAT::ExecLogicUtils::GetL1OrderBound(smv_->shortcode(), l1_order_lower_bound_, l1_order_upper_bound_, watch_,
                                         trading_start_utc_mfm_, trading_end_utc_mfm_, dbglogger_,
                                         l1_order_lower_percentile_, l1_order_upper_percentile_,
                                         use_l1_order_static_value_);
  l1_avg_order_lower_bound_ = l1_order_lower_bound_;
  l1_avg_order_upper_bound_ = l1_order_upper_bound_;

  if (l1_avg_order_lower_bound_ == 0 || l1_avg_order_upper_bound_ == 0) {
    ExitVerbose(kSampleDataError, "l1_avg_order_bound_");
  }

  if (l1_order_upper_bound_ - l1_order_lower_bound_ >= 1.0) {
    l1_order_norm_factor_ = 1.0 / (l1_order_upper_bound_ - l1_order_lower_bound_);
  } else {
    l1_order_norm_factor_ = 0.0;
  }

  computing_l1_order_bias_ = true;
  paramset->max_min_diff_order_ *= smv_->min_price_increment();
}

void IndicatorHelper::ComputeHighUTSFactor(ParamSet* paramset) {
  int t_fractional_seconds_ = HFSAT::ExecLogicUtils::GetL1SizeDurationForHighUTS(smv_->shortcode());

  l1_size_indicator_ = L1SizeTrend::GetUniqueInstance(
      dbglogger_, watch_, *ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(smv_->shortcode()),
      t_fractional_seconds_, kPriceTypeMktSizeWPrice);

  if (l1_size_indicator_ == nullptr) {
    ExitVerbose(kExitErrorCodeGeneral, " Could not create bid or ask l1size indicator in IndicatorHelper\n");
  }

  bid_size_indicator_ = L1SizeTrend::GetUniqueInstance(
      dbglogger_, watch_, *ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(smv_->shortcode()),
      t_fractional_seconds_, kPriceTypeBidPrice);
  ask_size_indicator_ = L1SizeTrend::GetUniqueInstance(
      dbglogger_, watch_, *ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(smv_->shortcode()),
      t_fractional_seconds_, kPriceTypeAskPrice);

  if (bid_size_indicator_ == nullptr || ask_size_indicator_ == nullptr) {
    ExitVerbose(kExitErrorCodeGeneral, " Could not create bid or ask l1size indicator in IndicatorHelper\n");
  }

  HFSAT::ExecLogicUtils::GetHighUTSBound(smv_->shortcode(), l1_size_lower_bound_high_uts_,
                                         l1_size_upper_bound_high_uts_, watch_, trading_start_utc_mfm_,
                                         trading_end_utc_mfm_, dbglogger_);

  if (l1_size_upper_bound_high_uts_ - l1_size_lower_bound_high_uts_ >= 1.0) {
    l1_norm_factor_high_uts_ = 1.0 / (l1_size_upper_bound_high_uts_ - l1_size_lower_bound_high_uts_);
  } else {
    l1_norm_factor_high_uts_ = 0;
  }
  computing_high_uts_factor_ = true;
}

void IndicatorHelper::ComputeLongevity(ParamSet* paramset) {
  if (paramset->longevity_version_ == 1) {
    stdev_calculator_onoff_ = SlowStdevCalculator::GetUniqueInstance(dbglogger_, watch_, smv_->shortcode(),
                                                                     paramset->longevity_stdev_dur_ * 1000u, 0.00);
    longevity_index_ = indices_used_so_far_;
    stdev_calculator_onoff_->add_indicator_listener(longevity_index_, this, 1.00);
    indices_used_so_far_++;
    PcaWeightsManager& pca_manager = PcaWeightsManager::GetUniqueInstance();
    offline_stdev_ = pca_manager.GetShortcodeStdevsLongevity(smv_->shortcode());
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " stdev_offline_: " << offline_stdev_ << DBGLOG_ENDL_FLUSH;
    }
  }

  longevity_thresh_ = paramset->longevity_support_;
  longevity_version_ = paramset->longevity_version_;
  longevity_min_stdev_ratio_ = paramset->longevity_min_stdev_ratio_;
  computing_longevity_ = true;
}

void IndicatorHelper::ComputePositioning(ParamSet* paramset) {
  paramset->positioning_indicator_ = Positioning::GetUniqueInstance(
      dbglogger_, watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(smv_->shortcode())),
      paramset->bucket_size_, kPriceTypeMidprice);
  paramset->positioning_thresh_decrease_ *= smv_->min_price_increment();
  computing_positioning_ = true;
}

void IndicatorHelper::ComputeStdevTriggerForPCNBL(ParamSet* paramset) {
  stdev_calculator_suppress_non_best_indc_ = SlowStdevCalculator::GetUniqueInstance(
      dbglogger_, watch_, smv_->shortcode(), paramset->stdev_suppress_non_best_level_duration_ * 1000u);
  stdev_calculator_suppress_non_best_index_ = indices_used_so_far_;
  stdev_calculator_suppress_non_best_indc_->add_unweighted_indicator_listener(stdev_calculator_suppress_non_best_index_,
                                                                              this);
  stdev_suppress_non_best_level_threshold_ =
      paramset->stdev_suppress_non_best_level_threshold_ * smv_->min_price_increment();
  indices_used_so_far_++;
}

void IndicatorHelper::ComputeBigTrades(ParamSet* paramset) {
  moving_average_buy_indc_ = MovingAvgTradeImpact::GetUniqueInstance(
      dbglogger_, watch_, *smv_, (paramset->bigtrades_window_msecs_), 0, paramset->bigtrades_decay_factor_);
  moving_avg_buy_index_ = indices_used_so_far_;
  moving_average_buy_indc_->add_indicator_listener(moving_avg_buy_index_, this, 1.00);
  indices_used_so_far_++;

  moving_average_sell_indc_ = MovingAvgTradeImpact::GetUniqueInstance(
      dbglogger_, watch_, *smv_, (paramset->bigtrades_window_msecs_), 1, paramset->bigtrades_decay_factor_);
  moving_avg_sell_index_ = indices_used_so_far_;
  moving_average_sell_indc_->add_indicator_listener(moving_avg_sell_index_, this, 1.00);
  indices_used_so_far_++;

  if (paramset->read_bigtrades_cancel_) {
    bigtrades_cancel_threshold_ = paramset->bigtrades_cancel_threshold_;
  }

  computing_bigtrades_ = true;
}

void IndicatorHelper::ComputeBigTradesIndep(ParamSet* paramset) {
  moving_average_buy_indc_ = MovingAvgTradeImpact::GetUniqueInstance(
      dbglogger_, watch_, *ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(paramset->bigtrades_source_id_),
      paramset->bigtrades_window_msecs_, 0, paramset->bigtrades_decay_factor_);
  moving_avg_buy_index_ = indices_used_so_far_;
  moving_average_buy_indc_->add_indicator_listener(moving_avg_buy_index_, this, 1.00);
  indices_used_so_far_++;

  moving_average_sell_indc_ = MovingAvgTradeImpact::GetUniqueInstance(
      dbglogger_, watch_, *ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(paramset->bigtrades_source_id_),
      paramset->bigtrades_window_msecs_, 1, paramset->bigtrades_decay_factor_);
  moving_avg_sell_index_ = indices_used_so_far_;
  moving_average_sell_indc_->add_indicator_listener(moving_avg_sell_index_, this, 1.00);
  indices_used_so_far_++;

  if (paramset->read_bigtrades_cancel_) {
    bigtrades_cancel_threshold_ = paramset->bigtrades_cancel_threshold_;
  }
}

void IndicatorHelper::ComputeBigTradesPlaceIndep(ParamSet* paramset) {
  moving_average_buy_place_indc_ = MovingAvgTradeImpact::GetUniqueInstance(
      dbglogger_, watch_,
      *ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(paramset->bigtrades_place_source_id_),
      paramset->bigtrades_place_window_msecs_, 0, paramset->bigtrades_place_decay_factor_);
  moving_avg_buy_place_index_ = indices_used_so_far_;
  moving_average_buy_place_indc_->add_indicator_listener(moving_avg_buy_place_index_, this, 1.00);
  indices_used_so_far_++;

  moving_average_sell_place_indc_ = MovingAvgTradeImpact::GetUniqueInstance(
      dbglogger_, watch_,
      *ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(paramset->bigtrades_place_source_id_),
      paramset->bigtrades_place_window_msecs_, 1, paramset->bigtrades_place_decay_factor_);
  moving_avg_sell_place_index_ = indices_used_so_far_;
  moving_average_sell_place_indc_->add_indicator_listener(moving_avg_sell_place_index_, this, 1.00);
  indices_used_so_far_++;
  bigtrades_place_threshold_ = paramset->bigtrades_place_threshold_;
  bigtrades_place_cooloff_interval_ = paramset->bigtrades_place_cooloff_interval_;
}

void IndicatorHelper::ComputeBigTradesAggressIndep(ParamSet* paramset) {
  moving_average_buy_aggress_indc_ = MovingAvgTradeImpact::GetUniqueInstance(
      dbglogger_, watch_,
      *ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(paramset->bigtrades_aggress_source_id_),
      paramset->bigtrades_aggress_window_msecs_, 0, paramset->bigtrades_aggress_decay_factor_);
  moving_avg_buy_aggress_index_ = indices_used_so_far_;
  moving_average_buy_aggress_indc_->add_indicator_listener(moving_avg_buy_aggress_index_, this, 1.00);
  indices_used_so_far_++;

  moving_average_sell_aggress_indc_ = MovingAvgTradeImpact::GetUniqueInstance(
      dbglogger_, watch_,
      *ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(paramset->bigtrades_aggress_source_id_),
      paramset->bigtrades_aggress_window_msecs_, 1, paramset->bigtrades_aggress_decay_factor_);
  moving_avg_sell_aggress_index_ = indices_used_so_far_;
  moving_average_sell_aggress_indc_->add_indicator_listener(moving_avg_sell_aggress_index_, this, 1.00);
  indices_used_so_far_++;
  bigtrades_aggress_threshold_ = paramset->bigtrades_aggress_threshold_;
  bigtrades_aggress_cooloff_interval_ = paramset->bigtrades_aggress_cooloff_interval_;
}

void IndicatorHelper::ComputeL1BidAskSizeFlow(ParamSet* paramset) {
  l1_bid_ask_size_flow_cancel_indc_ =
      L1BidAskSizeFlow::GetUniqueInstance(dbglogger_, watch_, *smv_, (paramset->l1_bid_ask_flow_num_events_));
  l1_bid_ask_size_flow_cancel_index_ = indices_used_so_far_;
  l1_bid_ask_size_flow_cancel_indc_->add_indicator_listener(l1_bid_ask_size_flow_cancel_index_, this, 1.00);
  indices_used_so_far_++;
  l1_bid_ask_size_flow_cancel_thresh_ = paramset->l1_bid_ask_flow_cancel_thresh_;
  l1_bid_ask_size_flow_cancel_keep_thresh_diff_ = paramset->l1_bid_ask_flow_cancel_keep_thresh_diff_;
  l1_bias_cancel_thresh_ = paramset->l1_bias_cancel_thresh_ * smv_->min_price_increment();
  min_size_to_flow_based_cancel_ = paramset->min_size_to_flow_based_cancel_;
  l1_bid_ask_size_flow_keep_thresh =
      l1_bid_ask_size_flow_cancel_thresh_ - l1_bid_ask_size_flow_cancel_keep_thresh_diff_;
}

void IndicatorHelper::ComputeScaledMaxPos(ParamSet* paramset) {
  double t_trading_volume_expected_ = 0;
  if (paramset->read_source_for_dmur_) {
    paramset->volume_ratio_indicator_ = RecentSimpleVolumeMeasure::GetUniqueInstance(
        dbglogger_, watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(paramset->source_for_dmur_)),
        paramset->volume_history_secs_);

    paramset->stdev_ratio_normalised_indicator_ = StdevRatioNormalised::GetUniqueInstance(
        dbglogger_, watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(smv_->shortcode())),
        paramset->source_for_dmur_, paramset->volume_history_secs_);
    paramset->stdev_ratio_normalised_indicator_->add_unweighted_indicator_listener(indices_used_so_far_, this);
    indices_used_so_far_++;

    t_trading_volume_expected_ =
        HFSAT::ExecLogicUtils::GetMURVolBound(paramset->source_for_dmur_, watch_, paramset->volume_history_secs_,
                                              trading_start_utc_mfm_, trading_end_utc_mfm_);

    paramset->self_volume_ratio_indicator_ = RecentSimpleVolumeMeasure::GetUniqueInstance(
        dbglogger_, watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(smv_->shortcode())),
        paramset->volume_history_secs_);
    paramset->self_volume_expected_ = HFSAT::ExecLogicUtils::GetMURVolBound(
        smv_->shortcode(), watch_, paramset->volume_history_secs_, trading_start_utc_mfm_, trading_end_utc_mfm_);

  } else {
    paramset->volume_ratio_indicator_ = RecentSimpleVolumeMeasure::GetUniqueInstance(
        dbglogger_, watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(smv_->shortcode())),
        paramset->volume_history_secs_);
    t_trading_volume_expected_ = HFSAT::ExecLogicUtils::GetMURVolBound(
        smv_->shortcode(), watch_, paramset->volume_history_secs_, trading_start_utc_mfm_, trading_end_utc_mfm_);
  }

  if (t_trading_volume_expected_ > 0 && (paramset->volume_upper_bound_ratio_ > paramset->volume_lower_bound_ratio_)) {
    paramset->volume_lower_bound_ = t_trading_volume_expected_ * paramset->volume_lower_bound_ratio_;
    paramset->volume_upper_bound_ = t_trading_volume_expected_ * paramset->volume_upper_bound_ratio_;
  } else {
    paramset->volume_lower_bound_ = 0.0;
    paramset->volume_upper_bound_ = 1000000.0;
  }

  paramset->volume_norm_factor_ =
      (double)(((int)floor(paramset->max_position_ / paramset->unit_trade_size_) - paramset->base_mur_) /
               (paramset->volume_upper_bound_ - paramset->volume_lower_bound_));
}

void IndicatorHelper::ComputeRegimes(const std::string& regime_indicator_string) {
  const unsigned int kModelLineBufferLen = 1024;
  char readline_buffer_[kModelLineBufferLen];
  bzero(readline_buffer_, kModelLineBufferLen);
  std::string temp_str_ = regime_indicator_string;
  strncpy(readline_buffer_, temp_str_.c_str(), temp_str_.length());
  PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);
  const std::vector<const char*>& tokens_ = st_.GetTokens();
  if (tokens_.size() >= 3) {
    regime_indicator_ = GetIndicatorFromTokens(dbglogger_, watch_, tokens_, kPriceTypeMidprice);
    if (regime_indicator_ == NULL) {
      std::cerr << "BaseTrading::BaseTrading Can't create param_regime_indicator\n";
      exit(1);
    }

    regime_indicator_index_ = indices_used_so_far_;

    regime_indicator_->add_unweighted_indicator_listener(regime_indicator_index_, this);
    indices_used_so_far_++;
  }
}

void IndicatorHelper::ComputeRegimesToTrade(ParamSet* paramset) {
  getflat_regime_indicator_start_index_ = indices_used_so_far_;
  getflat_regime_indicator_end_index_ = indices_used_so_far_;
  for (unsigned id = 0; id < paramset->regime_indicator_string_vec_.size(); id++) {
    const unsigned int kModelLineBufferLen = 1024;
    char readline_buffer_[kModelLineBufferLen];
    bzero(readline_buffer_, kModelLineBufferLen);
    std::string this_indicator_string = "INDICATOR 1.00 " + paramset->regime_indicator_string_vec_[id];
    strncpy(readline_buffer_, this_indicator_string.c_str(), this_indicator_string.length());
    PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);  // Perishable string readline_buffer_
    const std::vector<const char*>& tokens_ = st_.GetTokens();
    CommonIndicator* indicator =
        (GetUniqueInstanceFunc(tokens_[2]))(dbglogger_, watch_, tokens_, kPriceTypeMktSizeWPrice);
    if (indicator != nullptr) {
      regime_indicator_vec_.push_back(indicator);
      trading_vec_.push_back(true);
      indicator->add_unweighted_indicator_listener(getflat_regime_indicator_end_index_++, this);
    } else {
      ExitVerbose(kExitErrorCodeGeneral, " Regime indicator could not be created");
    }
    DBGLOG_TIME_CLASS_FUNC << " RegimeIndicator : " << paramset->regime_indicator_string_vec_[id]
                           << " TradingInRegimes: ";
    for (unsigned reg_idx = 0; reg_idx < paramset->regimes_to_trade_[id].size(); reg_idx++) {
      dbglogger_ << paramset->regimes_to_trade_[id][reg_idx] << " ";
    }
    dbglogger_ << "\n";
    dbglogger_.CheckToFlushBuffer();
  }

  indices_used_so_far_ = getflat_regime_indicator_end_index_ + 1;
  regime_indicator_string_vec_ = paramset->regime_indicator_string_vec_;
  regimes_to_trade_ = paramset->regimes_to_trade_;
}

void IndicatorHelper::ComputeTradeBias(ParamSet* paramset) {
  trade_bias_indicator_start_index_ = indices_used_so_far_;
  trade_bias_indicator_end_index_ = indices_used_so_far_;

  const unsigned int kModelLineBufferLen = 1024;
  char readline_buffer_[kModelLineBufferLen];
  bzero(readline_buffer_, kModelLineBufferLen);
  trade_bias_indicator_string_ = "INDICATOR 1.00 " + paramset->trade_indicator_string_;
  strncpy(readline_buffer_, trade_bias_indicator_string_.c_str(), trade_bias_indicator_string_.length());
  PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);  // Perishable string readline_buffer_
  const std::vector<const char*>& tokens_ = st_.GetTokens();
  if (trade_bias_indicator_string_ != cancel_bias_indicator_string_) {
    CommonIndicator* indicator =
        (GetUniqueInstanceFunc(tokens_[2]))(dbglogger_, watch_, tokens_, kPriceTypeMktSizeWPrice);

    if (indicator != nullptr) {
      //	      regime_indicator_vec_.push_back(indicator);
      //	      trading_vec_.push_back(true);
      indicator->add_unweighted_indicator_listener(trade_bias_indicator_end_index_++, this);
    } else {
      ExitVerbose(kExitErrorCodeGeneral, " Trade indicator could not be created");
    }
  }

  DBGLOG_TIME_CLASS_FUNC << " TradeIndicator : " << paramset->trade_indicator_string_;
  dbglogger_ << "\n";
  dbglogger_.CheckToFlushBuffer();

  indices_used_so_far_ = trade_bias_indicator_end_index_ + 1;
  trade_indicator_string_ = paramset->trade_indicator_string_;
  factor_trade_bias_ = paramset->factor_trade_bias_;
}

void IndicatorHelper::ComputeCancelBias(ParamSet* paramset) {
  cancel_bias_indicator_start_index_ = indices_used_so_far_;
  cancel_bias_indicator_end_index_ = indices_used_so_far_;

  const unsigned int kModelLineBufferLen = 1024;
  char readline_buffer_[kModelLineBufferLen];
  bzero(readline_buffer_, kModelLineBufferLen);
  cancel_bias_indicator_string_ = "INDICATOR 1.00 " + paramset->cancel_indicator_string_;
  strncpy(readline_buffer_, cancel_bias_indicator_string_.c_str(), cancel_bias_indicator_string_.length());
  PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);  // Perishable string readline_buffer_
  const std::vector<const char*>& tokens_ = st_.GetTokens();
  if (trade_bias_indicator_string_ != cancel_bias_indicator_string_) {
    CommonIndicator* indicator =
        (GetUniqueInstanceFunc(tokens_[2]))(dbglogger_, watch_, tokens_, kPriceTypeMktSizeWPrice);

    if (indicator != nullptr) {
      //	      regime_indicator_vec_.push_back(indicator);
      //	      trading_vec_.push_back(true);
      indicator->add_unweighted_indicator_listener(cancel_bias_indicator_end_index_++, this);
    } else {
      ExitVerbose(kExitErrorCodeGeneral, " Cancel indicator could not be created");
    }
  }
  DBGLOG_TIME_CLASS_FUNC << " CancelIndicator : " << paramset->cancel_indicator_string_;
  dbglogger_ << "\n";
  dbglogger_.CheckToFlushBuffer();

  indices_used_so_far_ = cancel_bias_indicator_end_index_ + 1;
  cancel_indicator_string_ = paramset->cancel_indicator_string_;
  factor_cancel_bias_ = paramset->factor_cancel_bias_;
}

void IndicatorHelper::ComputeImpliedPrice(ParamSet* paramset) {
  implied_mkt_indicator_start_index_ = indices_used_so_far_;
  implied_mkt_indicator_end_index_ = indices_used_so_far_;

  const unsigned int kModelLineBufferLen = 1024;
  char readline_buffer_[kModelLineBufferLen];
  bzero(readline_buffer_, kModelLineBufferLen);
  implied_mkt_indicator_string_ = "INDICATOR 1.00 " + paramset->implied_mkt_indicator_string_;
  strncpy(readline_buffer_, implied_mkt_indicator_string_.c_str(), implied_mkt_indicator_string_.length());
  PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);  // Perishable string readline_buffer_
  const std::vector<const char*>& tokens_ = st_.GetTokens();
  CommonIndicator* indicator =
      (GetUniqueInstanceFunc(tokens_[2]))(dbglogger_, watch_, tokens_, kPriceTypeMktSizeWPrice);

  if (indicator != nullptr) {
    //	      regime_indicator_vec_.push_back(indicator);
    //	      trading_vec_.push_back(true);
    indicator->add_unweighted_indicator_listener(implied_mkt_indicator_end_index_++, this);
  } else {
    ExitVerbose(kExitErrorCodeGeneral, " Implied Market indicator could not be created");
  }
  DBGLOG_TIME_CLASS_FUNC << " ImpliedMktIndicator : " << paramset->implied_mkt_indicator_string_;
  dbglogger_ << "\n";
  dbglogger_.CheckToFlushBuffer();

  indices_used_so_far_ = implied_mkt_indicator_end_index_ + 1;
}

/*
 * Compute moving average bid-ask-spread
 */
void IndicatorHelper::ComputeMovingBidAskSpread(ParamSet* paramset) {
  if (moving_average_spread_index_ == -1) {  // only if we haven't subscribed it yet
    moving_average_spread_ =
        MovingAvgBidAskSpread::GetUniqueInstance(dbglogger_, watch_, smv_, paramset->moving_bidask_spread_duration_);
    moving_average_spread_index_ = indices_used_so_far_;
    moving_average_spread_->add_indicator_listener(moving_average_spread_index_, this, 1.00);
    indices_used_so_far_++;
  }
}

void IndicatorHelper::ComputeMovingStdev(ParamSet* paramset) {
  if (moving_stdev_index_ == -1) {
    use_sqrt_stdev_ = paramset->use_sqrt_stdev_;
    stdev_fact_ticks_ = paramset->stdev_fact_ticks_;
    stdev_cap_ = paramset->stdev_cap_;
    // Currently setting minimum value of stdev as 0.1 times of tick.
    slow_stdev_calculator_ = SlowStdevCalculator::GetUniqueInstance(dbglogger_, watch_, smv_->shortcode(),
                                                                    (unsigned int)paramset->stdev_duration_ * 1000u,
                                                                    smv_->min_price_increment() * 0.1);
    moving_stdev_index_ = indices_used_so_far_;
    slow_stdev_calculator_->add_indicator_listener(moving_stdev_index_, this, 1.00);
    indices_used_so_far_++;
  }
}

double IndicatorHelper::volume_adj_max_pos(ParamSet* paramset) {
  double t_recent_vol_ = paramset->volume_ratio_indicator_->recent_volume();
  int t_vol_scaled_mur_ = 0;
  if (t_recent_vol_ > paramset->volume_upper_bound_) {
    t_vol_scaled_mur_ = (paramset->volume_upper_bound_ - paramset->volume_lower_bound_) * paramset->volume_norm_factor_;
  } else if (t_recent_vol_ < paramset->volume_lower_bound_) {
    t_vol_scaled_mur_ = 0;
  } else {
    t_vol_scaled_mur_ = (t_recent_vol_ - paramset->volume_lower_bound_) * paramset->volume_norm_factor_;
  }
  if (paramset->read_source_for_dmur_) {
    double t_recent_stdev_ratio_ = paramset->stdev_ratio_normalised_indicator_->stdev_ratio();
    double t_recent_self_vol_ = paramset->self_volume_ratio_indicator_->recent_volume();
    if (t_recent_stdev_ratio_ > paramset->self_stdev_ratio_threshold_ ||
        t_recent_self_vol_ < paramset->self_volume_expected_ * paramset->self_volume_ratio_threshold_) {
      t_vol_scaled_mur_ = 0;
    }
  }
  return (paramset->base_mur_ + t_vol_scaled_mur_) * paramset->unit_trade_size_;
}

void IndicatorHelper::ComputeFlagsAsPerMktCondition(ParamSet* paramset) {
  //  int t_fractional_seconds_ = HFSAT::ExecLogicUtils::GetDurationForAvgL1Size(smv_->shortcode()); TODO
  int t_fractional_seconds_ = 300;

  src_trend_indicator_ = SimpleTrend::GetUniqueInstance(dbglogger_, watch_, paramset->source_for_mkt_condition_,
                                                        t_fractional_seconds_, kPriceTypeMktSizeWPrice);
  if (src_trend_indicator_ == nullptr) {
    ExitVerbose(kExitErrorCodeGeneral,
                "Could not create SimpleTrend indicator in IndicatorHelper in ComputeFlagsAsPerMktConditions\n");
  }
  src_trend_index_ = indices_used_so_far_;
  indices_used_so_far_++;
  src_trend_indicator_->add_indicator_listener(src_trend_index_, this, 1.00);

  src_stdev_indicator_ = SlowStdevCalculator::GetUniqueInstance(
      dbglogger_, watch_, paramset->source_for_mkt_condition_, (unsigned int)t_fractional_seconds_ * 1000u,
      ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(paramset->source_for_mkt_condition_)
              ->min_price_increment() *
          0.1);
  if (src_stdev_indicator_ == nullptr) {
    ExitVerbose(kExitErrorCodeGeneral,
                "Could not create SlowStdevCalculator indicator IndicatorHelper in ComputeFlagsAsPerMktConditions\n");
  }
  src_stdev_index_ = indices_used_so_far_;
  indices_used_so_far_++;
  src_stdev_indicator_->add_indicator_listener(src_stdev_index_, this, 1.00);

  src_trend_thresh_ = paramset->src_trend_factor_ * HFSAT::SampleDataUtil::GetPercentileForPeriod(
                                                        paramset->source_for_mkt_condition_, watch_.YYYYMMDD(), 60,
                                                        trading_start_utc_mfm_, trading_end_utc_mfm_, "TREND",
                                                        paramset->src_trend_percentile_);

  src_trend_keep_thresh_ = paramset->src_trend_keep_factor_ * src_trend_thresh_;

  src_stdev_thresh_ = paramset->src_trend_factor_ * HFSAT::SampleDataUtil::GetPercentileForPeriod(
                                                        paramset->source_for_mkt_condition_, watch_.YYYYMMDD(), 60,
                                                        trading_start_utc_mfm_, trading_end_utc_mfm_, "STDEV",
                                                        paramset->src_trend_percentile_);
}

void IndicatorHelper::InitiateStdevL1Bias(ParamSet* paramset) {
  sd_l1bias_ =
      new StdevCalculator(dbglogger_, watch_, paramset->l1bias_model_stdev_duration_, smv_->min_price_increment());
  offline_l1bias_stdev_ = smv_->min_price_increment() * paramset->l1bias_model_stdev_;
  l1bias_weight_cap_ = paramset->l1bias_weight_cap_;
}

double IndicatorHelper::ComputeScaledBaseMidDiff(double basemid_diff_) {
  double l1bias_sd_ = sd_l1bias_->AddCurrentValue(basemid_diff_);
  double scaled_basemid_diff_;
  if (l1bias_sd_ > 0) {
    scaled_basemid_diff_ = basemid_diff_ * std::min(l1bias_weight_cap_, offline_l1bias_stdev_ / l1bias_sd_);
  } else {
    scaled_basemid_diff_ = basemid_diff_;
  };
  return (scaled_basemid_diff_);
}

void IndicatorHelper::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if (computing_l1_bias_) {
    int t_start_mfm_ =
        std::max(std::min(watch_.msecs_from_midnight(), trading_end_utc_mfm_), trading_start_utc_mfm_ - 900000);
    int t_end_mfm_ = std::max(std::min(watch_.msecs_from_midnight() + 900000, trading_end_utc_mfm_ + 900000),
                              trading_start_utc_mfm_);

    HFSAT::ExecLogicUtils::GetL1SizeBound(smv_->shortcode(), l1_size_lower_bound_, l1_size_upper_bound_, watch_,
                                          t_start_mfm_, t_end_mfm_, dbglogger_, l1_size_lower_percentile_,
                                          l1_size_upper_percentile_, use_l1_size_static_value_);

    if (l1_size_upper_bound_ == 0 || l1_size_lower_bound_ == 0) {
      l1_size_lower_bound_ = l1_avg_size_lower_bound_;
      l1_size_upper_bound_ = l1_avg_size_upper_bound_;
    }

    if (l1_size_upper_bound_ - l1_size_lower_bound_ >= 1.0) {
      l1_norm_factor_ = 1.0 / (l1_size_upper_bound_ - l1_size_lower_bound_);
    } else {
      l1_norm_factor_ = 0;
    }
  }
  if (computing_l1_order_bias_) {
    int t_start_mfm_ =
        std::max(std::min(watch_.msecs_from_midnight(), trading_end_utc_mfm_ - 900000), trading_start_utc_mfm_);
    int t_end_mfm_ = std::max(std::min(watch_.msecs_from_midnight() + 900000, trading_end_utc_mfm_),
                              trading_start_utc_mfm_ + 900000);

    HFSAT::ExecLogicUtils::GetL1OrderBound(smv_->shortcode(), l1_order_lower_bound_, l1_order_upper_bound_, watch_,
                                           t_start_mfm_, t_end_mfm_, dbglogger_, l1_order_lower_percentile_,
                                           l1_order_upper_percentile_, use_l1_order_static_value_);

    if (l1_order_upper_bound_ == 0 || l1_order_lower_bound_ == 0) {
      l1_order_lower_bound_ = l1_avg_order_lower_bound_;
      l1_order_upper_bound_ = l1_avg_order_upper_bound_;
    }

    if (l1_order_upper_bound_ - l1_order_lower_bound_ >= 1.0) {
      l1_order_norm_factor_ = 1.0 / (l1_order_upper_bound_ - l1_order_lower_bound_);
    } else {
      l1_order_norm_factor_ = 0.0;
    }
  }

  if (computing_high_uts_factor_) {
    int t_start_mfm_ =
        std::max(std::min(watch_.msecs_from_midnight(), trading_end_utc_mfm_ - 900000), trading_start_utc_mfm_);
    int t_end_mfm_ = std::max(std::min(watch_.msecs_from_midnight() + 900000, trading_end_utc_mfm_),
                              trading_start_utc_mfm_ + 900000);

    HFSAT::ExecLogicUtils::GetHighUTSBound(smv_->shortcode(), l1_size_lower_bound_high_uts_,
                                           l1_size_upper_bound_high_uts_, watch_, t_start_mfm_, t_end_mfm_, dbglogger_);

    if (l1_size_upper_bound_high_uts_ - l1_size_lower_bound_high_uts_ >= 1.0) {
      l1_norm_factor_high_uts_ = 1.0 / (l1_size_upper_bound_high_uts_ - l1_size_lower_bound_high_uts_);
    } else {
      l1_norm_factor_high_uts_ = 0;
    }
  }
}

void IndicatorHelper::set_mkt_condition_flags() {
  if (avoid_short_market_ == false) {
    if (src_trend_val_ > src_trend_thresh_) {
      avoid_short_market_ = true;
      avoid_short_market_aggressively_ = true;
      stdev_nnbl_switch_ = true;
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " Avoid Short market set "
                               << " SrcTrendVal: " << src_trend_val_ << " SrcTrendThresh: " << src_trend_thresh_
                               << " src_stdev_thresh_: " << src_stdev_val_ << " SrcStdevThresh: " << src_stdev_thresh_
                               << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
    }
  } else if (avoid_short_market_ == true) {
    if (src_trend_val_ < src_trend_keep_thresh_ && src_stdev_val_ < src_stdev_thresh_) {
      avoid_short_market_ = false;
      avoid_short_market_aggressively_ = false;
      stdev_nnbl_switch_ = false;
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " Avoid Short market Unset "
                               << " SrcTrendVal: " << src_trend_val_ << " SrcTrendThresh: " << src_trend_thresh_
                               << " src_stdev_thresh_: " << src_stdev_val_ << " SrcStdevThresh: " << src_stdev_thresh_
                               << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
    }
  }

  if (avoid_long_market_ == false) {
    if (src_trend_val_ < -src_trend_thresh_) {
      avoid_long_market_ = true;
      avoid_long_market_aggressively_ = true;
      stdev_nnbl_switch_ = true;
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " Avoid Long market set "
                               << " SrcTrendVal: " << src_trend_val_ << " SrcTrendThresh: " << src_trend_thresh_
                               << " src_stdev_thresh_: " << src_stdev_val_ << " SrcStdevThresh: " << src_stdev_thresh_
                               << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
    }
  } else if (avoid_long_market_ == true) {
    if (src_trend_val_ > -src_trend_keep_thresh_ && src_stdev_val_ < src_stdev_thresh_) {
      avoid_long_market_ = false;
      avoid_long_market_aggressively_ = false;
      stdev_nnbl_switch_ = false;
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " Avoid Long market Unset "
                               << " SrcTrendVal: " << src_trend_val_ << " SrcTrendThresh: " << src_trend_thresh_
                               << " src_stdev_thresh_: " << src_stdev_val_ << " SrcStdevThresh: " << src_stdev_thresh_
                               << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
    }
  }
}
}
