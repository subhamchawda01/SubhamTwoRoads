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
#include "dvctrade/ExecLogic/directional_aggressive_trading_single_order.hpp"
#include "dvctrade/ExecLogic/trade_vars.hpp"
#include "dvctrade/InitCommon/paramset.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/OrderRouting/base_order_manager.hpp"
//#include "dvccode/CDef/math_utils.hpp"
//// exec_logic_code / defines.hpp was empty

namespace HFSAT {

DirectionalAggressiveTradingSingleOrder::DirectionalAggressiveTradingSingleOrder(
    DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
    SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
    MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
    const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int t_runtime_id_,
    const std::vector<std::string> _this_model_source_shortcode_vec_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_) {
  worst_case_long_position_ = 0;
  worst_case_short_position_ = 0;

  // initialize variables for target and existing prices
  bid_int_price_to_place_at_ = -1;
  ask_int_price_to_place_at_ = -1;
  existing_bid_int_price_ = -1;
  existing_ask_int_price_ = -1;
  unsequenced_bid_order_present_ = false;
  unsequenced_ask_order_present_ = false;
}

void DirectionalAggressiveTradingSingleOrder::TradingLogic() {
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << " Pos: " << my_position_ << " # "
                           << " tgtbias:" << targetbias_numbers_ << DBGLOG_ENDL_FLUSH;
  }

  existing_bid_int_price_ = -1;
  existing_ask_int_price_ = -1;

  unsequenced_bid_order_present_ = false;
  unsequenced_ask_order_present_ = false;

  // Get price of existing bid order ( if it exists )
  ComputeExistingBidPrices();
  // Symmetrically find price of existing ask order
  ComputeExistingAskPrices();

  // TODO -1 denotes invalid. Would need to add support for spreads later
  bid_int_price_to_place_at_ = -1;
  ask_int_price_to_place_at_ = -1;

  // if based on current risk level or if trading is stopped ... check if we have any allowance to place orders at top
  // level
  if (!unsequenced_bid_order_present_ &&  // Just for optimization, since we don't intend to Place/Cancel if true
      current_tradevarset_.l1bid_trade_size_ > 0) {
    // first check for cooloff_interval
    if ((last_buy_msecs_ > 0) && (watch_.msecs_from_midnight() - last_buy_msecs_ < param_set_.cooloff_interval_) &&
        (best_nonself_bid_int_price_ >= last_buy_int_price_)) {
      // no bids at this or higher prices now
    } else {
      SetBidPlaceDirectives();
    }
  }

