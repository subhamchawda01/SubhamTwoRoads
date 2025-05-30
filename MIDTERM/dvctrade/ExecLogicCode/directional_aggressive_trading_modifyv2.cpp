/**

   \file ExecLogicCode/directional_agrressive_trading_modifyv2.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include <algorithm>
#include <string>
#include <typeinfo>
#include <vector>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvctrade/ExecLogic/directional_aggressive_trading_modifyv2.hpp"
#include "dvctrade/ExecLogic/trade_vars.hpp"
#include "dvctrade/InitCommon/paramset.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/OrderRouting/base_order_manager.hpp"
//#include "dvccode/CDef/math_utils.hpp"
//// exec_logic_code / defines.hpp was empty

namespace HFSAT {

DirectionalAggressiveTradingModifyV2::DirectionalAggressiveTradingModifyV2(
    DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
    SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
    MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
    const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int t_runtime_id_,
    const std::vector<std::string> _this_model_source_shortcode_vec_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_),
      dep_market_view_(_dep_market_view_) {
  _order_manager_.SetUseModifyFlag(true);
}

void DirectionalAggressiveTradingModifyV2::TradingLogic() {
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
      // first check for cooloff_interval
      ((last_buy_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
       (last_buy_int_price_ - best_nonself_bid_int_price_ <
        (param_set_.px_band_ - 1)))  // later change setting of last_buy_int_price_ to include px_band_
      ) {
    SetBidPlaceDirectives();
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
       (last_sell_int_price_ - best_nonself_ask_int_price_ <
        (param_set_.px_band_ - 1)))  // later change setting of last_sell_int_price_ to include px_band_
      ) {
    SetAskPlaceDirectives();
  }

  // After setting top-level directives ...
  // get to order placing or canceling part

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " Directives: tap: " << top_ask_place_ << " tai: " << top_ask_improve_
                                << " tak: " << top_ask_keep_ << " tbh: " << top_bid_hit_
                                << " aik: " << ask_improve_keep_ << " == "
                                << " tbp: " << top_bid_place_ << " tbi: " << top_bid_improve_
                                << " tbk: " << top_bid_keep_ << " tal: " << top_ask_lift_
                                << " bik: " << bid_improve_keep_ << " [ "
                                << " Mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << "  ---  "
                                << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << " ] "
                                << DBGLOG_ENDL_FLUSH;
  }
  // Active BID order management
  PlaceModifyBidOrders();

  // Active ASK order management
  PlaceModifyAskOrders();

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

void DirectionalAggressiveTradingModifyV2::SetBidPlaceDirectives() {
  // check if the margin of buying
  // i.e. ( targetbias_numbers_ )
  // exceeds the threshold current_tradevarset_.l1bid_place_

  if (CanPlaceBid()) {
    top_bid_place_ = true;
    top_bid_keep_ = true;

    if (watch_.msecs_from_midnight() - last_agg_buy_msecs_ > param_set_.agg_cooloff_interval_) {
      /* aggressive and improve */
      if (CanBidAggress()) {
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
        if (CanBidImprove()) {
          top_bid_improve_ = true;
        } else {
          top_bid_improve_ = false;
        }
      }
    }
  } else {  // signal is not strong enough for placing bids at best_nonself_bid_price_
    // check if we should retain existing bids due to place_in_line

    // TODO : perhaps we should make place_in_line effect bestbid_queue_hysterisis_ smarter
    //        for instance when short term volatility in the market is very high
    //        then being high in the queue should count for less.

    if ((targetbias_numbers_ +
         ((dep_market_view_.spread_increments() > moving_avg_dep_bidask_spread_) ? (param_set_.high_spread_allowance_)
                                                                                 : 0.0) +
         bestbid_queue_hysterisis_) >=
        current_bid_keep_tradevarset_.l1bid_keep_ - l1_bias_ - short_positioning_bias_ - l1_order_bias_) {
      top_bid_keep_ = true;
    } else {
      top_bid_keep_ = false;
    }
  }

  if ((dep_market_view_.spread_increments() > 1) &&
      (targetbias_numbers_ + ((dep_market_view_.spread_increments() > moving_avg_dep_bidask_spread_)
                                  ? (param_set_.high_spread_allowance_)
                                  : 0.0)) >=
          current_tradevarset_.l1bid_improve_keep_ - l1_bias_ - short_positioning_bias_ - l1_order_bias_) {
    bid_improve_keep_ = true;
  } else {
    bid_improve_keep_ = false;
  }
}

