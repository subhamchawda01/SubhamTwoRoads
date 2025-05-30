/**
   \file ExecLogicCode/price_based_aggressive_trading.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/ExecLogic/price_based_aggressive_trading2.hpp"
// exec_logic_code / defines.hpp was empty

namespace HFSAT {

PriceBasedAggressiveTrading2::PriceBasedAggressiveTrading2(
    DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
    SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
    MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
    const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int t_runtime_id_,
    const std::vector<std::string> _this_model_source_shortcode_vec_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_) {
  // We need to compute the levels
  dep_market_view_.subscribe_price_type(this, kPriceTypeBandPrice);
  band_lower_bid_px_ = 0;
  band_lower_ask_px_ = 0;
  existing_bid_int_price_ = 0;
  existing_ask_int_price_ = 0;
}

void PriceBasedAggressiveTrading2::InitialLogging() {
  // setting top level directives
  if (dbglogger_.CheckLoggingLevel(MUM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "last_buy: " << last_buy_msecs_ << " last_sell: " << last_sell_msecs_
                           << " last_agg_buy: " << last_agg_buy_msecs_ << " last_agg_sell: " << last_agg_sell_msecs_
                           << " my_position: " << my_position_ << " target_price: " << target_price_
                           << " best_nonself_bid" << best_nonself_bid_price_ << " best_nonself_ask "
                           << best_nonself_ask_price_ << " " << DBGLOG_ENDL_FLUSH;
    ShowParams();
  }

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << " pos: " << my_position_ << " thresh: " << current_tradevarset_.l1bid_place_ << " # "
                           << current_tradevarset_.l1ask_place_ << " # " << current_tradevarset_.l1bid_keep_ << " # "
                           << current_tradevarset_.l1ask_keep_ << " Tgt_Px: " << target_price_
                           << " Tgt_Num: " << targetbias_numbers_ << " best_bid: " << best_nonself_bid_price_
                           << " best_ask: " << best_nonself_ask_price_
                           << " best_int_bid: " << best_nonself_bid_int_price_
                           << " best_int_ask: " << best_nonself_ask_int_price_
                           << " vl_mkt: " << dep_market_view_.band_mkt_price() << DBGLOG_ENDL_FLUSH;
  }
}

void PriceBasedAggressiveTrading2::ComputeExistingBidPrices() {
  existing_bid_int_price_ = best_nonself_bid_int_price_;

  int t_order_vec_top_bid_index_ = order_manager_.GetOrderVecTopBidIndex();
  if (order_manager_.GetUnSequencedBids().size() > 0) {
    existing_bid_int_price_ = order_manager_.GetUnSequencedBids()[0]->int_price();
  } else if (t_order_vec_top_bid_index_ != -1) {  // -1 denotes no active orders in order-manager
    existing_bid_int_price_ = order_manager_.GetBidIntPrice(t_order_vec_top_bid_index_);
  }
}

void PriceBasedAggressiveTrading2::ComputeExistingAskPrices() {
  existing_ask_int_price_ = best_nonself_ask_int_price_;

  int t_order_vec_top_ask_index_ = order_manager_.GetOrderVecTopAskIndex();
  if (order_manager_.GetUnSequencedAsks().size() > 0) {
    existing_ask_int_price_ = order_manager_.GetUnSequencedAsks()[0]->int_price();
  } else if (t_order_vec_top_ask_index_ != -1) {  // -1 denotes no active orders in order-manager
    existing_ask_int_price_ = order_manager_.GetAskIntPrice(t_order_vec_top_ask_index_);
  }
}

void PriceBasedAggressiveTrading2::TradingLogic() {
  best_nonself_ask_int_price_ = dep_market_view_.ask_side_band_int_price_;
  best_nonself_ask_price_ = dep_market_view_.GetDoublePx(dep_market_view_.ask_side_band_int_price_);
  best_nonself_ask_size_ = dep_market_view_.ask_side_band_size_;

  best_nonself_bid_int_price_ = dep_market_view_.bid_side_band_int_price_;
  best_nonself_bid_price_ = dep_market_view_.GetDoublePx(dep_market_view_.bid_side_band_int_price_);
  best_nonself_bid_size_ = dep_market_view_.ask_side_band_size_;

  band_lower_bid_px_ = dep_market_view_.band_lower_bid_int_price_;
  band_lower_ask_px_ = dep_market_view_.band_lower_ask_int_price_;

  InitialLogging();

  ComputeExistingBidPrices();
  ComputeExistingAskPrices();

  SetBidSideDirectives();

  SetAskSideDirectives();

  // After setting top-level directives ...
  // get to order placing or canceling part

  ManageBidOrders();
  ManageAskOrders();

  // for zero loging mode have moved to a higher dbg log level
  if ((dbglogger_.CheckLoggingLevel(TRADING_INFO) && (placed_bids_this_round_ || placed_asks_this_round_))) {
    PrintFullStatus();
    order_manager_.LogFullStatus();
  }

  // temporarily printing indicators every time
  if ((livetrading_ || dbglogger_.CheckLoggingLevel(TRADING_INFO)) &&
      (placed_bids_this_round_ || placed_asks_this_round_ || canceled_bids_this_round_ || canceled_asks_this_round_)) {
    dump_inds = true;
  }
}

void PriceBasedAggressiveTrading2::SetBidSideDirectives() {
  top_bid_place_ = false;
  top_bid_keep_ = false;
  top_bid_improve_ = false;
  top_ask_lift_ = false;
  bid_improve_keep_ = false;

  if (CanProcessBidSide()) {
    if (CanPlaceBestBid()) {
      top_bid_place_ = true;
      top_bid_keep_ = true;

      if (watch_.msecs_from_midnight() - last_agg_buy_msecs_ > param_set_.agg_cooloff_interval_) {
        /*
         * conditions to place aggressive orders:
         * ALLOWED_TO_AGGRESS position is not too long already, spread narrow, signal strong
         *
         */

        if (CanPlaceBidAggress()) {
          top_ask_lift_ = true;

          // when we are already long and ask_lift is set to true,
          // place and keep at top bid are set false so that
          // we cancel top level orders before we place new ones at aggressive prices
          if (my_position_ >= param_set_.max_position_to_cancel_on_lift_) {
            top_bid_place_ = false;
            top_bid_keep_ = false;
          }
        } else {
          top_ask_lift_ = false;

          // conditions to place market improving bid orders:
          // ALLOWED_TO_IMPROVE, position is not too long already, spread wide, signal strong
          if (CanPlaceBidImprove()) {
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
      if ((true &&
           (target_price_ - best_nonself_bid_price_ + bestbid_queue_hysterisis_) >=
               (current_bid_keep_tradevarset_.l1bid_keep_ - l1_bid_trade_bias_ - l1_bias_ - l1_order_bias_ -
                short_positioning_bias_ + param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1))) &&

          (target_price_ - dep_market_view_.GetDoublePx(existing_bid_int_price_) <
           dep_market_view_.min_price_increment() * (double)param_set_.max_distance_to_keep_from_band_price_)) {
        top_bid_keep_ = true;
      } else {
        top_bid_keep_ = false;
      }
    }

    if (((dep_market_view_.spread_increments() > 1) &&

         ((target_price_ - best_nonself_bid_price_) >=
          (current_tradevarset_.l1bid_improve_keep_ - l1_bias_ - l1_order_bias_ - short_positioning_bias_ +
           param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1))))) {
      bid_improve_keep_ = true;
    } else {
      bid_improve_keep_ = false;
    }
  }
}

