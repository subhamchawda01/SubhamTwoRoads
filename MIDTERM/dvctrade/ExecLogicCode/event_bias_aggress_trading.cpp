/**
   \file ExecLogicCode/event_directional_agrressive_trading.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include "dvctrade/ExecLogic/event_bias_aggress_trading.hpp"
// exec_logic_code / defines.hpp was empty
#define EVENT_FAT_FINGER_FACTOR \
  50  // the predicted price change is assuemd to be atmost EVENT_FAT_FINGER_FACTOR * stdev(product). If it is more than
      // that,
      // the msg is assumed to be junk
#define STDEV_NUM_DAYS_HISTORY 60
#define ORDERPLACE_TIMEOUT 200

namespace HFSAT {

EventBiasAggressiveTrading::EventBiasAggressiveTrading(
    DebugLogger &_dbglogger_, const Watch &_watch_, SecurityMarketView &_dep_market_view_,
    SmartOrderManager &_order_manager_, const std::string &_paramfilename_, const bool _livetrading_,
    MulticastSenderSocket *_p_strategy_param_sender_socket_, EconomicEventsManager &t_economic_events_manager_,
    const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int t_runtime_id_,
    const std::vector<std::string> _this_model_source_shortcode_vec_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_),
      event_pr_st_msecs_(10000),
      moving_sumpx_pr_(0),
      count_px_pr_(0),
      avgpx_pr_(0),
      max_pxch_(0),
      dd_margin_(0),
      max_source_pxch_(0),
      source_dd_margin_(0),
      use_source_trend_(false),
      source_moving_sumpx_pr_(0),
      source_avgpx_pr_(0),
      source_pxch_pred_(0),
      source_pred_sign_(0),
      source_price_indicator_(NULL),
      use_source_trade_(false),
      source_trade_secs_(0.5),
      source_trade_index_(10),
      source_trade_indicator_(NULL),
      event_signal_(0),
      prev_event_signal_(0),
      pxch_pred_(0),
      pred_sign_(0),
      is_active_(false),
      af_feeds_recv_(false),
      position_to_take_(0),
      VAL_EPSILON(VAL_EPSILON_INT * dep_market_view_.min_price_increment()),
      last_getflat_mfm_(0),
      last_bidorder_msecs_(0),
      last_askorder_msecs_(0) {
  // economic_events_manager_.SetComputeTradability ( true ) ;
  // economic_events_manager_.GetTradedEventsForToday();
  // is_event_based_ = true;
  DBGLOG_TIME_CLASS_FUNC << "In EventBiasAggressiveTrading contructor" << DBGLOG_ENDL_FLUSH;
  event_.getflat_margin_ = 1000 * dep_market_view_.min_price_increment();
  event_.max_uts_pxch_ = 1000 * dep_market_view_.min_price_increment();
  event_.order_scale_ = 1;

  AF_MSGSPECS::AF_MsgParser &af_msgparser_ = AF_MSGSPECS::AF_MsgParser::GetUniqueInstance(&dbglogger_);
  catg_ = af_msgparser_.getCategoryforId((uint16_t)param_set_.af_event_id_);
  if (catg_ == NULL) {
    DBGLOG_TIME_CLASS_FUNC << "No Category found for Id: " << param_set_.af_event_id_ << DBGLOG_ENDL_FLUSH;
    exit(1);
  }
  event_.cat_id_ = catg_->category_id_;

  dep_stdev_ = SampleDataUtil::GetAvgForPeriod(dep_market_view_.shortcode(), watch_.YYYYMMDD(), STDEV_NUM_DAYS_HISTORY,
                                               trading_start_utc_mfm_, trading_end_utc_mfm_, "STDEV", false);

  dep_decayed_trade_info_manager_ =
      TimeDecayedTradeInfoManager::GetUniqueInstance(_dbglogger_, _watch_, _dep_market_view_, 1.0);
  dep_decayed_trade_info_manager_->compute_sumpxsz();
  dep_decayed_trade_info_manager_->compute_sumsz();

  if (param_set_.af_source_shc_ != "") {
    DBGLOG_TIME_CLASS_FUNC << "Using Source Trend for " << param_set_.af_source_shc_ << DBGLOG_ENDL_FLUSH;
    use_source_trend_ = true;
    SecurityMarketView &source_market_view_ =
        *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(param_set_.af_source_shc_));

    source_price_indicator_ =
        SimplePriceType::GetUniqueInstance(dbglogger_, watch_, source_market_view_, kPriceTypeMidprice);

    source_decayed_trade_info_manager_ =
        TimeDecayedTradeInfoManager::GetUniqueInstance(_dbglogger_, _watch_, source_market_view_, 1.0);
    source_decayed_trade_info_manager_->compute_sumpxsz();
    source_decayed_trade_info_manager_->compute_sumsz();

    if (param_set_.af_use_source_trade_) {
      DBGLOG_TIME_CLASS_FUNC << "Using Source Trade (TDSumTDiffTSize) for " << param_set_.af_source_shc_
                             << DBGLOG_ENDL_FLUSH;

      use_source_trade_ = true;
      source_trade_indicator_ = TDSumTDiffTSize::GetUniqueInstance(
          dbglogger_, watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(param_set_.af_source_shc_)),
          source_trade_secs_, kPriceTypeMidprice);
      source_trade_indicator_->add_unweighted_indicator_listener(source_trade_index_, this);
    }
  }

  readEventsScale();  // reads the lm-beta (scale_factor) for the event
  readMarginFile();   // reads the the maximum pxchange in reverse direction for the product and the scale for dynamic
                      // order size
  readEstimateEvents();  // reads the estimate values for the event

  /*  Position scaling for staggered getflat */
  if (param_set_.af_staggered_getflat_mins_.size() > 0) {
    std::sort(param_set_.af_staggered_getflat_mins_.begin(), param_set_.af_staggered_getflat_mins_.end());
    unsigned staggered_size_ = param_set_.af_staggered_getflat_mins_.size();

    for (unsigned t_indx_ = 0; t_indx_ < staggered_size_; t_indx_++) {
      staggered_getflat_mfms_.push_back(param_set_.af_staggered_getflat_mins_[t_indx_] * 60000);

      double pos_scaling_ = (staggered_size_ - t_indx_) / (staggered_size_ + 1.0);
      staggered_getflat_maxpos_.push_back((int)(param_set_.max_position_ * pos_scaling_));

      DBGLOG_TIME_CLASS_FUNC << " Staggered Getflat: msecs_passed: " << staggered_getflat_mfms_[t_indx_]
                             << ", maxposition: " << staggered_getflat_maxpos_[t_indx_] << DBGLOG_ENDL_FLUSH;
    }
  }

  /*  calc the decay factors for all durations */
  double val_decay_factor_ = MathUtils::CalcDecayFactor((int)param_set_.af_event_halflife_secs_);
  double t_decay_val_ = 1;
  for (unsigned i = 0; i < VOLUME_DECAY_MAX_LEN; i++) {
    val_decay_vec_.push_back(t_decay_val_);
    t_decay_val_ *= val_decay_factor_;
  }

  /*  calc the decay factors for all durations */
  val_decay_factor_ = MathUtils::CalcDecayFactor((int)param_set_.af_event_drawdown_secs_);
  if (param_set_.af_event_drawdown_secs_ < 0 && use_source_trend_) {
    val_decay_factor_ = MathUtils::CalcDecayFactor(100);
  }
  t_decay_val_ = 1;
  for (unsigned i = 0; i < VOLUME_DECAY_MAX_LEN; i++) {
    drawdown_decay_vec_.push_back(t_decay_val_);
    t_decay_val_ *= val_decay_factor_;
  }

  last_viewed_msecs_ = -1;

  /*  subscribe to 100msecs timeperiod to regularly call the ProcessSignalUpdate in the unlikely event that marketupdate
   * is not called */
  watch_.subscribe_TimePeriod(this);

  /*  subscribe to Alphaflash feeds  */
  CombinedMDSMessagesAflashProcessor *aflash_mds_processor_ =
      HFSAT::CombinedMDSMessagesAflashProcessor::GetUniqueInstance(dbglogger_);
  aflash_mds_processor_->AddAflashListener((AflashListener *)this);

  /* Setup the params for Aggress Getflat and disable GetFlat by FOK */
  getflat_aggress_on_dd_ = false;
  getflatfokmode_ = false;
  getflat_yet_ = false;
}

