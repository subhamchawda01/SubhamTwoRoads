// =====================================================================================
//
//       Filename:  trading_logic.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  04/07/2015 03:10:11 PM
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

#include "dvctrade/ExecLogic/pairs_trading.hpp"

namespace HFSAT {

void PairsTrading::DatTradingLogic(int _product_index_) {
  current_product_ = product_vec_[_product_index_];
  my_risk_ = (int)current_product_->beta_adjusted_position_;
  double new_targetbias_numbers = targetbias_numbers_vec_[_product_index_];

  TradeVars_t &current_global_tradevarset_ = current_global_tradevarset_vec_[_product_index_];
  ParamSet *t_paramset_ = prod_paramset_vec_[_product_index_];
  SecurityMarketView *p_dep_market_view_ = dep_market_view_vec_[_product_index_];

  best_nonself_ask_int_price_ = current_product_->best_nonself_ask_int_price_;
  best_nonself_ask_price_ = current_product_->best_nonself_ask_price_;
  best_nonself_ask_size_ = current_product_->best_nonself_ask_size_;
  best_nonself_bid_int_price_ = current_product_->best_nonself_bid_int_price_;
  best_nonself_bid_price_ = current_product_->best_nonself_bid_price_;
  best_nonself_bid_size_ = current_product_->best_nonself_bid_size_;

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " TargetBiasNumber: " << new_targetbias_numbers
                                << " Targetprice: " << target_price_vec_[_product_index_] << " "
                                << dep_market_view_vec_[_product_index_]->mid_price() << " "
                                << dep_market_view_vec_[_product_index_]->shortcode()
                                << " stdev: " << current_product_->m_stdev_
                                << " histsprd: " << current_product_->hist_avg_spread_
                                << " cursprd: " << current_product_->moving_avg_spread_
                                << " Best_Bid_Place_Cxl_Px: " << best_int_bid_place_cxl_px_
                                << " Best_Ask_Place_Cxl_Px: " << best_int_ask_place_cxl_px_ << " thresh: bid "
                                << current_global_tradevarset_.l1bid_place_
                                << " ask: " << current_global_tradevarset_.l1ask_place_ << " pos: " << my_risk_
                                << " prodPos: " << current_product_->position_ << DBGLOG_ENDL_FLUSH;
  }

  top_bid_place_ = false;
  top_bid_keep_ = false;
  top_bid_improve_ = false;
  top_ask_lift_ = false;
  bid_improve_keep_ = false;