bool PriceBasedAggressiveTrading2::CanProcessBidSide() {
  // check if we have any allowance to place orders at top level
  return ((current_tradevarset_.l1bid_trade_size_ > 0) &&
          // first check for cooloff_interval
          ((last_buy_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
           // later change setting of last_buy_int_price_ to include px_band_
           ((best_nonself_bid_int_price_ - last_buy_int_price_) < (param_set_.px_band_ - 1))) &&
          (last_bigtrades_bid_cancel_msecs_ <= 0 || (watch_.msecs_from_midnight() - last_bigtrades_bid_cancel_msecs_ >=
                                                     param_set_.bigtrades_cooloff_interval_)));
}

bool PriceBasedAggressiveTrading2::CanPlaceBestBid() {
  // check if the margin of buying at the price ( best_nonself_bid_price_ )
  // i.e. ( target_price_ - best_nonself_bid_price_ )
  // exceeds the threshold current_tradevarset_.l1bid_place_

  return ((best_nonself_bid_size_ > param_set_.safe_distance_) ||
          ((target_price_ - best_nonself_bid_price_ >=
            current_bid_tradevarset_.l1bid_place_ - l1_bias_ - l1_order_bias_ - short_positioning_bias_ +
                param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1)) &&
           // Don't place any orders at a level if less than X size there
           (best_nonself_bid_size_ > param_set_.min_size_to_join_) &&
           (param_set_.place_on_trade_update_implied_quote_ || !dep_market_view_.trade_update_implied_quote())));
}

