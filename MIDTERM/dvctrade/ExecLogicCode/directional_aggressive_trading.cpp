/**
   \file ExecLogicCode/directional_agrressive_trading.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/ExecLogic/directional_aggressive_trading.hpp"
// exec_logic_code / defines.hpp was empty

namespace HFSAT {

DirectionalAggressiveTrading::DirectionalAggressiveTrading(
    DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
    SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
    MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
    const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int t_runtime_id_,
    const std::vector<std::string> _this_model_source_shortcode_vec_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_) {}

void DirectionalAggressiveTrading::TradingLogic() {
#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance().End(25);
  HFSAT::CpucycleProfiler::GetUniqueInstance().Start(26);
  HFSAT::CpucycleProfiler::GetUniqueInstance().Start(27);
#endif

  int num_max_orders_ = 1;
  int our_bid_orders_ = 0;
  int our_ask_orders_ = 0;
  int effective_bid_position_ = my_position_;
  int effective_ask_position_ = my_position_;
  int effective_bid_position_to_keep_ = my_position_;
  int effective_ask_position_to_keep_ = my_position_;

  if (param_set_.place_multiple_orders_) {
    our_bid_orders_ = order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_) +
                      order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_);
    our_ask_orders_ = order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_) +
                      order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_);

    num_max_orders_ = param_set_.max_unit_size_at_level_;
    effective_bid_position_ =
        our_bid_orders_ + my_position_ - order_manager_.SumBidSizeCancelRequested(best_nonself_bid_int_price_);
    effective_ask_position_ =
        -our_ask_orders_ + my_position_ + order_manager_.SumAskSizeCancelRequested(best_nonself_ask_int_price_);
    effective_bid_position_to_keep_ = effective_bid_position_ - current_tradevarset_.l1bid_trade_size_;
    effective_ask_position_to_keep_ = effective_ask_position_ + current_tradevarset_.l1ask_trade_size_;

    current_bid_tradevarset_ = position_tradevarset_map_[GetPositonToTradeVarsetMapIndex(effective_bid_position_)];
    current_bid_keep_tradevarset_ =
        position_tradevarset_map_[GetPositonToTradeVarsetMapIndex(effective_bid_position_to_keep_)];
    current_ask_keep_tradevarset_ =
        position_tradevarset_map_[GetPositonToTradeVarsetMapIndex(effective_ask_position_to_keep_)];
    current_ask_tradevarset_ = position_tradevarset_map_[GetPositonToTradeVarsetMapIndex(effective_ask_position_)];
    UpdateThresholds();  // too many calls?
  }

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    our_bid_orders_ = order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_) +
                      order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_);
    our_ask_orders_ = order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_) +
                      order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_);
    DBGLOG_TIME_CLASS_FUNC
        << " Shc: " << dep_market_view_.shortcode() << " BidOrders: " << our_bid_orders_
        << " AskOrders: " << our_ask_orders_ << " Pos: " << my_position_ << " # " << effective_bid_position_ << " # "
        << effective_ask_position_ << " Thresh: "
        << position_tradevarset_map_[GetPositonToTradeVarsetMapIndex(effective_bid_position_)].l1bid_place_ << " # "
        << position_tradevarset_map_[(GetPositonToTradeVarsetMapIndex(effective_ask_position_))].l1ask_place_
        << " tgtbias:" << targetbias_numbers_ << DBGLOG_ENDL_FLUSH;
  }

  // setting top level directives
  top_bid_place_ = false;
  top_bid_keep_ = false;
  top_bid_improve_ = false;
  top_ask_lift_ = false;
  bid_improve_keep_ = false;

  // if based on current risk level or if trading is stopped ... check if we have any allowance to place orders at top
  // level
  if (  // check if we have any allowance to place orders at top level
      (current_tradevarset_.l1bid_trade_size_ > 0) &&
      // first check for cooloff_interval or the new price is better than the last price
      ((last_buy_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
       ((best_nonself_bid_int_price_ - last_buy_int_price_) < (param_set_.px_band_ - 1))) &&
      // later change setting of last_buy_int_price_ to include px_band_
      // in case we are using big trades cancel then check if the cooloff has passed
      // else don't trade
      (last_bigtrades_bid_cancel_msecs_ <= 0 ||
       (watch_.msecs_from_midnight() - last_bigtrades_bid_cancel_msecs_ >= param_set_.bigtrades_cooloff_interval_)) &&
      (!cancel_l1_bid_ask_flow_buy_)) {
    // check if the margin of buying
    // i.e. ( targetbias_numbers_ )
    // exceeds the threshold current_tradevarset_.l1bid_place_
    if ((best_nonself_bid_size_ > param_set_.safe_distance_) ||
        // If the buy trades on the source crosses place threshold then it will place on best level irrespective of the
        // indicators
        (watch_.msecs_from_midnight() - last_bigtrades_bid_place_msecs_ <=
         param_set_.bigtrades_place_cooloff_interval_) ||
        // If the buy trades on the source crosses aggress threshold then it will place on best level irrespective of
        // the indicators
        (watch_.msecs_from_midnight() - last_bigtrades_bid_aggress_msecs_ <=
         param_set_.bigtrades_aggress_cooloff_interval_) ||
        ((targetbias_numbers_ + ((dep_market_view_.spread_increments() > moving_avg_dep_bidask_spread_)
                                     ? (param_set_.high_spread_allowance_)
                                     : 0.0) >=
          current_bid_tradevarset_.l1bid_place_ - trade_bias_buy_ + cancel_bias_buy_ - l1_bias_ - l1_order_bias_ -
              short_positioning_bias_) &&
         (best_nonself_bid_size_ > param_set_.min_size_to_join_) &&
         (exec_logic_indicators_helper_->l1_trade_volume_bias() >= 0))) {
      top_bid_place_ = true;
      top_bid_keep_ = true;

      if (watch_.msecs_from_midnight() - last_agg_buy_msecs_ > param_set_.agg_cooloff_interval_) {
        /* aggressive and improve */
        if ((param_set_.allowed_to_aggress_) && /* external control on aggressing */
            (  // If the buy trades on the source crosses aggress threshold then it will aggress
                (watch_.msecs_from_midnight() - last_bigtrades_bid_aggress_msecs_ <=
                 param_set_.bigtrades_aggress_cooloff_interval_) ||
                (targetbias_numbers_ >=
                 current_tradevarset_.l1bid_aggressive_)) &&  // only LIFT offer if the margin of buying exceeds the
                                                              // threshold current_tradevarset_.l1bid_aggressive_
            (my_position_ <=
             param_set_.max_position_to_lift_) && /* Don't LIFT offer when my_position_ is already decently long */
            (dep_market_view_.spread_increments() <=
             param_set_.max_int_spread_to_cross_) &&  // Don't LIFT when effective spread is to much
            ((last_buy_msecs_ <= 0) ||
             (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
             ((best_nonself_ask_int_price_ - last_buy_int_price_) <
              (param_set_.px_band_ - 1)))  // TODO_OPT : later change setting of last_buy_int_price_ to include px_band_
            )

        {
          top_ask_lift_ = true;

          // when we are already long and ask_lift is set to true,
          // place and keep at top bid are set false so that
          // we cancel top level orders before we place new ones at aggressive prices */
          if (my_position_ >= param_set_.max_position_to_cancel_on_lift_) {
            top_bid_place_ = false;
            top_bid_keep_ = false;
          }
        } else {
          top_ask_lift_ = false;

          // conditions to place market improving bid orders:
          // ALLOWED_TO_IMPROVE
          // position is not too long already
          // spread wide
          // signal strong
          if ((param_set_.allowed_to_improve_) &&
              (my_position_ <=
               param_set_
                   .max_position_to_bidimprove_) && /* Don't improve bid when my_position_ is already decently long */
              (dep_market_view_.spread_increments() >= param_set_.min_int_spread_to_improve_) &&
              ((targetbias_numbers_ >= current_tradevarset_.l1bid_improve_) ||
               // If the buy trades on the source crosses aggress threshold then it will try to improve the best level
               (watch_.msecs_from_midnight() - last_bigtrades_bid_aggress_msecs_ <=
                param_set_.bigtrades_aggress_cooloff_interval_)) &&
              ((last_buy_msecs_ <= 0) ||
               (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
               (((best_nonself_bid_int_price_ + 1) - last_buy_int_price_) <
                (param_set_.px_band_ -
                 1)))  // TODO_OPT : later change setting of last_buy_int_price_ to include px_band_
              ) {
            top_bid_improve_ = true;
          } else {
            top_bid_improve_ = false;
          }
        }
      }

    } else {  // signal is not strong enough for placing bids at best_nonself_bid_price_
      // check if we should retain exisiting bids due to place_in_line

      // TODO : perhaps we should make place_in_line effect bestbid_queue_hysterisis_ smarter
      //        for instance when short term volatility in the market is very high
      //        then being high in the queue should count for less.

      if ((targetbias_numbers_ +
           ((dep_market_view_.spread_increments() > moving_avg_dep_bidask_spread_) ? (param_set_.high_spread_allowance_)
                                                                                   : 0.0) +
           bestbid_queue_hysterisis_) >= current_bid_keep_tradevarset_.l1bid_keep_ - trade_bias_buy_ +
                                             cancel_bias_buy_ - l1_bias_ - short_positioning_bias_ - l1_order_bias_) {
        top_bid_keep_ = true;
      } else {
        top_bid_keep_ = false;
      }
    }

    if ((dep_market_view_.spread_increments() > 1) &&
        (targetbias_numbers_ + ((dep_market_view_.spread_increments() > moving_avg_dep_bidask_spread_)
                                    ? (param_set_.high_spread_allowance_)
                                    : 0.0)) >= current_tradevarset_.l1bid_improve_keep_ - trade_bias_buy_ +
                                                   cancel_bias_buy_ - l1_bias_ - short_positioning_bias_ -
                                                   l1_order_bias_) {
      bid_improve_keep_ = true;
    } else {
      bid_improve_keep_ = false;
    }
  }

  top_ask_place_ = false;
  top_ask_keep_ = false;
  top_ask_improve_ = false;
  top_bid_hit_ = false;
  ask_improve_keep_ = false;

  // if based on current risk level or if trading is stopped ... check if we have any allowance to place orders at top
  // level
  if (  // check if we have any allowance to place orders at top level
      (current_tradevarset_.l1ask_trade_size_ > 0) &&
      // first check for cooloff_interval
      ((last_sell_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
       (last_sell_int_price_ - best_nonself_ask_int_price_ < (param_set_.px_band_ - 1))) &&
      // in case we are using big trades cancel then check if the cooloff has passed
      // else don't trade
      (last_bigtrades_ask_cancel_msecs_ <= 0 ||
       (watch_.msecs_from_midnight() - last_bigtrades_ask_cancel_msecs_ >=
        param_set_.bigtrades_cooloff_interval_)) &&  // later change setting of last_sell_int_price_ to include px_band_
      (!cancel_l1_bid_ask_flow_sell_)) {
    // check if the margin of placing limit ask orders at the price ( best_nonself_ask_price_ )
    // i.e. ( best_nonself_ask_price_ - target_price_ )
    //      exceeds the threshold current_tradevarset_.l1ask_place_

    if ((best_nonself_ask_size_ > param_set_.safe_distance_) ||
        // If the sell trades on the source crosses place threshold then it will place on best level irrespective of the
        // indicators
        (watch_.msecs_from_midnight() - last_bigtrades_ask_place_msecs_ <=
         param_set_.bigtrades_place_cooloff_interval_) ||
        // If the sell trades on the source crosses aggress threshold then it will place on best level irrespective of
        // the indicators
        (watch_.msecs_from_midnight() - last_bigtrades_ask_aggress_msecs_ <=
         param_set_.bigtrades_aggress_cooloff_interval_) ||
        (((-targetbias_numbers_ + ((dep_market_view_.spread_increments() > moving_avg_dep_bidask_spread_)
                                       ? (param_set_.high_spread_allowance_)
                                       : 0.0)) >= current_ask_tradevarset_.l1ask_place_ - trade_bias_sell_ +
                                                      cancel_bias_sell_ - l1_bias_ - long_positioning_bias_ -
                                                      l1_order_bias_) &&
         (best_nonself_ask_size_ > param_set_.min_size_to_join_) &&
         (exec_logic_indicators_helper_->l1_trade_volume_bias() <= 0))) {
      top_ask_place_ = true;
      top_ask_keep_ = true;

      if (watch_.msecs_from_midnight() - last_agg_sell_msecs_ > param_set_.agg_cooloff_interval_) {
        /* aggressive and improve */
        // conditions to place aggressive orders:
        // ALLOWED_TO_AGGRESS
        // position is not too short already
        // spread narrow
        // signal strong
        if ((param_set_.allowed_to_aggress_) && /* external control on aggressing */
            (my_position_ >=
             param_set_.min_position_to_hit_) && /* Don't HIT bid when my_position_ is already decently short */
            (best_nonself_ask_int_price_ - best_nonself_bid_int_price_ <=
             param_set_.max_int_spread_to_cross_) && /* Don't HIT ( cross ) when effective spread is to much */
            (  // If the sell trades on the source crosses aggress threshold then it will aggress
                (watch_.msecs_from_midnight() - last_bigtrades_ask_aggress_msecs_ <=
                 param_set_.bigtrades_aggress_cooloff_interval_) ||
                (-targetbias_numbers_ >= current_tradevarset_.l1ask_aggressive_)) &&
            ((last_sell_msecs_ <= 0) ||
             (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
             ((last_sell_int_price_ - best_nonself_bid_int_price_) <
              (param_set_.px_band_ -
               1)))  // TODO_OPT : later change setting of last_sell_int_price_ to include px_band_
            )

        {
          top_bid_hit_ = true;

          /* when we are already short and bit_hit is set to true,
                       place and keep at top ask are set false so that
                       we cancel top level orders before we place new ones at aggressive prices */
          if (my_position_ <= param_set_.min_position_to_cancel_on_hit_) {
            top_ask_place_ = false;
            top_ask_keep_ = false;
          }
        } else {
          top_bid_hit_ = false;
          if ((param_set_.allowed_to_improve_) &&
              (my_position_ >=
               param_set_
                   .min_position_to_askimprove_) && /* Don't improve ask when my_position_ is already decently short */
              (best_nonself_ask_int_price_ - best_nonself_bid_int_price_ >= param_set_.min_int_spread_to_improve_) &&
              (  // If the sell trades on the source crosses aggress threshold then it will try to improve the best
                  // level
                  (watch_.msecs_from_midnight() - last_bigtrades_ask_aggress_msecs_ <=
                   param_set_.bigtrades_aggress_cooloff_interval_) ||
                  (-targetbias_numbers_ >= current_tradevarset_.l1ask_improve_)) &&
              ((last_sell_msecs_ <= 0) ||
               (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
               (((last_sell_int_price_ - 1) - best_nonself_ask_int_price_) <
                (param_set_.px_band_ -
                 1)))  // TODO_OPT : later change setting of last_sell_int_price_ to include px_band_
              ) {
            top_ask_improve_ = true;
          } else {
            top_ask_improve_ = false;
          }
        }
      }

    } else {
      // signal not strog enough to place limit orders at the best ask level
      if ((-targetbias_numbers_ +
           ((dep_market_view_.spread_increments() > moving_avg_dep_bidask_spread_) ? (param_set_.high_spread_allowance_)
                                                                                   : 0.0) +
           bestask_queue_hysterisis_) >=
          current_ask_keep_tradevarset_.l1ask_keep_ - trade_bias_sell_ + cancel_bias_sell_ - l1_bias_ - l1_order_bias_ -
              long_positioning_bias_) {  // but with place in line effect enough to keep the live order there
        top_ask_keep_ = true;
      } else {
        top_ask_keep_ = false;
      }
    }
    if ((dep_market_view_.spread_increments() > 1) &&
        (-targetbias_numbers_ + ((dep_market_view_.spread_increments() > moving_avg_dep_bidask_spread_)
                                     ? (param_set_.high_spread_allowance_)
                                     : 0.0)) >= current_tradevarset_.l1ask_improve_keep_ - trade_bias_sell_ +
                                                    cancel_bias_sell_ - l1_bias_ - l1_order_bias_ -
                                                    long_positioning_bias_) {
      ask_improve_keep_ = true;
    } else {
      ask_improve_keep_ = false;
    }
  }

  if (param_set_.improve_cancel_ && livetrading_) {
    // Cancel all ask improve orders
    if (best_nonself_ask_size_ <= order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) &&
        watch_.msecs_from_midnight() - order_manager_.GetTopConfirmedAskOrder()->placed_msecs() > 1) {
      order_manager_.CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_);
      improve_cancel_counter_++;
    }
    // Cancel all bid improve orders
    if (best_nonself_bid_size_ <= order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) &&
        watch_.msecs_from_midnight() - order_manager_.GetTopConfirmedBidOrder()->placed_msecs() > 1) {
      order_manager_.CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_);
      improve_cancel_counter_++;
    }
  }

  // After setting top-level directives ...
  // get to order placing or canceling part

  // Active BID rder management
  placed_bids_this_round_ = false;
  canceled_bids_this_round_ = false;
  if (top_ask_lift_) {
    // only place aggressive orders when there is no active unconfirmed order and no confirmed orders above the best
    // price
    // and no confirmed orders above the best_nonself_bid_price_
    if ((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_ + 1) == 0) &&
        (order_manager_.SumBidSizeConfirmedAboveIntPrice(best_nonself_bid_int_price_) == 0)) {
      int _canceled_size_ = 0;
      if (!top_bid_keep_) {  // if due to position placing aggressive LIFT order requires us to cancel bids from all
                             // active levels
        _canceled_size_ += order_manager_.CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_);
        if (_canceled_size_ > 0) {
          canceled_bids_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC_LINE << " Shc: " << dep_market_view_.shortcode()
                                        << " Cancelling bids eq above : " << best_nonself_bid_int_price_
                                        << " Size: " << _canceled_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }

      int aggress_size = MathUtils::GetFlooredMultipleOf(
          std::min(std::min(param_set_.max_position_ - my_position_, current_tradevarset_.l1bid_trade_size_),
                   best_nonself_ask_size_),
          param_set_.min_order_size_);

      int allowance_for_aggressive_buy_ =
          my_position_ + order_manager_.SumBidSizes() + aggress_size - param_set_.worst_case_position_;

      if (param_set_.use_new_aggress_logic_) {
        allowance_for_aggressive_buy_ = my_position_ + order_manager_.SumBidSizes() + aggress_size -
                                        std::max(param_set_.worst_case_position_, param_set_.max_position_);
      }

      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the bid side
      if (allowance_for_aggressive_buy_ >= 0) {
        // and if size canceled already is less than l1bid_trade_size_
        if (allowance_for_aggressive_buy_ > _canceled_size_) {
          // then cancel Bids from bottom levels for the required size
          _canceled_size_ += order_manager_.CancelBidsFromFar(aggress_size);

          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC_LINE << " Shc: " << dep_market_view_.shortcode()
                                        << " Cancelling bids eq above : " << best_nonself_bid_int_price_
                                        << " Size: " << _canceled_size_ << DBGLOG_ENDL_FLUSH;
          }
        }

      } else {
        /* Place new order */
        order_manager_.SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_, aggress_size, kTradeTypeBuy,
                                 'A');
        placed_bids_this_round_ = true;
        last_agg_buy_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << " Shc: " << dep_market_view_.shortcode() << "Sending aggressive B at px "
                                 << best_nonself_ask_int_price_ << " position " << my_position_ << " cancelled size "
                                 << _canceled_size_ << " low bid sizes " << DBGLOG_ENDL_FLUSH;
        }
      }
    } else {
      int canceled_size_ = order_manager_.CancelBidsAboveIntPrice(
          best_nonself_bid_int_price_);  // only after canceling them can we be allowed to place aggressive orders
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " Shc: " << dep_market_view_.shortcode()
                                    << " Cancelling bids eq above : " << best_nonself_bid_int_price_
                                    << " Size: " << canceled_size_ << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  if ((!placed_bids_this_round_) && (top_bid_improve_)) {
    // only place Bid Improve orders when there is no active unconfirmed order and no confirmed orders above the best
    // price
    if ((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_ + 1) == 0) &&
        (order_manager_.SumBidSizeConfirmedAboveIntPrice(best_nonself_bid_int_price_) == 0)) {
      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the bid side
      int improve_size = MathUtils::GetFlooredMultipleOf(
          std::min(param_set_.max_position_ - my_position_, current_tradevarset_.l1bid_trade_size_),
          param_set_.min_order_size_);
      if (my_position_ + order_manager_.SumBidSizes() + improve_size >= param_set_.worst_case_position_) {
        int _canceled_size_ = 0;
        // then cancel Bids from bottom levels for the required size
        _canceled_size_ += order_manager_.CancelBidsFromFar(improve_size);

        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " Shc: " << dep_market_view_.shortcode()
                                      << " Cancelling bids eq above : " << best_nonself_bid_int_price_
                                      << " Size: " << _canceled_size_ << DBGLOG_ENDL_FLUSH;
        }

      } else {
        /* Place new order */
        order_manager_.SendTradeIntPx((best_nonself_bid_int_price_ + 1), improve_size, kTradeTypeBuy, 'I');
        last_agg_buy_msecs_ = watch_.msecs_from_midnight();
        placed_bids_this_round_ = true;
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << " Shc: " << dep_market_view_.shortcode() << "Sending improve B at px "
                                 << best_nonself_bid_int_price_ + 1 << " position " << my_position_
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    }

  } else {
    if ((dep_market_view_.spread_increments() > 1) &&
        (!bid_improve_keep_))  // can have size check here for optimization purpose
    {
      int cancelled_size_ = order_manager_.CancelBidsAboveIntPrice(best_nonself_bid_int_price_);
      if (cancelled_size_ > 0) {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " Shc: " << dep_market_view_.shortcode()
                                 << " Cancelled Improve Bid orders Above: " << best_nonself_bid_int_price_
                                 << " Position: " << my_position_
                                 << " tgt_bias: " << targetbias_numbers_ / dep_market_view_.min_price_increment()
                                 << " thresh_t: "
                                 << current_tradevarset_.l1bid_improve_keep_ / dep_market_view_.min_price_increment()
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  if (!placed_bids_this_round_) {  // only come here if no aggressive or improve bid orders sent this cycle
    if (top_bid_place_) {
      // only place Bid orders when there is no active unconfirmed order and no confirmed order at or above the best
      // price
      if (((param_set_.place_multiple_orders_ && CanPlaceNextOrder(best_nonself_bid_int_price_, kTradeTypeBuy) &&
            (our_bid_orders_ < num_max_orders_ * param_set_.unit_trade_size_) &&
            ((our_bid_orders_ + my_position_ + current_tradevarset_.l1bid_trade_size_) <= param_set_.max_position_)) ||
           ((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_) == 0) &&
            (order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) == 0))) &&
          (stdev_ <= param_set_.low_stdev_lvl_ ||
           (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_place_)))  // Don't place any new
                                                                                            // orders in inside market
                                                                                            // if the spread is too wide
      // ( ( param_set_.worst_case_position_ > 0 ) ||
      //   ( ( order_manager_.SumBidSizeConfirmedEqAboveIntPrice ( best_nonself_bid_int_price_ - 100 ) +
      //       order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice ( best_nonself_bid_int_price_ - 100 ) ) <=
      //       param_set_.worst_case_position_ ) ) ) // TODO ... maintain a sum_bid_placed in ordermanager
      {
        order_manager_.SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                 current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'B');
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << " Shc: " << dep_market_view_.shortcode() << "SendTrade B of "
                                 << current_tradevarset_.l1bid_trade_size_ << " @ " << best_nonself_bid_price_
                                 << " tgt_bias: " << targetbias_numbers_ / dep_market_view_.min_price_increment()
                                 << " thresh_t: "
                                 << current_bid_tradevarset_.l1bid_place_ / dep_market_view_.min_price_increment()
                                 << " Int Px: " << best_nonself_bid_int_price_ << " tMktSz: " << best_nonself_bid_size_
                                 << " Mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << "  ---  "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
        }
      }
    } else {
      if (!top_bid_keep_) {  // cancel all orders at best_nonself_bid_price_

        int canceled_size_ = 0;
        if (param_set_.place_multiple_orders_) {
          canceled_size_ =
              order_manager_.CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_, param_set_.unit_trade_size_);
        } else {
          canceled_size_ = order_manager_.CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_);
        }

        if (canceled_size_ > 0) {
          canceled_bids_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
          {
            DBGLOG_TIME << " Shc: " << dep_market_view_.shortcode() << "Canceled B of " << canceled_size_ << " EqAbove "
                        << best_nonself_bid_price_
                        << " tgt_bias: " << targetbias_numbers_ / dep_market_view_.min_price_increment()
                        << " thresh_t: "
                        << current_bid_keep_tradevarset_.l1bid_keep_ / dep_market_view_.min_price_increment()
                        << " tMktSz: " << best_nonself_bid_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
  }

  // Active ASK order management
  placed_asks_this_round_ = false;
  canceled_asks_this_round_ = false;
  if (top_bid_hit_) {
    // only place aggressive orders when there is no active unconfirmed order and no confirmed orders above the best
    // price
    if ((order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_ - 1) == 0) &&
        (order_manager_.SumAskSizeConfirmedAboveIntPrice(best_nonself_ask_int_price_) == 0)) {
      int _canceled_size_ = 0;
      if (!top_ask_keep_) {  // if due to position placing aggressive LIFT order requires us to cancel asks from all
                             // active levels
        _canceled_size_ += order_manager_.CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_);
      }

      int aggress_size = MathUtils::GetFlooredMultipleOf(
          std::min(std::min(param_set_.max_position_ + my_position_, current_tradevarset_.l1ask_trade_size_),
                   best_nonself_bid_size_),
          param_set_.min_order_size_);

      int allowance_for_aggressive_sell_ =
          -my_position_ + order_manager_.SumAskSizes() + aggress_size - param_set_.worst_case_position_;

      if (param_set_.use_new_aggress_logic_) {
        allowance_for_aggressive_sell_ = -my_position_ + order_manager_.SumAskSizes() + aggress_size -
                                         std::max(param_set_.worst_case_position_, param_set_.max_position_);
      }

      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the ask side
      if (allowance_for_aggressive_sell_ >= 0) {
        // and if size canceled already is less than l1ask_trade_size_
        if (allowance_for_aggressive_sell_ > _canceled_size_) {
          _canceled_size_ += order_manager_.CancelAsksFromFar(
              current_tradevarset_.l1ask_trade_size_);  ///< then cancel Asks from bottom levels for the required size
        }

      } else {
        /* Place new order */
        order_manager_.SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_, aggress_size, kTradeTypeSell,
                                 'A');
        placed_asks_this_round_ = true;
        last_agg_sell_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << " Shc: " << dep_market_view_.shortcode() << "Sending aggressive S at px "
                                 << best_nonself_bid_int_price_ << " position " << my_position_ << DBGLOG_ENDL_FLUSH;
        }
      }

    } else {
      order_manager_.CancelAsksAboveIntPrice(
          best_nonself_ask_int_price_);  // only after canceling them can we be allowed to place aggressive orders
    }
  }

  if ((!placed_asks_this_round_) && (top_ask_improve_)) {
    // only place Ask Improve orders when there is no active unconfirmed order and no confirmed orders above the best
    // price
    if ((order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_ - 1) == 0) &&
        (order_manager_.SumAskSizeConfirmedAboveIntPrice(best_nonself_ask_int_price_) == 0)) {
      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the ask side
      int improve_size = MathUtils::GetFlooredMultipleOf(
          std::min(param_set_.max_position_ + my_position_, current_tradevarset_.l1ask_trade_size_),
          param_set_.min_order_size_);

      if (-my_position_ + order_manager_.SumAskSizes() + improve_size >= param_set_.worst_case_position_) {
        int _canceled_size_ = 0;
        ///< then cancel Asks from bottom levels for the required size
        _canceled_size_ += order_manager_.CancelAsksFromFar(improve_size);

      } else {
        /* Place new order */
        order_manager_.SendTradeIntPx((best_nonself_ask_int_price_ - 1), improve_size, kTradeTypeSell, 'I');
        placed_asks_this_round_ = true;
        last_agg_sell_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << " Shc: " << dep_market_view_.shortcode() << "Sending improve S at px "
                                 << best_nonself_ask_int_price_ - 1 << " position " << my_position_
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    }

  } else {
    if ((dep_market_view_.spread_increments() > 1) && (!ask_improve_keep_))
    /*
     * Need to cancel these kind of orders only when the spread > 1
     * In other cases it wont be benificial except wasting 1 message
     */
    {
      int cancelled_size_ = order_manager_.CancelAsksAboveIntPrice(best_nonself_ask_int_price_);
      if (cancelled_size_ > 0) {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " Shc: " << dep_market_view_.shortcode()
                                 << " Cancelling Improve Ask orders Above: " << best_nonself_ask_int_price_
                                 << " position: " << my_position_
                                 << " tgt_bias: " << targetbias_numbers_ / dep_market_view_.min_price_increment()
                                 << " thresh_t: "
                                 << current_tradevarset_.l1ask_improve_keep_ / dep_market_view_.min_price_increment()
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  if (!placed_asks_this_round_) {
    // get to this location if no aggressive or improve orders placed in this cycle
    if (top_ask_place_) {
      // only place Ask orders when there is no active unconfirmed order at or above best_nonself_ask_int_price_
      //      and no confirmed order at or above the best price
      if (((param_set_.place_multiple_orders_ && CanPlaceNextOrder(best_nonself_ask_int_price_, kTradeTypeSell) &&
            (our_ask_orders_ < param_set_.unit_trade_size_ * num_max_orders_) &&
            ((our_ask_orders_ - my_position_ + current_tradevarset_.l1ask_trade_size_) <= param_set_.max_position_)) ||
           ((order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_) == 0) &&
            (order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) == 0))) &&
          (stdev_ <= param_set_.low_stdev_lvl_ ||
           (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_place_)))  // Don't place any new
                                                                                            // orders in inside market
                                                                                            // if the spread is too wide
      // ( ( param_set_.worst_case_position_ > 0 ) ||
      //   ( ( order_manager_.SumAskSizeConfirmedEqAboveIntPrice ( best_nonself_ask_int_price_ + 100 ) +
      //       order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice ( best_nonself_ask_int_price_ + 100 ) ) <=
      //       param_set_.worst_case_position_ ) ) ) // TODO ... maintain a sum_ask_placed in ordermanager
      {
        order_manager_.SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                 current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'B');
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << " Shc: " << dep_market_view_.shortcode() << "SendTrade S of "
                                 << current_tradevarset_.l1ask_trade_size_ << " @ " << best_nonself_ask_price_
                                 << " tgt_bias: " << -targetbias_numbers_ / dep_market_view_.min_price_increment()
                                 << " thresh_t: "
                                 << current_ask_tradevarset_.l1ask_place_ / dep_market_view_.min_price_increment()
                                 << " Int Px: " << best_nonself_ask_int_price_ << " tMktSz: " << best_nonself_ask_size_
                                 << " Mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " ---- "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
        }
      }
    } else {
      if (!top_ask_keep_) {
        // cancel all orders at best_nonself_ask_price_
        int canceled_size_ = 0;
        if (param_set_.place_multiple_orders_) {
          order_manager_.CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_, param_set_.unit_trade_size_);
        } else {
          order_manager_.CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_);
        }
        if (canceled_size_ > 0) {
          canceled_asks_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
          {
            DBGLOG_TIME << " Shc: " << dep_market_view_.shortcode() << "Canceled S of " << canceled_size_ << " EqAbove "
                        << best_nonself_ask_price_
                        << " tgt_bias: " << -targetbias_numbers_ / dep_market_view_.min_price_increment()
                        << " thresh_t: "
                        << current_ask_keep_tradevarset_.l1ask_keep_ / dep_market_view_.min_price_increment()
                        << " tMktSz: " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
  }

  // zero logging
  if ((dbglogger_.CheckLoggingLevel(TRADING_INFO) && (placed_bids_this_round_ || placed_asks_this_round_))) {
    PrintFullStatus();
  }

  // temporarily printing indicators every time
  if ((livetrading_ || dbglogger_.CheckLoggingLevel(TRADING_INFO)) &&
      (placed_bids_this_round_ || placed_asks_this_round_ || canceled_bids_this_round_ || canceled_asks_this_round_)) {
    dump_inds = true;
  }
}

void DirectionalAggressiveTrading::PrintFullStatus() {
  DBGLOG_TIME << " Shc: " << dep_market_view_.shortcode() << "tgt: " << target_price_ << " ExpBidPft@ "
              << best_nonself_bid_price_ << " X " << best_nonself_bid_size_ << ' '
              << (target_price_ - best_nonself_bid_price_) << " ExpAskPft@ " << best_nonself_ask_price_ << " X "
              << best_nonself_ask_size_ << ' ' << (best_nonself_ask_price_ - target_price_)
              << " signalbias: " << (targetbias_numbers_ / dep_market_view_.min_price_increment())
              << " pos: " << my_position_ << " smaps "
              << order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_) << " "
              << order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) << " "
              << order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) << " "
              << order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_)
              << " opnl: " << order_manager_.base_pnl().opentrade_unrealized_pnl() << " gpos: " << my_global_position_
              << DBGLOG_ENDL_FLUSH;
}
}