  if (current_global_tradevarset_.l1bid_trade_size_ > 0) {
    // first check for cooloff_interval
    if ((current_product_->last_buy_msecs_ > 0) &&
        (watch_.msecs_from_midnight() - current_product_->last_buy_msecs_ < t_paramset_->cooloff_interval_) &&
        (best_nonself_bid_int_price_ >= current_product_->last_buy_int_price_)) {
      // no bids at this or higher prices now
    } else {
      // check if the margin of buying
      // i.e. ( new_targetbias_numbers )
      // exceeds the threshold current_global_tradevarset_.l1bid_place_
      if ((best_nonself_bid_size_ > t_paramset_->safe_distance_) ||
          ((new_targetbias_numbers + ((p_dep_market_view_->spread_increments() > current_product_->moving_avg_spread_)
                                          ? (t_paramset_->high_spread_allowance_)
                                          : 0.0) >=
            current_global_tradevarset_.l1bid_place_ - l1_bias_vec_[_product_index_] -
                l1_order_bias_vec_[_product_index_] - short_positioning_bias_vec_[_product_index_]) &&
           (best_nonself_bid_size_ > t_paramset_->min_size_to_join_) &&
           (t_paramset_->place_on_trade_update_implied_quote_ || !p_dep_market_view_->trade_update_implied_quote()))) {
        top_bid_place_ = true;
        top_bid_keep_ = true;

        if (watch_.msecs_from_midnight() - last_agg_buy_msecs_ > t_paramset_->agg_cooloff_interval_) {
          /* aggressive and improve */
          if ((t_paramset_->allowed_to_aggress_) &&
              /* external control on aggressing */
              (new_targetbias_numbers >= current_global_tradevarset_.l1bid_aggressive_) &&
              // only LIFT offer if the margin of buying exceeds the threshold
              // current_global_tradevarset_.l1bid_aggressive_
              (my_risk_ <= t_paramset_->max_position_to_lift_) &&
              /* Don't LIFT offer when my_position_ is already decently long */
              (p_dep_market_view_->spread_increments() <= t_paramset_->max_int_spread_to_cross_) &&
              // Don't LIFT when effective spread is to much
              ((current_product_->last_buy_msecs_ <= 0) ||
               (watch_.msecs_from_midnight() - current_product_->last_buy_msecs_ >= t_paramset_->cooloff_interval_) ||
               ((best_nonself_ask_int_price_ - current_product_->last_buy_int_price_) < (t_paramset_->px_band_ - 1)))
              // TODO_OPT : later change setting of current_product_->last_buy_int_price_ to include px_band_
              )

          {
            top_ask_lift_ = true;

            // when we are already long and ask_lift is set to true,
            // place and keep at top bid are set false so that
            // we cancel top level orders before we place new ones at aggressive prices */
            if (my_risk_ >= t_paramset_->max_position_to_cancel_on_lift_) {
              top_bid_place_ = false;
              top_bid_keep_ = false;
            }
          } else {
            top_ask_lift_ = false;

            // conditions to place market improving bid orders:
            // ALLOWED_TO_IMPROVE
            // position is not too long already, spread wide, signal strong
            if ((t_paramset_->allowed_to_improve_) && (my_risk_ <= t_paramset_->max_position_to_bidimprove_) &&
                /* Don't improve bid when my_position_ is already decently long */
                (p_dep_market_view_->spread_increments() >= t_paramset_->min_int_spread_to_improve_) &&
                (new_targetbias_numbers >= current_global_tradevarset_.l1bid_improve_) &&
                ((current_product_->last_buy_msecs_ <= 0) ||
                 (watch_.msecs_from_midnight() - current_product_->last_buy_msecs_ >= t_paramset_->cooloff_interval_))
                // TODO_OPT : later change setting of current_product_->last_buy_int_price_ to include px_band_
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

        if ((new_targetbias_numbers + ((p_dep_market_view_->spread_increments() > current_product_->moving_avg_spread_)
                                           ? (t_paramset_->high_spread_allowance_)
                                           : 0.0) +
             bestbid_queue_hysterisis_) >= current_global_tradevarset_.l1bid_keep_ - l1_bias_vec_[_product_index_] -
                                               short_positioning_bias_vec_[_product_index_] - l1_order_bias_) {
          top_bid_keep_ = true;
        } else {
          top_bid_keep_ = false;
        }
      }

      if ((p_dep_market_view_->spread_increments() > 1) &&
          (new_targetbias_numbers + ((p_dep_market_view_->spread_increments() > current_product_->moving_avg_spread_)
                                         ? (t_paramset_->high_spread_allowance_)
                                         : 0.0)) >=
              current_global_tradevarset_.l1bid_improve_keep_ - l1_bias_vec_[_product_index_] -
                  short_positioning_bias_vec_[_product_index_] - l1_order_bias_vec_[_product_index_]) {
        bid_improve_keep_ = true;
      } else {
        bid_improve_keep_ = false;
      }
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
      (current_global_tradevarset_.l1ask_trade_size_ > 0) &&
      // first check for cooloff_interval
      ((current_product_->last_sell_msecs_ <= 0) ||
       (watch_.msecs_from_midnight() - current_product_->last_sell_msecs_ >= t_paramset_->cooloff_interval_))
      // later change setting of current_product_->last_sell_int_price_ to include px_band_
      ) {
    // check if the margin of placing limit ask orders at the price ( best_nonself_ask_price_ )
    // i.e. ( best_nonself_ask_price_ - target_price_ )
    //      exceeds the threshold current_global_tradevarset_.l1ask_place_

    if ((best_nonself_ask_size_ > t_paramset_->safe_distance_) ||
        (((-new_targetbias_numbers +
           ((p_dep_market_view_->spread_increments() > 1) ? (t_paramset_->high_spread_allowance_) : 0.0)) >=
          current_global_tradevarset_.l1ask_place_ - l1_bias_vec_[_product_index_] -
              long_positioning_bias_vec_[_product_index_] - l1_order_bias_vec_[_product_index_]) &&
         (best_nonself_ask_size_ > t_paramset_->min_size_to_join_) &&
         (t_paramset_->place_on_trade_update_implied_quote_ || !p_dep_market_view_->trade_update_implied_quote()))) {
      top_ask_place_ = true;
      top_ask_keep_ = true;

      if (watch_.msecs_from_midnight() - current_product_->last_agg_sell_msecs_ > t_paramset_->agg_cooloff_interval_) {
        /* aggressive and improve */
        // conditions to place aggressive orders:
        // ALLOWED_TO_AGGRESS
        // position is not too short already, spread narrow, signal strong
        if ((t_paramset_->allowed_to_aggress_) && /* external control on aggressing */

            (my_risk_ >= t_paramset_->min_position_to_hit_) &&
            /* Don't HIT bid when my_position_ is already decently short */
            (best_nonself_ask_int_price_ - best_nonself_bid_int_price_ <= t_paramset_->max_int_spread_to_cross_) &&
            /* Don't HIT ( cross ) when effective spread is to much */
            (-new_targetbias_numbers >= current_global_tradevarset_.l1ask_aggressive_) &&
            ((current_product_->last_sell_msecs_ <= 0) ||
             (watch_.msecs_from_midnight() - current_product_->last_sell_msecs_ >= t_paramset_->cooloff_interval_))
            // TODO_OPT : later change setting of, current_product_->last_sell_int_price_ to include px_band_
            )

        {
          top_bid_hit_ = true;

          /* when we are already short and bit_hit is set to true,
             place and keep at top ask are set false so that
             we cancel top level orders before we place new ones at aggressive prices */
          if (my_risk_ <= t_paramset_->min_position_to_cancel_on_hit_) {
            top_ask_place_ = false;
            top_ask_keep_ = false;
          }
        } else {
          top_bid_hit_ = false;
          if ((t_paramset_->allowed_to_improve_) && (my_risk_ >= t_paramset_->min_position_to_askimprove_) &&
              /* Don't improve ask when my_position_ is already decently short */
              (best_nonself_ask_int_price_ - best_nonself_bid_int_price_ >= t_paramset_->min_int_spread_to_improve_) &&
              (new_targetbias_numbers >= current_global_tradevarset_.l1ask_improve_) &&
              ((current_product_->last_sell_msecs_ <= 0) ||
               (watch_.msecs_from_midnight() - current_product_->last_sell_msecs_ >= t_paramset_->cooloff_interval_))
              // TODO_OPT : later change setting of, current_product_->last_sell_int_price_ to include px_band_
              ) {
            top_ask_improve_ = true;
          } else {
            top_ask_improve_ = false;
          }
        }
      }

    } else {
      // signal not strog enough to place limit orders at the best ask level
      if ((-new_targetbias_numbers + ((p_dep_market_view_->spread_increments() > current_product_->moving_avg_spread_)
                                          ? (t_paramset_->high_spread_allowance_)
                                          : 0.0) +
           bestask_queue_hysterisis_) >= current_global_tradevarset_.l1ask_keep_ - l1_bias_vec_[_product_index_] -
                                             l1_order_bias_vec_[_product_index_] -
                                             long_positioning_bias_vec_[_product_index_]) {
        // but with place in line, effect enough to keep, the live order there
        top_ask_keep_ = true;
      } else {
        top_ask_keep_ = false;
      }
    }
    if ((p_dep_market_view_->spread_increments() > 1) &&
        (-new_targetbias_numbers + ((p_dep_market_view_->spread_increments() > current_product_->moving_avg_spread_)
                                        ? (t_paramset_->high_spread_allowance_)
                                        : 0.0)) >=
            current_global_tradevarset_.l1ask_improve_keep_ - l1_bias_vec_[_product_index_] -
                l1_order_bias_vec_[_product_index_] - long_positioning_bias_vec_[_product_index_]) {
      ask_improve_keep_ = true;
    } else {
      ask_improve_keep_ = false;
    }
  }

  // After setting top-level directives ...
  // get to order placing or canceling part

  // Active BID rder management
  placed_bids_this_round_ = false;
  canceled_bids_this_round_ = false;
  if (top_ask_lift_) {
    // only place aggressive orders when there is no active unconfirmed order and no confirmed orders above the best
    // price and no confirmed orders above the best_nonself_bid_price_
    if ((order_manager_vec_[_product_index_]->SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_ + 1) ==
         0) &&
        (order_manager_vec_[_product_index_]->SumBidSizeConfirmedAboveIntPrice(best_nonself_bid_int_price_) == 0)) {
      int _canceled_size_ = 0;
      if (!top_bid_keep_) {  // if due to position placing aggressive LIFT order requires us to cancel bids from all
                             // active levels
        _canceled_size_ += order_manager_vec_[_product_index_]->CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_);
        if (_canceled_size_ > 0) {
          canceled_bids_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC_LINE << " Cancelling bids eq above : " << best_nonself_bid_int_price_
                                        << " Size: " << _canceled_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }

      int allowance_for_aggressive_buy_ = my_risk_ + order_manager_vec_[_product_index_]->SumBidSizes() +
                                          current_global_tradevarset_.l1bid_trade_size_ -
                                          t_paramset_->worst_case_position_;

      if (t_paramset_->use_new_aggress_logic_) {
        allowance_for_aggressive_buy_ = my_risk_ + order_manager_vec_[_product_index_]->SumBidSizes() +
                                        current_global_tradevarset_.l1bid_trade_size_ -
                                        std::max(t_paramset_->worst_case_position_, t_paramset_->max_position_);
      }

      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the bid side
      if (allowance_for_aggressive_buy_ >= 0) {
        // and if size canceled already is less than l1bid_trade_size_
        if (allowance_for_aggressive_buy_ > _canceled_size_) {
          // then cancel Bids from bottom levels for the required size
          _canceled_size_ +=
              order_manager_vec_[_product_index_]->CancelBidsFromFar(current_global_tradevarset_.l1bid_trade_size_);

          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC_LINE << " Cancelling bids eq above : " << best_nonself_bid_int_price_
                                        << " Size: " << _canceled_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      } else {
        /* Place new order */
        order_manager_vec_[_product_index_]->SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                                       current_global_tradevarset_.l1bid_trade_size_, kTradeTypeBuy,
                                                       'A');
        placed_bids_this_round_ = true;
        current_product_->last_agg_buy_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "Sending aggressive B at px " << best_nonself_ask_int_price_ << " position "
                                 << my_risk_ << " cancelled size " << _canceled_size_ << " low bid sizes "
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    } else {
      int canceled_size_ = order_manager_vec_[_product_index_]->CancelBidsAboveIntPrice(
          best_nonself_bid_int_price_);  // only after canceling them can we be allowed to place aggressive orders
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " Cancelling bids eq above : " << best_nonself_bid_int_price_
                                    << " Size: " << canceled_size_ << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  if ((!placed_bids_this_round_) && (top_bid_improve_)) {
    // only place Bid Improve orders when there is no active unconfirmed order and no confirmed orders above the best
    // price
    if ((order_manager_vec_[_product_index_]->SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_ + 1) ==
         0) &&
        (order_manager_vec_[_product_index_]->SumBidSizeConfirmedAboveIntPrice(best_nonself_bid_int_price_) == 0)) {
      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the bid side
      if (my_risk_ + order_manager_vec_[_product_index_]->SumBidSizes() +
              current_global_tradevarset_.l1bid_trade_size_ >=
          t_paramset_->worst_case_position_) {
        int _canceled_size_ = 0;
        // then cancel Bids from bottom levels for the required size
        _canceled_size_ +=
            order_manager_vec_[_product_index_]->CancelBidsFromFar(current_global_tradevarset_.l1bid_trade_size_);

        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " Cancelling bids eq above : " << best_nonself_bid_int_price_
                                      << " Size: " << _canceled_size_ << DBGLOG_ENDL_FLUSH;
        }
      } else {
        /* Place new order */
        order_manager_vec_[_product_index_]->SendTradeIntPx(
            (best_nonself_bid_int_price_ + 1), current_global_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'I');
        current_product_->last_agg_buy_msecs_ = watch_.msecs_from_midnight();
        placed_bids_this_round_ = true;
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "Sending improve B at px " << best_nonself_bid_int_price_ + 1 << " position "
                                 << my_risk_ << DBGLOG_ENDL_FLUSH;
        }
      }
    }

  } else {
    if ((p_dep_market_view_->spread_increments() > 1) &&
        (!bid_improve_keep_))  // can have size check here for optimization purpose
    {
      int cancelled_size_ = order_manager_vec_[_product_index_]->CancelBidsAboveIntPrice(best_nonself_bid_int_price_);
      if (cancelled_size_ > 0) {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " Cancelled Improve Bid orders Above: " << best_nonself_bid_int_price_
                                 << " Position: " << my_risk_
                                 << " tgt_bias: " << new_targetbias_numbers / p_dep_market_view_->min_price_increment()
                                 << " thresh_t: "
                                 << current_global_tradevarset_.l1bid_improve_keep_ /
                                        p_dep_market_view_->min_price_increment() << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  if (!placed_bids_this_round_) {  // only come here if no aggressive or improve bid orders sent this cycle
    if (top_bid_place_) {
      // only place Bid orders when there is no active unconfirmed order and no confirmed order at or above the best
      // price
      if ((((order_manager_vec_[_product_index_]->SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_) ==
             0) &&
            (order_manager_vec_[_product_index_]->SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) ==
             0))) &&
          (stdev_ <= t_paramset_->low_stdev_lvl_ ||
           (p_dep_market_view_->spread_increments() <=
            t_paramset_
                ->max_int_spread_to_place_)))  // Don't place any new orders in inside market if the spread is too wide
      {
        order_manager_vec_[_product_index_]->SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                                       current_global_tradevarset_.l1bid_trade_size_, kTradeTypeBuy,
                                                       'B');
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "SendTrade B of " << current_global_tradevarset_.l1bid_trade_size_ << " @ "
                                 << best_nonself_bid_price_
                                 << " tgt_bias: " << new_targetbias_numbers / p_dep_market_view_->min_price_increment()
                                 << " thresh_t: "
                                 << current_global_tradevarset_.l1bid_place_ / p_dep_market_view_->min_price_increment()
                                 << " Int Px: " << best_nonself_bid_int_price_ << " tMktSz: " << best_nonself_bid_size_
                                 << " Mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << "  ---  "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
        }
      }
    } else {
      if (!top_bid_keep_) {  // cancel all orders at best_nonself_bid_price_

        int canceled_size_ = 0;
        canceled_size_ = order_manager_vec_[_product_index_]->CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_);
        if (canceled_size_ > 0) {
          canceled_bids_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
          {
            DBGLOG_TIME << "Canceled B of " << canceled_size_ << " EqAbove " << best_nonself_bid_price_
                        << " tgt_bias: " << new_targetbias_numbers / p_dep_market_view_->min_price_increment()
                        << " thresh_t: "
                        << current_global_tradevarset_.l1bid_keep_ / p_dep_market_view_->min_price_increment()
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
    if ((order_manager_vec_[_product_index_]->SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_ - 1) ==
         0) &&
        (order_manager_vec_[_product_index_]->SumAskSizeConfirmedAboveIntPrice(best_nonself_ask_int_price_) == 0)) {
      int _canceled_size_ = 0;
      if (!top_ask_keep_) {  // if due to position placing aggressive LIFT order requires us to cancel asks from all
                             // active levels
        _canceled_size_ += order_manager_vec_[_product_index_]->CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_);
      }

      int allowance_for_aggressive_sell_ = -my_risk_ + order_manager_vec_[_product_index_]->SumAskSizes() +
                                           current_global_tradevarset_.l1ask_trade_size_ -
                                           t_paramset_->worst_case_position_;

      if (t_paramset_->use_new_aggress_logic_) {
        allowance_for_aggressive_sell_ = -my_risk_ + order_manager_vec_[_product_index_]->SumAskSizes() +
                                         current_global_tradevarset_.l1ask_trade_size_ -
                                         std::max(t_paramset_->max_position_, t_paramset_->max_position_);
      }

      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the ask side
      if (allowance_for_aggressive_sell_ >= 0) {
        // and if size canceled already is less than l1ask_trade_size_
        if (allowance_for_aggressive_sell_ > _canceled_size_) {
          _canceled_size_ += order_manager_vec_[_product_index_]->CancelAsksFromFar(
              current_global_tradevarset_
                  .l1ask_trade_size_);  ///< then cancel Asks from bottom levels for the required size
        }
      } else {
        /* Place new order */
        order_manager_vec_[_product_index_]->SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                                       current_global_tradevarset_.l1ask_trade_size_, kTradeTypeSell,
                                                       'A');
        placed_asks_this_round_ = true;
        current_product_->last_agg_sell_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "Sending aggressive S at px " << best_nonself_bid_int_price_ << " position "
                                 << my_risk_ << DBGLOG_ENDL_FLUSH;
        }
      }

    } else {
      order_manager_vec_[_product_index_]->CancelAsksAboveIntPrice(
          best_nonself_ask_int_price_);  // only after canceling them can we be allowed to place aggressive orders
    }
  }

  if ((!placed_asks_this_round_) && (top_ask_improve_)) {
    // only place Ask Improve orders when there is no active unconfirmed order and no confirmed orders above the best
    // price
    if ((order_manager_vec_[_product_index_]->SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_ - 1) ==
         0) &&
        (order_manager_vec_[_product_index_]->SumAskSizeConfirmedAboveIntPrice(best_nonself_ask_int_price_) == 0)) {
      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the ask side
      if (-my_risk_ + order_manager_vec_[_product_index_]->SumAskSizes() +
              current_global_tradevarset_.l1ask_trade_size_ >=
          t_paramset_->worst_case_position_) {
        int _canceled_size_ = 0;
        _canceled_size_ += order_manager_vec_[_product_index_]->CancelAsksFromFar(
            current_global_tradevarset_
                .l1ask_trade_size_);  ///< then cancel Asks from bottom levels for the required size
      } else {
        /* Place new order */
        order_manager_vec_[_product_index_]->SendTradeIntPx(
            (best_nonself_ask_int_price_ - 1), current_global_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'I');
        placed_asks_this_round_ = true;
        current_product_->last_agg_sell_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "Sending improve S at px " << best_nonself_ask_int_price_ - 1 << " position "
                                 << my_risk_ << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  } else {
    if ((p_dep_market_view_->spread_increments() > 1) && (!ask_improve_keep_))
    /*
     * Need to cancel these kind of orders only when the spread > 1
     * In other cases it wont be benificial except wasting 1 message
     */
    {
      int cancelled_size_ = order_manager_vec_[_product_index_]->CancelAsksAboveIntPrice(best_nonself_ask_int_price_);
      if (cancelled_size_ > 0) {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " Cancelling Improve Ask orders Above: " << best_nonself_ask_int_price_
                                 << " position: " << my_risk_
                                 << " tgt_bias: " << new_targetbias_numbers / p_dep_market_view_->min_price_increment()
                                 << " thresh_t: "
                                 << current_global_tradevarset_.l1ask_improve_keep_ /
                                        p_dep_market_view_->min_price_increment() << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  if (!placed_asks_this_round_) {
    // get to this location if no aggressive or improve orders placed in this cycle
    if (top_ask_place_) {
      // only place Ask orders when there is no active unconfirmed order at or above best_nonself_ask_int_price_
      //      and no confirmed order at or above the best price
      if ((((order_manager_vec_[_product_index_]->SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_) ==
             0) &&
            (order_manager_vec_[_product_index_]->SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) ==
             0))) &&
          (stdev_ <= t_paramset_->low_stdev_lvl_ ||
           (p_dep_market_view_->spread_increments() <=
            t_paramset_
                ->max_int_spread_to_place_)))  // Don't place any new orders in inside market if the spread is too wide
      {
        order_manager_vec_[_product_index_]->SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                                       current_global_tradevarset_.l1ask_trade_size_, kTradeTypeSell,
                                                       'B');
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "SendTrade S of " << current_global_tradevarset_.l1ask_trade_size_ << " @ "
                                 << best_nonself_ask_price_
                                 << " tgt_bias: " << -new_targetbias_numbers / p_dep_market_view_->min_price_increment()
                                 << " thresh_t: "
                                 << current_global_tradevarset_.l1ask_place_ / p_dep_market_view_->min_price_increment()
                                 << " Int Px: " << best_nonself_ask_int_price_ << " tMktSz: " << best_nonself_ask_size_
                                 << " Mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " ---- "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
        }
      }
    } else {
      if (!top_ask_keep_) {
        // cancel all orders at best_nonself_ask_price_
        int canceled_size_ = 0;
        order_manager_vec_[_product_index_]->CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_);
        if (canceled_size_ > 0) {
          canceled_asks_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
          {
            DBGLOG_TIME << "Canceled S of " << canceled_size_ << " EqAbove " << best_nonself_ask_price_
                        << " tgt_bias: " << -new_targetbias_numbers / p_dep_market_view_->min_price_increment()
                        << " thresh_t: "
                        << current_global_tradevarset_.l1ask_keep_ / p_dep_market_view_->min_price_increment()
                        << " tMktSz: " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
  }

  // zero logging
  if ((dbglogger_.CheckLoggingLevel(TRADING_INFO) && (placed_bids_this_round_ || placed_asks_this_round_))) {
    PrintFullStatus(_product_index_);
  }

  // temporarily printing indicators every time
  if ((livetrading_ || dbglogger_.CheckLoggingLevel(TRADING_INFO)) &&
      (placed_bids_this_round_ || placed_asks_this_round_ || canceled_bids_this_round_ || canceled_asks_this_round_)) {
    dump_inds = true;
  }
}

void PairsTrading::PbatTradingLogic(int _product_index_) {
  int our_bid_orders_ = 0;
  int our_ask_orders_ = 0;

  current_product_ = product_vec_[_product_index_];
  my_risk_ = (int)current_product_->beta_adjusted_position_;
  // double new_targetbias_numbers = targetbias_numbers_vec_[_product_index_];
  double new_target_price = target_price_vec_[_product_index_];

  TradeVars_t &current_global_tradevarset_ = current_global_tradevarset_vec_[_product_index_];
  ParamSet *t_paramset_ = prod_paramset_vec_[_product_index_];
  SecurityMarketView *p_dep_market_view_ = dep_market_view_vec_[_product_index_];

  best_nonself_ask_int_price_ = current_product_->best_nonself_ask_int_price_;
  best_nonself_ask_price_ = current_product_->best_nonself_ask_price_;
  best_nonself_ask_size_ = current_product_->best_nonself_ask_size_;
  best_nonself_bid_int_price_ = current_product_->best_nonself_bid_int_price_;
  best_nonself_bid_price_ = current_product_->best_nonself_bid_price_;
  best_nonself_bid_size_ = current_product_->best_nonself_bid_size_;

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " TargetBiasNumber: " << targetbias_numbers_vec_[_product_index_]
                                << " TargetPrice: " << target_price_vec_[_product_index_] << " "
                                << " PortfolioTgtBias: " << targetbias_numbers_
                                << " PortfolioTgtPrice: " << target_price_ << p_dep_market_view_->mid_price() << " "
                                << p_dep_market_view_->shortcode() << " StdevVal: " << current_product_->m_stdev_
                                << " HistSprd: " << current_product_->hist_avg_spread_
                                << " CurSprd: " << current_product_->moving_avg_spread_
                                << " Best_Bid_Place_Cxl_Px: " << best_int_bid_place_cxl_px_
                                << " Best_Ask_Place_Cxl_Px: " << best_int_ask_place_cxl_px_
                                << " Thresh_Bid: " << current_global_tradevarset_.l1bid_place_
                                << " Thresh_Ask : " << current_global_tradevarset_.l1ask_place_
                                << " GlobalPos: " << my_risk_ << " ProdPos: " << current_product_->position_
                                << DBGLOG_ENDL_FLUSH;
  }

  top_bid_place_ = false;
  top_bid_keep_ = false;
  top_bid_improve_ = false;
  top_ask_lift_ = false;
  bid_improve_keep_ = false;

  if (  // check if we have any allowance to place orders at top level
      (current_global_tradevarset_.l1bid_trade_size_ > 0) &&
      // first check for cooloff_interval
      ((current_product_->last_buy_msecs_ <= 0) ||
       (watch_.msecs_from_midnight() - current_product_->last_buy_msecs_ >= t_paramset_->cooloff_interval_) ||
       ((best_nonself_bid_int_price_ - current_product_->last_buy_int_price_) < (t_paramset_->px_band_ - 1))) &&
      // later change setting of last_buy_int_price_ to include px_band_
      (last_bigtrades_bid_cancel_msecs_ <= 0 ||
       (watch_.msecs_from_midnight() - last_bigtrades_bid_cancel_msecs_ >= t_paramset_->bigtrades_cooloff_interval_))) {
    // check if the margin of buying at the price ( best_nonself_bid_price_ )
    // i.e. ( target_price_ - best_nonself_bid_price_ )
    // exceeds the threshold current_tradevarset_.l1bid_place_
    if ((best_nonself_bid_size_ > t_paramset_->safe_distance_) ||
        ((new_target_price - best_nonself_bid_price_ >=
          current_global_tradevarset_.l1bid_place_ - l1_bias_vec_[_product_index_] -
              l1_order_bias_vec_[_product_index_] - short_positioning_bias_vec_[_product_index_] +
              t_paramset_->spread_increase_ * (p_dep_market_view_->spread_increments() - 1)) &&
         (best_nonself_bid_size_ > t_paramset_->min_size_to_join_) &&
         // Don't place any orders at a level if less than X size there
         (t_paramset_->place_on_trade_update_implied_quote_ || !p_dep_market_view_->trade_update_implied_quote()))) {
      top_bid_place_ = true;
      top_bid_keep_ = true;

      if (watch_.msecs_from_midnight() - current_product_->last_agg_buy_msecs_ > t_paramset_->agg_cooloff_interval_) {
        // conditions to place aggressive orders:
        // ALLOWED_TO_AGGRESS
        // position is not too long already, spread narrow, signal strong
        if ((t_paramset_->allowed_to_aggress_) &&
            // external control on aggressing
            (new_target_price - best_nonself_ask_price_ >=
             current_global_tradevarset_.l1bid_aggressive_ +
                 t_paramset_->spread_increase_ * (p_dep_market_view_->spread_increments() - 1)) &&
            // only LIFT offer if the margin of buying here i.e. (
            // new_target_price - best_nonself_ask_price_ ) exceeds the
            // threshold current_tradevarset_.l1bid_aggressive_
            (my_risk_ <= t_paramset_->max_position_to_lift_) &&
            // Don't LIFT offer when my_position_ is already decently long
            (p_dep_market_view_->spread_increments() <= t_paramset_->max_int_spread_to_cross_) &&
            // Don't LIFT when effective spread is to much
            ((current_product_->last_buy_msecs_ <= 0) ||
             (watch_.msecs_from_midnight() - current_product_->last_buy_msecs_ >= t_paramset_->cooloff_interval_) ||
             ((best_nonself_ask_int_price_ - current_product_->last_buy_int_price_) < (t_paramset_->px_band_ - 1)))
            // TODO_OPT : later change setting of last_buy_int_price_ to include px_band_
            ) {
          top_ask_lift_ = true;

          // when we are already long and ask_lift is set to true,
          // place and keep at top bid are set false so that
          // we cancel top level orders before we place new ones at aggressive prices
          if (my_risk_ >= t_paramset_->max_position_to_cancel_on_lift_) {
            top_bid_place_ = false;
            top_bid_keep_ = false;
          }
        } else {
          top_ask_lift_ = false;

          // conditions to place market improving bid orders:
          // ALLOWED_TO_IMPROVE
          // position is not too long already, spread wide, signal strong
          if ((t_paramset_->allowed_to_improve_) && (my_risk_ <= t_paramset_->max_position_to_bidimprove_) &&
              // Don't improve bid when my_position_ is already decently long
              (p_dep_market_view_->spread_increments() >= t_paramset_->min_int_spread_to_improve_) &&
              (new_target_price - best_nonself_bid_price_ - p_dep_market_view_->min_price_increment() >=
               current_global_tradevarset_.l1bid_improve_) &&
              ((current_product_->last_buy_msecs_ <= 0) ||
               (watch_.msecs_from_midnight() - current_product_->last_buy_msecs_ >= t_paramset_->cooloff_interval_) ||
               (((best_nonself_bid_int_price_ + 1) - current_product_->last_buy_int_price_) <
                (t_paramset_->px_band_ - 1)))
              // TODO_OPT : later change setting of last_buy_int_price_ to include px_band_
              ) {
            top_bid_improve_ = true;
          } else {
            top_bid_improve_ = false;
          }
        }
      }

    } else {
      // either size less than min_size_to_join_
      // or signal is not strong enough for placing bids at best_nonself_bid_price_
      // check if we should retain existing bids due to place_in_line

      // TODO : perhaps we should make place_in_line effect bestbid_queue_hysterisis_ smarter
      //        for instance when short term volatility in the market is very high
      //        then being high in the queue should count for less.
      if ((new_target_price - best_nonself_bid_price_ + bestbid_queue_hysterisis_) >=
          current_global_tradevarset_.l1bid_keep_ - l1_bid_trade_bias_ - l1_bias_vec_[_product_index_] -
              l1_order_bias_vec_[_product_index_] - short_positioning_bias_vec_[_product_index_] +
              t_paramset_->spread_increase_ * (p_dep_market_view_->spread_increments() - 1)) {
        top_bid_keep_ = true;
      } else {
        top_bid_keep_ = false;
      }
    }

    if (((p_dep_market_view_->spread_increments() > 1) &&
         (new_target_price - best_nonself_bid_price_) >=
             current_global_tradevarset_.l1bid_improve_keep_ - l1_bias_vec_[_product_index_] -
                 l1_order_bias_vec_[_product_index_] - short_positioning_bias_vec_[_product_index_] +
                 t_paramset_->spread_increase_ * (p_dep_market_view_->spread_increments() - 1))) {
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

  if (  // check if we have any allowance to place orders at top level
      (current_global_tradevarset_.l1ask_trade_size_ > 0) &&
      // first check for cooloff_interval
      ((current_product_->last_sell_msecs_ <= 0) ||
       (watch_.msecs_from_midnight() - current_product_->last_sell_msecs_ >= t_paramset_->cooloff_interval_) ||
       (current_product_->last_sell_int_price_ - best_nonself_ask_int_price_ < (t_paramset_->px_band_ - 1))) &&
      // later change setting of last_sell_int_price_ to include px_band_
      (last_bigtrades_ask_cancel_msecs_ <= 0 ||
       (watch_.msecs_from_midnight() - last_bigtrades_ask_cancel_msecs_ >= t_paramset_->bigtrades_cooloff_interval_))) {
    // check if the margin of placing limit ask orders at the price ( best_nonself_ask_price_ )
    // i.e. ( best_nonself_ask_price_ - new_target_price )
    //      exceeds the threshold current_tradevarset_.l1ask_place_
    if ((best_nonself_ask_size_ > t_paramset_->safe_distance_) ||
        ((best_nonself_ask_price_ - new_target_price >=
          current_global_tradevarset_.l1ask_place_ - l1_bias_vec_[_product_index_] -
              l1_order_bias_vec_[_product_index_] - long_positioning_bias_vec_[_product_index_] +
              t_paramset_->spread_increase_ * (p_dep_market_view_->spread_increments() - 1)) &&
         (best_nonself_ask_size_ > t_paramset_->min_size_to_join_) &&
         (t_paramset_->place_on_trade_update_implied_quote_ || !p_dep_market_view_->trade_update_implied_quote()))) {
      top_ask_place_ = true;
      top_ask_keep_ = true;

      if (watch_.msecs_from_midnight() - current_product_->last_agg_sell_msecs_ > t_paramset_->agg_cooloff_interval_) {
        // conditions to place aggressive orders:
        // ALLOWED_TO_AGGRESS, position is not too short already, spread narrow, signal strong
        if ((t_paramset_->allowed_to_aggress_) &&  // external control on aggressing
            (my_risk_ >= t_paramset_->min_position_to_hit_) &&
            // Don't HIT bid when my_position_ is already decently short
            (best_nonself_ask_int_price_ - best_nonself_bid_int_price_ <= t_paramset_->max_int_spread_to_cross_) &&
            // Don't HIT ( cross ) when effective spread is to much
            (best_nonself_bid_price_ - new_target_price >=
             current_global_tradevarset_.l1ask_aggressive_ +
                 t_paramset_->spread_increase_ * (p_dep_market_view_->spread_increments() - 1)) &&
            ((current_product_->last_sell_msecs_ <= 0) ||
             (watch_.msecs_from_midnight() - current_product_->last_sell_msecs_ >= t_paramset_->cooloff_interval_) ||
             ((current_product_->last_sell_int_price_ - best_nonself_bid_int_price_) < (t_paramset_->px_band_ - 1)))
            // TODO_OPT : later change setting of last_sell_int_price_ to include px_band_
            )

        {
          top_bid_hit_ = true;

          // when we are already short and bit_hit is set to true,
          // place and keep at top ask are set false so that
          // we cancel top level orders before we place new ones at aggressive prices
          if (my_risk_ <= t_paramset_->min_position_to_cancel_on_hit_) {
            top_ask_place_ = false;
            top_ask_keep_ = false;
          }
        } else {
          top_bid_hit_ = false;
          if ((t_paramset_->allowed_to_improve_) &&
              (my_risk_ >=
               t_paramset_
                   ->min_position_to_askimprove_) &&  // Don't improve ask when my_position_ is already decently short
              (best_nonself_ask_int_price_ - best_nonself_bid_int_price_ >= t_paramset_->min_int_spread_to_improve_) &&
              (best_nonself_ask_price_ - new_target_price - p_dep_market_view_->min_price_increment() >=
               current_global_tradevarset_.l1ask_improve_) &&
              ((current_product_->last_sell_msecs_ <= 0) ||
               (watch_.msecs_from_midnight() - current_product_->last_sell_msecs_ >= param_set_.cooloff_interval_) ||
               (((current_product_->last_sell_int_price_ - 1) - best_nonself_ask_int_price_) <
                (param_set_.px_band_ - 1)))
              // TODO_OPT : later change setting of last_sell_int_price_ to include px_band_
              ) {
            top_ask_improve_ = true;
          } else {
            top_ask_improve_ = false;
          }
        }
      }
    } else {
      // signal not strog enough to place limit orders at the best ask level
      if ((best_nonself_ask_price_ - new_target_price + bestask_queue_hysterisis_) >=
          current_global_tradevarset_.l1ask_keep_ - l1_ask_trade_bias_ - l1_bias_vec_[_product_index_] -
              l1_order_bias_vec_[_product_index_] - long_positioning_bias_vec_[_product_index_] +
              t_paramset_->spread_increase_ * (p_dep_market_view_->spread_increments() - 1)) {
        // but with place in line effect enough to keep the live order there
        top_ask_keep_ = true;
      } else {
        top_ask_keep_ = false;
      }
    }

    if ((p_dep_market_view_->spread_increments() > 1) &&
        ((best_nonself_ask_price_ - new_target_price) >=
         current_global_tradevarset_.l1ask_improve_keep_ - l1_bias_vec_[_product_index_] -
             l1_order_bias_vec_[_product_index_] - long_positioning_bias_vec_[_product_index_] +
             t_paramset_->spread_increase_ * (p_dep_market_view_->spread_increments() - 1))) {
      ask_improve_keep_ = true;
    } else {
      ask_improve_keep_ = false;
    }
  }

  // After setting top-level directives ...
  // get to order placing or canceling part

  // Active BID order management
  placed_bids_this_round_ = false;
  canceled_bids_this_round_ = false;

  if (top_ask_lift_) {
    // only place aggressive bid orders when there is no active unconfirmed order at or above best_nonself_bid_price_
    // and no confirmed orders above the best_nonself_bid_price_
    if ((order_manager_vec_[_product_index_]->SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_ + 1) ==
         0) &&
        (order_manager_vec_[_product_index_]->SumBidSizeConfirmedAboveIntPrice(best_nonself_bid_int_price_) == 0)) {
      int _canceled_size_ = 0;
      if (!top_bid_keep_) {
        // if because of my_position_ being greater than max_position_to_cancel_on_lift_
        // placing aggressive LIFT order requires us to cancel bids from all active levels
        // where an actve level is >= best_nonself_bid_price_
        _canceled_size_ += order_manager_vec_[_product_index_]->CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_);
        if (_canceled_size_ > 0) canceled_bids_this_round_ = true;
      }

      int allowance_for_aggressive_buy_ = my_risk_ + order_manager_vec_[_product_index_]->SumBidSizes() +
                                          current_global_tradevarset_.l1bid_trade_size_ -
                                          t_paramset_->worst_case_position_;

      if (t_paramset_->use_new_aggress_logic_) {
        allowance_for_aggressive_buy_ = my_risk_ + order_manager_vec_[_product_index_]->SumBidSizes() +
                                        current_global_tradevarset_.l1bid_trade_size_ -
                                        std::max(t_paramset_->worst_case_position_, t_paramset_->max_position_);
      }

      if (allowance_for_aggressive_buy_ >= 0) {
        if (allowance_for_aggressive_buy_ > _canceled_size_) {
          _canceled_size_ +=
              order_manager_vec_[_product_index_]->CancelBidsFromFar(current_global_tradevarset_.l1bid_trade_size_);
        }
      }

      else {
        // Place new order
        order_manager_vec_[_product_index_]->SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                                       current_global_tradevarset_.l1bid_trade_size_, kTradeTypeBuy,
                                                       'A');
        placed_bids_this_round_ = true;
        current_product_->last_agg_buy_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "Sending aggressive B at px " << best_nonself_ask_price_ << " risk " << my_risk_
                                 << DBGLOG_ENDL_FLUSH;
          model_math_vec_[_product_index_]->ShowIndicatorValues();
        }
      }
    } else {
      // only after canceling them can we be allowed to place aggressive orders
      order_manager_vec_[_product_index_]->CancelBidsAboveIntPrice(best_nonself_bid_int_price_);
    }
  }

  if ((!placed_bids_this_round_) && (top_bid_improve_)) {
    // only place Bid Improve orders when there is no active unconfirmed order at or above the
    // best_nonself_bid_int_price_
    //    and no confirmed orders above the best price
    if ((order_manager_vec_[_product_index_]->SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_ + 1) ==
         0) &&
        (order_manager_vec_[_product_index_]->SumBidSizeConfirmedAboveIntPrice(best_nonself_bid_int_price_) == 0)) {
      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the bid side
      if (my_risk_ + order_manager_vec_[_product_index_]->SumBidSizes() +
              current_global_tradevarset_.l1bid_trade_size_ >=
          t_paramset_->worst_case_position_) {
        int _canceled_size_ = 0;
        // then cancel Bids from bottom levels for the required size
        _canceled_size_ +=
            order_manager_vec_[_product_index_]->CancelBidsFromFar(current_global_tradevarset_.l1bid_trade_size_);

      } else {
        // Place new order
        order_manager_vec_[_product_index_]->SendTradeIntPx(
            (best_nonself_bid_int_price_ + 1), current_global_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'I');
        placed_bids_this_round_ = true;
        current_product_->last_agg_buy_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "Sending improve B at px " << best_nonself_bid_int_price_ + 1 << " position "
                                 << my_risk_ << DBGLOG_ENDL_FLUSH;
          model_math_vec_[_product_index_]->ShowIndicatorValues();
        }
      }
    }

  } else {
    if ((p_dep_market_view_->spread_increments() > 1) && (!bid_improve_keep_)) {
      int cancelled_size_ = order_manager_vec_[_product_index_]->CancelBidsAboveIntPrice(best_nonself_bid_int_price_);
      if (cancelled_size_ > 0) {
        canceled_bids_this_round_ = true;
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " Cancelled Improve Bid orders Above: " << best_nonself_bid_int_price_
                                 << " Position: " << my_risk_ << " ebp_t: "
                                 << (new_target_price - best_nonself_bid_price_ + bestbid_queue_hysterisis_) /
                                        p_dep_market_view_->min_price_increment() << " thresh_t: "
                                 << current_global_tradevarset_.l1bid_improve_keep_ /
                                        p_dep_market_view_->min_price_increment() << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  if (!placed_bids_this_round_) {  // only come here if no aggressive or improve bid orders sent this cycle
    if (top_bid_place_) {
      // only place best level Bid orders when there is no active unconfirmed order at or above
      // best_nonself_bid_int_price_
      // and no confirmed order at or above the best price
      if ((((order_manager_vec_[_product_index_]->SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_) ==
             0) &&
            (order_manager_vec_[_product_index_]->SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) ==
             0))) &&
          (stdev_ <= t_paramset_->low_stdev_lvl_ ||
           (p_dep_market_view_->spread_increments() <= t_paramset_->max_int_spread_to_place_)))  // Don't place any new
      // orders in inside market
      // if the spread is too wide
      {
        order_manager_vec_[_product_index_]->SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                                       current_global_tradevarset_.l1bid_trade_size_, kTradeTypeBuy,
                                                       'B');
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "SendTrade B of " << current_global_tradevarset_.l1bid_trade_size_ << " @ "
                                 << best_nonself_bid_price_ << " ebp_t: "
                                 << (new_target_price - best_nonself_bid_price_) /
                                        p_dep_market_view_->min_price_increment() << " thresh_t: "
                                 << current_bid_tradevarset_.l1bid_place_ / p_dep_market_view_->min_price_increment()
                                 << " IntPx: " << best_nonself_bid_int_price_ << " tMktSz: " << best_nonself_bid_size_
                                 << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_
                                 << " StdevSpreadFactor: " << stdev_scaled_capped_in_ticks_ << DBGLOG_ENDL_FLUSH;
          model_math_vec_[_product_index_]->ShowIndicatorValues();
        }

        placed_bids_this_round_ = true;
      }
    } else {
      if (!top_bid_keep_) {  // cancel all orders at best_nonself_bid_price_
        int canceled_size_ = 0;
        canceled_size_ = order_manager_vec_[_product_index_]->CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_);

        // int canceled_size_ = order_manager_.CancelBidsEqAboveIntPrice ( best_nonself_bid_int_price_ ) ;
        if (canceled_size_ > 0) {
          canceled_bids_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "Canceled B of " << canceled_size_ << " eqAbove " << best_nonself_bid_price_
                                   << " SizeToCancel: " << our_bid_orders_ + my_risk_ << " ebp_t: "
                                   << (new_target_price - best_nonself_bid_price_ + bestbid_queue_hysterisis_) /
                                          p_dep_market_view_->min_price_increment() << " thresh_t: "
                                   << current_global_tradevarset_.l1bid_keep_ /
                                          p_dep_market_view_->min_price_increment()
                                   << " tMktSz: " << best_nonself_bid_size_ << " mkt: " << best_nonself_bid_size_
                                   << " @ " << best_nonself_bid_price_ << " X " << best_nonself_ask_price_ << " @ "
                                   << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
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
    if ((order_manager_vec_[_product_index_]->SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_ - 1) ==
         0) &&
        (order_manager_vec_[_product_index_]->SumAskSizeConfirmedAboveIntPrice(best_nonself_ask_int_price_) == 0)) {
      int _canceled_size_ = 0;
      if (!top_ask_keep_) {  // if due to position placing aggressive LIFT order requires us to cancel asks from all
                             // active levels
        _canceled_size_ += order_manager_vec_[_product_index_]->CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_);
      }

      int allowance_for_aggressive_sell_ = -my_risk_ + order_manager_vec_[_product_index_]->SumAskSizes() +
                                           current_global_tradevarset_.l1ask_trade_size_ -
                                           t_paramset_->worst_case_position_;

      if (t_paramset_->use_new_aggress_logic_) {
        allowance_for_aggressive_sell_ = -my_risk_ + order_manager_vec_[_product_index_]->SumAskSizes() +
                                         current_global_tradevarset_.l1ask_trade_size_ -
                                         std::max(t_paramset_->worst_case_position_, t_paramset_->max_position_);
      }

      if (allowance_for_aggressive_sell_ >= 0) {
        if (allowance_for_aggressive_sell_ > _canceled_size_) {
          _canceled_size_ += order_manager_vec_[_product_index_]->CancelAsksFromFar(
              current_global_tradevarset_
                  .l1ask_trade_size_);  ///< then cancel Asks from bottom levels for the required size
        }
      } else {
        // Place new order
        order_manager_vec_[_product_index_]->SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                                       current_global_tradevarset_.l1ask_trade_size_, kTradeTypeSell,
                                                       'A');
        placed_asks_this_round_ = true;
        current_product_->last_agg_sell_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "Sending aggressive S at px " << best_nonself_bid_price_ << " position " << my_risk_
                                 << DBGLOG_ENDL_FLUSH;
          model_math_vec_[_product_index_]->ShowIndicatorValues();
        }
      }
    } else {
      order_manager_vec_[_product_index_]->CancelAsksAboveIntPrice(
          best_nonself_ask_int_price_);  // only after canceling them can we be allowed to place aggressive orders
    }
  }
  if ((!placed_asks_this_round_) && (top_ask_improve_)) {
    // only place Ask Improve orders when there is no active unconfirmed order and no confirmed orders above the best
    // price
    if ((order_manager_vec_[_product_index_]->SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_ - 1) ==
         0) &&
        (order_manager_vec_[_product_index_]->SumAskSizeConfirmedAboveIntPrice(best_nonself_ask_int_price_) == 0)) {
      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the ask side
      if (-my_risk_ + order_manager_vec_[_product_index_]->SumAskSizes() +
              current_global_tradevarset_.l1ask_trade_size_ + current_global_tradevarset_.l1ask_trade_size_ >
          t_paramset_->worst_case_position_) {
        int _canceled_size_ = 0;
        _canceled_size_ += order_manager_vec_[_product_index_]->CancelAsksFromFar(
            current_global_tradevarset_
                .l1ask_trade_size_);  ///< then cancel Asks from bottom levels for the required size
      } else {
        // Place new order
        order_manager_vec_[_product_index_]->SendTradeIntPx(
            (best_nonself_ask_int_price_ - 1), current_global_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'I');
        placed_asks_this_round_ = true;
        current_product_->last_agg_sell_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "Sending improve S at px " << best_nonself_ask_int_price_ - 1 << " position "
                                 << my_risk_ << DBGLOG_ENDL_FLUSH;
          model_math_vec_[_product_index_]->ShowIndicatorValues();
        }
      }
    }

  } else {
    if (p_dep_market_view_->spread_increments() > 1 && !ask_improve_keep_) {
      int cancelled_size_ = order_manager_vec_[_product_index_]->CancelAsksAboveIntPrice(best_nonself_ask_int_price_);

      if (cancelled_size_ > 0) {
        canceled_asks_this_round_ = true;
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " Cancelling Improve Ask orders Above: " << best_nonself_ask_int_price_
                                 << " position: " << my_risk_ << " eap_t: "
                                 << (best_nonself_ask_price_ - new_target_price + bestask_queue_hysterisis_) /
                                        p_dep_market_view_->min_price_increment() << " thresh_t: "
                                 << current_global_tradevarset_.l1ask_improve_keep_ /
                                        p_dep_market_view_->min_price_increment() << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  if (!placed_asks_this_round_) {
    // get to this location if no aggressive or improve orders placed in this cycle
    if (top_ask_place_) {
      // only place Ask orders when there is no active unconfirmed order at or above best_nonself_ask_int_price_
      //      and no confirmed order at or above the best price
      if ((((order_manager_vec_[_product_index_]->SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_) ==
             0) &&
            (order_manager_vec_[_product_index_]->SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) ==
             0))) &&
          (stdev_ <= t_paramset_->low_stdev_lvl_ ||
           (p_dep_market_view_->spread_increments() <= t_paramset_->max_int_spread_to_place_)))  // Don't place any new
      // orders in inside market
      // if the spread is too wide
      {
        order_manager_vec_[_product_index_]->SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                                       current_global_tradevarset_.l1ask_trade_size_, kTradeTypeSell,
                                                       'B');
        placed_asks_this_round_ = true;
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "SendTrade S of " << current_global_tradevarset_.l1ask_trade_size_ << " @ "
                                 << best_nonself_ask_price_ << " eap_t: "
                                 << (best_nonself_ask_price_ - new_target_price) /
                                        p_dep_market_view_->min_price_increment() << " thresh_t: "
                                 << current_ask_tradevarset_.l1ask_place_ / p_dep_market_view_->min_price_increment()
                                 << " IntPx: " << best_nonself_ask_int_price_ << " tMktSz: " << best_nonself_ask_size_
                                 << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_
                                 << " StdevSpreadFactor: " << stdev_scaled_capped_in_ticks_ << DBGLOG_ENDL_FLUSH;
          model_math_vec_[_product_index_]->ShowIndicatorValues();
        }
      }
    } else {
      if (!top_ask_keep_) {
        // cancel all orders at best_nonself_ask_price_
        int canceled_size_ = 0;
        canceled_size_ = order_manager_vec_[_product_index_]->CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_);

        if (canceled_size_ > 0) {
          canceled_asks_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "Canceled S of " << canceled_size_ << " EqAbove " << best_nonself_ask_price_
                                   << " SizeToCancel " << (-our_ask_orders_ + my_risk_) << " eap_t: "
                                   << (best_nonself_ask_price_ - new_target_price + bestask_queue_hysterisis_) /
                                          p_dep_market_view_->min_price_increment() << " thresh_t: "
                                   << current_global_tradevarset_.l1ask_keep_ /
                                          p_dep_market_view_->min_price_increment()
                                   << " tMktSz: " << best_nonself_ask_size_ << " mkt: " << best_nonself_bid_size_
                                   << " @ " << best_nonself_bid_price_ << " X " << best_nonself_ask_price_ << " @ "
                                   << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
  }
  // for zero loging mode have moved to a higher dbg log level
  if ((dbglogger_.CheckLoggingLevel(TRADING_INFO) && (placed_bids_this_round_ || placed_asks_this_round_))) {
    PrintFullStatus(_product_index_);
    order_manager_vec_[_product_index_]->LogFullStatus();
  }

  // temporarily printing indicators every time
  if ((livetrading_ || dbglogger_.CheckLoggingLevel(TRADING_INFO)) &&
      (placed_bids_this_round_ || placed_asks_this_round_ || canceled_bids_this_round_ || canceled_asks_this_round_)) {
    dump_inds = true;
  }
}

