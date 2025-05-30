/**

   \file ExecLogicCode/directional_agrressive_trading_modify.cpp

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
#include "dvctrade/ExecLogic/directional_aggressive_trading_modify.hpp"
#include "dvctrade/ExecLogic/trade_vars.hpp"
#include "dvctrade/InitCommon/paramset.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/OrderRouting/base_order_manager.hpp"
//#include "dvccode/CDef/math_utils.hpp"
//// exec_logic_code / defines.hpp was empty

namespace HFSAT {

DirectionalAggressiveTradingModify::DirectionalAggressiveTradingModify(
    DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
    SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
    MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
    const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int t_runtime_id_,
    const std::vector<std::string> _this_model_source_shortcode_vec_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_) {
  cancel_asks_below_ = 0;
  cancel_bids_below_ = 0;
  cancel_asks_above_ = 0;
  cancel_bids_above_ = 0;
  num_max_orders_ = 0;
  our_bid_orders_ = 0;
  our_ask_orders_ = 0;

  cancel_asks_from_far_size_ = 0;
  cancel_bids_from_far_size_ = 0;

  effective_bid_position_ = 0;
  effective_ask_position_ = 0;

  effective_bid_position_to_keep_ = 0;
  effective_ask_position_to_keep_ = 0;

  worst_case_long_position_ = 0;
  worst_case_short_position_ = 0;

  last_bid_nonbest_level_om_ = 0;
  last_ask_nonbest_level_om_ = 0;
}

void DirectionalAggressiveTradingModify::TradingLogic() {
  TradeVarsForMultOrder();
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC
        << " BidOrders: " << our_bid_orders_ << " AskOrders: " << our_ask_orders_ << " Pos: " << my_position_ << " # "
        << effective_bid_position_ << " # " << effective_ask_position_ << " Thresh: "
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
  if (current_tradevarset_.l1bid_trade_size_ > 0) {
    // first check for cooloff_interval
    if ((last_buy_msecs_ > 0) && (watch_.msecs_from_midnight() - last_buy_msecs_ < param_set_.cooloff_interval_) &&
        (best_nonself_bid_int_price_ >= last_buy_int_price_)) {
      // no bids at this or higher prices now
    } else {
      SetBidPlaceDirectives();
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
       (last_sell_int_price_ - best_nonself_ask_int_price_ <
        (param_set_.px_band_ - 1)))  // later change setting of last_sell_int_price_ to include px_band_
      ) {
    SetAskPlaceDirectives();
  }
  HandleORSIndicators();

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
  placed_bids_this_round_ = false;
  canceled_bids_this_round_ = false;
  PlaceCancelBidOrders();

  // Active ASK order management
  placed_asks_this_round_ = false;
  canceled_asks_this_round_ = false;
  PlaceCancelAskOrders();

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

void DirectionalAggressiveTradingModify::SetBidPlaceDirectives() {
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
    // check if we should retain exisiting bids due to place_in_line

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

void DirectionalAggressiveTradingModify::SetAskPlaceDirectives() {
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
    // signal not strog enough to place limit orders at the best ask level
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

bool DirectionalAggressiveTradingModify::CanPlaceBid() {
  return (best_nonself_bid_size_ > param_set_.safe_distance_) ||
         ((targetbias_numbers_ + ((dep_market_view_.spread_increments() > moving_avg_dep_bidask_spread_)
                                      ? (param_set_.high_spread_allowance_)
                                      : 0.0) >=
           current_bid_tradevarset_.l1bid_place_ - l1_bias_ - l1_order_bias_ - short_positioning_bias_) &&
          (best_nonself_bid_size_ > param_set_.min_size_to_join_) &&
          (param_set_.place_on_trade_update_implied_quote_ || !dep_market_view_.trade_update_implied_quote()));
}

bool DirectionalAggressiveTradingModify::CanPlaceAsk() {
  return (best_nonself_ask_size_ > param_set_.safe_distance_) ||
         (((-targetbias_numbers_ +
            ((dep_market_view_.spread_increments() > 1) ? (param_set_.high_spread_allowance_) : 0.0)) >=
           current_ask_tradevarset_.l1ask_place_ - l1_bias_ - long_positioning_bias_ - l1_order_bias_) &&
          (best_nonself_ask_size_ > param_set_.min_size_to_join_) &&
          (param_set_.place_on_trade_update_implied_quote_ || !dep_market_view_.trade_update_implied_quote()));
}

bool DirectionalAggressiveTradingModify::CanBidAggress() {
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

bool DirectionalAggressiveTradingModify::CanAskAggress() {
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

bool DirectionalAggressiveTradingModify::CanBidImprove() {
  return ((param_set_.allowed_to_improve_) &&
          /* Don't improve bid when my_position_ is already decently long */
          (my_position_ <= param_set_.max_position_to_bidimprove_) &&
          (dep_market_view_.spread_increments() >= param_set_.min_int_spread_to_improve_) &&
          (targetbias_numbers_ >= current_tradevarset_.l1bid_improve_) &&
          ((last_buy_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
           (((best_nonself_bid_int_price_ + 1) - last_buy_int_price_) < (param_set_.px_band_ - 1))));
  // TODO_OPT : later change setting of last_buy_int_price_ to include px_band_
}