void DirectionalAggressiveTradingModifyV2::SetAskPlaceDirectives() {
  // check if the margin of placing limit ask orders at the price ( best_nonself_ask_price_ )
  // i.e. ( best_nonself_ask_price_ - target_price_ )
  //      exceeds the threshold current_tradevarset_.l1ask_place_

  if (CanPlaceAsk()) {
    top_ask_place_ = true;
    top_ask_keep_ = true;

    if (watch_.msecs_from_midnight() - last_agg_sell_msecs_ > param_set_.agg_cooloff_interval_) {
      /* aggressive and improve */
      // conditions to place aggressive orders:
      // ALLOWED_TO_AGGRESS
      // position is not too short already
      // spread narrow
      // signal strong
      if (CanAskAggress()) {
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
        if (CanAskImprove()) {
          top_ask_improve_ = true;
        } else {
          top_ask_improve_ = false;
        }
      }
    }

  } else {
    // signal not strong enough to place limit orders at the best ask level
    if ((-targetbias_numbers_ +
         ((dep_market_view_.spread_increments() > moving_avg_dep_bidask_spread_) ? (param_set_.high_spread_allowance_)
                                                                                 : 0.0) +
         bestask_queue_hysterisis_) >=
        current_ask_keep_tradevarset_.l1ask_keep_ - l1_bias_ - l1_order_bias_ -
            long_positioning_bias_) {  // but with place in line effect enough to keep the live order there
      top_ask_keep_ = true;
    } else {
      top_ask_keep_ = false;
    }
  }
  if ((dep_market_view_.spread_increments() > 1) &&
      (-targetbias_numbers_ + ((dep_market_view_.spread_increments() > moving_avg_dep_bidask_spread_)
                                   ? (param_set_.high_spread_allowance_)
                                   : 0.0)) >=
          current_tradevarset_.l1ask_improve_keep_ - l1_bias_ - l1_order_bias_ - long_positioning_bias_) {
    ask_improve_keep_ = true;
  } else {
    ask_improve_keep_ = false;
  }
}

bool DirectionalAggressiveTradingModifyV2::CanPlaceBid() {
  return (best_nonself_bid_size_ > param_set_.safe_distance_) ||
         ((targetbias_numbers_ + ((dep_market_view_.spread_increments() > moving_avg_dep_bidask_spread_)
                                      ? (param_set_.high_spread_allowance_)
                                      : 0.0) >=
           current_bid_tradevarset_.l1bid_place_ - l1_bias_ - l1_order_bias_ - short_positioning_bias_) &&
          (best_nonself_bid_size_ > param_set_.min_size_to_join_) &&
          (param_set_.place_on_trade_update_implied_quote_ || !dep_market_view_.trade_update_implied_quote()));
}

bool DirectionalAggressiveTradingModifyV2::CanPlaceAsk() {
  return (best_nonself_ask_size_ > param_set_.safe_distance_) ||
         (((-targetbias_numbers_ +
            ((dep_market_view_.spread_increments() > 1) ? (param_set_.high_spread_allowance_) : 0.0)) >=
           current_ask_tradevarset_.l1ask_place_ - l1_bias_ - long_positioning_bias_ - l1_order_bias_) &&
          (best_nonself_ask_size_ > param_set_.min_size_to_join_) &&
          (param_set_.place_on_trade_update_implied_quote_ || !dep_market_view_.trade_update_implied_quote()));
}