void PairsTrading::CallPlaceCancelNonBestLevels(int _product_index_) { PlaceCancelNonBestLevels(_product_index_); }

void PairsTrading::PlaceCancelNonBestLevels(int _product_index_) {
  if (order_placing_logic_ == kDirectionalAggressiveTrading || order_placing_logic_ == kPriceBasedAggressiveTrading) {
    current_product_ = product_vec_[_product_index_];
    my_risk_ = (int)current_product_->beta_adjusted_position_;
    TradeVars_t &current_global_tradevarset_ = current_global_tradevarset_vec_[_product_index_];
    ParamSet *t_paramset_ = prod_paramset_vec_[_product_index_];
    SecurityMarketView *p_dep_market_view_ = dep_market_view_vec_[_product_index_];

    best_nonself_ask_int_price_ = current_product_->best_nonself_ask_int_price_;
    best_nonself_ask_price_ = current_product_->best_nonself_ask_price_;
    best_nonself_ask_size_ = current_product_->best_nonself_ask_size_;
    best_nonself_bid_int_price_ = current_product_->best_nonself_bid_int_price_;
    best_nonself_bid_price_ = current_product_->best_nonself_bid_price_;
    best_nonself_bid_size_ = current_product_->best_nonself_bid_size_;

    base_bid_price_ = target_price_vec_[_product_index_] - current_product_->m_stdev_;
    base_ask_price_ = target_price_vec_[_product_index_] + current_product_->m_stdev_;

    // compute them less frequently
    int t_position_ = my_risk_;

    const int t_worst_case_long_position_ =
        (t_paramset_->read_explicit_max_long_position_ ? t_paramset_->explicit_worst_case_long_position_
                                                       : t_paramset_->worst_case_position_);
    const int t_worst_case_short_position_ =
        (t_paramset_->read_explicit_max_short_position_ ? t_paramset_->explicit_worst_case_short_position_
                                                        : t_paramset_->worst_case_position_);

    // placing supporting orders to num_non_best_bid_levels_monitored_
    if (my_position_ <= (t_paramset_->max_position_ / 2) ||
        t_paramset_->ignore_max_pos_check_for_non_best_) {  // position is not very long
      unsigned int _start_index_ = 1;

      _start_index_ = std::max(_start_index_, t_paramset_->min_distance_for_non_best_);

      if (t_paramset_->min_size_ahead_for_non_best_ > 0) {
        unsigned int _size_ahead_ = 0;
        auto i = 0u;
        for (;
             _size_ahead_ <= t_paramset_->min_size_ahead_for_non_best_ && i <= t_paramset_->max_distance_for_non_best_;
             i++) {
          _size_ahead_ += p_dep_market_view_->bid_size(i);
        }
        _start_index_ = std::max(_start_index_, i);
      }

      for (unsigned int _level_index_ = _start_index_;
           _level_index_ < (_start_index_ + t_paramset_->num_non_best_bid_levels_monitored_ - 1);
           _level_index_++) {  // check levels from 2 to t_paramset_->num_non_best_bid_levels_monitored_

        if ((int)(t_position_ + order_manager_vec_[_product_index_]->SumBidSizes() +
                  (2 * t_paramset_->unit_trade_size_)) <
            t_worst_case_long_position_) {  // placing orders still keeps the total active orders within
                                            // worst_case_position_ then place orders at this level

          int _this_bid_int_price_ = (best_nonself_bid_int_price_ - _level_index_);
          if (order_manager_vec_[_product_index_]->GetTotalBidSizeOrderedAtIntPx(_this_bid_int_price_) ==
              0) {  // if no orders at this level
            order_manager_vec_[_product_index_]->SendTradeIntPx(
                _this_bid_int_price_, current_global_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'S');
            if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
              DBGLOG_TIME_CLASS_FUNC << "SupportingSendTrade B of " << current_global_tradevarset_.l1bid_trade_size_
                                     << " @ " << (p_dep_market_view_->min_price_increment() * _this_bid_int_price_)
                                     << " IntPx: " << _this_bid_int_price_ << " level: " << _level_index_
                                     << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                     << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << " sumbidsizes "
                                     << order_manager_vec_[_product_index_]->SumBidSizes() << " + my_position_ "
                                     << t_position_ << DBGLOG_ENDL_FLUSH;
            }
          }
        }
      }
    }

    if (my_position_ >= (-t_paramset_->max_position_ / 2) ||
        t_paramset_->ignore_max_pos_check_for_non_best_) {  // position is not too short

      unsigned int _start_index_ = 1;
      _start_index_ = std::max(_start_index_, t_paramset_->min_distance_for_non_best_);
      if (t_paramset_->min_size_ahead_for_non_best_ > 0) {
        unsigned int _size_ahead_ = 0;
        auto i = 0u;
        for (;
             _size_ahead_ <= t_paramset_->min_size_ahead_for_non_best_ && i <= t_paramset_->max_distance_for_non_best_;
             i++) {
          _size_ahead_ += p_dep_market_view_->bid_size(i);
        }
        _start_index_ = std::max(_start_index_, i);
      }

      for (unsigned int _level_index_ = _start_index_;
           _level_index_ < (_start_index_ + t_paramset_->num_non_best_ask_levels_monitored_ - 1);
           _level_index_++) {  // check levels from 2 to t_paramset_->num_non_best_ask_levels_monitored_

        if ((int)(-t_position_ + order_manager_vec_[_product_index_]->SumAskSizes() +
                  (2 * t_paramset_->unit_trade_size_)) <
            t_worst_case_short_position_) {  // placing orders still keeps the total active orders within
                                             // worst_case_position_ then place orders at this level

          int _this_ask_int_price_ = best_nonself_ask_int_price_ + _level_index_;
          if (order_manager_vec_[_product_index_]->GetTotalAskSizeOrderedAtIntPx(_this_ask_int_price_) ==
              0) {  // if no orders at this level
            order_manager_vec_[_product_index_]->SendTradeIntPx(
                _this_ask_int_price_, current_global_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'S');
            if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
              DBGLOG_TIME_CLASS_FUNC << "SupportingSendTrade S of " << current_global_tradevarset_.l1ask_trade_size_
                                     << " @ " << (p_dep_market_view_->min_price_increment() * _this_ask_int_price_)
                                     << " IntPx: " << _this_ask_int_price_ << " level: " << _level_index_
                                     << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                     << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << " sumasksizes "
                                     << order_manager_vec_[_product_index_]->SumAskSizes() << " + -my_position_ "
                                     << -t_position_ << DBGLOG_ENDL_FLUSH;
            }
          }
        }
      }
    }

    {
      // setting up aliases
      std::vector<int> &sum_bid_confirmed_ = order_manager_vec_[_product_index_]->SumBidConfirmed();
      int confirmed_top_bid_index_ = order_manager_vec_[_product_index_]->GetConfirmedTopBidIndex();
      int confirmed_bottom_bid_index_ = order_manager_vec_[_product_index_]->GetConfirmedBottomBidIndex();
      int confirmed_bid_index_ = confirmed_top_bid_index_;

      std::vector<int> &sum_bid_unconfirmed_ = order_manager_vec_[_product_index_]->SumBidUnconfirmed();
      int unconfirmed_top_bid_index_ = order_manager_vec_[_product_index_]->GetUnconfirmedTopBidIndex();
      int unconfirmed_bottom_bid_index_ = order_manager_vec_[_product_index_]->GetUnconfirmedBottomBidIndex();
      int unconfirmed_bid_index_ = unconfirmed_top_bid_index_;

      // std::vector < std::vector < BaseOrder * > > & bid_order_vec_ = order_manager_vec_[_product_index_]->BidOrderVec
      // ( );
      int order_vec_top_bid_index_ = order_manager_vec_[_product_index_]->GetOrderVecTopBidIndex();
      int order_vec_bottom_bid_index_ = order_manager_vec_[_product_index_]->GetOrderVecBottomBidIndex();

      unsigned int _size_seen_so_far_ = 0;
      int _max_size_resting_bids_ = t_worst_case_long_position_ - std::max(0, t_position_);
      if (order_vec_top_bid_index_ != -1) {
        for (int order_vec_index_ = order_vec_top_bid_index_; order_vec_index_ >= order_vec_bottom_bid_index_;
             order_vec_index_--) {
          // the following code makes this not look at top level orders
          // hence top level orders are unaffected by worst_case_position_
          if (order_manager_vec_[_product_index_]->GetBidIntPrice(order_vec_index_) >= best_nonself_bid_int_price_)
            continue;

          // _size_seen_so_far_ adds up all confirmed and unconfirmed sizes of bids at levels at or above ( closer to
          // midprice ) than this level
          // where this level refers to the int_price_ at order_vec_index_

          while (unconfirmed_bottom_bid_index_ != -1 && unconfirmed_bid_index_ >= unconfirmed_bottom_bid_index_ &&
                 order_manager_vec_[_product_index_]->GetBidIntPrice(unconfirmed_bid_index_) >=
                     order_manager_vec_[_product_index_]->GetBidIntPrice(
                         order_vec_index_))  // TODO: replace with index_ comparison
          {
            _size_seen_so_far_ += sum_bid_unconfirmed_[unconfirmed_bid_index_];
            unconfirmed_bid_index_--;
          }

          while (confirmed_bottom_bid_index_ != -1 && confirmed_bid_index_ >= confirmed_bottom_bid_index_ &&
                 order_manager_vec_[_product_index_]->GetBidIntPrice(confirmed_bid_index_) >=
                     order_manager_vec_[_product_index_]->GetBidIntPrice(
                         order_vec_index_))  // TODO: replace with index_ comparison
          {
            _size_seen_so_far_ += sum_bid_confirmed_[confirmed_bid_index_];
            confirmed_bid_index_--;
          }

          if ((int)_size_seen_so_far_ > _max_size_resting_bids_) {
            order_manager_vec_[_product_index_]->CancelBidsEqBelowIntPrice(
                order_manager_vec_[_product_index_]->GetBidIntPrice(order_vec_index_));

            if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
              DBGLOG_TIME_CLASS_FUNC << "CancelBidsEqBelowIntPrice "
                                     << order_manager_vec_[_product_index_]->GetBidIntPrice(order_vec_index_)
                                     << " since _size_seen_so_far_ = " << _size_seen_so_far_
                                     << " _max_size_resting_bids_ = " << _max_size_resting_bids_ << DBGLOG_ENDL_FLUSH;
            }
            break;
          }
        }
      }
    }

    {
      // setting up aliases
      std::vector<int> &sum_ask_confirmed_ = order_manager_vec_[_product_index_]->SumAskConfirmed();
      int confirmed_top_ask_index_ = order_manager_vec_[_product_index_]->GetConfirmedTopAskIndex();
      int confirmed_bottom_ask_index_ = order_manager_vec_[_product_index_]->GetConfirmedBottomAskIndex();
      int confirmed_ask_index_ = confirmed_top_ask_index_;

      std::vector<int> &sum_ask_unconfirmed_ = order_manager_vec_[_product_index_]->SumAskUnconfirmed();
      int unconfirmed_top_ask_index_ = order_manager_vec_[_product_index_]->GetUnconfirmedTopAskIndex();
      int unconfirmed_bottom_ask_index_ = order_manager_vec_[_product_index_]->GetUnconfirmedBottomAskIndex();
      int unconfirmed_ask_index_ = unconfirmed_top_ask_index_;

      // std::vector < std::vector < BaseOrder * > > & ask_order_vec_ = order_manager_vec_[_product_index_]->AskOrderVec
      // ( );
      int order_vec_top_ask_index_ = order_manager_vec_[_product_index_]->GetOrderVecTopAskIndex();
      int order_vec_bottom_ask_index_ = order_manager_vec_[_product_index_]->GetOrderVecBottomAskIndex();

      int _size_seen_so_far_ = 0;
      int _max_size_resting_asks_ = t_worst_case_short_position_ + std::min(0, t_position_);
      if (order_vec_top_ask_index_ != -1) {
        for (int order_vec_index_ = order_vec_top_ask_index_; order_vec_index_ >= order_vec_bottom_ask_index_;
             order_vec_index_--) {
          // the following code makes this not look at top level orders
          // hence top level orders are unaffected by worst_case_position_
          if (order_manager_vec_[_product_index_]->GetAskIntPrice(order_vec_index_) <= best_nonself_ask_int_price_)
            continue;

          // _size_seen_so_far_ adds up all confirmed and unconfirmed sizes of asks at levels at or above ( closer to
          // midprice ) than this level
          // where this level refers to the key of _intpx_2_ask_order_vec_iter_

          while (unconfirmed_top_ask_index_ != -1 && unconfirmed_ask_index_ >= unconfirmed_bottom_ask_index_ &&
                 order_manager_vec_[_product_index_]->GetAskIntPrice(unconfirmed_ask_index_) <=
                     order_manager_vec_[_product_index_]->GetAskIntPrice(order_vec_index_)) {
            _size_seen_so_far_ += sum_ask_unconfirmed_[unconfirmed_ask_index_];
            unconfirmed_ask_index_--;
          }

          while (confirmed_top_ask_index_ != -1 && confirmed_ask_index_ >= confirmed_bottom_ask_index_ &&
                 order_manager_vec_[_product_index_]->GetAskIntPrice(confirmed_ask_index_) <=
                     order_manager_vec_[_product_index_]->GetAskIntPrice(order_vec_index_)) {
            _size_seen_so_far_ += sum_ask_confirmed_[confirmed_ask_index_];
            confirmed_ask_index_--;
          }

          // if simply all orders in calculating the _size_seen_so_far_ get executed in a sweep then our position will
          // reach
          // _size_seen_so_far_ + my_position_
          // and if this number equals or exceeds ( t_paramset_->worst_case_position_ )
          // then cancel all orders we can at this level or below
          // to avoid confusion perhaps we should avoid placing multiple orders atthe same level
          // since otherwise this could cause placing and canceling
          if ((int)_size_seen_so_far_ > _max_size_resting_asks_) {
            order_manager_vec_[_product_index_]->CancelAsksEqBelowIntPrice(
                order_manager_vec_[_product_index_]->GetAskIntPrice(order_vec_index_));

            if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
              DBGLOG_TIME_CLASS_FUNC << "CancelAsksEqBelowIntPrice "
                                     << order_manager_vec_[_product_index_]->GetAskIntPrice(order_vec_index_)
                                     << " since _size_seen_so_far_ = " << _size_seen_so_far_
                                     << " _max_size_resting_asks_ = " << _max_size_resting_asks_ << DBGLOG_ENDL_FLUSH;
            }
            break;
          }
        }
      }
    }
  }
}