void EventBiasAggressiveTrading::OnControlUpdate(const ControlMessage &control_message, const char *symbol,
                                                 const int trader_id) {
  switch (control_message.message_code_) {
    case kControlMessageReloadAfEstimates: {
      DBGLOG_TIME_CLASS << "BaseTrading::Got reloadafestimates user_msg " << DBGLOG_ENDL_FLUSH;
      event_.estimate_vals_.clear();
      readEstimateEvents();
    } break;
    default: { BaseTrading::OnControlUpdate(control_message, symbol, trader_id); } break;
  }
}

void EventBiasAggressiveTrading::ProcessSignalUpdate(const int num_pages_to_add_) {
  /*  If the source indicator for trend is NOT ready Yet, return */
  if (!dep_market_view_.is_ready_ || (use_source_trend_ && !source_price_indicator_->IsIndicatorReady())) {
    return;
  }

  int msecs_to_event_ = event_.event_mfm_ - watch_.msecs_from_midnight();

  /*  if event is yet to occur then update the prior price computation */
  bool t_is_ready_ = true;
  if (msecs_to_event_ >= 0 && !is_active_) {
    if (msecs_to_event_ < event_pr_st_msecs_) {
      moving_sumpx_pr_ += best_nonself_mid_price_;
      if (use_source_trend_) {
        source_moving_sumpx_pr_ += source_price_indicator_->indicator_value(t_is_ready_);
      }
      count_px_pr_ += 1;
    } else {
      moving_sumpx_pr_ = best_nonself_mid_price_;
      if (use_source_trend_) {
        source_moving_sumpx_pr_ = source_price_indicator_->indicator_value(t_is_ready_);
      }
      count_px_pr_ = 1;
    }
  }

  /*  event has occured */
  else if (is_active_)  // changed to else,come to this part only when the message has been received and the event has
                        // occurred
  {
    int secs_passed_since_ = std::max(0, (int)((watch_.msecs_from_midnight() - event_.mfm_received_) / 1000));
    int decay_secs_ = std::min(secs_passed_since_, VAL_DECAY_MAX_LEN - 1);

    /*  either the first time here OR new updates exist */
    if (last_viewed_msecs_ == -1 || num_pages_to_add_ > 0) {
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " Curr_Price: " << best_nonself_bid_price_ << "," << best_nonself_ask_price_
                               << "; Prior_Price: " << avgpx_pr_ << ", Predicted_PriceChange "
                               << pxch_pred_ *val_decay_vec_[decay_secs_] << " Decay " << val_decay_vec_[decay_secs_]
                               << DBGLOG_ENDL_FLUSH;
      }

      double pxch_act_ = 0, pxch_rem_ = 0, curr_dd_margin_ = -1;
      bool pxch_in_pred_direction_ = true;

      double source_pxch_ = 0, curr_source_dd_margin_ = -1;
      bool source_in_pred_direction_ = true;

      pxch_act_ = best_nonself_mid_price_ - avgpx_pr_;  // pricechange_actual
      pxch_in_pred_direction_ = pxch_act_ * pred_sign_ > 0;

      pxch_rem_ = pxch_pred_ - pxch_act_;  // price_change_expected - price_change_observed_yet
      double bid_ask_spread_half_ = (best_nonself_ask_price_ - best_nonself_bid_price_) / 2;
      if (pred_sign_ > 0) {
        pxch_rem_ -= (bid_ask_spread_half_ + current_tradevarset_.l1bid_aggressive_);
      } else {
        pxch_rem_ += (bid_ask_spread_half_ + current_tradevarset_.l1bid_aggressive_);
      }

      double pxch_tr_decayed_ = pxch_act_;
      if (dep_decayed_trade_info_manager_->sumsz_ > 0) {
        pxch_tr_decayed_ =
            (dep_decayed_trade_info_manager_->sumpxsz_ / dep_decayed_trade_info_manager_->sumsz_) - avgpx_pr_;
      }

      if (param_set_.af_event_drawdown_secs_ > 0) {
        curr_dd_margin_ = dd_margin_ * std::max(0.3, drawdown_decay_vec_[decay_secs_]);
      }

      if (pred_sign_ * pxch_tr_decayed_ > fabs(max_pxch_)) {
        max_pxch_ = pxch_tr_decayed_;
      }
      // DBGLOG_TIME_CLASS_FUNC << " drawdown: " << fabs( max_pxch_ - pxch_act_)
      //<< ", margin: " << curr_dd_margin_ << DBGLOG_ENDL_FLUSH;

      if (use_source_trend_) {
        source_pxch_ = source_price_indicator_->indicator_value(t_is_ready_) - source_avgpx_pr_;
        source_in_pred_direction_ = source_pxch_ * source_pred_sign_ > 0;

        curr_source_dd_margin_ = source_dd_margin_ * drawdown_decay_vec_[decay_secs_];
        curr_source_dd_margin_ = std::max(curr_source_dd_margin_, fabs(0.3 * source_pxch_pred_));

        double source_pxch_tr_decayed_ = source_pxch_;
        if (source_decayed_trade_info_manager_->sumsz_ > 0) {
          source_pxch_tr_decayed_ =
              (source_decayed_trade_info_manager_->sumpxsz_ / source_decayed_trade_info_manager_->sumsz_) -
              source_avgpx_pr_;
        }

        if (source_pred_sign_ * source_pxch_tr_decayed_ > fabs(max_source_pxch_)) {
          max_source_pxch_ = source_pxch_tr_decayed_;
        }

        DBGLOG_TIME_CLASS_FUNC << " max_source_pxch_: " << max_source_pxch_ << " source_pxch_: " << source_pxch_
                               << " curr_source_dd_margin_ : " << curr_source_dd_margin_ << DBGLOG_ENDL_FLUSH;
      }

      if (getflat_due_to_lpm_ && pxch_in_pred_direction_ && source_in_pred_direction_) {
        getflat_due_to_lpm_ = false;
      }

      /* both pxch_act_ & pxch_pred_ are in same direction
       * OR magnitude of pxchange_actual is small
       * AND drawdown is within margin
       * AND source drawdown is within margin */
      if ((pxch_in_pred_direction_ || fabs(pxch_act_) < event_.getflat_margin_) &&
          (curr_dd_margin_ < 0 || pred_sign_ * (max_pxch_ - pxch_act_) < curr_dd_margin_) &&
          (curr_source_dd_margin_ < 0 ||
           source_pred_sign_ * (max_source_pxch_ - source_pxch_) < curr_source_dd_margin_)) {
        /*  pxch_act_ doesn't offshoot the predicted price */
        if (pxch_rem_ * pred_sign_ > 0 && secs_passed_since_ < VAL_DECAY_MAX_LEN && !getflat_yet_) {
          event_signal_ = pxch_pred_ * val_decay_vec_[decay_secs_];

          if (use_source_trade_) {
            double aflash_signal_weight_ = (decay_secs_ < 2) ? 1 : ((decay_secs_ < 5) ? 0.5 : 0);
            event_signal_ = event_signal_ * aflash_signal_weight_ +
                            (1 - aflash_signal_weight_) * source_trade_indicator_->indicator_value(t_is_ready_);
          }
        } else {
          if (event_signal_ != 0) {
            if (pxch_rem_ * pxch_pred_ <= 0) {
              DBGLOG_TIME_CLASS_FUNC << " Movement Overshooted the Predicted PriceChange. Placing no more orders"
                                     << DBGLOG_ENDL_FLUSH;
            }
            event_signal_ = 0;
          }
        }
      }

      else {  // this is the worst possible scenario.We should close all trades immediately
        event_signal_ = 0;
        if (!getflat_due_to_lpm_) {
          DBGLOG_TIME_CLASS_FUNC << " Movement in Opposite direction: pricechange_actual: " << pxch_act_
                                 << ", getflat margin: " << event_.getflat_margin_ << ", max_pxch: " << max_pxch_
                                 << ", drawdown margin:" << curr_dd_margin_ << ", Getting flat.." << DBGLOG_ENDL_FLUSH;
          getflat_due_to_lpm_ = true;
          getflat_yet_ = true;
          ProcessGetFlat();
        }
      }

      /*  if event_signal is very tiny then reset it to 0 */
      if (event_signal_ > 0 && fabs(event_signal_) < VAL_EPSILON) {
        DBGLOG_TIME_CLASS_FUNC << " Signal is very tiny. Resetting it to 0" << DBGLOG_ENDL_FLUSH;
        event_signal_ = 0;
      }
      last_viewed_msecs_ = watch_.msecs_from_midnight();
    }
  }

  if (!getflat_due_to_close_ && !getflat_due_to_lpm_ && !getflat_due_to_external_getflat_ &&
      (throttle_manager_->allowed_through_throttle(watch_.msecs_from_midnight()))) {
    if (abs(my_position_) >= param_set_.max_position_) {
      int position_to_close_ =
          (my_position_ < 0) ? (my_position_ + param_set_.max_position_) : (my_position_ - param_set_.max_position_);

      GetFlatTradingLogic(position_to_close_);
    } else {
      TradingLogic();
    }
  } else {
    GetFlatFokTradingLogic();
  }
}