  // if based on current risk level or if trading is stopped ... check if we have any allowance to place orders at top
  // level
  if (!unsequenced_ask_order_present_ &&  // Just for optimization, since we don't intend to Place/Cancel if true
      // check if we have any allowance to place orders at top level
      (current_tradevarset_.l1ask_trade_size_ > 0) &&
      // first check for cooloff_interval
      ((last_sell_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
       // later change setting of last_sell_int_price_ to include px_band_
       (last_sell_int_price_ < best_nonself_ask_int_price_))) {
    SetAskPlaceDirectives();
  }

  // After setting top-level directives ...
  // get to order placing or canceling part

  /*  if( ( existing_bid_int_price_ >= 0 && fabs( best_nonself_bid_int_price_ - existing_bid_int_price_ ) > 1000 ) ||
        ( existing_ask_int_price_ >= 0 && fabs( best_nonself_ask_int_price_ - existing_ask_int_price_ ) > 1000 ) )
      std::cout << " Check " << " SBP " << existing_bid_int_price_ << " BBP " << best_nonself_bid_int_price_ << " TBP "
     << bid_int_price_to_place_at_ << " SAP " << existing_ask_int_price_ << " BSP " << best_nonself_ask_int_price_ << "
     TAP " << ask_int_price_to_place_at_ << '\n'; */

  // Active BID order management
  placed_bids_this_round_ = false;
  canceled_bids_this_round_ = false;

  /*
   * If there are unsequenced orders present
   * Assuming it's not possible to   cancel/modify these orders without confirmation
   * Do not call PlaceCancelBidOrders
   */

  if (!unsequenced_bid_order_present_) {
    PlaceCancelBidOrders();
  }

  // Active ASK order management
  placed_asks_this_round_ = false;
  canceled_asks_this_round_ = false;

  /*
   * If there are unsequenced orders present
   * Assuming it's not possible to   cancel/modify these orders without confirmation
   * Do not call PlaceCancelAskOrders
   */

  if (!unsequenced_ask_order_present_) {
    PlaceCancelAskOrders();
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

void DirectionalAggressiveTradingSingleOrder::ComputeExistingBidPrices() {
  // strict check - there should never be 2 orders active
  int t_order_vec_top_bid_index_ = order_manager_.GetOrderVecTopBidIndex();
  int t_order_vec_bottom_bid_index_ = order_manager_.GetOrderVecBottomBidIndex();
  if (t_order_vec_top_bid_index_ != t_order_vec_bottom_bid_index_) {
    DBGLOG_TIME_CLASS_FUNC << "More than one bid orders in DAT_SingleOrder \n" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  if (order_manager_.GetUnSequencedBids().size() > 1) {
    DBGLOG_TIME_CLASS_FUNC << " More than one unsequenced bid order in DAT_SingleOrder \n" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  if (order_manager_.GetUnSequencedBids().size() > 0) {
    existing_bid_int_price_ = order_manager_.GetUnSequencedBids()[0]->int_price();
    unsequenced_bid_order_present_ = true;
  } else if (t_order_vec_top_bid_index_ != -1) {  // -1 denotes no active orders in ordermanager
    existing_bid_int_price_ = order_manager_.GetBidIntPrice(t_order_vec_top_bid_index_);
  }
}

void DirectionalAggressiveTradingSingleOrder::ComputeExistingAskPrices() {
  // Strict check, there should never be 2 orders active
  int t_order_vec_top_ask_index_ = order_manager_.GetOrderVecTopAskIndex();
  int t_order_vec_bottom_ask_index_ = order_manager_.GetOrderVecBottomAskIndex();
  if (t_order_vec_top_ask_index_ != t_order_vec_bottom_ask_index_) {
    DBGLOG_TIME_CLASS_FUNC << "More than one ask orders in DAT_SingleOrder \n" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  if (order_manager_.GetUnSequencedAsks().size() > 1) {
    DBGLOG_TIME_CLASS_FUNC << " More than on unsequenced ask order in DAT_SingleOrder \n" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  if (order_manager_.GetUnSequencedAsks().size() > 0) {
    existing_ask_int_price_ = order_manager_.GetUnSequencedAsks()[0]->int_price();
    unsequenced_ask_order_present_ = true;
  } else if (t_order_vec_top_ask_index_ != -1) {  // -1 denotes no active orders in ordermanager
    existing_ask_int_price_ = order_manager_.GetAskIntPrice(t_order_vec_top_ask_index_);
  }
}

// Current version does not support px_band - TODO
void DirectionalAggressiveTradingSingleOrder::SetBidPlaceDirectives() {
  // tradevarset l1 size > 0 for this call so we need to return a valid price irrespective
  // or directives
  if (CanBidAggress()) {
    bid_int_price_to_place_at_ = best_nonself_ask_int_price_;
  } else if (CanBidImprove() /*|| CanKeepBidImprove()*/) {
    bid_int_price_to_place_at_ = best_nonself_bid_int_price_ + 1;
  } else if (CanPlaceBid() || CanKeepBid()) {
    bid_int_price_to_place_at_ = best_nonself_bid_int_price_;
  } else {
    bid_int_price_to_place_at_ =
        -1;  // best_nonself_bid_int_price_ - std::max( 1, (int)std::ceil( moving_avg_dep_bidask_spread_ ) );
  }
}

void DirectionalAggressiveTradingSingleOrder::SetAskPlaceDirectives() {
  if (CanAskAggress()) {
    ask_int_price_to_place_at_ = best_nonself_bid_int_price_;
  } else if (CanAskImprove() /*|| CanKeepAskImprove()*/) {
    ask_int_price_to_place_at_ = best_nonself_ask_int_price_ - 1;
  } else if (CanPlaceAsk() || CanKeepAsk()) {
    ask_int_price_to_place_at_ = best_nonself_ask_int_price_;
  } else {
    ask_int_price_to_place_at_ =
        -1;  // best_nonself_ask_int_price_ + std::max( 1, (int)std::ceil( moving_avg_dep_bidask_spread_ ) );
  }
}

bool DirectionalAggressiveTradingSingleOrder::CanPlaceBid() {
  return (best_nonself_bid_size_ > param_set_.safe_distance_) ||
         ((targetbias_numbers_ + ((dep_market_view_.spread_increments() > moving_avg_dep_bidask_spread_)
                                      ? (param_set_.high_spread_allowance_)
                                      : 0.0) >=
           current_bid_tradevarset_.l1bid_place_ - l1_bias_ - l1_order_bias_ - short_positioning_bias_) &&
          (best_nonself_bid_size_ > param_set_.min_size_to_join_));
}

bool DirectionalAggressiveTradingSingleOrder::CanKeepBid() {
  return (existing_bid_int_price_ == best_nonself_bid_int_price_ &&
          targetbias_numbers_ >=
              current_bid_tradevarset_.l1bid_keep_ - l1_bias_ - l1_order_bias_ - short_positioning_bias_);
}

bool DirectionalAggressiveTradingSingleOrder::CanKeepAsk() {
  return (existing_ask_int_price_ == best_nonself_ask_int_price_ &&
          -targetbias_numbers_ >=
              current_bid_tradevarset_.l1ask_keep_ - l1_bias_ - l1_order_bias_ - short_positioning_bias_);
}

bool DirectionalAggressiveTradingSingleOrder::CanKeepBidImprove() {
  return (existing_bid_int_price_ > best_nonself_bid_int_price_ &&
          targetbias_numbers_ >=
              current_bid_tradevarset_.l1bid_improve_keep_ - l1_bias_ - l1_order_bias_ - short_positioning_bias_);
}

bool DirectionalAggressiveTradingSingleOrder::CanKeepAskImprove() {
  return (existing_ask_int_price_ < best_nonself_ask_int_price_ &&
          -targetbias_numbers_ >=
              current_bid_tradevarset_.l1ask_improve_keep_ - l1_bias_ - l1_order_bias_ - short_positioning_bias_);
}

bool DirectionalAggressiveTradingSingleOrder::CanPlaceAsk() {
  return (best_nonself_ask_size_ > param_set_.safe_distance_) ||
         (((-targetbias_numbers_ +
            ((dep_market_view_.spread_increments() > 1) ? (param_set_.high_spread_allowance_) : 0.0)) >=
           current_ask_tradevarset_.l1ask_place_ - l1_bias_ - long_positioning_bias_ - l1_order_bias_) &&
          (best_nonself_ask_size_ > param_set_.min_size_to_join_) &&
          (param_set_.place_on_trade_update_implied_quote_ || !dep_market_view_.trade_update_implied_quote()));
}

bool DirectionalAggressiveTradingSingleOrder::CanBidAggress() {
  return (
      /* external control on aggressing */
      (param_set_.allowed_to_aggress_) &&
      // only LIFT offer if the margin of buying exceeds the threshold current_tradevarset_.l1bid_aggressive_
      (targetbias_numbers_ >= current_tradevarset_.l1bid_aggressive_) &&
      /* Don't LIFT offer when my_position_ is already decently long */
      (my_position_ <= param_set_.max_position_to_lift_) &&
      // Don't LIFT when effective spread is to much
      (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_cross_));
}

bool DirectionalAggressiveTradingSingleOrder::CanAskAggress() {
  return (/* external control on aggressing */
          (param_set_.allowed_to_aggress_) &&
          /* Don't HIT bid when my_position_ is already decently short */
          (my_position_ >= param_set_.min_position_to_hit_) &&
          /* Don't HIT ( cross ) when effective spread is to much */
          (best_nonself_ask_int_price_ - best_nonself_bid_int_price_ <= param_set_.max_int_spread_to_cross_) &&
          (-targetbias_numbers_ >= current_tradevarset_.l1ask_aggressive_));
}

bool DirectionalAggressiveTradingSingleOrder::CanBidImprove() {
  return ((param_set_.allowed_to_improve_) &&
          /* Don't improve bid when my_position_ is already decently long */
          (my_position_ <= param_set_.max_position_to_bidimprove_) &&
          (dep_market_view_.spread_increments() >= param_set_.min_int_spread_to_improve_) &&
          (targetbias_numbers_ >= current_tradevarset_.l1bid_improve_) &&
          ((last_buy_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_)));
  // TODO_OPT : later change setting of last_buy_int_price_ to include px_band_
}

bool DirectionalAggressiveTradingSingleOrder::CanAskImprove() {
  return (
      (param_set_.allowed_to_improve_) &&
      /* Don't improve ask when my_position_ is already decently short */
      (my_position_ >= param_set_.min_position_to_askimprove_) &&
      (dep_market_view_.spread_increments() >= param_set_.min_int_spread_to_improve_) &&
      (-targetbias_numbers_ >= current_tradevarset_.l1ask_improve_) &&
      ((last_sell_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_)));
  // TODO_OPT : later change setting of last_sell_int_price_ to include px_band_
}

void DirectionalAggressiveTradingSingleOrder::PlaceCancelBidOrders() {
  // if no order currently exists in the order manager and we need to place an order
  // note to reduce message count - we ignore the supporting order case
  if (existing_bid_int_price_ == -1 && bid_int_price_to_place_at_ >= best_nonself_bid_int_price_) {
    SendTradeAndLog(bid_int_price_to_place_at_, current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy);
    placed_bids_this_round_ = true;
  } else if (existing_bid_int_price_ >= best_nonself_ask_int_price_) {
    // we have an aggressive order in the market that is likely filled. we do nothing.
    /*  } else if (existing_bid_int_price_ > best_nonself_bid_int_price_ ) {
        //we have an improve order. probably not confirmed yet. we do nothing. -- causes spurious issues in SIM */
  } else if (existing_bid_int_price_ != -1) {
    // see comment in PlaceCancelAskOrders function.
    if (bid_int_price_to_place_at_ != -1) {
      BaseOrder* t_order_ = order_manager_.GetBottomBidOrderAtIntPx(existing_bid_int_price_);
      if (((bid_int_price_to_place_at_ != existing_bid_int_price_) &&
           ((existing_bid_int_price_ > best_nonself_bid_int_price_ &&
             bid_int_price_to_place_at_ <= best_nonself_bid_int_price_) ||
            (bid_int_price_to_place_at_ > best_nonself_bid_int_price_ &&
             existing_bid_int_price_ <= best_nonself_bid_int_price_) ||
            (bid_int_price_to_place_at_ == best_nonself_bid_int_price_) ||
            (existing_bid_int_price_ == best_nonself_bid_int_price_))) ||
          ((current_tradevarset_.l1bid_trade_size_ != t_order_->size_remaining()) &&
           (existing_bid_int_price_ == best_nonself_bid_int_price_) &&
           (existing_bid_int_price_ == bid_int_price_to_place_at_))) {
        order_manager_.ModifyOrderAndLog(t_order_, dep_market_view_.GetDoublePx(bid_int_price_to_place_at_),
                                         bid_int_price_to_place_at_, current_tradevarset_.l1bid_trade_size_);
        placed_bids_this_round_ = true;
      }
    } else {
      order_manager_.CancelBidsAtIntPrice(existing_bid_int_price_);
      canceled_bids_this_round_ = true;
    }
  }
}

void DirectionalAggressiveTradingSingleOrder::PlaceCancelAskOrders() {
  // if no order currently exists in the order manager and we need to place an order
  // note to reduce message count - we ignore the supporting order case
  if (existing_ask_int_price_ == -1 && ask_int_price_to_place_at_ <= best_nonself_ask_int_price_ &&
      ask_int_price_to_place_at_ > 0) {
    SendTradeAndLog(ask_int_price_to_place_at_, current_tradevarset_.l1ask_trade_size_, kTradeTypeSell);
    placed_asks_this_round_ = true;
  } else if (existing_ask_int_price_ <= best_nonself_bid_int_price_ && existing_ask_int_price_ > 0) {
    // we have an aggressive order in the market that is likely filled. we do nothing.
    /*  } else if (existing_ask_int_price_ < best_nonself_ask_int_price_ && existing_ask_int_price_ > 0 ) {
        //we have an improve order. probably not confirmed yet. we do nothing. --will cause spurious issues in sim */
  } else if (existing_ask_int_price_ != -1) {
    if (ask_int_price_to_place_at_ != -1) {
      // This case corresponds to the situation where we have an existing order at a non-agg price and a desired order.
      // To incorporate message ratio considerations we do the following. Send a modify iff
      //( a ) Existing order is an improve order and desired order is non-improve; or
      //( b ) Desired orders is an improve/aggress order and existing order is non-improve/aggress
      //( c ) Existing order is best and desired order is supporting
      //( d ) Desired order is best and existing order is supporting
      //( e ) prices are same but sizes differ and order is best level
      BaseOrder* t_order_ = order_manager_.GetBottomAskOrderAtIntPx(existing_ask_int_price_);
      if (((ask_int_price_to_place_at_ != existing_ask_int_price_) &&
           ((existing_ask_int_price_ < best_nonself_ask_int_price_ &&
             ask_int_price_to_place_at_ >= best_nonself_ask_int_price_) ||
            (ask_int_price_to_place_at_ < best_nonself_ask_int_price_ &&
             existing_ask_int_price_ >= best_nonself_ask_int_price_) ||
            (existing_ask_int_price_ == best_nonself_ask_int_price_) ||
            (ask_int_price_to_place_at_ == best_nonself_ask_int_price_))) ||
          ((current_tradevarset_.l1ask_trade_size_ != t_order_->size_remaining()) &&
           (existing_ask_int_price_ == ask_int_price_to_place_at_) &&
           (existing_ask_int_price_ == best_nonself_ask_int_price_))) {
        order_manager_.ModifyOrderAndLog(t_order_, dep_market_view_.GetDoublePx(ask_int_price_to_place_at_),
                                         ask_int_price_to_place_at_, current_tradevarset_.l1ask_trade_size_);
        placed_asks_this_round_ = true;
      }
    } else {
      order_manager_.CancelAsksAtIntPrice(existing_ask_int_price_);
      canceled_asks_this_round_ = true;
    }
  }
}

///< Trade Selection and execution during getflat mode
void DirectionalAggressiveTradingSingleOrder::GetFlatTradingLogic() {
  existing_bid_int_price_ = -1;
  existing_ask_int_price_ = -1;

  unsequenced_bid_order_present_ = false;
  unsequenced_ask_order_present_ = false;

  int t_position_ = GetPositionToClose();
  if (t_position_ == 0) {
    order_manager_.CancelAllOrders();
  } else if (t_position_ > 0) {
    order_manager_.CancelAllBidOrders();
    ComputeExistingAskPrices();
    ask_int_price_to_place_at_ = best_nonself_ask_int_price_;
    if (!unsequenced_ask_order_present_) {
      int trade_size_required = MathUtils::GetFlooredMultipleOf(std::min(t_position_, param_set_.unit_trade_size_),
                                                                dep_market_view_.min_order_size());
      if (existing_ask_int_price_ != -1) {
        BaseOrder* t_order = order_manager_.GetTopAskOrderAtIntPx(existing_ask_int_price_);
        if (  // Either the order is non best level order
            (existing_ask_int_price_ > ask_int_price_to_place_at_) ||
            // Or the order is best/improve but the size is not matching
            (existing_ask_int_price_ <= ask_int_price_to_place_at_ &&
             existing_ask_int_price_ > best_nonself_bid_price_ &&
             // Currently ignoring queue size considerations while modifying
             t_order->size_remaining() != trade_size_required)) {
          order_manager_.ModifyOrderAndLog(t_order, dep_market_view_.GetDoublePx(ask_int_price_to_place_at_),
                                           ask_int_price_to_place_at_, trade_size_required);
        }
      } else {
        SendTradeAndLog(ask_int_price_to_place_at_, trade_size_required, kTradeTypeSell);
      }
    }

  } else if (t_position_ < 0) {
    order_manager_.CancelAllAskOrders();
    ComputeExistingBidPrices();
    bid_int_price_to_place_at_ = best_nonself_bid_int_price_;
    if (!unsequenced_bid_order_present_) {
      int trade_size_required = MathUtils::GetFlooredMultipleOf(std::min(-t_position_, param_set_.unit_trade_size_),
                                                                dep_market_view_.min_order_size());
      if (existing_bid_int_price_ != -1) {
        BaseOrder* t_order = order_manager_.GetTopBidOrderAtIntPx(existing_bid_int_price_);
        if (  // Either the order is Non best level order
            (existing_bid_int_price_ < bid_int_price_to_place_at_) ||
            // Or the order is best/improve but size is not matching
            (existing_bid_int_price_ >= bid_int_price_to_place_at_ &&
             existing_bid_int_price_ < best_nonself_ask_int_price_ &&
             // Currently ignoring queue size considerations while modifying
             t_order->size_remaining() != trade_size_required)) {
          order_manager_.ModifyOrderAndLog(t_order, dep_market_view_.GetDoublePx(bid_int_price_to_place_at_),
                                           bid_int_price_to_place_at_, trade_size_required);
        }
      } else {
        SendTradeAndLog(bid_int_price_to_place_at_, trade_size_required, kTradeTypeBuy);
      }
    }
  }
}

char DirectionalAggressiveTradingSingleOrder::GetOrderLevelIndicator(TradeType_t order_side, int int_order_px) {
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

void DirectionalAggressiveTradingSingleOrder::SendTradeAndLog(int int_price, int size, TradeType_t buysell) {
  order_manager_.SendTradeIntPx(int_price, size, buysell, GetOrderLevelIndicator(buysell, int_price), kOrderDay);
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "SendTrade B of " << size << " @ " << int_price
                           << " tgt_bias: " << targetbias_numbers_ / dep_market_view_.min_price_increment()
                           << " Mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << "  ---  "
                           << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
  }
}

void DirectionalAggressiveTradingSingleOrder::PrintFullStatus() {
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
}