void PairsTrading::SetComputeTresholds(InstrumentInfo *current_product, ParamSet *t_paramset_,
                                       SecurityMarketView *p_dep_market_view_) {
  l1_size_indicator_vec_.push_back(nullptr);
  l1_order_indicator_vec_.push_back(nullptr);
  l1_bias_vec_.push_back(0);
  l1_order_bias_vec_.push_back(0);
  short_positioning_bias_vec_.push_back(0);
  long_positioning_bias_vec_.push_back(0);

  if (t_paramset_->read_max_min_diff_ || t_paramset_->read_high_uts_factor_) {
    int t_fractional_seconds_ = HFSAT::ExecLogicUtils::GetDurationForAvgL1Size(dep_market_view_.shortcode());
    l1_size_indicator_vec_[l1_size_indicator_vec_.size() - 1] = L1SizeTrend::GetUniqueInstance(
        dbglogger_, watch_,
        *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(p_dep_market_view_->shortcode())),
        t_fractional_seconds_, kPriceTypeMktSizeWPrice);
  }

  if (t_paramset_->read_max_min_diff_) {
    HFSAT::ExecLogicUtils::GetL1SizeBound(
        p_dep_market_view_->shortcode(), current_product->l1_size_lower_bound_, current_product->l1_size_upper_bound_,
        watch_, current_product->trading_start_utc_mfm_, current_product->trading_end_utc_mfm_, dbglogger_);
    if (current_product->l1_size_upper_bound_ - current_product->l1_size_lower_bound_ >= 1.0) {
      current_product->l1_norm_factor_ =
          1.0 / (current_product->l1_size_upper_bound_ - current_product->l1_size_lower_bound_);
    } else {
      current_product->l1_norm_factor_ = 0;
    }
    t_paramset_->max_min_diff_ *= p_dep_market_view_->min_price_increment();
  }

  if (t_paramset_->read_bucket_size_ && t_paramset_->read_positioning_thresh_decrease_) {
    //      t_paramset_->positioning_indicator_ = Positioning::GetUniqueInstance(
    //          dbglogger_, watch_,
    //          *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(p_dep_market_view_->shortcode())),
    //          t_paramset_->bucket_size_, kPriceTypeMidprice);
    t_paramset_->positioning_thresh_decrease_ *= p_dep_market_view_->min_price_increment();
  }

  if (t_paramset_->read_max_min_diff_order_) {
    int t_fractional_seconds_ = HFSAT::ExecLogicUtils::GetDurationForAvgL1Size(p_dep_market_view_->shortcode());
    HFSAT::ExecLogicUtils::GetL1OrderBound(
        p_dep_market_view_->shortcode(), current_product->l1_order_lower_bound_, current_product->l1_order_upper_bound_,
        watch_, current_product->trading_start_utc_mfm_, current_product->trading_end_utc_mfm_, dbglogger_);
    if (current_product->l1_order_upper_bound_ - current_product->l1_order_lower_bound_ >= 1.0) {
      current_product->l1_order_norm_factor_ =
          1.0 / (current_product->l1_order_upper_bound_ - current_product->l1_order_lower_bound_);
    } else {
      current_product->l1_order_norm_factor_ = 0.0;
    }

    l1_order_indicator_vec_[l1_order_indicator_vec_.size() - 1] = L1OrderTrend::GetUniqueInstance(
        dbglogger_, watch_,
        *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(p_dep_market_view_->shortcode())),
        t_fractional_seconds_, kPriceTypeMktSizeWPrice);
    t_paramset_->max_min_diff_order_ *= dep_market_view_.min_price_increment();
  }

  if (t_paramset_->read_scale_max_pos_) {
    t_paramset_->volume_ratio_indicator_ = RecentSimpleVolumeMeasure::GetUniqueInstance(
        dbglogger_, watch_,
        *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(p_dep_market_view_->shortcode())),
        t_paramset_->volume_history_secs_);

    double t_trading_volume_expected_ = HFSAT::ExecLogicUtils::GetMURVolBound(
        p_dep_market_view_->shortcode(), watch_, t_paramset_->volume_history_secs_,
        current_product->trading_start_utc_mfm_, current_product->trading_end_utc_mfm_);

    if (t_trading_volume_expected_ > 0 &&
        (t_paramset_->volume_upper_bound_ratio_ > t_paramset_->volume_lower_bound_ratio_)) {
      t_paramset_->volume_lower_bound_ = t_trading_volume_expected_ * t_paramset_->volume_lower_bound_ratio_;
      t_paramset_->volume_upper_bound_ = t_trading_volume_expected_ * t_paramset_->volume_upper_bound_ratio_;
    } else {
      t_paramset_->volume_lower_bound_ = 0.0;
      t_paramset_->volume_upper_bound_ = 1000000.0;
    }

    t_paramset_->volume_norm_factor_ =
        (double)(((int)floor(t_paramset_->max_position_ / t_paramset_->unit_trade_size_) - t_paramset_->base_mur_) /
                 (t_paramset_->volume_upper_bound_ - t_paramset_->volume_lower_bound_));
  }
}