/*  Called every 100msecs */
void EventBiasAggressiveTrading::ProcessTimePeriodUpdate(const int num_pages_to_add_) {
  if (is_active_) {
    int msecs_passed_ = watch_.msecs_from_midnight() - event_.mfm_received_;
    for (auto it = staggered_getflat_mfms_.begin(); it != staggered_getflat_mfms_.end(); it++) {
      int t_position_ = staggered_getflat_maxpos_[it - staggered_getflat_mfms_.begin()];
      if (msecs_passed_ > *it && param_set_.max_position_ > t_position_) {
        DBGLOG_TIME_CLASS_FUNC << " Updating the MaxPos from " << param_set_.max_position_ << " to " << t_position_
                               << DBGLOG_ENDL_FLUSH;
        param_set_.max_position_ = t_position_;
      }
    }
  }
  ProcessGetFlat();
  ProcessSignalUpdate(num_pages_to_add_);

  /* No need to call BaseTrading::ProcessTimePeriodUpdate ( )
   * updates the several params which are of little consequence in this execlogic */
  // BaseTrading::ProcessTimePeriodUpdate ( num_pages_to_add_ );
}

void EventBiasAggressiveTrading::OnMarketUpdate(const unsigned int _security_id_,
                                                const MarketUpdateInfo &_market_update_info_) {
  ProcessSignalUpdate(1);

  /* Called the base_trading function as it calls the NonSelfMarketUpdate ( ) */
  BaseTrading::OnMarketUpdate(_security_id_, _market_update_info_);
}

void EventBiasAggressiveTrading::OnTradePrint(const unsigned int _security_id_,
                                              const TradePrintInfo &_trade_print_info_,
                                              const MarketUpdateInfo &_market_update_info_) {
  ProcessSignalUpdate(1);

  /* Called the base_trading function as it calls the NonSelfMarketUpdate ( ) */
  BaseTrading::OnTradePrint(_security_id_, _trade_print_info_, _market_update_info_);
}