bool PriceBasedAggressiveTrading2::CanPlaceBidAggress() {
  return ((param_set_.allowed_to_aggress_) &&  // external control on aggressing

          /* only LIFT offer if the margin of buying here i.e.
           * ( target_price_ - best_nonself_ask_price_ ) exceeds the threshold current_tradevarset_.l1bid_aggressive_
           */
          (target_price_ - best_nonself_ask_price_ >=
           current_tradevarset_.l1bid_aggressive_ +
               param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1)) &&

          // Don't LIFT offer when my_position_ is already decently long
          (my_position_ <= param_set_.max_position_to_lift_) &&

          // Don't LIFT when effective spread is to much
          (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_cross_) &&

          ((last_buy_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
           // TODO_OPT : later change setting of last_buy_int_price_ to include px_band_
           ((best_nonself_ask_int_price_ - last_buy_int_price_) < (param_set_.px_band_ - 1))));
}

bool PriceBasedAggressiveTrading2::CanPlaceBidImprove() {
  return ((param_set_.allowed_to_improve_) &&

          // Don't improve bid when my_position_ is already decently long
          (my_position_ <= param_set_.max_position_to_bidimprove_) &&

          (dep_market_view_.spread_increments() >= param_set_.min_int_spread_to_improve_) &&

          (target_price_ - best_nonself_bid_price_ - dep_market_view_.min_price_increment() >=
           current_tradevarset_.l1bid_improve_) &&

          ((last_buy_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
           // TODO_OPT : later change setting of last_buy_int_price_ to include px_band_
           (((best_nonself_bid_int_price_ + 1) - last_buy_int_price_) < (param_set_.px_band_ - 1))));
}

void PriceBasedAggressiveTrading2::ManageBidOrders() {
  // Active BID order management
  placed_bids_this_round_ = false;
  canceled_bids_this_round_ = false;
  if (top_ask_lift_) {
    // only place aggressive bid orders when there is no active unconfirmed order at or above best_nonself_bid_price_
    // and no confirmed orders above the best_nonself_bid_price_
    if ((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(dep_market_view_.bestbid_int_price() + 1) == 0) &&
        (order_manager_.SumBidSizeConfirmedAboveIntPrice(dep_market_view_.bestbid_int_price()) == 0)) {
      int _canceled_size_ = 0;
      if (!top_bid_keep_) {  // if because of my_position_ being greater than max_position_to_cancel_on_lift_
        // placing aggressive LIFT order requires us to cancel bids from all active levels
        // where an actve level is >= best_nonself_bid_price_
        _canceled_size_ += order_manager_.CancelBidsEqAboveIntPrice(existing_bid_int_price_);
        if (_canceled_size_ > 0) canceled_bids_this_round_ = true;
      }

      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the bid side

      int allowance_for_aggressive_buy_ = my_position_ + order_manager_.SumBidSizes() +
                                          current_tradevarset_.l1bid_trade_size_ - param_set_.worst_case_position_;

      if (param_set_.use_new_aggress_logic_) {
        allowance_for_aggressive_buy_ = my_position_ + order_manager_.SumBidSizes() +
                                        current_tradevarset_.l1bid_trade_size_ -
                                        std::max(param_set_.worst_case_position_, param_set_.max_position_);
      }

      if (allowance_for_aggressive_buy_ >= 0) {
        if (allowance_for_aggressive_buy_ > _canceled_size_) {
          _canceled_size_ += order_manager_.CancelBidsFromFar(current_tradevarset_.l1bid_trade_size_);
        }
      } else {
        // Place new order
        order_manager_.SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                 current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'A');
        placed_bids_this_round_ = true;
        last_agg_buy_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "Sending aggressive B at px " << best_nonself_ask_price_ << " position "
                                 << my_position_ << DBGLOG_ENDL_FLUSH;
          p_base_model_math_->ShowIndicatorValues();
        }
      }
    } else {
      // only after canceling them can we be allowed to place aggressive orders
      order_manager_.CancelBidsAboveIntPrice(dep_market_view_.bestbid_int_price());
    }
  }

  if ((!placed_bids_this_round_) && (top_bid_improve_)) {
    // only place Bid Improve orders when there is no active unconfirmed order at or above the
    // best_nonself_bid_int_price_
    //    and no confirmed orders above the best price
    if ((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(dep_market_view_.bestbid_int_price() + 1) == 0) &&
        (order_manager_.SumBidSizeConfirmedAboveIntPrice(dep_market_view_.bestbid_int_price()) == 0)) {
      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the bid side
      if (my_position_ + order_manager_.SumBidSizes() + current_tradevarset_.l1bid_trade_size_ >=
          param_set_.worst_case_position_) {
        int _canceled_size_ = 0;
        // then cancel Bids from bottom levels for the required size
        _canceled_size_ += order_manager_.CancelBidsFromFar(current_tradevarset_.l1bid_trade_size_);

      } else {
        // Place new order
        order_manager_.SendTradeIntPx((dep_market_view_.bestbid_int_price() + 1),
                                      current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'I');
        placed_bids_this_round_ = true;
        last_agg_buy_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "Sending improve B at px " << dep_market_view_.bestbid_int_price() + 1
                                 << " position " << my_position_ << DBGLOG_ENDL_FLUSH;
          p_base_model_math_->ShowIndicatorValues();
        }
      }
    }

  } else {
    if ((dep_market_view_.spread_increments() > 1) && (!bid_improve_keep_)) {
      int cancelled_size_ = order_manager_.CancelBidsAboveIntPrice(dep_market_view_.bestbid_int_price());
      if (cancelled_size_ > 0) {
        canceled_bids_this_round_ = true;
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " Cancelled Improve Bid orders Above: " << dep_market_view_.bestbid_int_price()
                                 << " Position: " << my_position_ << " ebp_t: "
                                 << (target_price_ - best_nonself_bid_price_ + bestbid_queue_hysterisis_) /
                                        dep_market_view_.min_price_increment() << " thresh_t: "
                                 << current_tradevarset_.l1bid_improve_keep_ / dep_market_view_.min_price_increment()
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  if (!placed_bids_this_round_) {  // only come here if no aggressive or improve bid orders sent this cycle
    if (top_bid_place_) {
      // only place best level Bid orders when there is no active unconfirmed order at or above
      // best_nonself_bid_int_price_
      // and no confirmed order at or above the best price
      if ((((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(band_lower_bid_px_) == 0) &&
            (order_manager_.SumBidSizeConfirmedEqAboveIntPrice(band_lower_bid_px_) == 0))) &&
          // Don't place any new orders in inside market if the spread is too wide
          (stdev_ <= param_set_.low_stdev_lvl_ ||
           (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_place_))) {
        order_manager_.SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                 current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'B');
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "SendTrade B of " << current_tradevarset_.l1bid_trade_size_ << " @ "
                                 << best_nonself_bid_price_ << " ebp_t: "
                                 << (target_price_ - best_nonself_bid_price_) / dep_market_view_.min_price_increment()
                                 << " thresh_t: "
                                 << current_bid_tradevarset_.l1bid_place_ / dep_market_view_.min_price_increment()
                                 << " IntPx: " << best_nonself_bid_int_price_ << " tMktSz: " << best_nonself_bid_size_
                                 << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_
                                 << " StdevSpreadFactor: " << stdev_scaled_capped_in_ticks_ << DBGLOG_ENDL_FLUSH;
          p_base_model_math_->ShowIndicatorValues();
        }

        placed_bids_this_round_ = true;
      }
    } else {
      if (!top_bid_keep_) {  // cancel all orders at best_nonself_bid_price_
        int canceled_size_ = 0;
        canceled_size_ = order_manager_.CancelBidsEqAboveIntPrice(existing_bid_int_price_);

        // int canceled_size_ = order_manager_.CancelBidsEqAboveIntPrice ( best_nonself_bid_int_price_ ) ;
        if (canceled_size_ > 0) {
          canceled_bids_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "Canceled B of " << canceled_size_ << " eqAbove " << best_nonself_bid_price_
                                   << " SizeToCancel: " << my_position_ << " ebp_t: "
                                   << (target_price_ - best_nonself_bid_price_ + bestbid_queue_hysterisis_) /
                                          dep_market_view_.min_price_increment() << " thresh_t: "
                                   << current_bid_keep_tradevarset_.l1bid_keep_ / dep_market_view_.min_price_increment()
                                   << " tMktSz: " << best_nonself_bid_size_ << " mkt: " << best_nonself_bid_size_
                                   << " @ " << best_nonself_bid_price_ << " X " << best_nonself_ask_price_ << " @ "
                                   << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
  }
}

void PriceBasedAggressiveTrading2::SetAskSideDirectives() {
  top_ask_place_ = false;
  top_ask_keep_ = false;
  top_ask_improve_ = false;
  top_bid_hit_ = false;
  ask_improve_keep_ = false;

  if (CanProcessAskSide()) {
    // check if the margin of placing limit ask orders at the price ( best_nonself_ask_price_ )
    // i.e. ( best_nonself_ask_price_ - target_price_ )
    //      exceeds the threshold current_tradevarset_.l1ask_place_
    if (CanPlaceBestAsk()) {
      top_ask_place_ = true;
      top_ask_keep_ = true;

      if (watch_.msecs_from_midnight() - last_agg_sell_msecs_ > param_set_.agg_cooloff_interval_) {
        // conditions to place aggressive orders:,ALLOWED_TO_AGGRESS, position is not too short already, spread narrow
        // signal strong
        if (CanPlaceAskAggress()) {
          top_bid_hit_ = true;

          /* when we are already short and bit_hit is set to true,
           * place and keep at top ask are set false so that
           * we cancel top level orders before we place new ones at aggressive prices
           */
          if (my_position_ <= param_set_.min_position_to_cancel_on_hit_) {
            top_ask_place_ = false;
            top_ask_keep_ = false;
          }
        } else {
          top_bid_hit_ = false;
          if (CanPlaceAskImprove()) {
            top_ask_improve_ = true;
          } else {
            top_ask_improve_ = false;
          }
        }
      }
    } else {
      // signal not strog enough to place limit orders at the best ask level
      if ((true &&
           (best_nonself_ask_price_ - target_price_ + bestask_queue_hysterisis_) >=
               (current_ask_keep_tradevarset_.l1ask_keep_ - l1_ask_trade_bias_ - l1_bias_ - l1_order_bias_ -
                long_positioning_bias_ + param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1))) &&

          (dep_market_view_.GetDoublePx(existing_ask_int_price_) - target_price_ >
           dep_market_view_.min_price_increment() * (double)param_set_.max_distance_to_keep_from_band_price_)) {
        // but with place in line effect enough to keep the live order there
        top_ask_keep_ = true;
      } else {
        top_ask_keep_ = false;
      }
    }

    if ((dep_market_view_.spread_increments() > 1) &&
        ((best_nonself_ask_price_ - target_price_) >=
         current_tradevarset_.l1ask_improve_keep_ - l1_bias_ - l1_order_bias_ - long_positioning_bias_ +
             param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1))) {
      ask_improve_keep_ = true;
    } else {
      ask_improve_keep_ = false;
    }
  }
}

bool PriceBasedAggressiveTrading2::CanProcessAskSide() {
  // check if we have any allowance to place orders at top level
  return (
      (current_tradevarset_.l1ask_trade_size_ > 0) &&
      // first check for cooloff_interval
      ((last_sell_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
       // later change setting of last_sell_int_price_ to include px_band_
       (last_sell_int_price_ - best_nonself_ask_int_price_ < (param_set_.px_band_ - 1))) &&

      (last_bigtrades_ask_cancel_msecs_ <= 0 ||
       (watch_.msecs_from_midnight() - last_bigtrades_ask_cancel_msecs_ >= param_set_.bigtrades_cooloff_interval_)));
}

bool PriceBasedAggressiveTrading2::CanPlaceBestAsk() {
  return ((best_nonself_ask_size_ > param_set_.safe_distance_) ||
          ((best_nonself_ask_price_ - target_price_ >=
            current_ask_tradevarset_.l1ask_place_ - l1_bias_ - l1_order_bias_ - long_positioning_bias_ +
                param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1)) &&

           (best_nonself_ask_size_ > param_set_.min_size_to_join_) &&
           (param_set_.place_on_trade_update_implied_quote_ || !dep_market_view_.trade_update_implied_quote())));
}

bool PriceBasedAggressiveTrading2::CanPlaceAskAggress() {
  return ((param_set_.allowed_to_aggress_) &&  // external control on aggressing
          // Don't HIT bid when my_position_ is already decently short
          (my_position_ >= param_set_.min_position_to_hit_) &&

          // Don't HIT ( cross ) when effective spread is to much
          (best_nonself_ask_int_price_ - best_nonself_bid_int_price_ <= param_set_.max_int_spread_to_cross_) &&

          (best_nonself_bid_price_ - target_price_ >=
           current_tradevarset_.l1ask_aggressive_ +
               param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1)) &&

          ((last_sell_msecs_ <= 0) ||
           (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
           // TODO_OPT : later change setting of last_sell_int_price_ to include px_band_
           ((last_sell_int_price_ - best_nonself_bid_int_price_) < (param_set_.px_band_ - 1))));
}

bool PriceBasedAggressiveTrading2::CanPlaceAskImprove() {
  return ((param_set_.allowed_to_improve_) &&

          // Don't improve ask when my_position_ is already decently short
          (my_position_ >= param_set_.min_position_to_askimprove_) &&

          (best_nonself_ask_int_price_ - best_nonself_bid_int_price_ >= param_set_.min_int_spread_to_improve_) &&

          (best_nonself_ask_price_ - target_price_ - dep_market_view_.min_price_increment() >=
           current_tradevarset_.l1ask_improve_) &&

          ((last_sell_msecs_ <= 0) ||
           (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
           // TODO_OPT : later change setting of last_sell_int_price_ to include px_band_
           (((last_sell_int_price_ - 1) - best_nonself_ask_int_price_) < (param_set_.px_band_ - 1))));
}

void PriceBasedAggressiveTrading2::ManageAskOrders() {
  // Active ASK order management
  placed_asks_this_round_ = false;
  canceled_asks_this_round_ = false;
  if (top_bid_hit_) {
    // only place aggressive orders when there is no active unconfirmed order and no confirmed orders above the best
    // price
    if ((order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(dep_market_view_.bestask_int_price() - 1) == 0) &&
        (order_manager_.SumAskSizeConfirmedAboveIntPrice(dep_market_view_.bestask_int_price()) == 0)) {
      int _canceled_size_ = 0;
      if (!top_ask_keep_) {  // if due to position placing aggressive LIFT order requires us to cancel asks from all
                             // active levels
        _canceled_size_ += order_manager_.CancelAsksEqAboveIntPrice(band_lower_ask_px_);
      }

      int allowance_for_aggressive_sell_ = -my_position_ + order_manager_.SumAskSizes() +
                                           current_tradevarset_.l1ask_trade_size_ - param_set_.worst_case_position_;

      if (param_set_.use_new_aggress_logic_) {
        allowance_for_aggressive_sell_ = -my_position_ + order_manager_.SumAskSizes() +
                                         current_tradevarset_.l1ask_trade_size_ -
                                         std::max(param_set_.worst_case_position_, param_set_.max_position_);
      }

      if (allowance_for_aggressive_sell_ >= 0) {
        if (allowance_for_aggressive_sell_ > _canceled_size_) {
          _canceled_size_ += order_manager_.CancelAsksFromFar(
              current_tradevarset_.l1ask_trade_size_);  ///< then cancel Asks from bottom levels for the required size
        }
      } else {
        // Place new order
        order_manager_.SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                 current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'A');
        placed_asks_this_round_ = true;
        last_agg_sell_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "Sending aggressive S at px " << best_nonself_bid_price_ << " position "
                                 << my_position_ << DBGLOG_ENDL_FLUSH;
          p_base_model_math_->ShowIndicatorValues();
        }
      }
    } else {
      // only after canceling them can we be allowed to place aggressive orders
      order_manager_.CancelAsksAboveIntPrice(dep_market_view_.bestask_int_price() - 1);
    }
  }

  if ((!placed_asks_this_round_) && (top_ask_improve_)) {
    // only place Ask Improve orders when there is no active unconfirmed order and no confirmed orders above the best
    // price
    if ((order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(dep_market_view_.bestask_int_price() - 1) == 0) &&
        (order_manager_.SumAskSizeConfirmedAboveIntPrice(dep_market_view_.bestask_int_price()) == 0)) {
      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the ask side
      if (-my_position_ + order_manager_.SumAskSizes() + current_tradevarset_.l1ask_trade_size_ +
              current_tradevarset_.l1ask_trade_size_ >
          param_set_.worst_case_position_) {
        int _canceled_size_ = 0;
        ///< then cancel Asks from bottom levels for the required size
        _canceled_size_ += order_manager_.CancelAsksFromFar(current_tradevarset_.l1ask_trade_size_);
      } else {
        // Place new order
        order_manager_.SendTradeIntPx((dep_market_view_.bestask_int_price() - 1),
                                      current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'I');
        placed_asks_this_round_ = true;
        last_agg_sell_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "Sending improve S at px " << dep_market_view_.bestask_int_price() - 1
                                 << " position " << my_position_ << DBGLOG_ENDL_FLUSH;
          p_base_model_math_->ShowIndicatorValues();
        }
      }
    }

  } else {
    if (dep_market_view_.spread_increments() > 1 && !ask_improve_keep_) {
      int cancelled_size_ = order_manager_.CancelAsksAboveIntPrice(dep_market_view_.bestask_int_price());

      if (cancelled_size_ > 0) {
        canceled_asks_this_round_ = true;
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " Cancelling Improve Ask orders Above: " << dep_market_view_.bestask_int_price()
                                 << " position: " << my_position_ << " eap_t: "
                                 << (best_nonself_ask_price_ - target_price_ + bestask_queue_hysterisis_) /
                                        dep_market_view_.min_price_increment() << " thresh_t: "
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
      if ((((order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(band_lower_ask_px_) == 0) &&
            (order_manager_.SumAskSizeConfirmedEqAboveIntPrice(band_lower_ask_px_) == 0))) &&
          // Don't place any new orders in inside market if the spread is too wide
          (stdev_ <= param_set_.low_stdev_lvl_ ||
           (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_place_))) {
        order_manager_.SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                 current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'B');
        placed_asks_this_round_ = true;
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << "SendTrade S of " << current_tradevarset_.l1ask_trade_size_ << " @ "
                                 << best_nonself_ask_price_ << " eap_t: "
                                 << (best_nonself_ask_price_ - target_price_) / dep_market_view_.min_price_increment()
                                 << " thresh_t: "
                                 << current_ask_tradevarset_.l1ask_place_ / dep_market_view_.min_price_increment()
                                 << " IntPx: " << best_nonself_ask_int_price_ << " tMktSz: " << best_nonself_ask_size_
                                 << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_
                                 << " StdevSpreadFactor: " << stdev_scaled_capped_in_ticks_ << DBGLOG_ENDL_FLUSH;
          p_base_model_math_->ShowIndicatorValues();
        }
      }
    } else {
      if (!top_ask_keep_) {
        // cancel all orders at best_nonself_ask_price_
        int canceled_size_ = 0;

        canceled_size_ = order_manager_.CancelAsksEqAboveIntPrice(band_lower_ask_px_);

        if (canceled_size_ > 0) {
          canceled_asks_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "Canceled S of " << canceled_size_ << " EqAbove " << best_nonself_ask_price_
                                   << " SizeToCancel " << my_position_ << " eap_t: "
                                   << (best_nonself_ask_price_ - target_price_ + bestask_queue_hysterisis_) /
                                          dep_market_view_.min_price_increment() << " thresh_t: "
                                   << current_ask_keep_tradevarset_.l1ask_keep_ / dep_market_view_.min_price_increment()
                                   << " tMktSz: " << best_nonself_ask_size_ << " mkt: " << best_nonself_bid_size_
                                   << " @ " << best_nonself_bid_price_ << " X " << best_nonself_ask_price_ << " @ "
                                   << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
  }
}

void PriceBasedAggressiveTrading2::PrintFullStatus() {
  DBGLOG_TIME << "tgt: " << target_price_ << " mkt " << best_nonself_bid_price_ << " X " << best_nonself_bid_size_
              << " pft: " << (target_price_ - best_nonself_bid_price_) << " -- " << best_nonself_ask_price_ << " X "
              << best_nonself_ask_size_ << " pft: " << (best_nonself_ask_price_ - target_price_) << " bias: "
              << ((target_price_ - dep_market_view_.mkt_size_weighted_price()) / dep_market_view_.min_price_increment())
              << " pos: " << my_position_ << " smaps "
              << order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_) << " "
              << order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) << " "
              << order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) << " "
              << order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_)
              << " opnl: " << order_manager_.base_pnl().opentrade_unrealized_pnl() << " gpos: " << my_global_position_
              << DBGLOG_ENDL_FLUSH;
}
}