bool DirectionalAggressiveTradingModifyV2::CanBidAggress() {
  return (
      /* external control on aggressing */
      (param_set_.allowed_to_aggress_) &&
      // only LIFT offer if the margin of buying exceeds the threshold current_tradevarset_.l1bid_aggressive_
      (targetbias_numbers_ >= current_tradevarset_.l1bid_aggressive_) &&
      /* Don't LIFT offer when my_position_ is already decently long */
      (my_position_ <= param_set_.max_position_to_lift_) &&
      // Don't LIFT when effective spread is to much
      (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_cross_) &&
      ((last_buy_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
       ((best_nonself_ask_int_price_ - last_buy_int_price_) < (param_set_.px_band_ - 1))));
  // TODO_OPT : later change setting of last_buy_int_price_ to include px_band_
}

bool DirectionalAggressiveTradingModifyV2::CanAskAggress() {
  return (/* external control on aggressing */
          (param_set_.allowed_to_aggress_) &&
          /* Don't HIT bid when my_position_ is already decently short */
          (my_position_ >= param_set_.min_position_to_hit_) &&
          /* Don't HIT ( cross ) when effective spread is to much */
          (best_nonself_ask_int_price_ - best_nonself_bid_int_price_ <= param_set_.max_int_spread_to_cross_) &&
          (-targetbias_numbers_ >= current_tradevarset_.l1ask_aggressive_) &&
          ((last_sell_msecs_ <= 0) ||
           (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
           ((last_sell_int_price_ - best_nonself_bid_int_price_) < (param_set_.px_band_ - 1))));
  // TODO_OPT : later change setting of last_sell_int_price_ to include px_band_
}

bool DirectionalAggressiveTradingModifyV2::CanBidImprove() {
  return ((param_set_.allowed_to_improve_) &&
          /* Don't improve bid when my_position_ is already decently long */
          (my_position_ <= param_set_.max_position_to_bidimprove_) &&
          (dep_market_view_.spread_increments() >= param_set_.min_int_spread_to_improve_) &&
          (targetbias_numbers_ >= current_tradevarset_.l1bid_improve_) &&
          ((last_buy_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
           (((best_nonself_bid_int_price_ + 1) - last_buy_int_price_) < (param_set_.px_band_ - 1))));
  // TODO_OPT : later change setting of last_buy_int_price_ to include px_band_
}

bool DirectionalAggressiveTradingModifyV2::CanAskImprove() {
  return ((param_set_.allowed_to_improve_) &&
          /* Don't improve ask when my_position_ is already decently short */
          (my_position_ >= param_set_.min_position_to_askimprove_) &&
          (best_nonself_ask_int_price_ - best_nonself_bid_int_price_ >= param_set_.min_int_spread_to_improve_) &&
          (targetbias_numbers_ >= current_tradevarset_.l1ask_improve_) &&
          ((last_sell_msecs_ <= 0) ||
           (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
           (((last_sell_int_price_ - 1) - best_nonself_ask_int_price_) < (param_set_.px_band_ - 1))));
  // TODO_OPT : later change setting of last_sell_int_price_ to include px_band_
}

void DirectionalAggressiveTradingModifyV2::PrintFullStatus() {
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

void DirectionalAggressiveTradingModifyV2::PlaceModifyBidOrders() {
  placed_bids_this_round_ = false;
  canceled_bids_this_round_ = false;

  if (top_ask_lift_) {
    // aggress orders
    int aggress_size = MathUtils::GetFlooredMultipleOf(
        std::min(std::min(param_set_.max_position_ - my_position_, current_tradevarset_.l1bid_trade_size_),
                 best_nonself_ask_size_),
        param_set_.min_order_size_);

    if ((order_manager_.GetTotalBidSizeAboveIntPx(best_nonself_bid_int_price_) == 0)) {
      if (!top_bid_keep_) {
        // if due to position placing aggressive LIFT order requires us to cancel bids from all
        // active levels we modify from top
        BaseOrder* order_to_modify = order_manager_.GetTopModifiableBidOrder();
        ModifyOrder(order_to_modify, best_nonself_ask_price_, best_nonself_ask_int_price_, aggress_size, kTradeTypeBuy,
                    'A');
      }

      if (!placed_bids_this_round_) {
        if (WillExceedWorstCasePos(kTradeTypeBuy, aggress_size)) {
          // modify from bottom
          BaseOrder* order_to_modify = order_manager_.GetBottomModifiableBidOrder();
          ModifyOrder(order_to_modify, best_nonself_ask_price_, best_nonself_ask_int_price_, aggress_size,
                      kTradeTypeBuy, 'A');
        } else {
          // place new order
          SendTradeAndLog(best_nonself_ask_price_, best_nonself_ask_int_price_, aggress_size, kTradeTypeBuy, 'A');
        }
      }
    } else {
      // orders above best level exists, modify from top
      BaseOrder* order_to_modify = order_manager_.GetTopModifiableBidOrder();
      ModifyOrder(order_to_modify, best_nonself_ask_price_, best_nonself_ask_int_price_, aggress_size, kTradeTypeBuy,
                  'A');
    }
  }

  if (!placed_bids_this_round_) {
    if (CanPlaceBestBid2()) {
      // best orders
      if (WillExceedWorstCasePos(kTradeTypeBuy, current_tradevarset_.l1bid_trade_size_)) {
        // modify from bottom
        BaseOrder* order_to_modify = order_manager_.GetBottomModifiableBidOrder();
        ModifyOrder(order_to_modify, best_nonself_bid_price_, best_nonself_bid_int_price_,
                    current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'B');
      } else {
        // place new order
        SendTradeAndLog(best_nonself_bid_price_, best_nonself_bid_int_price_, current_tradevarset_.l1bid_trade_size_,
                        kTradeTypeBuy, 'B');
      }
    } else {
      if (!top_bid_keep_) {
        ModifyBestBidOrdersToNonBest(best_nonself_bid_int_price_);

        // cancel all orders at best_nonself_bid_price_
        int canceled_size_ = 0;
        canceled_size_ = order_manager_.CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_);

        if (canceled_size_ > 0) {
          canceled_bids_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
          {
            DBGLOG_TIME << " Shc: " << dep_market_view_.shortcode() << "Canceled as keep false B of " << canceled_size_
                        << " EqAbove " << best_nonself_bid_price_
                        << " tgt_bias: " << targetbias_numbers_ / dep_market_view_.min_price_increment()
                        << " thresh_t: "
                        << current_bid_keep_tradevarset_.l1bid_keep_ / dep_market_view_.min_price_increment()
                        << " tMktSz: " << best_nonself_bid_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
  }
}

void DirectionalAggressiveTradingModifyV2::PlaceModifyAskOrders() {
  placed_asks_this_round_ = false;
  canceled_asks_this_round_ = false;

  if (top_bid_hit_) {
    // aggress orders
    int aggress_size = MathUtils::GetFlooredMultipleOf(
        std::min(std::min(param_set_.max_position_ + my_position_, current_tradevarset_.l1ask_trade_size_),
                 best_nonself_bid_size_),
        param_set_.min_order_size_);

    if ((order_manager_.GetTotalAskSizeAboveIntPx(best_nonself_ask_int_price_) == 0)) {
      if (!top_ask_keep_) {
        // if due to position placing aggressive LIFT order requires us to cancel asks from all
        // active levels modify from top
        BaseOrder* order_to_modify = order_manager_.GetTopModifiableAskOrder();
        ModifyOrder(order_to_modify, best_nonself_bid_price_, best_nonself_bid_int_price_, aggress_size, kTradeTypeSell,
                    'A');
      }

      if (!placed_asks_this_round_) {
        if (WillExceedWorstCasePos(kTradeTypeSell, aggress_size)) {
          // modify from bottom
          BaseOrder* order_to_modify = order_manager_.GetBottomModifiableAskOrder();
          ModifyOrder(order_to_modify, best_nonself_bid_price_, best_nonself_bid_int_price_, aggress_size,
                      kTradeTypeSell, 'A');
        } else {
          SendTradeAndLog(best_nonself_bid_price_, best_nonself_bid_int_price_, aggress_size, kTradeTypeSell, 'A');
        }
      }
    } else {
      // orders above best level exists, modify from top
      BaseOrder* order_to_modify = order_manager_.GetTopModifiableAskOrder();
      ModifyOrder(order_to_modify, best_nonself_bid_price_, best_nonself_bid_int_price_, aggress_size, kTradeTypeSell,
                  'A');
    }
  }

  if (!placed_asks_this_round_) {
    if (CanPlaceBestAsk2()) {
      // best order
      if (WillExceedWorstCasePos(kTradeTypeSell, current_tradevarset_.l1ask_trade_size_)) {
        // modify from bottom
        BaseOrder* order_to_modify = order_manager_.GetBottomModifiableAskOrder();
        ModifyOrder(order_to_modify, best_nonself_ask_price_, best_nonself_ask_int_price_,
                    current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'B');
      } else {
        // place new order
        SendTradeAndLog(best_nonself_ask_price_, best_nonself_ask_int_price_, current_tradevarset_.l1ask_trade_size_,
                        kTradeTypeSell, 'B');
      }
    } else {
      if (!top_ask_keep_) {
        ModifyBestAskOrdersToNonBest(best_nonself_ask_int_price_);

        // cancel all orders at best_nonself_ask_price_
        int canceled_size_ = 0;
        canceled_size_ = order_manager_.CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_);

        if (canceled_size_ > 0) {
          canceled_asks_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
          {
            DBGLOG_TIME << " Shc: " << dep_market_view_.shortcode() << "Canceled as keep false S of " << canceled_size_
                        << " EqAbove " << best_nonself_ask_price_
                        << " tgt_bias: " << -targetbias_numbers_ / dep_market_view_.min_price_increment()
                        << " thresh_t: "
                        << current_ask_keep_tradevarset_.l1ask_keep_ / dep_market_view_.min_price_increment()
                        << " tMktSz: " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
  }
}

bool DirectionalAggressiveTradingModifyV2::CanPlaceBestBid2() {
  // only place Bid orders when there is no active unconfirmed order and no confirmed order at or above the best
  // price
  // Don't place any new
  // orders in inside market
  // if the spread is too wide
  if (top_bid_place_ && (order_manager_.GetTotalBidSizeEqAboveIntPx(best_nonself_bid_int_price_) == 0) &&
      (stdev_ <= param_set_.low_stdev_lvl_ ||
       (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_place_)))
    return true;
  else
    return false;
}

bool DirectionalAggressiveTradingModifyV2::CanPlaceBestAsk2() {
  // only place Ask orders when there is no active unconfirmed order and no confirmed order at or above the best
  // price
  // Don't place any new
  // orders in inside market
  // if the spread is too wide
  if (top_ask_place_ && (order_manager_.GetTotalAskSizeEqAboveIntPx(best_nonself_ask_int_price_) == 0) &&
      (stdev_ <= param_set_.low_stdev_lvl_ ||
       (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_place_)))
    return true;
  else
    return false;
}

bool DirectionalAggressiveTradingModifyV2::WillExceedWorstCasePos(TradeType_t buysell, int order_size) {
  int worst_pos = std::max(param_set_.worst_case_position_, param_set_.max_position_);
  if (buysell == kTradeTypeBuy) {
    int estimated_worst_case_pos = my_position_ + order_size + order_manager_.SumBidSizes();
    if (estimated_worst_case_pos > worst_pos)
      return true;
    else
      return false;
  } else {
    int estimated_worst_case_pos = -my_position_ + order_size + order_manager_.SumAskSizes();
    if (estimated_worst_case_pos > worst_pos)
      return true;
    else
      return false;
  }
}

char DirectionalAggressiveTradingModifyV2::GetOrderLevelIndicator(TradeType_t order_side, int int_order_px) {
  if (order_side == kTradeTypeBuy) {
    if (int_order_px >= best_nonself_ask_int_price_) {
      return 'A';
    } else if (int_order_px > best_nonself_bid_int_price_) {
      return 'I';
    } else if (int_order_px == best_nonself_bid_int_price_) {
      return 'B';
    } else
      return 'S';
  } else {
    if (int_order_px <= best_nonself_bid_int_price_) {
      return 'A';
    } else if (int_order_px < best_nonself_ask_int_price_) {
      return 'I';
    } else if (int_order_px == best_nonself_ask_int_price_) {
      return 'B';
    } else
      return 'S';
  }
}

void DirectionalAggressiveTradingModifyV2::SendTradeAndLog(const double price, const int intpx, int size_requested,
                                                           TradeType_t t_buysell, char placed_at_level_indicator) {
  order_manager_.SendTrade(price, intpx, size_requested, t_buysell, placed_at_level_indicator);
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
  {
    DBGLOG_TIME_CLASS_FUNC << " Shc: " << dep_market_view_.shortcode() << "SendTrade buysell:" << t_buysell
                           << " level: " << placed_at_level_indicator << " size:" << size_requested
                           << " price: " << price << " int price: " << intpx << " Mkt: " << best_nonself_bid_size_
                           << " @ " << best_nonself_bid_price_ << " ---- " << best_nonself_ask_price_ << " @ "
                           << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
  }

  if (t_buysell == kTradeTypeBuy) {
    placed_bids_this_round_ = true;
    if (placed_at_level_indicator == 'A') {
      last_agg_buy_msecs_ = watch_.msecs_from_midnight();
    }
  } else {
    placed_asks_this_round_ = true;
    if (placed_at_level_indicator == 'A') {
      last_agg_sell_msecs_ = watch_.msecs_from_midnight();
    }
  }
}

void DirectionalAggressiveTradingModifyV2::ModifyOrder(BaseOrder* order_to_modify, double price, int int_price,
                                                       int size, TradeType_t t_buysell,
                                                       char placed_at_level_indicator) {
  if (order_to_modify != NULL) {
    if (order_manager_.ModifyOrderAndLog(order_to_modify, price, int_price, size)) {
      if (t_buysell == kTradeTypeBuy) {
        placed_bids_this_round_ = true;
        if (placed_at_level_indicator == 'A') {
          last_agg_buy_msecs_ = watch_.msecs_from_midnight();
          if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "Aggressive buy modify" << DBGLOG_ENDL_FLUSH;
          }
        }
      } else {
        placed_asks_this_round_ = true;
        if (placed_at_level_indicator == 'A') {
          last_agg_sell_msecs_ = watch_.msecs_from_midnight();
          if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "Aggressive sell modify" << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
  }
}

// modify best and above bid orders to empty prices below current bottom vec
void DirectionalAggressiveTradingModifyV2::ModifyBestBidOrdersToNonBest(int best_nonself_bid_int_price_) {
  int best_index = order_manager_.GetBidIndex(best_nonself_bid_int_price_);
  int top_mod_bid_index = order_manager_.GetTopModifiableBidIndex();
  if (top_mod_bid_index != -1) {
    // iterate through best or above orders
    for (int i = top_mod_bid_index; i >= best_index; i--) {
      BaseOrder* t_order = order_manager_.GetBidOrderAtIndex(i);
      if (t_order != NULL) {
        int new_int_price = order_manager_.GetNonBestEmptyBidIntPrice();
        int worst_pos = std::max(param_set_.worst_case_position_, param_set_.max_position_);
        int estimated_pos = my_position_ + order_manager_.SumBidSizes();
        int new_size = MathUtils::GetFlooredMultipleOf(std::min(worst_pos - estimated_pos, param_set_.unit_trade_size_),
                                                       param_set_.min_order_size_);

        if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " Trying to modify eqabove best bid to nonbest: "
                                 << " CAOS: " << t_order->client_assigned_order_sequence()
                                 << " bs: " << ((t_order->buysell_ == kTradeTypeBuy) ? "BUY" : "SELL")
                                 << " oldsz: " << t_order->size_requested_ << " newsz: " << new_size
                                 << " oldpx: " << t_order->price_
                                 << " newpx: " << dep_market_view_.GetDoublePx(new_int_price)
                                 << " oldintpx: " << t_order->int_price_ << " newintpx: " << new_int_price
                                 << DBGLOG_ENDL_FLUSH;
        }

        if (new_size > 0)
          order_manager_.ModifyOrderAndLog(t_order, dep_market_view_.GetDoublePx(new_int_price), new_int_price,
                                           new_size);
      }
    }
  }
}

// modify best and above ask orders to empty prices below current bottom vec
void DirectionalAggressiveTradingModifyV2::ModifyBestAskOrdersToNonBest(int best_nonself_ask_int_price_) {
  int best_index = order_manager_.GetAskIndex(best_nonself_ask_int_price_);
  int top_mod_ask_index = order_manager_.GetTopModifiableAskIndex();
  if (top_mod_ask_index != -1) {
    // iterate through best or above orders
    for (int i = top_mod_ask_index; i >= best_index; i--) {
      BaseOrder* t_order = order_manager_.GetAskOrderAtIndex(i);
      if (t_order != NULL) {
        int new_int_price = order_manager_.GetNonBestEmptyAskIntPrice();
        int worst_pos = std::max(param_set_.worst_case_position_, param_set_.max_position_);
        int estimated_pos = -my_position_ + order_manager_.SumAskSizes();
        int new_size = MathUtils::GetFlooredMultipleOf(std::min(worst_pos - estimated_pos, param_set_.unit_trade_size_),
                                                       param_set_.min_order_size_);

        if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " Trying to modify eqabove best ask to nonbest: "
                                 << " CAOS: " << t_order->client_assigned_order_sequence()
                                 << " bs: " << ((t_order->buysell_ == kTradeTypeBuy) ? "BUY" : "SELL")
                                 << " oldsz: " << t_order->size_requested_ << " newsz: " << new_size
                                 << " oldpx: " << t_order->price_
                                 << " newpx: " << dep_market_view_.GetDoublePx(new_int_price)
                                 << " oldintpx: " << t_order->int_price_ << " newintpx: " << new_int_price
                                 << DBGLOG_ENDL_FLUSH;
        }

        if (new_size > 0)
          order_manager_.ModifyOrderAndLog(t_order, dep_market_view_.GetDoublePx(new_int_price), new_int_price,
                                           new_size);
      }
    }
  }
}
}