void EventBiasAggressiveTrading::TradingLogic() {
  top_ask_lift_ = false;
  top_bid_hit_ = false;

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO) &&  // fabs(event_signal_) > 0) &&
      fabs(event_signal_ - prev_event_signal_) > 0) {
    DBGLOG_TIME_CLASS_FUNC << " Signal: " << event_signal_ << ", l1bid: " << current_tradevarset_.l1bid_aggressive_
                           << ", l1ask: " << current_tradevarset_.l1ask_aggressive_
                           << ", best_bid: " << best_nonself_bid_price_ << ", best_ask: " << best_nonself_ask_price_
                           << ", maxposl: " << param_set_.max_position_to_lift_
                           << ", maxposh: " << param_set_.min_position_to_hit_ << " last_msecs: " << last_buy_msecs_
                           << " " << last_sell_msecs_ << DBGLOG_ENDL_FLUSH;
    prev_event_signal_ = event_signal_;
  }

  int num_levels_ = 0;

  if (best_nonself_ask_price_ <= best_nonself_bid_price_) {
    DBGLOG_TIME_CLASS_FUNC << " BidAsk Spread <= 0 best_bid: " << best_nonself_bid_price_
                           << ", best_ask: " << best_nonself_ask_price_ << DBGLOG_ENDL_FLUSH;
  } else if ((event_signal_ > 0) && (event_signal_ > current_tradevarset_.l1bid_aggressive_ +
                                                         (best_nonself_ask_price_ - best_nonself_bid_price_)) &&
             (my_position_ <=
              param_set_.max_position_to_lift_) && /*  Don't LIFT offer when my_position_ is already decently long */
             (watch_.msecs_from_midnight() - last_bidorder_msecs_ >= ORDERPLACE_TIMEOUT) &&
             ((last_buy_msecs_ <= 0) || /* either the position is short OR last_buy is past the cooloff interval */
              (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_))
             //          ( ( best_nonself_ask_int_price_ - last_buy_int_price_ ) < ( param_set_.px_band_ - 0 ) ) ) //
             //          (OR the
             //          the price to aggress hasn't moved a lot)
             ) {
    top_ask_lift_ = true;
    num_levels_ = (int)((event_signal_ - (current_tradevarset_.l1bid_aggressive_ +
                                          (best_nonself_ask_price_ - best_nonself_bid_price_))) /
                        dep_market_view_.min_price_increment());
    num_levels_ = (int)(0.3 * num_levels_);  // To place agg order atleast a little farther from best
  } else if ((event_signal_ < 0) && (-event_signal_ > current_tradevarset_.l1ask_aggressive_ +
                                                          (best_nonself_ask_price_ - best_nonself_bid_price_)) &&
             (my_position_ >=
              param_set_.min_position_to_hit_) && /*  Don't HIT offer when my_position_ is already decently short */
             (watch_.msecs_from_midnight() - last_askorder_msecs_ >= ORDERPLACE_TIMEOUT) &&
             ((last_sell_msecs_ <= 0) || /*  either the position is long OR last_buy is past the cooloff interval */
              (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_))
             //          ( ( last_sell_int_price_ - best_nonself_bid_int_price_ ) < ( param_set_.px_band_ - 0 ) ) )
             ) {
    top_bid_hit_ = true;
    num_levels_ = (int)((-event_signal_ - (current_tradevarset_.l1ask_aggressive_ +
                                           (best_nonself_ask_price_ - best_nonself_bid_price_))) /
                        dep_market_view_.min_price_increment());
    num_levels_ = (int)(0.3 * num_levels_);  // To place agg order atleast a little farther from best
  }

  /* After setting top-level directives ...
   * get to order placing or canceling part */

  /* Active BID order management */
  placed_bids_this_round_ = false;
  if (top_ask_lift_) {
    int aggress_signal_size_ = MathUtils::GetFlooredMultipleOf(std::max(0, position_to_take_ - my_position_),
                                                               dep_market_view_.min_order_size());

    /*  Update the ordersize left to be placed */
    int aggress_signal_size_left_ =
        aggress_signal_size_ - order_manager_.GetTotalBidSizeAboveIntPx(best_nonself_bid_int_price_);

    double aggress_int_price_ = best_nonself_ask_int_price_;
    if (param_set_.af_use_mult_levels_) {
      aggress_int_price_ += num_levels_;
    }

    while (aggress_signal_size_left_ > 0) {
      int aggress_buy_size_ = std::min(current_tradevarset_.l1bid_trade_size_,
                                       std::max(aggress_signal_size_left_, dep_market_view_.min_order_size()));
      //      DBGLOG_TIME_CLASS_FUNC << "aggress_debug: " << aggress_buy_size_ << ", " << aggress_signal_size_left_ <<
      //      ", " << aggress_signal_size_ << ", " << order_manager_.SumBidSizes() << DBGLOG_ENDL_FLUSH;
      int _canceled_size_ = 0;

      /*  if Worst_case_position is 0, then cancel the existing Inactive ASK orders */
      if (param_set_.worst_case_position_ == 0) {
        _canceled_size_ += order_manager_.CancelBidsEqBelowIntPrice(best_nonself_bid_int_price_);

        int sum_inactive_bid_sizes_ =
            order_manager_.SumBidSizes() - order_manager_.GetTotalBidSizeAboveIntPx(best_nonself_bid_int_price_) -
            order_manager_.SumBidSizeCancelRequestedInRange(
                best_nonself_bid_int_price_, order_manager_.GetBidIntPrice(order_manager_.GetOrderVecBottomBidIndex()));

        if (sum_inactive_bid_sizes_ > 0) {
          aggress_buy_size_ = 0;
        }
      }
      /*  Else, cancel, if so required, any orders from bottom */
      else {
        int allowance_for_aggressive_buy_ = my_position_ + order_manager_.SumBidSizes() + aggress_buy_size_ -
                                            std::min(param_set_.worst_case_position_, param_set_.max_position_);

        if (allowance_for_aggressive_buy_ > 0) {
          //          DBGLOG_TIME_CLASS_FUNC << "Cancelling Bid orders from far, size: " << aggress_buy_size_ <<
          //          DBGLOG_ENDL_FLUSH;
          _canceled_size_ += order_manager_.CancelBidsFromFar(
              aggress_buy_size_);  ///< cancel Asks from bottom levels for the required size

          if (allowance_for_aggressive_buy_ > _canceled_size_) {
            aggress_buy_size_ = 0;
          }
        }
      }
      /* Place new order */
      if (aggress_buy_size_ != 0) {
        order_manager_.SendTradeIntPx(aggress_int_price_, aggress_buy_size_, kTradeTypeBuy, 'A');
        DBGLOG_TIME_CLASS_FUNC << "Sending aggressive B of size: " << aggress_buy_size_
                               << " at int_px: " << aggress_int_price_ << " position " << my_position_
                               << " cancelled size " << _canceled_size_ << DBGLOG_ENDL_FLUSH;
        placed_bids_this_round_ = true;
        last_bidorder_msecs_ = watch_.msecs_from_midnight();
      }
      /*  Else, stop placing orders alltogether  */
      else {
        break;
      }
      /*  Update the ordersize left to be placed */
      aggress_signal_size_left_ -=
          aggress_buy_size_;  // order_manager_.GetTotalBidSizeEqAboveIntPx(best_nonself_ask_int_price_);
    }
  }

  /* Active ASK order management */
  placed_asks_this_round_ = false;
  if (top_bid_hit_) {
    int aggress_signal_size_ = MathUtils::GetFlooredMultipleOf(std::max(0, position_to_take_ + my_position_),
                                                               dep_market_view_.min_order_size());

    /*  Update the ordersize left to be placed */
    int aggress_signal_size_left_ =
        aggress_signal_size_ - order_manager_.GetTotalAskSizeAboveIntPx(best_nonself_ask_int_price_);

    double aggress_int_price_ = best_nonself_bid_int_price_;
    if (param_set_.af_use_mult_levels_) {
      aggress_int_price_ -= num_levels_;
    }

    while (aggress_signal_size_left_ > 0) {
      int aggress_sell_size_ = std::min(current_tradevarset_.l1ask_trade_size_,
                                        std::max(aggress_signal_size_left_, dep_market_view_.min_order_size()));
      //      DBGLOG_TIME_CLASS_FUNC << "aggress_debug: " << aggress_sell_size_ << ", " << aggress_signal_size_left_ <<
      //      ", " << aggress_signal_size_ << ", " << order_manager_.SumAskSizes() << DBGLOG_ENDL_FLUSH;

      int _canceled_size_ = 0;
      /*   if Worst_case_position is 0, then cancel all existing Inactive ASK orders */
      if (param_set_.worst_case_position_ == 0) {
        _canceled_size_ += order_manager_.CancelAsksEqBelowIntPrice(best_nonself_ask_int_price_);

        int sum_inactive_ask_sizes_ =
            order_manager_.SumAskSizes() - order_manager_.GetTotalAskSizeAboveIntPx(best_nonself_ask_int_price_) -
            order_manager_.SumAskSizeCancelRequestedInRange(
                best_nonself_ask_int_price_, order_manager_.GetAskIntPrice(order_manager_.GetOrderVecBottomAskIndex()));

        if (sum_inactive_ask_sizes_ > 0) {
          aggress_sell_size_ = 0;
        }
      }
      /*  Else, cancel, if so required, any orders from bottom */
      else {
        int allowance_for_aggressive_sell_ = -my_position_ + order_manager_.SumAskSizes() + aggress_sell_size_ -
                                             std::min(param_set_.worst_case_position_, param_set_.max_position_);

        if (allowance_for_aggressive_sell_ > 0) {
          //          DBGLOG_TIME_CLASS_FUNC << "Cancelling Ask orders from far, size: " << aggress_sell_size_ <<
          //          DBGLOG_ENDL_FLUSH;
          _canceled_size_ += order_manager_.CancelAsksFromFar(
              aggress_sell_size_);  ///< cancel Asks from bottom levels for the required size

          if (allowance_for_aggressive_sell_ > _canceled_size_) {
            aggress_sell_size_ = 0;
          }
        }
      }
      /* If ordersize > 0, Place new order */
      if (aggress_sell_size_ != 0) {
        order_manager_.SendTradeIntPx(aggress_int_price_, aggress_sell_size_, kTradeTypeSell, 'A');
        DBGLOG_TIME_CLASS_FUNC << "Sending aggressive S of size: " << aggress_sell_size_
                               << " at int_px: " << aggress_int_price_ << " position " << my_position_
                               << " cancelled size: " << _canceled_size_ << DBGLOG_ENDL_FLUSH;
        placed_asks_this_round_ = true;
        last_askorder_msecs_ = watch_.msecs_from_midnight();
      }
      /*  Else, stop placing orders alltogether  */
      else {
        break;
      }
      /*  Update the ordersize left to be placed */
      aggress_signal_size_left_ -=
          aggress_sell_size_;  // order_manager_.GetTotalAskSizeEqAboveIntPx(best_nonself_bid_int_price_);
    }
  }

  // zero logging
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO) && (placed_bids_this_round_ || placed_asks_this_round_)) {
    PrintFullStatus();
  }

  if ((livetrading_ || dbglogger_.CheckLoggingLevel(TRADING_INFO)) &&
      (placed_bids_this_round_ || placed_asks_this_round_)) {
    dump_inds = true;
  }
}