void PairsTrading::ProcessTimePeriodUpdate(int num_pages_to_add_) {
  if (!initialized_) {
    Initialize();
  }

  BaseTrading::ComputeCurrentSeverity();
  if (order_placing_logic_ != kDirectionalAggressiveTrading && order_placing_logic_ != kPriceBasedAggressiveTrading) {
    return;
  }

  for (unsigned i = 0; i < product_vec_.size(); i++) {
    current_product_ = product_vec_[i];
    ParamSet *t_paramset_ = prod_paramset_vec_[i];

    my_risk_ = (int)current_product_->beta_adjusted_position_;

    if (l1_size_indicator_vec_[i] != NULL) {
      current_product_->moving_avg_l1_size_ = l1_size_indicator_vec_[i]->GetL1Factor();
    }

    if (t_paramset_->read_max_min_diff_) {
      double t_l1_factor_ = 0;
      if (current_product_->moving_avg_l1_size_ > current_product_->l1_size_upper_bound_) {
        t_l1_factor_ = 1.0;
      } else if (current_product_->moving_avg_l1_size_ < current_product_->l1_size_lower_bound_) {
        t_l1_factor_ = 0.0;
      } else {
        t_l1_factor_ = (current_product_->moving_avg_l1_size_ - current_product_->l1_size_lower_bound_) *
                       current_product_->l1_norm_factor_;
      }

      l1_bias_vec_[i] = t_l1_factor_ * t_paramset_->max_min_diff_;
    }

    if (t_paramset_->read_bucket_size_ && t_paramset_->read_positioning_thresh_decrease_) {
      double t_positioning_ = t_paramset_->positioning_indicator_->GetPositioning();
      long_positioning_bias_vec_[i] = 0.0;
      short_positioning_bias_vec_[i] = 0.0;

      if (t_positioning_ > t_paramset_->positioning_threshold_ && my_risk_ > 0) {
        long_positioning_bias_vec_[i] = abs(my_risk_ / t_paramset_->unit_trade_size_) * fabs(t_positioning_) *
                                        t_paramset_->positioning_thresh_decrease_;
      } else if (t_positioning_ < -t_paramset_->positioning_threshold_ && my_risk_ < 0) {
        short_positioning_bias_vec_[i] = abs(my_risk_ / t_paramset_->unit_trade_size_) * fabs(t_positioning_) *
                                         t_paramset_->positioning_thresh_decrease_;
      }
    }

    if (t_paramset_->read_max_min_diff_order_) {
      double t_l1_factor_ = 0;
      current_product_->moving_avg_l1_order_ = l1_order_indicator_vec_[i]->GetL1Factor();
      if (current_product_->moving_avg_l1_order_ > current_product_->l1_order_upper_bound_) {
        t_l1_factor_ = 1.0;
      } else if (current_product_->moving_avg_l1_order_ < current_product_->l1_order_lower_bound_) {
        t_l1_factor_ = 0.0;
      } else {
        t_l1_factor_ = (current_product_->moving_avg_l1_order_ - current_product_->l1_order_lower_bound_) *
                       current_product_->l1_order_norm_factor_;
      }

      l1_order_bias_vec_[i] = t_l1_factor_ * t_paramset_->max_min_diff_order_;
    }

    if (t_paramset_->read_scale_max_pos_) {
      double t_recent_vol_ = t_paramset_->volume_ratio_indicator_->recent_volume();
      int t_vol_scaled_mur_ = 0;
      if (t_recent_vol_ > t_paramset_->volume_upper_bound_) {
        t_vol_scaled_mur_ =
            (t_paramset_->volume_upper_bound_ - t_paramset_->volume_lower_bound_) * t_paramset_->volume_norm_factor_;
      } else if (t_recent_vol_ < t_paramset_->volume_lower_bound_) {
        t_vol_scaled_mur_ = 0;
      } else {
        t_vol_scaled_mur_ = (t_recent_vol_ - t_paramset_->volume_lower_bound_) * t_paramset_->volume_norm_factor_;
      }

      volume_adj_max_pos_ = (t_paramset_->base_mur_ + t_vol_scaled_mur_) * t_paramset_->unit_trade_size_;
    }
  }
}
}
