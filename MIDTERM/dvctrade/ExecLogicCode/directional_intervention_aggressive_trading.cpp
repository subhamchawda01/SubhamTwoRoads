/**
   \file ExecLogicCode/directional_intervention_agrressive_trading.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/ExecLogic/directional_intervention_aggressive_trading.hpp"
// exec_logic_code / defines.hpp was empty

namespace HFSAT {

DirectionalInterventionAggressiveTrading::DirectionalInterventionAggressiveTrading(
    DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
    SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
    MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
    const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int t_runtime_id_,
    const std::vector<std::string> _this_model_source_shortcode_vec_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_) {}

bool DirectionalInterventionAggressiveTrading::IsLargeBidOrderPresent() {
  for (auto i = 0u; i < 2u; ++i) {
    if (non_standard_market_condition_bid_orders_[i] &&
        (non_standard_market_condition_bid_size_[i] / non_standard_market_condition_bid_orders_[i]) >=
            param_set_.non_standard_market_condition_max_avg_order_size_ &&
        non_standard_market_condition_ask_size_[0] < param_set_.non_standard_market_condition_min_counter_order_size_) {
      return true;
    }
  }
  return false;
}

bool DirectionalInterventionAggressiveTrading::IsLargeAskOrderPresent() {
  for (auto i = 0u; i < 2u; ++i) {
    if (non_standard_market_condition_ask_orders_[i] &&
        (non_standard_market_condition_ask_size_[i] / non_standard_market_condition_ask_orders_[i]) >=
            param_set_.non_standard_market_condition_max_avg_order_size_ &&
        non_standard_market_condition_bid_size_[0] < param_set_.non_standard_market_condition_min_counter_order_size_) {
      return true;
    }
  }
  return false;
}

bool DirectionalInterventionAggressiveTrading::AreSafeMarketConditions() {
  bool are_safe_market_conditions_ =
      (non_standard_market_condition_spread_ <= 1 &&
       std::min(non_standard_market_condition_bid_size_[0], non_standard_market_condition_ask_size_[0]) >=
           param_set_.non_standard_market_condition_min_best_level_size_ &&
       std::min(non_standard_market_condition_bid_orders_[0], non_standard_market_condition_ask_orders_[0]) >=
           param_set_.non_standard_market_condition_min_best_level_order_count_);

  return are_safe_market_conditions_;
}

bool DirectionalInterventionAggressiveTrading::AreNonStandardMarketConditions() {
  // removed costly check since by the fact that we don't have this strategy name for other securities
  // this is only for DOL
  // if ( dep_market_view_.shortcode ( ).compare ( "BR_DOL_0" ) != 0 )
  //   { // Currently just for DOL.
  // 	return false;
  //   }

  bool is_non_standard_market_conditions_ = false;

  for (auto i = 0u; i < 2; ++i) {
    if ((non_standard_market_condition_bid_orders_[i] &&
         (non_standard_market_condition_bid_size_[i] / non_standard_market_condition_bid_orders_[i]) >=
             param_set_.non_standard_market_condition_max_avg_order_size_ &&
         non_standard_market_condition_ask_size_[0] <
             param_set_.non_standard_market_condition_min_counter_order_size_) ||
        (non_standard_market_condition_ask_orders_[i] &&
         (non_standard_market_condition_ask_size_[i] / non_standard_market_condition_ask_orders_[i]) >=
             param_set_.non_standard_market_condition_max_avg_order_size_ &&
         non_standard_market_condition_bid_size_[0] <
             param_set_.non_standard_market_condition_min_counter_order_size_)) {
      DBGLOG_TIME_CLASS_FUNC << " " << watch_.tv() << " [ " << i << " ]=" << non_standard_market_condition_bid_size_[i]
                             << " " << non_standard_market_condition_bid_orders_[i] << " [ " << i
                             << " ]=" << non_standard_market_condition_ask_size_[i] << " "
                             << non_standard_market_condition_ask_orders_[i] << DBGLOG_ENDL_FLUSH;

      // is_non_standard_market_conditions_ = true;
      break;
    }
  }

  return is_non_standard_market_conditions_;
}

bool DirectionalInterventionAggressiveTrading::AreNonStandardMarketConditions(
    const MarketUpdateInfo& _market_update_info_) {
  // removed costly check since by the fact that we don't have this strategy name for other securities
  // this is only for DOL
  // if ( dep_market_view_.shortcode ( ).compare ( "BR_DOL_0" ) )
  //   { // Currently just for DOL.
  // 	return false;
  //   }

  bool is_non_standard_market_conditions_ = false;

  if (_market_update_info_.spread_increments_ >= param_set_.non_standard_market_condition_max_spread_) {
    is_non_standard_market_conditions_ = true;
  } else {
    is_non_standard_market_conditions_ = AreNonStandardMarketConditions();
  }

  return is_non_standard_market_conditions_;
}

void DirectionalInterventionAggressiveTrading::UpdateNonStandardMarketConditionVars(
    const MarketUpdateInfo& _market_update_info_) {
  // removed costly check since by the fact that we don't have this strategy name for other securities
  // this is only for DOL
  // if ( dep_market_view_.shortcode ( ).compare ( "BR_DOL_0" ) )
  //   { // Currently just for DOL.
  // 	return;
  //   }

  non_standard_market_condition_spread_ = _market_update_info_.spread_increments_;
  non_standard_market_condition_bid_size_[0] = _market_update_info_.bestbid_size_;
  non_standard_market_condition_ask_size_[0] = _market_update_info_.bestask_size_;
  non_standard_market_condition_bid_orders_[0] = _market_update_info_.bestbid_ordercount_;
  non_standard_market_condition_ask_orders_[0] = _market_update_info_.bestask_ordercount_;

  for (unsigned int i = 1; i < 3; ++i) {
    non_standard_market_condition_bid_size_[i] = dep_market_view_.bid_size(i);
    non_standard_market_condition_ask_size_[i] = dep_market_view_.ask_size(i);
    non_standard_market_condition_bid_orders_[i] = dep_market_view_.bid_order(i);
    non_standard_market_condition_ask_orders_[i] = dep_market_view_.ask_order(i);
  }
}

void DirectionalInterventionAggressiveTrading::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if (getflat_due_to_non_standard_market_conditions_) {
    if (last_getflat_due_to_non_standard_market_conditions_triggered_at_msecs_ > 0 &&
        watch_.msecs_from_midnight() - last_getflat_due_to_non_standard_market_conditions_triggered_at_msecs_ >
            param_set_.non_standard_market_condition_check_short_msecs_) {
      // Check whether we should exit the non-standard market conditions mode.
      const int max_non_standard_period_price_ = non_standard_market_conditions_mode_prices_.size()
                                                     ? (non_standard_market_conditions_mode_prices_.rbegin()->first)
                                                     : 0;
      const int min_non_standard_period_price_ = non_standard_market_conditions_mode_prices_.size()
                                                     ? (non_standard_market_conditions_mode_prices_.begin()->first)
                                                     : 0;

      const int int_price_move_ = max_non_standard_period_price_ - min_non_standard_period_price_;

      if (int_price_move_ <= 10 && AreSafeMarketConditions() &&
          !AreNonStandardMarketConditions()) {  // Completely normal market
        getflat_due_to_non_standard_market_conditions_ = false;
        ProcessGetFlat();
      } else if (int_price_move_ < 10) {  // Prices didn't move a lot , intervention if any has passed , check for
                                          // post-intervention mkt-conditions
        DBGLOG_TIME_CLASS_FUNC << "Staying in non_standard_market_conditions mode"
                               << " spread_=" << non_standard_market_condition_spread_ << " mkt_bid_size_=[ "
                               << non_standard_market_condition_bid_size_[0] << " "
                               << non_standard_market_condition_bid_size_[1] << " "
                               << non_standard_market_condition_bid_size_[2] << " ]"
                               << " mkt_ask_size_=[ " << non_standard_market_condition_ask_size_[0] << " "
                               << non_standard_market_condition_ask_size_[1] << " "
                               << non_standard_market_condition_ask_size_[2] << " ]"
                               << " mkt_bid_orders_=[ " << non_standard_market_condition_bid_orders_[0] << " "
                               << non_standard_market_condition_bid_orders_[1] << " "
                               << non_standard_market_condition_bid_orders_[2] << " ]"
                               << " mkt_ask_orders_=[ " << non_standard_market_condition_ask_orders_[0] << " "
                               << non_standard_market_condition_ask_orders_[1] << " "
                               << non_standard_market_condition_ask_orders_[2] << " ]" << DBGLOG_ENDL_FLUSH;
        non_standard_market_conditions_mode_prices_.clear();

        last_getflat_due_to_non_standard_market_conditions_triggered_at_msecs_ =
            watch_.msecs_from_midnight();  // Wait for 20 secs.
      } else {                             // Prices moved greater than 10 ticks , most likely in the past 20 secs ,
        // an intervention occurred , getflat for an extended period of time.
        DBGLOG_TIME_CLASS_FUNC << "Staying in non_standard_market_conditions mode"
                               << " int_price_move_=" << int_price_move_ << DBGLOG_ENDL_FLUSH;
        non_standard_market_conditions_mode_prices_.clear();

        last_getflat_due_to_non_standard_market_conditions_triggered_at_msecs_ =
            watch_.msecs_from_midnight() +
            (param_set_.non_standard_market_condition_check_long_msecs_ -
             param_set_.non_standard_market_condition_check_short_msecs_);  // Wait for 15 mins.
      }
    }
  }

  ProcessTimePeriodUpdate(num_pages_to_add_);
}

void DirectionalInterventionAggressiveTrading::TradingLogic() {
  // setting top level directives
  top_bid_place_ = false;
  top_bid_keep_ = false;
  top_bid_improve_ = false;
  top_ask_lift_ = false;

  // if based on current risk level or if trading is stopped ... check if we have any allowance to place orders at top
  // level
  if ((current_tradevarset_.l1bid_trade_size_ > 0) &&
      ((my_position_ < param_set_.non_standard_market_condition_max_position_) || (!IsLargeAskOrderPresent()))) {
    // first check for cooloff_interval
    if ((last_buy_msecs_ > 0) && (watch_.msecs_from_midnight() - last_buy_msecs_ < param_set_.cooloff_interval_) &&
        (best_nonself_bid_int_price_ >= last_buy_int_price_)) {
      // no bids at this or higher prices now
    } else {
      // check if the margin of buying
      // i.e. ( targetbias_numbers_ )
      // exceeds the threshold current_tradevarset_.l1bid_place_
      if ((best_nonself_bid_size_ > param_set_.safe_distance_) ||
          ((targetbias_numbers_ +
                ((dep_market_view_.spread_increments() > 1) ? (param_set_.high_spread_allowance_) : 0.0) >=
            current_tradevarset_.l1bid_place_) &&
           (best_nonself_bid_size_ > param_set_.min_size_to_join_) &&
           (!dep_market_view_.trade_update_implied_quote()))) {
        top_bid_place_ = true;
        top_bid_keep_ = true;

        if (watch_.msecs_from_midnight() - last_agg_buy_msecs_ > param_set_.agg_cooloff_interval_) {
          /* aggressive and improve */
          if ((param_set_.allowed_to_aggress_) && /* external control on aggressing */
              (targetbias_numbers_ >=
               current_tradevarset_.l1bid_aggressive_) &&  // only LIFT offer if the margin of buying exceeds the
                                                           // threshold current_tradevarset_.l1bid_aggressive_
              (my_position_ <=
               param_set_.max_position_to_lift_) && /* Don't LIFT offer when my_position_ is already decently long */
              (dep_market_view_.spread_increments() <=
               param_set_.max_int_spread_to_cross_) &&  // Don't LIFT when effective spread is to much
              (param_set_.max_size_to_aggress_ <= 0 ||
               best_nonself_ask_size_ <
                   param_set_.max_size_to_aggress_) &&  // Don't LIFT offer if offer side is very thick
              ((last_buy_msecs_ <= 0) ||
               (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
               ((best_nonself_ask_int_price_ - last_buy_int_price_) <
                (param_set_.px_band_ -
                 1)))  // TODO_OPT : later change setting of last_buy_int_price_ to include px_band_
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
                (targetbias_numbers_ >= current_tradevarset_.l1bid_improve_) &&
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
             ((dep_market_view_.spread_increments() > 1) ? (param_set_.high_spread_allowance_) : 0.0) +
             bestbid_queue_hysterisis_) >= current_tradevarset_.l1bid_keep_) {
          top_bid_keep_ = true;
        } else {
          top_bid_keep_ = false;
        }
      }
    }
  }

  top_ask_place_ = false;
  top_ask_keep_ = false;
  top_ask_improve_ = false;
  top_bid_hit_ = false;

  // if based on current risk level or if trading is stopped ... check if we have any allowance to place orders at top
  // level
  if (  // check if we have any allowance to place orders at top level
      (current_tradevarset_.l1ask_trade_size_ > 0) &&
      ((my_position_ > -param_set_.non_standard_market_condition_max_position_) || (!IsLargeBidOrderPresent())) &&
      // first check for cooloff_interval
      ((last_sell_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
       (last_sell_int_price_ - best_nonself_ask_int_price_ <
        (param_set_.px_band_ - 1)))  // later change setting of last_sell_int_price_ to include px_band_
      ) {
    // check if the margin of placing limit ask orders at the price ( best_nonself_ask_price_ )
    // i.e. ( best_nonself_ask_price_ - target_price_ )
    //      exceeds the threshold current_tradevarset_.l1ask_place_

    if ((best_nonself_ask_size_ > param_set_.safe_distance_) ||
        (((-targetbias_numbers_ +
           ((dep_market_view_.spread_increments() > 1) ? (param_set_.high_spread_allowance_) : 0.0)) >=
          current_tradevarset_.l1ask_place_) &&
         (best_nonself_ask_size_ > param_set_.min_size_to_join_) && (!dep_market_view_.trade_update_implied_quote()))) {
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
            (-targetbias_numbers_ >= current_tradevarset_.l1ask_aggressive_) &&
            (param_set_.max_size_to_aggress_ <= 0 ||
             best_nonself_bid_size_ < param_set_.max_size_to_aggress_) &&  // Don't HIT bid if bid side is very thick
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
              (targetbias_numbers_ >= current_tradevarset_.l1ask_improve_) &&
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
           ((dep_market_view_.spread_increments() > 1) ? (param_set_.high_spread_allowance_) : 0.0) +
           bestask_queue_hysterisis_) >=
          current_tradevarset_.l1ask_keep_) {  // but with place in line effect enough to keep the live order there
        top_ask_keep_ = true;
      } else {
        top_ask_keep_ = false;
      }
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
    if ((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_) == 0) &&
        (order_manager_.SumBidSizeConfirmedAboveIntPrice(best_nonself_bid_int_price_) == 0)) {
      int _canceled_size_ = 0;
      if (!top_bid_keep_) {  // if due to position placing aggressive LIFT order requires us to cancel bids from all
                             // active levels
        _canceled_size_ += order_manager_.CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_);
        if (_canceled_size_ > 0) canceled_bids_this_round_ = true;
      }

      int allowance_for_aggressive_buy_ = my_position_ + order_manager_.SumBidSizes() +
                                          current_tradevarset_.l1bid_trade_size_ - param_set_.worst_case_position_;

      if (param_set_.use_new_aggress_logic_) {
        allowance_for_aggressive_buy_ = my_position_ + order_manager_.SumBidSizes() +
                                        current_tradevarset_.l1bid_trade_size_ -
                                        std::max(param_set_.worst_case_position_, param_set_.max_position_);
      }

      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the bid side
      if (allowance_for_aggressive_buy_ >= 0) {
        // and if size canceled already is less than l1bid_trade_size_
        if (allowance_for_aggressive_buy_ > _canceled_size_) {
          // then cancel Bids from bottom levels for the required size
          _canceled_size_ += order_manager_.CancelBidsFromFar(current_tradevarset_.l1bid_trade_size_);
        }

        // if ( _canceled_size_ >= current_tradevarset_.l1bid_trade_size_ )
        //   {
        //     order_manager_.SendTrade ( best_nonself_ask_price_, best_nonself_ask_int_price_,
        //     current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'A' ) ;
        //     placed_bids_this_round_ = true;
        //     last_agg_buy_msecs_ = watch_.msecs_from_midnight();
        // if ( dbglogger_.CheckLoggingLevel ( TRADING_ERROR ) )
        //   {
        //     DBGLOG_TIME_CLASS_FUNC
        //       << "Sending aggressive B at px " << best_nonself_ask_int_price_
        //       << " position " << my_position_ << " cancelled size " << _canceled_size_
        //       << " low bid sizes "
        //       << DBGLOG_ENDL_FLUSH ;
        //   }
        //   }
      } else {
        /* Place new order */
        order_manager_.SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                 current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'A');
        placed_bids_this_round_ = true;
        last_agg_buy_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "Sending aggressive B at px " << best_nonself_ask_int_price_ << " position "
                                 << my_position_ << " cancelled size " << _canceled_size_ << " low bid sizes "
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    } else {
      order_manager_.CancelBidsAboveIntPrice(
          best_nonself_bid_int_price_);  // only after canceling them can we be allowed to place aggressive orders
    }
  }

  if ((!placed_bids_this_round_) && (top_bid_improve_)) {
    // only place Bid Improve orders when there is no active unconfirmed order and no confirmed orders above the best
    // price
    if ((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_ + 1) == 0) &&
        (order_manager_.SumBidSizeConfirmedAboveIntPrice(best_nonself_bid_int_price_) == 0)) {
      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the bid side
      if (my_position_ + order_manager_.SumBidSizes() + current_tradevarset_.l1bid_trade_size_ >=
          param_set_.worst_case_position_) {
        int _canceled_size_ = 0;
        // then cancel Bids from bottom levels for the required size
        _canceled_size_ += order_manager_.CancelBidsFromFar(current_tradevarset_.l1bid_trade_size_);

        // if ( _canceled_size_ >= current_tradevarset_.l1bid_trade_size_ )
        //   {
        //     order_manager_.SendTradeIntPx ( (best_nonself_bid_int_price_ + 1),
        //     current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'I' ) ;
        //     last_agg_buy_msecs_ = watch_.msecs_from_midnight();
        //     placed_bids_this_round_ = true;
        // if ( dbglogger_.CheckLoggingLevel ( TRADING_ERROR ) )
        //   {
        //     DBGLOG_TIME_CLASS_FUNC
        //       << "Sending improve B at px " << best_nonself_bid_int_price_ +1
        //       << " position " << my_position_ << " cancelled size " << _canceled_size_
        //       << " low bid sizes "
        //       << DBGLOG_ENDL_FLUSH ;
        //   }
        //   }
      } else {
        /* Place new order */
        order_manager_.SendTradeIntPx((best_nonself_bid_int_price_ + 1), current_tradevarset_.l1bid_trade_size_,
                                      kTradeTypeBuy, 'I');
        last_agg_buy_msecs_ = watch_.msecs_from_midnight();
        placed_bids_this_round_ = true;
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "Sending improve B at px " << best_nonself_bid_int_price_ + 1 << " position "
                                 << my_position_ << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  if (!placed_bids_this_round_) {  // only come here if no aggressive or improve bid orders sent this cycle
    if (top_bid_place_) {
      // only place Bid orders when there is no active unconfirmed order and no confirmed order at or above the best
      // price
      if ((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_) == 0) &&
          (order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) == 0) &&
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
          DBGLOG_TIME_CLASS_FUNC << "SendTrade B of " << current_tradevarset_.l1bid_trade_size_ << " @ "
                                 << best_nonself_bid_price_
                                 << " tgt_bias: " << targetbias_numbers_ / dep_market_view_.min_price_increment()
                                 << " thresh_t: "
                                 << current_tradevarset_.l1bid_place_ / dep_market_view_.min_price_increment()
                                 << " Int Px: " << best_nonself_bid_int_price_ << " tMktSz: " << best_nonself_bid_size_
                                 << " Mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << "  ---  "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
        }
      }
    } else {
      if (!top_bid_keep_) {  // cancel all orders at best_nonself_bid_price_
        int canceled_size_ = order_manager_.CancelBidsAtIntPrice(best_nonself_bid_int_price_);
        if (canceled_size_ > 0) {
          canceled_bids_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
          {
            DBGLOG_TIME << "Canceled B of " << canceled_size_ << " @ " << best_nonself_bid_price_
                        << " tgt_bias: " << targetbias_numbers_ / dep_market_view_.min_price_increment()
                        << " thresh_t: " << current_tradevarset_.l1bid_keep_ / dep_market_view_.min_price_increment()
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

      int allowance_for_aggressive_sell_ = -my_position_ + order_manager_.SumAskSizes() +
                                           current_tradevarset_.l1ask_trade_size_ - param_set_.worst_case_position_;

      if (param_set_.use_new_aggress_logic_) {
        allowance_for_aggressive_sell_ = -my_position_ + order_manager_.SumAskSizes() +
                                         current_tradevarset_.l1ask_trade_size_ -
                                         std::max(param_set_.worst_case_position_, param_set_.max_position_);
      }

      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the ask side
      if (allowance_for_aggressive_sell_ >= 0) {
        // and if size canceled already is less than l1ask_trade_size_
        if (allowance_for_aggressive_sell_ > _canceled_size_) {
          _canceled_size_ += order_manager_.CancelAsksFromFar(
              current_tradevarset_.l1ask_trade_size_);  ///< then cancel Asks from bottom levels for the required size
        }

        // if ( _canceled_size_ >= current_tradevarset_.l1ask_trade_size_ )
        //   {
        //     order_manager_.SendTrade ( best_nonself_bid_price_, best_nonself_bid_int_price_,
        //     current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'A' ) ;
        //     last_agg_sell_msecs_ = watch_.msecs_from_midnight();
        //     placed_asks_this_round_ = true;
        // if ( dbglogger_.CheckLoggingLevel ( TRADING_ERROR ) )
        //   {
        //     DBGLOG_TIME_CLASS_FUNC
        //       << "Sending aggressive S at px " << best_nonself_bid_int_price_
        //       << " position " << my_position_ << " cancelled size " << _canceled_size_
        //       << " low ask sizes "
        //       << DBGLOG_ENDL_FLUSH ;
        //   }
        //   }
      } else {
        /* Place new order */
        order_manager_.SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                 current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'A');
        placed_asks_this_round_ = true;
        last_agg_sell_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "Sending aggressive S at px " << best_nonself_bid_int_price_ << " position "
                                 << my_position_ << DBGLOG_ENDL_FLUSH;
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
      if (-my_position_ + order_manager_.SumAskSizes() + current_tradevarset_.l1ask_trade_size_ >=
          param_set_.worst_case_position_) {
        int _canceled_size_ = 0;
        _canceled_size_ += order_manager_.CancelAsksFromFar(
            current_tradevarset_.l1ask_trade_size_);  ///< then cancel Asks from bottom levels for the required size

        // if ( _canceled_size_ >= current_tradevarset_.l1ask_trade_size_ )
        //   {
        //     order_manager_.SendTradeIntPx ( (best_nonself_ask_int_price_ - 1),
        //     current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'I' ) ;
        //     last_agg_sell_msecs_ = watch_.msecs_from_midnight();
        //     placed_asks_this_round_ = true;
        // if ( dbglogger_.CheckLoggingLevel ( TRADING_ERROR ) )
        //   {
        //     DBGLOG_TIME_CLASS_FUNC
        //       << "Sending improve S at px " << best_nonself_ask_int_price_ -1
        //       << " position " << my_position_ << " cancelled size " << _canceled_size_
        //       << " low ask sizes "
        //       << DBGLOG_ENDL_FLUSH ;
        //   }
        //   }
      } else {
        /* Place new order */
        order_manager_.SendTradeIntPx((best_nonself_ask_int_price_ - 1), current_tradevarset_.l1ask_trade_size_,
                                      kTradeTypeSell, 'I');
        placed_asks_this_round_ = true;
        last_agg_sell_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "Sending improve S at px " << best_nonself_ask_int_price_ - 1 << " position "
                                 << my_position_ << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  if (!placed_asks_this_round_) {
    // get to this location if no aggressive or improve orders placed in this cycle
    if (top_ask_place_) {
      // only place Ask orders when there is no active unconfirmed order at or above best_nonself_ask_int_price_
      //      and no confirmed order at or above the best price
      if ((order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_) == 0) &&
          (order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) == 0) &&
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
          DBGLOG_TIME_CLASS_FUNC << "SendTrade S of " << current_tradevarset_.l1ask_trade_size_ << " @ "
                                 << best_nonself_ask_price_
                                 << " tgt_bias: " << -targetbias_numbers_ / dep_market_view_.min_price_increment()
                                 << " thresh_t: "
                                 << current_tradevarset_.l1ask_place_ / dep_market_view_.min_price_increment()
                                 << " Int Px: " << best_nonself_ask_int_price_ << " tMktSz: " << best_nonself_ask_size_
                                 << " Mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " ---- "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
        }
      }
    } else {
      if (!top_ask_keep_) {
        // cancel all orders at best_nonself_ask_price_
        int canceled_size_ = order_manager_.CancelAsksAtIntPrice(best_nonself_ask_int_price_);
        if (canceled_size_ > 0) {
          canceled_asks_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
          {
            DBGLOG_TIME << "Canceled S of " << canceled_size_ << " @ " << best_nonself_ask_price_
                        << " tgt_bias: " << -targetbias_numbers_ / dep_market_view_.min_price_increment()
                        << " thresh_t: " << current_tradevarset_.l1ask_keep_ / dep_market_view_.min_price_increment()
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

void DirectionalInterventionAggressiveTrading::PrintFullStatus() {
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

void DirectionalInterventionAggressiveTrading::OnMarketUpdate(const unsigned int _security_id_,
                                                              const MarketUpdateInfo& _market_update_info_) {
  NonSelfMarketUpdate();
  /* no need to call TradingLogic since we expect a UpdateTarget call after this */

  UpdateNonStandardMarketConditionVars(_market_update_info_);

  if (!getflat_due_to_non_standard_market_conditions_ && AreNonStandardMarketConditions(_market_update_info_)) {
    getflat_due_to_non_standard_market_conditions_ = true;
    non_standard_market_conditions_mode_prices_.clear();
    last_getflat_due_to_non_standard_market_conditions_triggered_at_msecs_ = watch_.msecs_from_midnight();

    DBGLOG_TIME_CLASS_FUNC << " getflat_due_to_non_standard_market_conditions_ of " << dep_market_view_.shortcode()
                           << " spread_increments_=" << _market_update_info_.spread_increments_ << DBGLOG_ENDL_FLUSH;
    ProcessGetFlat();

    if (livetrading_ &&  // live-trading and within trading window
        (watch_.msecs_from_midnight() > trading_start_utc_mfm_) &&
        (watch_.msecs_from_midnight() < trading_end_utc_mfm_)) {  // Send out an email / alert
      char hostname_[128];
      hostname_[127] = '\0';
      gethostname(hostname_, 127);

      std::string getflat_email_string_ = "";
      {
        std::ostringstream t_oss_;
        t_oss_ << "Strategy: " << runtime_id_ << " getflat_due_to_non_standard_market_conditions_ of "
               << dep_market_view_.shortcode() << " = " << dep_market_view_.secname()
               << " spread_increments_= " << _market_update_info_.spread_increments_ << " on " << hostname_ << "\n";

        getflat_email_string_ = t_oss_.str();
      }

      HFSAT::Email email_;
      email_.setSubject(getflat_email_string_);
      email_.addRecepient("nseall@tworoads.co.in");
      email_.addSender("nseall@tworoads.co.in");
      email_.content_stream << getflat_email_string_ << "<br/>";
      email_.sendMail();
    }
  }
}
}