void EventBiasAggressiveTrading::GetFlatTradingLogic() {
  // only passive execution in getting out
  GetFlatTradingLogic(GetPositionToClose());
}

void EventBiasAggressiveTrading::GetFlatTradingLogic(int t_position_) {
  if (t_position_ == 0) {  // nothing to be done, cancel all remaining orders
    order_manager_.CancelAllOrders();
  } else if (t_position_ > 0) {
    // long hence cancel all bid orders
    order_manager_.CancelAllBidOrders();

    DBGLOG_TIME_CLASS_FUNC << "agg: " << getflat_aggress_on_dd_ << ", " << param_set_.max_spread_getflat_aggress_
                           << ", " << param_set_.max_size_to_aggress_ << DBGLOG_ENDL_FLUSH;
    bool done_for_this_round_ = false;
    if (getflat_due_to_lpm_ && getflat_aggress_on_dd_ &&
        dep_market_view_.spread_increments() <= param_set_.max_spread_getflat_aggress_ &&
        best_nonself_bid_size_ < param_set_.max_size_to_aggress_) {
      int agg_trade_size_required_ = MathUtils::GetFlooredMultipleOf(std::min(t_position_, param_set_.unit_trade_size_),
                                                                     dep_market_view_.min_order_size());
      int agg_size_ordered_ = order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) +
                              order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_);

      int size_ = agg_trade_size_required_ - agg_size_ordered_;
      if (size_ > 0) {
        SendTradeAndLog(best_nonself_bid_price_, best_nonself_bid_int_price_, size_, kTradeTypeSell, 'A',
                        "GetFlatAggSendTrade");
        done_for_this_round_ = true;
      }
    }

    if (!done_for_this_round_) {
      // haven't agressed or places any orders in this round
      int eqabove_best_size_ordered_ = order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) +
                                       order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_);

      if (getflat_mult_ord_) {
        // getflat by placing multiple orders
        int total_order_placed_ = 0;
        if (eqabove_best_size_ordered_ < t_position_) {
          while (total_order_placed_ < t_position_ - eqabove_best_size_ordered_ &&
                 total_order_placed_ + eqabove_best_size_ordered_ < max_orders_ * param_set_.unit_trade_size_) {
            int this_trade_size_ = MathUtils::GetFlooredMultipleOf(
                std::min(t_position_ - eqabove_best_size_ordered_ - total_order_placed_, param_set_.unit_trade_size_),
                dep_market_view_.min_order_size());
            order_manager_.SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_, this_trade_size_,
                                     kTradeTypeSell, 'B');
            if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
              DBGLOG_TIME_CLASS_FUNC << "GetFlatSendMultTrade S of " << this_trade_size_ << " @ "
                                     << best_nonself_ask_price_ << " IntPx: " << best_nonself_ask_int_price_
                                     << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                     << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
            }

            total_order_placed_ += this_trade_size_;
          }
        }
      } else {
        int trade_size_required_ = MathUtils::GetFlooredMultipleOf(std::min(t_position_, param_set_.unit_trade_size_),
                                                                   dep_market_view_.min_order_size());

        if (eqabove_best_size_ordered_ < trade_size_required_) {  // if the required size is not present then add more
          order_manager_.SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                   trade_size_required_ - eqabove_best_size_ordered_, kTradeTypeSell, 'B');

          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "GetFlatSendTrade S of " << trade_size_required_ - eqabove_best_size_ordered_
                                   << " @ " << best_nonself_ask_price_ << " IntPx: " << best_nonself_ask_int_price_
                                   << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                   << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
        // not doing anything if the placed size is more than required // well u can send a replace
      }
    }

    // instead of cancelling all non-best orders, cancel just enough to avoid overfill
    int t_total_asks_placed_ = order_manager_.SumAskSizes();
    if (!getflat_due_to_max_position_ && t_total_asks_placed_ > t_position_) {
      order_manager_.KeepAskSizeInPriceRange(t_position_);
    }
    if (getflat_due_to_max_position_ && (t_position_ - t_total_asks_placed_ < param_set_.max_position_)) {
      order_manager_.KeepAskSizeInPriceRange(t_position_ - param_set_.max_position_);
    }
  } else {  // my_position_ < 0
    // short hence cancel all sell orders
    order_manager_.CancelAllAskOrders();

    DBGLOG_TIME_CLASS_FUNC << "agg: " << getflat_aggress_on_dd_ << ", " << param_set_.max_spread_getflat_aggress_
                           << ", " << param_set_.max_size_to_aggress_ << DBGLOG_ENDL_FLUSH;
    bool done_for_this_round_ = false;
    if (getflat_due_to_lpm_ && getflat_aggress_on_dd_ &&
        dep_market_view_.spread_increments() <= param_set_.max_spread_getflat_aggress_ &&
        best_nonself_ask_size_ < param_set_.max_size_to_aggress_) {
      // aggress after proper checks
      int agg_trade_size_required_ = MathUtils::GetFlooredMultipleOf(
          std::min(-t_position_, param_set_.unit_trade_size_), dep_market_view_.min_order_size());
      int agg_size_ordered_ = order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) +
                              order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_);

      int size_ = agg_trade_size_required_ - agg_size_ordered_;
      if (size_ > 0) {
        SendTradeAndLog(best_nonself_ask_price_, best_nonself_ask_int_price_, size_, kTradeTypeBuy, 'A',
                        "GetFlatAggSendTrade");
        done_for_this_round_ = true;
      }
    }

    if (!done_for_this_round_) {
      // haven't agressed or places any orders in this round
      int eqabove_best_size_ordered_ = order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) +
                                       order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_);

      if (getflat_mult_ord_) {
        int total_order_placed_ = 0;
        if (eqabove_best_size_ordered_ < -t_position_) {
          while (total_order_placed_ < -t_position_ - eqabove_best_size_ordered_ &&
                 total_order_placed_ + eqabove_best_size_ordered_ < max_orders_ * param_set_.unit_trade_size_) {
            int this_trade_size_ = MathUtils::GetFlooredMultipleOf(
                std::min(-t_position_ - eqabove_best_size_ordered_ - total_order_placed_, param_set_.unit_trade_size_),
                dep_market_view_.min_order_size());
            order_manager_.SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_, this_trade_size_,
                                     kTradeTypeBuy, 'B');
            if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
              DBGLOG_TIME_CLASS_FUNC << "GetFlatSendMultTrade B of " << this_trade_size_ << " @ "
                                     << best_nonself_bid_price_ << " IntPx: " << best_nonself_bid_int_price_
                                     << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                     << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
            }

            total_order_placed_ += this_trade_size_;
          }
        }
      } else {
        int trade_size_required_ = MathUtils::GetFlooredMultipleOf(std::min(-t_position_, param_set_.unit_trade_size_),
                                                                   dep_market_view_.min_order_size());

        if (eqabove_best_size_ordered_ < trade_size_required_) {  // if the required size is not present then add more
          order_manager_.SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                   trade_size_required_ - eqabove_best_size_ordered_, kTradeTypeBuy, 'B');

          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "GetFlatSendTrade B of " << trade_size_required_ - eqabove_best_size_ordered_
                                   << " @ " << best_nonself_bid_price_ << " IntPx: " << best_nonself_bid_int_price_
                                   << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                   << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
        // not doing anything if the placed size is more than required
      }
      // instead of cancelling all non-best orders cancel, just enough to avoid overfill
      int t_total_bids_placed_ = order_manager_.SumBidSizes();
      if (!getflat_due_to_max_position_ && t_total_bids_placed_ > -t_position_) {
        order_manager_.KeepBidSizeInPriceRange(-t_position_);
      }
      if (getflat_due_to_max_position_ && (-t_position_ - t_total_bids_placed_ < param_set_.max_position_)) {
        order_manager_.KeepBidSizeInPriceRange(-t_position_ - param_set_.max_position_);
      }
    }
  }
}