bool DirectionalAggressiveTradingModify::CanAskImprove() {
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

void DirectionalAggressiveTradingModify::TradeVarsForMultOrder() {
  num_max_orders_ = 1;
  our_bid_orders_ = 0;
  our_ask_orders_ = 0;
  effective_bid_position_ = my_position_;
  effective_ask_position_ = my_position_;
  effective_bid_position_to_keep_ = my_position_;
  effective_ask_position_to_keep_ = my_position_;

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
}

void DirectionalAggressiveTradingModify::PrintFullStatus() {
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

void DirectionalAggressiveTradingModify::PlaceNewAskOrders(int& order_placed, int non_best_level_start_index,
                                                           int total_size_so_far) {
  TryPlacingAggressiveAsk2(order_placed);
  if (!placed_asks_this_round_) {
    TryPlacingImproveAsk2(order_placed);
  }
  if (!placed_asks_this_round_) {
    TryPlacingBestAsk2(order_placed);
  }
  for (int idx = non_best_level_start_index;
       idx < non_best_level_start_index + (int)param_set_.num_non_best_ask_levels_monitored_ - 1; idx++) {
    if ((int)(-my_position_ + total_size_so_far + order_placed + (2 * param_set_.unit_trade_size_)) <
        worst_case_short_position_) {
      ask_int_prices_to_place_at_.push_back(best_nonself_ask_int_price_ + idx);
      ask_sizes_to_place_at_.push_back(current_tradevarset_.l1ask_trade_size_);
    }
  }
}

void DirectionalAggressiveTradingModify::PlaceNewBidOrders(int& order_placed, int non_best_level_start_index,
                                                           int bid_size_so_far) {
  TryPlacingAggressiveBid2(order_placed);

  if (!placed_bids_this_round_) {
    TryPlacingImproveBid2(order_placed);
  }

  if (!placed_bids_this_round_) {
    TryPlacingBestBid2(order_placed);
  }

  for (int idx = non_best_level_start_index;
       idx < non_best_level_start_index + (int)param_set_.num_non_best_bid_levels_monitored_ - 1; idx++) {
    if ((int)(my_position_ + bid_size_so_far + order_placed + (2 * param_set_.unit_trade_size_)) <
        worst_case_long_position_) {
      bid_int_prices_to_place_at_.push_back(best_nonself_bid_int_price_ - idx);
      bid_sizes_to_place_at_.push_back(current_tradevarset_.l1bid_trade_size_);
    }
  }
}

void DirectionalAggressiveTradingModify::PlaceCancelBidOrders() {
  std::vector<std::vector<BaseOrder*> >& bid_order_vec_ = order_manager_.BidOrderVec();

  int order_vec_top_bid_index = order_manager_.GetOrderVecTopBidIndex();
  int order_vec_bottom_bid_index = order_manager_.GetOrderVecBottomBidIndex();

  std::vector<BaseOrder*> orders_to_cancel;
  orders_to_cancel.clear();
  bid_int_prices_to_place_at_.clear();
  bid_sizes_to_place_at_.clear();
  /**
   * Remaining :
   * 1 What would happen when no orders in mkt[done]
   * 2 If my best_order so far is below best_nonself_int_price - non_best_level_start_index
   * 3 Check for spread while canceling improve[done]
   */

  worst_case_long_position_ =
      (param_set_.read_explicit_max_long_position_ ? param_set_.explicit_worst_case_long_position_
                                                   : param_set_.worst_case_position_);

  int total_bid_size = order_manager_.SumBidSizes();

  int aggressive_order_placed = 0;
  int improve_order_placed = 0;
  int best_order_placed = 0;
  int non_best_order_placed = 0;
  int size_seen_so_far = 0;
  int max_size_resting_bids = worst_case_long_position_ - std::max(0, my_position_);

  bool place_non_best_level_orders_this_round = true;
  int non_best_level_start_index = CanPlaceNonBestLevelBidOrders();
  if (non_best_level_start_index == -1) {
    non_best_level_start_index = 0;
    place_non_best_level_orders_this_round = false;
  }

  bool managed_nonbest_level_orders_this_round = false;

  if (order_vec_top_bid_index == -1) {
    assert(order_vec_bottom_bid_index == -1);
    PlaceNewBidOrders(aggressive_order_placed, non_best_level_start_index, total_bid_size);
  } else {
    for (int idx = order_vec_top_bid_index; idx >= order_vec_bottom_bid_index; idx--) {
      int int_px = order_manager_.GetBidIntPrice(idx);
      size_seen_so_far += order_manager_.GetTotalBidSizeOrderedAtIntPx(int_px);
      if (int_px >= best_nonself_ask_int_price_) {
        int size_to_place = 0;
        if (top_ask_lift_) {
          size_to_place = std::max(current_tradevarset_.l1bid_trade_size_ - aggressive_order_placed, 0);
        }
        // Modify existing aggressive order if possible and different than required,
        // Not canceling here as would already got executed
        aggressive_order_placed +=
            PlaceModifyOrdersAtPrice(size_to_place, bid_order_vec_[idx], orders_to_cancel, false);

      } else if (int_px > best_nonself_bid_int_price_) {
        // In case there was no already existing aggressive order,try placing it here

        if (!placed_bids_this_round_) {
          TryPlacingAggressiveBid2(aggressive_order_placed);
        }

        int orders_placed_so_far = aggressive_order_placed;
        int size_to_place = 0;
        if (top_bid_improve_) {
          size_to_place = std::max(current_tradevarset_.l1bid_trade_size_ - orders_placed_so_far, 0);
        }

        if (!bid_improve_keep_ && dep_market_view_.spread_increments() > 1) {
          VectorUtils::UniqueVectorAdd(orders_to_cancel, bid_order_vec_[idx]);
        } else {
          improve_order_placed += PlaceModifyOrdersAtPrice(size_to_place, bid_order_vec_[idx], orders_to_cancel, false);
        }
      } else if (int_px == best_nonself_bid_int_price_) {
        // Place aggressive/improve orders here if already not placed
        if (!placed_bids_this_round_) {
          TryPlacingAggressiveBid2(aggressive_order_placed);
          if (!placed_bids_this_round_) {
            TryPlacingImproveBid2(improve_order_placed);
          }
        }

        int orders_placed_so_far = improve_order_placed + aggressive_order_placed;
        int size_to_place = 0;
        if (top_bid_place_) {
          size_to_place = std::max(current_tradevarset_.l1bid_trade_size_ - orders_placed_so_far, 0);
        }

        if (!top_bid_keep_) {
          VectorUtils::UniqueVectorAdd(orders_to_cancel, bid_order_vec_[idx]);
        } else {
          best_order_placed += PlaceModifyOrdersAtPrice(size_to_place, bid_order_vec_[idx], orders_to_cancel, false);
        }
      } else if (int_px <= best_nonself_bid_int_price_) {
        // Try placing agg/imp/best level orders if already not placed

        if (!placed_bids_this_round_) {
          TryPlacingAggressiveBid2(aggressive_order_placed);
          if (!placed_bids_this_round_) {
            TryPlacingImproveBid2(improve_order_placed);
            if (!placed_bids_this_round_) TryPlacingBestBid2(best_order_placed);
          }
        }

        if (worst_case_long_position_ == 0) {
          if (bid_order_vec_.size() > 0) {
            VectorUtils::UniqueVectorAdd(orders_to_cancel, bid_order_vec_[idx]);
          }
          continue;
        }

        if (last_bid_nonbest_level_om_ == 0 ||
            watch_.msecs_from_midnight() - last_bid_nonbest_level_om_ >=
                NON_BEST_LEVEL_ORDER_MANAGEMENT_INTERVAL_MSECS) {
          managed_nonbest_level_orders_this_round = true;
          // See total buy orders placed so far
          int current_total_bid_size = (total_bid_size + aggressive_order_placed + improve_order_placed +
                                        best_order_placed + non_best_order_placed);

          // Check if placing new order doesn't break worst case position
          if (place_non_best_level_orders_this_round &&
              (my_position_ + current_total_bid_size + 2 * param_set_.unit_trade_size_ < worst_case_long_position_) &&
              (order_manager_.GetTotalBidSizeOrderedAtIntPx(int_px) == 0) &&
              int_px <= best_nonself_bid_int_price_ - non_best_level_start_index) {
            bid_int_prices_to_place_at_.push_back(int_px);
            bid_sizes_to_place_at_.push_back(current_tradevarset_.l1bid_trade_size_);
            non_best_order_placed += current_tradevarset_.l1bid_trade_size_;
          } else if (size_seen_so_far > max_size_resting_bids) {
            if (bid_order_vec_.size() > 0) {
              VectorUtils::UniqueVectorAdd(orders_to_cancel, bid_order_vec_[idx]);
            }
          } else {
            non_best_order_placed += order_manager_.GetTotalBidSizeOrderedAtIntPx(int_px);
          }
        }
      }
    }
  }

  if (managed_nonbest_level_orders_this_round) {
    last_bid_nonbest_level_om_ = watch_.msecs_from_midnight();
  }
  DBGLOG_TIME_CLASS_FUNC_LINE << " last_nonbest: " << last_bid_nonbest_level_om_
                              << " diff: " << watch_.msecs_from_midnight() - last_bid_nonbest_level_om_
                              << DBGLOG_ENDL_FLUSH;

  // For each vector of place and cancel, send modify order
  unsigned int place_index = 0;
  unsigned int cancel_index = 0;
  while (place_index < bid_int_prices_to_place_at_.size() && cancel_index < orders_to_cancel.size()) {
    bool modified = order_manager_.ModifyOrderAndLog(
        orders_to_cancel[cancel_index], dep_market_view_.GetDoublePx(bid_int_prices_to_place_at_[place_index]),
        bid_int_prices_to_place_at_[place_index], bid_sizes_to_place_at_[place_index]);
    // if it can't be modified, it can't be canceled too
    if (modified || param_set_.place_only_after_cancel_) {
      place_index++;
    }
    cancel_index++;
  }

  while (cancel_index < orders_to_cancel.size()) {
    bool canceled = order_manager_.Cancel(*orders_to_cancel[cancel_index]);
    if (canceled && dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << " Canceled Bid order SAOS: "
                                  << orders_to_cancel[cancel_index]->server_assigned_order_sequence()
                                  << " px: " << orders_to_cancel[cancel_index]->price()
                                  << " Mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << "  ---  "
                                  << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
    }
    cancel_index++;
  }

  while (place_index < bid_int_prices_to_place_at_.size()) {
    order_manager_.SendTradeIntPx(
        bid_int_prices_to_place_at_[place_index], bid_sizes_to_place_at_[place_index], kTradeTypeBuy,
        GetOrderLevelIndicator(kTradeTypeBuy, bid_int_prices_to_place_at_[place_index]), kOrderDay);
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "SendTrade B of " << bid_sizes_to_place_at_[place_index] << " @ "
                             << bid_int_prices_to_place_at_[place_index]
                             << " tgt_bias: " << targetbias_numbers_ / dep_market_view_.min_price_increment()
                             << " thresh_t: "
                             << current_bid_tradevarset_.l1bid_place_ / dep_market_view_.min_price_increment()
                             << " Int Px: " << best_nonself_bid_int_price_ << " tMktSz: " << best_nonself_bid_size_
                             << " Mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << "  ---  "
                             << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
    }
    place_index++;
  }
}

void DirectionalAggressiveTradingModify::PlaceCancelAskOrders() {
  std::vector<std::vector<BaseOrder*> >& ask_order_vec = order_manager_.AskOrderVec();

  int order_vec_top_ask_index = order_manager_.GetOrderVecTopAskIndex();
  int order_vec_bottom_ask_index = order_manager_.GetOrderVecBottomAskIndex();

  ask_int_prices_to_place_at_.clear();
  ask_sizes_to_place_at_.clear();
  std::vector<BaseOrder*> orders_to_cancel;
  orders_to_cancel.clear();

  worst_case_short_position_ =
      (param_set_.read_explicit_max_short_position_ ? param_set_.explicit_worst_case_short_position_
                                                    : param_set_.worst_case_position_);

  int total_ask_size = order_manager_.SumAskSizes();

  int aggressive_order_placed = 0;
  int improve_order_placed = 0;
  int best_order_placed = 0;
  int non_best_order_placed = 0;
  int size_seen_so_far = 0;
  int max_size_resting_asks = worst_case_short_position_ + std::min(0, my_position_);

  bool place_non_best_level_orders_this_round = true;
  int non_best_level_start_index = CanPlaceNonBestLevelAskOrders();
  if (non_best_level_start_index < 0) {
    non_best_level_start_index = 0;
    place_non_best_level_orders_this_round = false;
  }

  bool managed_non_best_level_orders_this_round = false;

  if (order_vec_top_ask_index == -1) {
    assert(order_vec_bottom_ask_index == -1);
    PlaceNewAskOrders(aggressive_order_placed, non_best_level_start_index, total_ask_size);
  } else {
    for (int idx = order_vec_top_ask_index; idx >= order_vec_bottom_ask_index; idx--) {
      int int_px = order_manager_.GetAskIntPrice(idx);
      size_seen_so_far += order_manager_.GetTotalAskSizeOrderedAtIntPx(int_px);

      if (int_px <= best_nonself_bid_int_price_) {
        // If there's already aggressive order there try to modify size if different
        int size_to_place = 0;
        if (top_bid_hit_) {
          size_to_place = std::max(current_tradevarset_.l1ask_trade_size_ - aggressive_order_placed, 0);
        }
        aggressive_order_placed += PlaceModifyOrdersAtPrice(size_to_place, ask_order_vec[idx], orders_to_cancel, false);

      } else if (int_px < best_nonself_ask_int_price_) {
        // if There was no existing agg order,try placing it here
        if (!placed_asks_this_round_) {
          TryPlacingAggressiveAsk2(aggressive_order_placed);
        }

        int orders_placed_so_far = aggressive_order_placed;
        int size_to_place = 0;
        if (top_ask_improve_) {
          size_to_place = std::max(current_tradevarset_.l1ask_trade_size_ - orders_placed_so_far, 0);
        }

        if (!ask_improve_keep_ && dep_market_view_.spread_increments() > 1) {
          VectorUtils::UniqueVectorAdd(orders_to_cancel, ask_order_vec[idx]);
        } else {
          improve_order_placed += PlaceModifyOrdersAtPrice(size_to_place, ask_order_vec[idx], orders_to_cancel, false);
        }
      } else if (int_px == best_nonself_ask_int_price_) {
        // If there were no agg, imp existing order try placing them here
        if (!placed_asks_this_round_) {
          TryPlacingAggressiveAsk2(aggressive_order_placed);
          if (!placed_asks_this_round_) {
            TryPlacingImproveAsk2(improve_order_placed);
          }
        }

        int orders_placed_so_far = improve_order_placed + aggressive_order_placed;
        int size_to_place = 0;
        if (top_ask_place_) {
          size_to_place = std::max(current_tradevarset_.l1ask_trade_size_ - orders_placed_so_far, 0);
        }

        if (!top_ask_keep_) {
          VectorUtils::UniqueVectorAdd(orders_to_cancel, ask_order_vec[idx]);
        } else {
          best_order_placed += PlaceModifyOrdersAtPrice(size_to_place, ask_order_vec[idx], orders_to_cancel, false);
        }
      } else if (int_px >= best_nonself_ask_int_price_) {
        // Place agg/imp/best level orders if already not placed

        if (!placed_asks_this_round_) {
          TryPlacingAggressiveAsk2(aggressive_order_placed);
          if (!placed_asks_this_round_) {
            TryPlacingImproveAsk2(improve_order_placed);
            if (!placed_asks_this_round_) {
              TryPlacingBestAsk2(best_order_placed);
            }
          }
        }

        if (worst_case_short_position_ == 0) {
          if (ask_order_vec[idx].size() > 0) {
            VectorUtils::UniqueVectorAdd(orders_to_cancel, ask_order_vec[idx]);
          }
          continue;
        }

        if (last_ask_nonbest_level_om_ == 0 ||
            watch_.msecs_from_midnight() - last_ask_nonbest_level_om_ >=
                NON_BEST_LEVEL_ORDER_MANAGEMENT_INTERVAL_MSECS) {
          managed_non_best_level_orders_this_round = true;
          // See total buy orders placed so far
          int current_total_ask_size = (total_ask_size + aggressive_order_placed + improve_order_placed +
                                        best_order_placed + non_best_order_placed);
          // Check if placing new order doesn't break worst case position
          if (place_non_best_level_orders_this_round &&
              (int)(-my_position_ + current_total_ask_size + (2 * param_set_.unit_trade_size_)) <
                  worst_case_short_position_ &&
              (order_manager_.GetTotalAskSizeOrderedAtIntPx(int_px) == 0) &&
              int_px >= best_nonself_ask_int_price_ + non_best_level_start_index) {
            ask_int_prices_to_place_at_.push_back(int_px);
            ask_sizes_to_place_at_.push_back(current_tradevarset_.l1ask_trade_size_);
            non_best_order_placed += current_tradevarset_.l1ask_trade_size_;
          } else if (size_seen_so_far > max_size_resting_asks) {
            // Cancel all orders which do not satisfy worst cas pos
            if (ask_order_vec[idx].size() > 0) {
              VectorUtils::UniqueVectorAdd(orders_to_cancel, ask_order_vec[idx]);
            }
          } else {
            // Add the remaining orders here
            non_best_order_placed += (order_manager_.GetTotalAskSizeOrderedAtIntPx(int_px));
          }
        }
      }
    }
  }

  if (managed_non_best_level_orders_this_round) {
    last_ask_nonbest_level_om_ = watch_.msecs_from_midnight();
  }

  DBGLOG_TIME_CLASS_FUNC_LINE << " last_nonbest: " << last_ask_nonbest_level_om_
                              << " diff: " << watch_.msecs_from_midnight() - last_ask_nonbest_level_om_
                              << DBGLOG_ENDL_FLUSH;
  // For each vector of place and cancel, send modify order
  unsigned int place_index = 0;
  unsigned int cancel_index = 0;
  while (place_index < ask_int_prices_to_place_at_.size() && cancel_index < orders_to_cancel.size()) {
    bool modified = order_manager_.ModifyOrderAndLog(
        orders_to_cancel[cancel_index], dep_market_view_.GetDoublePx(ask_int_prices_to_place_at_[place_index]),
        ask_int_prices_to_place_at_[place_index], ask_sizes_to_place_at_[place_index]);
    // If the order could not be modified, it can't be canceled too as the conditions are same
    if (modified || param_set_.place_only_after_cancel_) {
      place_index++;
    }
    cancel_index++;
  }

  while (cancel_index < orders_to_cancel.size()) {
    bool canceled = order_manager_.Cancel(*orders_to_cancel[cancel_index]);
    if (canceled && dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << " Canceled Ask order SAOS: "
                                  << orders_to_cancel[cancel_index]->server_assigned_order_sequence()
                                  << " px: " << orders_to_cancel[cancel_index]->price()
                                  << " Mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << "  ---  "
                                  << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
    }
    cancel_index++;
  }

  while (place_index < ask_int_prices_to_place_at_.size()) {
    order_manager_.SendTradeIntPx(
        ask_int_prices_to_place_at_[place_index], ask_sizes_to_place_at_[place_index], kTradeTypeSell,
        GetOrderLevelIndicator(kTradeTypeSell, ask_int_prices_to_place_at_[place_index]), kOrderDay);
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
    {
      DBGLOG_TIME_CLASS_FUNC << "SendTrade S of " << ask_sizes_to_place_at_[place_index] << " @ "
                             << ask_int_prices_to_place_at_[place_index]
                             << " tgt_bias: " << targetbias_numbers_ / dep_market_view_.min_price_increment()
                             << " thresh_t: "
                             << " Int Px: " << best_nonself_bid_int_price_ << " tMktSz: " << best_nonself_bid_size_
                             << " Mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << "  ---  "
                             << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
    }
    place_index++;
  }
}

/**
 *
 * @param size_to_place - max size to maintain from the given vector
 * @param order_vector - order vector
 * @param orders_to_cancel - order_to_cancel_vec
 * @param cancel_orders - cancel existing orders which make total size > size_to_place
 * @return
 */
int DirectionalAggressiveTradingModify::PlaceModifyOrdersAtPrice(int size_to_place,
                                                                 std::vector<BaseOrder*>& order_vector,
                                                                 std::vector<BaseOrder*>& orders_to_cancel,
                                                                 bool cancel_orders) {
  int total_orders_placed = 0;
  int new_orders_placed = 0;
  for (unsigned lvl_idx = 0; lvl_idx < order_vector.size(); lvl_idx++) {
    BaseOrder* order = order_vector[lvl_idx];

    if (total_orders_placed < size_to_place) {
      if (total_orders_placed + order->size_remaining() > size_to_place ||
          ((total_orders_placed + order->size_remaining() < size_to_place) && (lvl_idx == order_vector.size() - 1))) {
        bool modified = order_manager_.ModifyOrderAndLog(order, order->price(), order->int_price(),
                                                         size_to_place - total_orders_placed);
        if (modified) {
          total_orders_placed = size_to_place - order->size_remaining();
          new_orders_placed += (size_to_place - total_orders_placed - order->size_remaining());
        }
      }
    } else if (cancel_orders && (total_orders_placed + order->size_remaining() > size_to_place)) {
      orders_to_cancel.push_back(order_vector[lvl_idx]);
    }
    total_orders_placed += order->size_remaining();
  }
  return new_orders_placed;
}

char DirectionalAggressiveTradingModify::GetOrderLevelIndicator(TradeType_t order_side, int int_order_px) {
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
}