void EventBiasAggressiveTrading::PrintFullStatus() {
  DBGLOG_TIME << "tgt: " << target_price_ << " ExpBidPft@ " << best_nonself_bid_price_ << " X "
              << best_nonself_bid_size_ << ' ' << (target_price_ - best_nonself_bid_price_) << " ExpAskPft@ "
              << best_nonself_ask_price_ << " X " << best_nonself_ask_size_ << ' '
              << (best_nonself_ask_price_ - target_price_)
              << " signalbias: " << (targetbias_numbers_ / dep_market_view_.min_price_increment())
              << " pos: " << my_position_ << " smaps "
              << order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_) << " "
              << order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) << " "
              << order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) << " "
              << order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_)
              << " opnl: " << order_manager_.base_pnl().opentrade_unrealized_pnl() << " gpos: " << my_global_position_
              << DBGLOG_ENDL_FLUSH;
}

std::vector<std::string> &EventBiasAggressiveTrading::split(const std::string &s, char delim,
                                                            std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

void EventBiasAggressiveTrading::readEventsScale() {
  std::string scale_file_ = "/spare/local/tradeinfo/Alphaflash/af_events_scale_ids2.txt";
  DBGLOG_TIME_CLASS_FUNC << " Reading EventScales from " << scale_file_ << DBGLOG_ENDL_FLUSH;
  std::ifstream scale_read_;
  scale_read_.open(scale_file_.c_str(), std::ifstream::in);

  if (scale_read_.is_open()) {
    std::string line_;
    std::string t_shc_ = dep_market_view_.shortcode();
    const int line_length_ = 1024;
    char readline_buffer_[line_length_];

    /*  Line Format:
     *  <shc> <event_catg_id> <fid1:beta1> <fid2:beta2> .. */
    while (scale_read_.good()) {
      bzero(readline_buffer_, line_length_);
      scale_read_.getline(readline_buffer_, line_length_);
      PerishableStringTokenizer st_(readline_buffer_, line_length_);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() > 2u) {
        if (!param_set_.read_af_scale_beta_ && t_shc_.compare(tokens_[0]) == 0 &&
            atoi(tokens_[1]) == (int)event_.cat_id_) {
          for (unsigned indx_ = 2; indx_ < tokens_.size(); indx_++) {
            std::vector<std::string> t_tokens_;
            split(std::string(tokens_[indx_]), ':', t_tokens_);
            uint8_t fid_ = (uint8_t)stoi(t_tokens_[0]);
            if (t_tokens_.size() < 2) {
              continue;
            }
            if (event_.scale_beta_.find(fid_) != event_.scale_beta_.end()) {
              DBGLOG_TIME_CLASS_FUNC << "Beta for Cat: " << event_.cat_id_ << ", " << fid_
                                     << " Already Present.. Ignoring.." << DBGLOG_ENDL_FLUSH;
              continue;
            }
            event_.scale_beta_[fid_] = stod(t_tokens_[1]);
            event_.cat_datum_.push_back(fid_);
            DBGLOG_TIME_CLASS_FUNC << " Assigned Cat: " << event_.cat_id_ << ", " << fid_ << " : "
                                   << event_.scale_beta_[fid_] << DBGLOG_ENDL_FLUSH;
          }
        } else if (use_source_trend_ && param_set_.af_source_shc_.compare(tokens_[0]) == 0 &&
                   atoi(tokens_[1]) == (int)event_.cat_id_) {
          for (unsigned indx_ = 2; indx_ < tokens_.size(); indx_++) {
            std::vector<std::string> t_tokens_;
            split(std::string(tokens_[indx_]), ':', t_tokens_);
            uint8_t fid_ = (uint8_t)stoi(t_tokens_[0]);
            if (t_tokens_.size() < 2) {
              continue;
            }
            if (source_scale_beta_.find(fid_) != source_scale_beta_.end()) {
              DBGLOG_TIME_CLASS_FUNC << "Beta for Cat: " << event_.cat_id_ << ", " << fid_
                                     << " Already Present.. Ignoring.." << DBGLOG_ENDL_FLUSH;
              continue;
            }
            source_scale_beta_[fid_] = stod(t_tokens_[1]);
            DBGLOG_TIME_CLASS_FUNC << " Assigned Cat: " << event_.cat_id_ << ", " << fid_ << " : "
                                   << source_scale_beta_[fid_] << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
  } else {
    std::cerr << " Error: Scale_File NOT opening: " << scale_file_ << std::endl;
    DBGLOG_TIME_CLASS_FUNC << " Error: Scale_File NOT opening: " << scale_file_ << DBGLOG_ENDL_FLUSH;
    exit(1);
  }

  if (param_set_.read_af_scale_beta_) {
    std::vector<std::string> tokens_;
    split(param_set_.af_scale_beta_, ',', tokens_);

    if (tokens_.size() > 0) {
      event_.scale_beta_.clear();
      DBGLOG_TIME_CLASS_FUNC << "Erasing All Self-Beta values since they are specified in the paramset as well"
                             << DBGLOG_ENDL_FLUSH;
      for (unsigned indx_ = 0; indx_ < tokens_.size(); indx_++) {
        std::vector<std::string> t_tokens_;
        split(std::string(tokens_[indx_]), ':', t_tokens_);
        uint8_t fid_ = (uint8_t)stoi(t_tokens_[0]);
        if (t_tokens_.size() < 2) {
          continue;
        }
        if (event_.scale_beta_.find(fid_) != event_.scale_beta_.end()) {
          DBGLOG_TIME_CLASS_FUNC << "Beta for Cat: " << event_.cat_id_ << ", " << fid_
                                 << " Already Present.. Ignoring.." << DBGLOG_ENDL_FLUSH;
          continue;
        }
        event_.scale_beta_[fid_] = stod(t_tokens_[1]);
        event_.cat_datum_.push_back(fid_);
        DBGLOG_TIME_CLASS_FUNC << " Assigned Cat: " << event_.cat_id_ << ", " << fid_ << " : "
                               << event_.scale_beta_[fid_] << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  if (event_.scale_beta_.empty()) {
    std::cerr << "Error: No Beta value provided for Event: " << event_.cat_id_ << std::endl;
    DBGLOG_TIME_CLASS_FUNC << "Error: No Beta value provided for Event: " << event_.cat_id_ << DBGLOG_ENDL_FLUSH;
    exit(1);
  }
  if (use_source_trend_ && source_scale_beta_.empty()) {
    std::cerr << "Error: No Source Beta value provided for Event: " << event_.cat_id_ << std::endl;
    DBGLOG_TIME_CLASS_FUNC << "Error: No Source Beta value provided for Event: " << event_.cat_id_ << DBGLOG_ENDL_FLUSH;
    exit(1);
  }
}

/* Updating pricechange for getflat (pxch_getflat)
 * pricechange for maximum position (max_uts_pxch)
 * position scaling factor (order_scale) */
void EventBiasAggressiveTrading::readMarginFile() {
  if (param_set_.read_af_event_pxch_getflat_) {
    event_.getflat_margin_ = param_set_.af_event_pxch_getflat_ * dep_market_view_.min_price_increment();
  }
  if (param_set_.read_af_event_max_uts_pxch_) {
    event_.max_uts_pxch_ = param_set_.af_event_max_uts_pxch_ * dep_market_view_.min_price_increment();
    event_.order_scale_ = param_set_.max_position_ / event_.max_uts_pxch_;
  }
  DBGLOG_TIME_CLASS_FUNC << " Assigned event_.getflat_margin_: " << event_.getflat_margin_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME_CLASS_FUNC << " Assigned event_.order_scale_: " << event_.order_scale_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME_CLASS_FUNC << " Assigned event_.max_uts_pxch_: " << event_.max_uts_pxch_ << DBGLOG_ENDL_FLUSH;
}

void EventBiasAggressiveTrading::readEstimateEvents() {
  int date_ = watch_.YYYYMMDD();
  const int SKIP_LIMIT = 10;
  int skipped_ = 0;

  while (skipped_ < SKIP_LIMIT) {
    std::string estimates_file_ =
        std::string("/spare/local/tradeinfo/Alphaflash/Estimates/estimates_") + std::to_string(date_);
    DBGLOG_TIME_CLASS_FUNC << " Reading Estimate Data from " << estimates_file_ << DBGLOG_ENDL_FLUSH;
    std::ifstream estimates_read_;
    estimates_read_.open(estimates_file_.c_str(), std::ifstream::in);
    DBGLOG_TIME_CLASS_FUNC << "Estimates File: " << estimates_file_ << DBGLOG_ENDL_FLUSH;

    bool found_ = false;
    if (estimates_read_.is_open()) {
      std::string line_;
      const int line_length_ = 1024;
      char readline_buffer_[line_length_];

      /*  Line Format:
       *  <shc> <event_catg_id> <fid1:estimate_val1> <fid2:estimate_val2> .. */
      while (estimates_read_.good()) {
        bzero(readline_buffer_, line_length_);
        estimates_read_.getline(readline_buffer_, line_length_);
        PerishableStringTokenizer st_(readline_buffer_, line_length_);
        const std::vector<const char *> &tokens_ = st_.GetTokens();

        if (tokens_.size() >= 3u) {
          uint16_t cat_id_t_ = (uint16_t)atoi(tokens_[0]);

          if (cat_id_t_ != event_.cat_id_) {
            continue;
          }
          found_ = true;
          event_.event_mfm_ = atoi(tokens_[1]);

          for (unsigned indx_ = 2; indx_ < tokens_.size(); indx_++) {
            std::vector<std::string> t_tokens_;
            split(std::string(tokens_[indx_]), ':', t_tokens_);
            uint8_t fid_ = (uint8_t)stoi(t_tokens_[0]);
            if (event_.scale_beta_.find(fid_) != event_.scale_beta_.end()) {
              event_.estimate_vals_[fid_] = stod(t_tokens_[1]);
              DBGLOG_TIME_CLASS_FUNC << "Estimates Value: " << event_.cat_id_ << ", " << fid_ << " : " << t_tokens_[1]
                                     << DBGLOG_ENDL_FLUSH;
            }
          }
        }
      }
    }
    if (found_) {
      break;
    }

    skipped_++;
    date_ = DateTime::CalcPrevDay(date_);
  }

  for (auto it = event_.cat_datum_.begin(); it != event_.cat_datum_.end(); ++it) {
    if (event_.estimate_vals_.find(*it) == event_.estimate_vals_.end()) {
      DBGLOG_TIME_CLASS_FUNC << "Estimate Cat_id_: " << event_.cat_id_ << ", datum_id_: " << (int)*it
                             << " Missing.. Hence ignoring this message" << DBGLOG_ENDL_FLUSH;
      std::cerr << "Estimate Cat_id_: " << event_.cat_id_ << ", datum_id_: " << (int)*it
                << " Missing.. Hence ignoring this message\n";
      exit(1);
    }
  }
}

/*  When An alphaflash msg arrives */
void EventBiasAggressiveTrading::onAflashMsgNew(int uid_, timeval time_, char symbol_[4], uint8_t type_,
                                                uint8_t version_, uint8_t nfields_,
                                                AFLASH_MDS::AFlashDatum fields[AFLASH_MDS::MAX_FIELDS],
                                                uint16_t category_id_) {
  if (!af_feeds_recv_) {
    DBGLOG_TIME_CLASS_FUNC << "Alphaflash Messages Received in EventBiasAggressiveTrading" << DBGLOG_ENDL_FLUSH;
    af_feeds_recv_ = true;
  }

  /*  If currently, inactive and the msg belong to this event and it of Release type */
  if (!is_active_ && category_id_ == event_.cat_id_ && type_ == (uint8_t)AF_MSGSPECS::kRelease) {
    // DBGLOG_TIME_CLASS_FUNC << next_event_->ToString() << DBGLOG_ENDL_FLUSH;

    AF_MSGSPECS::AF_MsgParser &af_msgparser_ = AF_MSGSPECS::AF_MsgParser::GetUniqueInstance(&dbglogger_);
    AF_MSGSPECS::Message *msg_ = af_msgparser_.getMsgFromCatg(catg_, (short)type_);  // AF_MSGSPECS::kRelease );
    for (int i = 0; i < nfields_; i++) {
      if (event_.estimate_vals_.find(fields[i].field_id_) != event_.estimate_vals_.end() &&
          event_.scale_beta_.find(fields[i].field_id_) != event_.scale_beta_.end()) {
        AF_MSGSPECS::Field *t_field_ = af_msgparser_.getFieldFromMsg(msg_, fields[i].field_id_);
        switch (t_field_->field_type_) {
          case AF_MSGSPECS::kFloat:
          case AF_MSGSPECS::kDouble:
            event_.actual_vals_[fields[i].field_id_] = fields[i].data_.vFloat;
            break;
          case AF_MSGSPECS::kShort_value_enumeration:
          case AF_MSGSPECS::kLong:
          case AF_MSGSPECS::kShort:
          case AF_MSGSPECS::kInt:
            event_.actual_vals_[fields[i].field_id_] = fields[i].data_.vInt;
            break;
          case AF_MSGSPECS::kBoolean:
            DBGLOG_TIME_CLASS_FUNC << "Error: Boolean value not identified" << DBGLOG_ENDL_FLUSH;
            break;
          default:
            DBGLOG_TIME_CLASS_FUNC << "Error: Not identified" << DBGLOG_ENDL_FLUSH;
            break;
        }
      }
    }
    if (category_id_ == 45 && event_.actual_vals_.find(1) != event_.actual_vals_.end() &&
        event_.actual_vals_.find(3) != event_.actual_vals_.end() &&
        (event_.actual_vals_[1] - event_.estimate_vals_[1]) * (event_.actual_vals_[3] - event_.estimate_vals_[3]) > 0) {
      DBGLOG_TIME_CLASS_FUNC << " WARN: For NFP, nonfarm signal and unemployment signal are in opposite direction.. "
                                "Hence, not entering into Active mode" << DBGLOG_ENDL_FLUSH;
      return;
    }
    /*
    for (auto it = event_.cat_datum_.begin(); it != event_.cat_datum_.end(); ++it) {
      if (event_.actual_vals_.find(*it) == event_.actual_vals_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "Message Cat_id_: " << event_.cat_id_ << ", datum_id_: " << *it
                               << " Missing.. Hence ignoring this message" << DBGLOG_ENDL_FLUSH;
        return;
      }
    }*/

    /*  Computes the predicted price_change */
    pxch_pred_ = 0;
    bool should_be_activated_ = true;
    event_.mfm_received_ = watch_.msecs_from_midnight();

    if (count_px_pr_ == 0) {
      DBGLOG_TIME_CLASS_FUNC << " ERROR: No prior price has been recorded.. Hence, not entering into Active mode"
                             << DBGLOG_ENDL_FLUSH;
      return;
    }
    avgpx_pr_ = moving_sumpx_pr_ / count_px_pr_;

    for (auto it = event_.cat_datum_.begin(); it != event_.cat_datum_.end(); ++it) {
      if (event_.actual_vals_.find(*it) != event_.actual_vals_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "Datum value " << event_.actual_vals_[*it] << " " << event_.estimate_vals_[*it]
                               << DBGLOG_ENDL_FLUSH;
        pxch_pred_ += event_.scale_beta_[*it] * (event_.actual_vals_[*it] - event_.estimate_vals_[*it]);
      }
    }
    pred_sign_ = (pxch_pred_ > 0) - (pxch_pred_ < 0);

    if (param_set_.af_dd_maxpxch_factor_ >= 0 && param_set_.af_dd_predpxch_factor_ >= 0) {
      dd_margin_ = event_.max_uts_pxch_ * param_set_.af_dd_maxpxch_factor_ +
                   fabs(pxch_pred_) * param_set_.af_dd_predpxch_factor_;
    } else {
      dd_margin_ = event_.getflat_margin_;
    }

    /* Position scaling:
     * risk_scaling=0: Position proportional to (pred_price)
     * risk_scaling=1: Position proportional to (pred_price)^2 */
    double aggress_thresh_ = current_tradevarset_.l1bid_aggressive_;
    double scale_bias_ =
        std::min(1.0, std::max(0.0, (fabs(pxch_pred_) - aggress_thresh_) / (event_.max_uts_pxch_ - aggress_thresh_)));
    position_to_take_ = (int)(param_set_.max_position_ * scale_bias_ + 0.5);

    if (param_set_.af_risk_scaling_ == 1) {
      position_to_take_ = (int)(param_set_.max_position_ * scale_bias_ * scale_bias_);
    }

    for (auto it = staggered_getflat_maxpos_.begin(); it != staggered_getflat_maxpos_.end(); it++) {
      *it = (int)(*it * scale_bias_ + 0.5);
    }

    DBGLOG_TIME_CLASS_FUNC << "EventBiasAggressiveTrading Initials: " << event_.mfm_received_
                           << " , pxch_pred: " << pxch_pred_ << ", prior_price: " << avgpx_pr_
                           << ", position_to_take: " << position_to_take_ << DBGLOG_ENDL_FLUSH;

    if (dep_stdev_ != 0) {
      // if something fishy about the HUGE signal
      if (fabs(pxch_pred_ / dep_stdev_) > EVENT_FAT_FINGER_FACTOR) {
        DBGLOG_TIME_CLASS_FUNC << " Possible Junk Message: predicted price: " << pxch_pred_
                               << ", dep_stdev: " << dep_stdev_ << DBGLOG_ENDL_FLUSH;
        should_be_activated_ = false;
      }
    }

    if (use_source_trend_) {
      source_pxch_pred_ = 0;
      double self_pxch_pred_ = 0;
      for (auto it = event_.cat_datum_.begin(); it != event_.cat_datum_.end(); ++it) {
        if (source_scale_beta_.find(*it) != source_scale_beta_.end() &&
            event_.actual_vals_.find(*it) != event_.actual_vals_.end()) {
          source_pxch_pred_ += source_scale_beta_[*it] * (event_.actual_vals_[*it] - event_.estimate_vals_[*it]);
          self_pxch_pred_ += event_.scale_beta_[*it] * (event_.actual_vals_[*it] - event_.estimate_vals_[*it]);
        }
      }
      source_pred_sign_ = (source_pxch_pred_ > 0) - (source_pxch_pred_ < 0);
      source_avgpx_pr_ = source_moving_sumpx_pr_ / count_px_pr_;
      source_dd_margin_ = dd_margin_ * fabs(source_pxch_pred_ / self_pxch_pred_);

      DBGLOG_TIME_CLASS_FUNC << " Source: avg_prior_price:" << source_avgpx_pr_ << ", pxch_pred: " << source_pxch_pred_
                             << DBGLOG_ENDL_FLUSH;
    }

    double pxch_pred_in_ticks_ = pxch_pred_ / dep_market_view_.min_price_increment();
    if (fabs(pxch_pred_in_ticks_) > 6 && param_set_.allow_to_aggress_on_getflat_) {
      getflat_aggress_on_dd_ = true;
      param_set_.max_spread_getflat_aggress_ = std::max(1.0, 0.3 * dep_stdev_ / dep_market_view_.min_price_increment());
      DBGLOG_TIME_CLASS_FUNC << " Enabling AggGetflat on DD breach, max_bidask_spread: "
                             << param_set_.max_spread_getflat_aggress_ << DBGLOG_ENDL_FLUSH;
    }

    // in case there is a junk value corresponding to this event or the event
    // has not yet occurred we should not attempt to trade
    is_active_ = should_be_activated_;  // && ( event_.event_mfm_ - watch_.msecs_from_midnight() <= 0 );
    ProcessSignalUpdate(1);             // CALL THE LOGIC
  }
}
}
