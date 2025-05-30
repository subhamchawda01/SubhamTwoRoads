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
#include "dvctrade/ExecLogic/price_based_aggressive_pro_rata_trading.hpp"

namespace HFSAT {

PriceBasedAggressiveProRataTrading::PriceBasedAggressiveProRataTrading(
    DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
    SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
    MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
    const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int t_runtime_id_,
    const std::vector<std::string> _this_model_source_shortcode_vec_, bool allow_exit)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_),
      avg_l1_size_(0) {
  //

  pro_rata_factor_ =
      1 - SecurityDefinitions::GetFIFOPercentageForSimShortcode(dep_market_view_.shortcode(), watch_.YYYYMMDD());

  pro_rata_adjusted_max_trade_size_ = (double)param_set_.max_trade_size_ / pro_rata_factor_;

  if (allow_exit) CheckForExit();

  if (!param_set_.read_max_unit_size_at_level_) {
    param_set_.max_unit_size_at_level_ = 10;
  }

  BuildTradeVarSets();
  current_bid_size_to_place_ = param_set_.unit_trade_size_;
  current_ask_size_to_place_ = param_set_.unit_trade_size_;
}

void PriceBasedAggressiveProRataTrading::CheckForExit() {
  if (pro_rata_factor_ <= 0.001) {
    ExitVerbose(kExitErrorCodeZeroValue, " ProRataFactor Too low to use this exec-logic");
  }

  if (!param_set_.read_max_trade_size_) {
    ExitVerbose(kExitErrorCodeZeroValue, "Provide MaxTradeSize..");
  }
}
/**
 * Computes the size to place with given total size
 *
 * The computation is on the assumption that at any point of time
 * we would not want to get filled by more than UTS ( or even less than in case we see a trade of size max_trade_size_)
 *
 * @param total_size
 * @param buysell trade-type
 * @param current_position position of current security
 */
int PriceBasedAggressiveProRataTrading::ComputeSizeToPlace(int total_size, TradeType_t buysell, int current_position) {
  auto l1_size = current_bid_size_to_place_;
  auto t_max_position = param_set_.max_position_;

  // In case it's sell side, then adjust the value
  if (buysell == kTradeTypeSell) {
    l1_size = current_ask_size_to_place_;
    t_max_position = -t_max_position;
  }

  // used for placing size, in case MUR < 1
  // auto mur_scale_threshold = std::min(1.0, param_set_.max_unit_ratio_);

  double new_max_pos_scale_threshold =
      (double)std::min(param_set_.max_position_, std::abs(t_max_position - current_position));
  auto new_mur_scale_threshold = new_max_pos_scale_threshold / (double)param_set_.unit_trade_size_;

  if (param_set_.is_mur_low_ && new_mur_scale_threshold < 1) {
    // If we can't afford to get filled for all of placed size

    if (total_size * new_mur_scale_threshold < pro_rata_adjusted_max_trade_size_) {
      // Total size is less than the ideal size we would expect in market to be filled for maximum std::min(UTS,
      // max_position)

      // Reduce the size in a way that we place < UTS size which is sufficient to get us filled for max_pos size

      // TODO This gives us best case scenario for now. With assumption that maximum trade size happens, we should be
      // placing the size below to reach max-pos

      auto size_to_be_placed =
          total_size * new_max_pos_scale_threshold / (param_set_.max_trade_size_ * pro_rata_factor_);

      if (buysell == kTradeTypeBuy)
        current_tradevarset_.l1bid_trade_size_ = std::min(current_bid_size_to_place_, (int)size_to_be_placed);

      if (buysell == kTradeTypeSell)
        current_tradevarset_.l1ask_trade_size_ = std::min(current_ask_size_to_place_, (int)size_to_be_placed);

      return size_to_be_placed;
    } else {
      // Total size is enough for us to place and get filled for max std::min(UTS, max_position)
      return (total_size * l1_size * new_mur_scale_threshold) / (param_set_.max_trade_size_ * pro_rata_factor_);
    }

  } else {
    // We can afford to get filled for all placed size, place the size from tradevarset builder
    if (total_size < pro_rata_adjusted_max_trade_size_) {
      // In case the l1 size is not more than max-trade size then place 1 order if directed by signal
      return l1_size;
    } else {
      // l1 size is higher than the max-trade size, and we can afford to get filled for > UTS size
      // we would want to place layered order
      return (total_size * l1_size) / (param_set_.max_trade_size_ * pro_rata_factor_);
    }
  }

  /*
  if (total_size < (param_set_.max_trade_size_ / (pro_rata_factor_ * mur_scale_threshold))) {
    // Total size is less than the ideal size we would expect in market to be filled for maximum std::min(UTS,
    // max_position)

    // Reduce the size in a way that we place < UTS size which is sufficient to get us filled for max_pos size

    // TODO This gives us best case scenario for now. With assumption that maximum trade size happens, we should be
    // placing the size below to reach max-pos

    auto size_to_be_placed = total_size *
                             std::min(param_set_.unit_trade_size_,
                                      std::min(param_set_.max_position_, std::abs(t_max_position - current_position))) /
                             (param_set_.max_trade_size_ * pro_rata_factor_);

    if (buysell == kTradeTypeBuy)
      current_tradevarset_.l1bid_trade_size_ = std::min(current_bid_size_to_place_, (int)size_to_be_placed);

    if (buysell == kTradeTypeSell)
      current_tradevarset_.l1ask_trade_size_ = std::min(current_ask_size_to_place_, (int)size_to_be_placed);
    return size_to_be_placed;
  }

  ///
  ///if (total_size < param_set_.max_trade_size_ / pro_rata_factor_) {
  ///  // In case the l1 size is not more than max-trade size then place 1 order if directed by signal
  ///  return l1_size;
  ///}


  return (total_size * l1_size * mur_scale_threshold) / (param_set_.max_trade_size_ * pro_rata_factor_);

  */
}

void PriceBasedAggressiveProRataTrading::TradingLogic() {
  auto our_bid_orders = order_manager_.SumBidSizes();
  auto our_ask_orders = order_manager_.SumAskSizes();

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << " BidOrders: " << our_bid_orders << " AskOrders: " << our_ask_orders
                           << " C_Pos: " << my_position_ << " Risk: " << my_risk_
                           << " Bid_Thresh: " << current_tradevarset_.l1bid_place_
                           << " Ask_Thresh: " << current_tradevarset_.l1ask_place_ << " # "
                           << current_tradevarset_.l1bid_keep_ << " # " << current_tradevarset_.l1ask_keep_
                           << " Tgt_Px: " << target_price_ << " best bid: " << best_nonself_bid_price_
                           << " Best_Ask: " << best_nonself_ask_price_ << DBGLOG_ENDL_FLUSH;
  }

  top_bid_place_ = false;
  top_bid_keep_ = false;
  top_bid_improve_ = false;
  top_ask_lift_ = false;
  bid_improve_keep_ = false;

  if (CanPlaceBidsThisRound())

  {
    // check if the margin of buying at the price ( best_nonself_bid_price_ )
    // i.e. ( target_price_ - best_nonself_bid_price_ )
    // exceeds the threshold current_tradevarset_.l1bid_place_
    if (CanPlaceBestBidThisRound())

    {
      top_bid_place_ = true;
      top_bid_keep_ = true;
      SetBidImproveAggressFlags();
    } else {
      // either size less than min_size_to_join_
      // or signal is not strong enough for placing bids at best_nonself_bid_price_
      // check if we should retain existing bids due to place_in_line

      // TODO : perhaps we should make place_in_line effect bestbid_queue_hysterisis_ smarter
      //        for instance when short term volatility in the market is very high
      //        then being high in the queue should count for less.

      SetBidKeepFlag();
    }
    SetBidImproveKeepFlag();
  }

  top_ask_place_ = false;
  top_ask_keep_ = false;
  top_ask_improve_ = false;
  top_bid_hit_ = false;
  ask_improve_keep_ = false;

  if (CanPlaceAsksThisRound()) {
    // check if the margin of placing limit ask orders at the price ( best_nonself_ask_price_ )
    // i.e. ( best_nonself_ask_price_ - target_price_ )
    //      exceeds the threshold current_tradevarset_.l1ask_place_
    if (CanPlaceBestAskThisRound()) {
      top_ask_place_ = true;
      top_ask_keep_ = true;

      SetAskImproveAggressFlags();
    } else {
      // signal not strong enough to place limit orders at the best ask level
      SetAskKeepFlag();
    }
    SetAskImproveKeepFlag();
  }

  // Active BID order management
  placed_bids_this_round_ = false;
  canceled_bids_this_round_ = false;
  if (top_ask_lift_) {
    // Adjusting the size in a way that we don't cross maxposition, ( case for MUR < 1)
    int aggress_size = MathUtils::GetFlooredMultipleOf(
        std::min(std::min(param_set_.max_position_ - my_position_, current_tradevarset_.l1bid_trade_size_),
                 best_nonself_ask_size_),
        param_set_.min_order_size_);

    // only place aggressive bid orders when there is no active unconfirmed order at or above best_nonself_bid_price_
    // and no confirmed orders above the best_nonself_bid_price_
    if ((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_ + 1) == 0) &&
        (order_manager_.SumBidSizeConfirmedAboveIntPrice(best_nonself_bid_int_price_) == 0)) {
      int _canceled_size_ = 0;
      if (!top_bid_keep_) {  // if because of my_position_ being greater than max_position_to_cancel_on_lift_
        // placing aggressive LIFT order requires us to cancel bids from all active levels
        // where an actve level is >= best_nonself_bid_price_
        _canceled_size_ += order_manager_.CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_);
        if (_canceled_size_ > 0) canceled_bids_this_round_ = true;
      }

      int allowance_for_aggressive_buy_ =
          my_position_ + order_manager_.SumBidSizes() + aggress_size - param_set_.worst_case_position_;

      if (param_set_.use_new_aggress_logic_) {
        allowance_for_aggressive_buy_ = my_position_ + order_manager_.SumBidSizes() + aggress_size -
                                        std::max(param_set_.worst_case_position_, param_set_.max_position_);
      }

      if (allowance_for_aggressive_buy_ >= 0) {
        if (allowance_for_aggressive_buy_ > _canceled_size_) {
          _canceled_size_ += order_manager_.CancelBidsFromFar(aggress_size);
        }
      } else {
        // Place new order
        PlaceOrderAndLog(best_nonself_ask_price_, best_nonself_ask_int_price_, aggress_size, kTradeTypeBuy, 'A');
        placed_bids_this_round_ = true;
        last_agg_buy_msecs_ = watch_.msecs_from_midnight();
      }
    } else {
      // only after canceling them can we be allowed to place aggressive orders
      order_manager_.CancelBidsAboveIntPrice(best_nonself_bid_int_price_);
    }
  }

  if ((!placed_bids_this_round_) && (top_bid_improve_)) {
    // In cases when MUR < 1, see agg side
    int improve_size = MathUtils::GetFlooredMultipleOf(
        std::min(param_set_.max_position_ - my_position_, current_tradevarset_.l1bid_trade_size_),
        param_set_.min_order_size_);

    // only place Bid Improve orders when there is no active unconfirmed order at or above the
    // best_nonself_bid_int_price_
    //    and no confirmed orders above the best price
    if ((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_ + 1) == 0) &&
        (order_manager_.SumBidSizeConfirmedAboveIntPrice(best_nonself_bid_int_price_) == 0)) {
      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the bid side
      if (my_position_ + order_manager_.SumBidSizes() + improve_size >= param_set_.worst_case_position_) {
        int _canceled_size_ = 0;
        // then cancel Bids from bottom levels for the required size
        _canceled_size_ += order_manager_.CancelBidsFromFar(improve_size);
      } else {
        // Place new order
        PlaceIntOrderAndLog(best_nonself_bid_int_price_ + 1, improve_size, kTradeTypeBuy, 'I');
        placed_bids_this_round_ = true;
        last_agg_buy_msecs_ = watch_.msecs_from_midnight();
      }
    }
  } else {
    if ((dep_market_view_.spread_increments() > 1) && (!bid_improve_keep_)) {
      int cancelled_size_ = order_manager_.CancelBidsAboveIntPrice(best_nonself_bid_int_price_);
      if (cancelled_size_ > 0) {
        canceled_bids_this_round_ = true;
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " Cancelled Improve Bid orders Above: " << best_nonself_bid_int_price_
                                 << " Position: " << my_position_ << " ebp_t: "
                                 << (target_price_ - best_nonself_bid_price_ + bestbid_queue_hysterisis_) /
                                        dep_market_view_.min_price_increment() << " thresh_t: "
                                 << current_tradevarset_.l1bid_improve_keep_ / dep_market_view_.min_price_increment()
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  PlaceBestBid();

  // Active ASK order management
  placed_asks_this_round_ = false;
  canceled_asks_this_round_ = false;
  if (top_bid_hit_) {
    // Adjusting the size in a way that we don't cross maxposition, ( case for MUR < 1)
    int aggress_size = MathUtils::GetFlooredMultipleOf(
        std::min(std::min(param_set_.max_position_ + my_position_, current_tradevarset_.l1ask_trade_size_),
                 best_nonself_bid_size_),
        param_set_.min_order_size_);

    // only place aggressive orders when there is no active unconfirmed order and no confirmed orders above the best
    // price
    if ((order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_ - 1) == 0) &&
        (order_manager_.SumAskSizeConfirmedAboveIntPrice(best_nonself_ask_int_price_) == 0)) {
      int _canceled_size_ = 0;
      if (!top_ask_keep_) {  // if due to position placing aggressive LIFT order requires us to cancel asks from all
                             // active levels
        _canceled_size_ += order_manager_.CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_);
      }

      int allowance_for_aggressive_sell_ =
          -my_position_ + order_manager_.SumAskSizes() + aggress_size - param_set_.worst_case_position_;

      if (param_set_.use_new_aggress_logic_) {
        allowance_for_aggressive_sell_ = -my_position_ + order_manager_.SumAskSizes() + aggress_size -
                                         std::max(param_set_.worst_case_position_, param_set_.max_position_);
      }

      if (allowance_for_aggressive_sell_ >= 0) {
        if (allowance_for_aggressive_sell_ > _canceled_size_) {
          _canceled_size_ += order_manager_.CancelAsksFromFar(
              current_tradevarset_.l1ask_trade_size_);  ///< then cancel Asks from bottom levels for the required size
        }
      } else {
        // Place new order
        PlaceOrderAndLog(best_nonself_bid_price_, best_nonself_bid_int_price_, aggress_size, kTradeTypeSell, 'A');
        placed_asks_this_round_ = true;
        last_agg_sell_msecs_ = watch_.msecs_from_midnight();
      }
    } else {
      order_manager_.CancelAsksAboveIntPrice(
          best_nonself_ask_int_price_);  // only after canceling them can we be allowed to place aggressive orders
    }
  }

  if ((!placed_asks_this_round_) && (top_ask_improve_)) {
    // In cases when MUR < 1, see agg side
    int improve_size = MathUtils::GetFlooredMultipleOf(
        std::min(param_set_.max_position_ + my_position_, current_tradevarset_.l1ask_trade_size_),
        param_set_.min_order_size_);

    // only place Ask Improve orders when there is no active unconfirmed order and no confirmed orders above the best
    // price
    if ((order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_ - 1) == 0) &&
        (order_manager_.SumAskSizeConfirmedAboveIntPrice(best_nonself_ask_int_price_) == 0)) {
      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the ask side
      if (-my_position_ + order_manager_.SumAskSizes() + improve_size > param_set_.worst_case_position_) {
        int _canceled_size_ = 0;
        ///< then cancel Asks from bottom levels for the required size
        _canceled_size_ += order_manager_.CancelAsksFromFar(improve_size);
      } else {
        // Place new order
        PlaceIntOrderAndLog(best_nonself_ask_int_price_ - 1, improve_size, kTradeTypeSell, 'I');
        placed_asks_this_round_ = true;
        last_agg_sell_msecs_ = watch_.msecs_from_midnight();
      }
    }
  } else {
    if (dep_market_view_.spread_increments() > 1 && !ask_improve_keep_) {
      int cancelled_size_ = order_manager_.CancelAsksAboveIntPrice(best_nonself_ask_int_price_);

      if (cancelled_size_ > 0) {
        canceled_asks_this_round_ = true;
        LogCancelOrder(best_nonself_ask_int_price_, cancelled_size_, "Improve Asks above");
      }
    }
  }

  PlaceBestAsk();

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

void PriceBasedAggressiveProRataTrading::PrintFullStatus() {
  DBGLOG_TIME << "tgt: " << target_price_ << " mkt " << best_nonself_bid_price_ << " X " << best_nonself_bid_size_
              << " pft: " << (target_price_ - best_nonself_bid_price_) << " -- " << best_nonself_ask_price_ << " X "
              << best_nonself_ask_size_ << " pft: " << (best_nonself_ask_price_ - target_price_)
              << " bias: " << targetbias_numbers_ << " pos: " << my_position_ << " risk: " << my_risk_
              << " smaps: " << order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_) << " "
              << order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) << " "
              << order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) << " "
              << order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_)
              << " opnl: " << order_manager_.base_pnl().opentrade_unrealized_pnl() << " gpos: " << my_global_position_
              << DBGLOG_ENDL_FLUSH;
}

bool PriceBasedAggressiveProRataTrading::CanPlaceBidsThisRound() {
  return (
      (current_tradevarset_.l1bid_trade_size_ > 0) &&
      ((last_buy_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
       (best_nonself_bid_int_price_ - last_buy_int_price_ < (param_set_.px_band_ - 1))) &&
      // If the sell trades on the source crosses cancel threshold then it will not place any orders on the buy level
      (last_bigtrades_bid_cancel_msecs_ <= 0 ||
       (watch_.msecs_from_midnight() - last_bigtrades_bid_cancel_msecs_ >= param_set_.bigtrades_cooloff_interval_)) &&
      (!cancel_l1_bid_ask_flow_buy_));
}

bool PriceBasedAggressiveProRataTrading::CanPlaceAsksThisRound() {
  return (
      (current_tradevarset_.l1ask_trade_size_ > 0) &&
      ((last_sell_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
       (last_sell_int_price_ - best_nonself_ask_int_price_ < (param_set_.px_band_ - 1))) &&
      // If the buy trades on the source crosses cancel threshold then it will not place any orders on the sell level
      (last_bigtrades_ask_cancel_msecs_ <= 0 ||
       (watch_.msecs_from_midnight() - last_bigtrades_ask_cancel_msecs_ >= param_set_.bigtrades_cooloff_interval_)) &&
      (!cancel_l1_bid_ask_flow_sell_));
}

void PriceBasedAggressiveProRataTrading::SetBidImproveAggressFlags() {
  if (watch_.msecs_from_midnight() - last_agg_buy_msecs_ > param_set_.agg_cooloff_interval_) {
    /*
     * position is not too long already, spread narrow, signal strong
     */
    if (CanBidAggressThisRound()) {
      top_ask_lift_ = true;
      /*
       * when we are already long and ask_lift is set to true,
       * place and keep at top bid are set false so that
       * we cancel top level orders before we place new ones at aggressive prices
       */
      if (my_position_ >= param_set_.max_position_to_cancel_on_lift_) {
        top_bid_place_ = false;
        top_bid_keep_ = false;
      }
    } else {
      top_ask_lift_ = false;
      /*
       *position is not too long already, spread wide, signal strong
       */
      if (CanBidImproveThisRound()) {
        top_bid_improve_ = true;
      } else {
        top_bid_improve_ = false;
      }
    }
  }
}

void PriceBasedAggressiveProRataTrading::SetAskImproveAggressFlags() {
  if (watch_.msecs_from_midnight() - last_agg_sell_msecs_ > param_set_.agg_cooloff_interval_) {
    /*
     *position is not too short already, spread narrow, signal strong
     */

    if (CanAskAggressThisRound()) {
      top_bid_hit_ = true;
      /*
       *when we are already short and bit_hit is set to true,
       *place and keep at top ask are set false so that
       *we cancel top level orders before we place new ones at aggressive prices
       */

      if (my_position_ <= param_set_.min_position_to_cancel_on_hit_) {
        top_ask_place_ = false;
        top_ask_keep_ = false;
      }
    } else {
      top_bid_hit_ = false;
      if (CanAskImproveThisRound()) {
        top_ask_improve_ = true;
      } else {
        top_ask_improve_ = false;
      }
    }
  }
}

bool PriceBasedAggressiveProRataTrading::CanPlaceBestBidThisRound() {
  return ((best_nonself_bid_size_ > param_set_.safe_distance_) ||
          (((target_price_ - best_nonself_bid_price_ - targetbias_numbers_) * param_set_.sumvars_scaling_factor_ +
                targetbias_numbers_ >=
            current_tradevarset_.l1bid_place_ - l1_bias_ - l1_order_bias_ - short_positioning_bias_ +
                param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1)) &&
           (best_nonself_bid_size_ >
            param_set_.min_size_to_join_) &&  // Don't place any orders at a level if less than X size there
           (param_set_.place_on_trade_update_implied_quote_ || !dep_market_view_.trade_update_implied_quote())) ||
          // If the buy trades on the source crosses place threshold then it will place on best level irrespective of
          // the indicators
          (watch_.msecs_from_midnight() - last_bigtrades_bid_place_msecs_ <=
           param_set_.bigtrades_place_cooloff_interval_) ||
          // If the buy trades on the source crosses aggress threshold then it will place on best level irrespective of
          // the indicators
          (watch_.msecs_from_midnight() - last_bigtrades_bid_aggress_msecs_ <=
           param_set_.bigtrades_aggress_cooloff_interval_));
}

bool PriceBasedAggressiveProRataTrading::CanPlaceBestAskThisRound() {
  return ((best_nonself_ask_size_ > param_set_.safe_distance_) ||
          (((best_nonself_ask_price_ - target_price_ + targetbias_numbers_) * param_set_.sumvars_scaling_factor_ -
                targetbias_numbers_ >=
            current_tradevarset_.l1ask_place_ - l1_bias_ - l1_order_bias_ - long_positioning_bias_ +
                param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1)) &&
           (best_nonself_ask_size_ > param_set_.min_size_to_join_) &&
           (param_set_.place_on_trade_update_implied_quote_ || !dep_market_view_.trade_update_implied_quote())) ||
          // If the sell trades on the source crosses place threshold then it will place on best level irrespective of
          // the indicators
          (watch_.msecs_from_midnight() - last_bigtrades_ask_place_msecs_ <=
           param_set_.bigtrades_place_cooloff_interval_) ||
          // If the sell trades on the source crosses aggress threshold then it will place on best level irrespective of
          // the indicators
          (watch_.msecs_from_midnight() - last_bigtrades_ask_aggress_msecs_ <=
           param_set_.bigtrades_aggress_cooloff_interval_));
}

bool PriceBasedAggressiveProRataTrading::CanBidImproveThisRound() {
  return (param_set_.allowed_to_improve_) &&
         (my_position_ <=
          param_set_.max_position_to_bidimprove_) &&  // Don't improve bid when my_position_ is already decently long
         (dep_market_view_.spread_increments() >= param_set_.min_int_spread_to_improve_) &&
         ((target_price_ - best_nonself_bid_price_ - dep_market_view_.min_price_increment() >=
           current_tradevarset_.l1bid_improve_) ||
          // If the buy trades on the source crosses aggress threshold then it will try to improve the best level
          (watch_.msecs_from_midnight() - last_bigtrades_bid_aggress_msecs_ <=
           param_set_.bigtrades_aggress_cooloff_interval_)) &&
         ((last_buy_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
          (((best_nonself_bid_int_price_ + 1) - last_buy_int_price_) < (param_set_.px_band_ - 1)));
}

bool PriceBasedAggressiveProRataTrading::CanAskImproveThisRound() {
  return (param_set_.allowed_to_improve_) &&
         (my_position_ >=
          param_set_.min_position_to_askimprove_) &&  // Don't improve ask when my_position_ is already decently short
         (best_nonself_ask_int_price_ - best_nonself_bid_int_price_ >= param_set_.min_int_spread_to_improve_) &&
         ((best_nonself_ask_price_ - target_price_ - dep_market_view_.min_price_increment() >=
           current_tradevarset_.l1ask_improve_) ||
          // If the sell trades on the source crosses aggress threshold then it will try to improve the best level
          (watch_.msecs_from_midnight() - last_bigtrades_ask_aggress_msecs_ <=
           param_set_.bigtrades_aggress_cooloff_interval_)) &&
         ((last_sell_msecs_ <= 0) ||
          (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
          (((last_sell_int_price_ - 1) - best_nonself_ask_int_price_) < (param_set_.px_band_ - 1)));
}

bool PriceBasedAggressiveProRataTrading::CanBidAggressThisRound() {
  return (param_set_.allowed_to_aggress_) &&  // external control on aggressing
         (((target_price_ - best_nonself_ask_price_ - targetbias_numbers_) * param_set_.sumvars_scaling_factor_ +
               targetbias_numbers_ >=
           current_tradevarset_.l1bid_aggressive_ +
               param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1)) ||
          // If the buy trades on the source crosses aggress threshold then it will aggress
          (watch_.msecs_from_midnight() - last_bigtrades_bid_aggress_msecs_ <=
           param_set_.bigtrades_aggress_cooloff_interval_)) &&  // only LIFT offer if the margin of buying here i.e. (
         // target_price_ - best_nonself_ask_price_ ) exceeds the threshold
         // current_tradevarset_.l1bid_aggressive_
         (my_position_ <=
          param_set_.max_position_to_lift_) &&  // Don't LIFT offer when my_position_ is already decently long
         (dep_market_view_.spread_increments() <=
          param_set_.max_int_spread_to_cross_) &&  // Don't LIFT when effective spread is to much
         ((last_buy_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
          ((best_nonself_ask_int_price_ - last_buy_int_price_) < (param_set_.px_band_ - 1)));
}

bool PriceBasedAggressiveProRataTrading::CanAskAggressThisRound() {
  return (param_set_.allowed_to_aggress_) &&  // external control on aggressing

         (my_position_ >=
          param_set_.min_position_to_hit_) &&  // Don't HIT bid when my_position_ is already decently short
         (best_nonself_ask_int_price_ - best_nonself_bid_int_price_ <=
          param_set_.max_int_spread_to_cross_) &&  // Don't HIT ( cross ) when effective spread is to much
         (((best_nonself_bid_price_ - target_price_ + targetbias_numbers_) * param_set_.sumvars_scaling_factor_ -
               targetbias_numbers_ >=
           current_tradevarset_.l1ask_aggressive_ +
               param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1)) ||
          // If the sell trades on the source crosses aggress threshold then it will aggress
          (watch_.msecs_from_midnight() - last_bigtrades_ask_aggress_msecs_ <=
           param_set_.bigtrades_aggress_cooloff_interval_)) &&
         ((last_sell_msecs_ <= 0) ||
          (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
          ((last_sell_int_price_ - best_nonself_bid_int_price_) < (param_set_.px_band_ - 1)));
}

void PriceBasedAggressiveProRataTrading::SetBidKeepFlag() {
  if (((target_price_ - best_nonself_bid_price_ - targetbias_numbers_) * param_set_.sumvars_scaling_factor_ +
       targetbias_numbers_ + bestbid_queue_hysterisis_) >=
      current_tradevarset_.l1bid_keep_ - l1_bid_trade_bias_ - l1_bias_ - l1_order_bias_ - short_positioning_bias_ +
          param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1)) {
    top_bid_keep_ = true;
  } else {
    top_bid_keep_ = false;
  }
}

void PriceBasedAggressiveProRataTrading::SetAskKeepFlag() {
  if (((best_nonself_ask_price_ - target_price_ + targetbias_numbers_) * param_set_.sumvars_scaling_factor_ -
       targetbias_numbers_ + bestask_queue_hysterisis_) >=
      current_tradevarset_.l1ask_keep_ - l1_ask_trade_bias_ - l1_bias_ - l1_order_bias_ - long_positioning_bias_ +
          param_set_.spread_increase_ * (dep_market_view_.spread_increments() -
                                         1)) {  // but with place in line effect enough to keep the live order there
    top_ask_keep_ = true;
  } else {
    top_ask_keep_ = false;
  }
}
void PriceBasedAggressiveProRataTrading::SetBidImproveKeepFlag() {
  if (((dep_market_view_.spread_increments() > 1) &&
       (target_price_ - best_nonself_bid_price_) >=
           current_tradevarset_.l1bid_improve_keep_ - l1_bias_ - l1_order_bias_ - short_positioning_bias_ +
               param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1))) {
    bid_improve_keep_ = true;
  } else {
    bid_improve_keep_ = false;
  }
}

void PriceBasedAggressiveProRataTrading::SetAskImproveKeepFlag() {
  if ((dep_market_view_.spread_increments() > 1) &&
      ((best_nonself_ask_price_ - target_price_) >=
       current_tradevarset_.l1ask_improve_keep_ - l1_bias_ - l1_order_bias_ - long_positioning_bias_ +
           param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1))) {
    ask_improve_keep_ = true;
  } else {
    ask_improve_keep_ = false;
  }
}

void PriceBasedAggressiveProRataTrading::ModifyOrder(double _old_price_, int _old_int_price_, int _old_size_,
                                                     TradeType_t _old_buysell_, char _old_order_level_indicator_,
                                                     double _new_price_, int _new_int_price_, int _new_size_,
                                                     TradeType_t _new_buysell_, char _new_order_level_indicator_) {
  if (_old_buysell_ == kTradeTypeBuy) {
    int canceled_size_ = order_manager_.CancelBidsEqAboveIntPrice(_old_int_price_, _old_size_);
    LogCancelOrder(_old_price_, canceled_size_, "Eq Above");
    PlaceOrderAndLog(_new_price_, _new_int_price_, _new_size_, _new_buysell_, _new_order_level_indicator_);
  } else if (_old_buysell_ == kTradeTypeSell) {
    int canceled_size_ = order_manager_.CancelAsksEqAboveIntPrice(_old_int_price_, _old_size_);
    LogCancelOrder(_old_price_, canceled_size_, "Eq Above");
    PlaceOrderAndLog(_new_price_, _new_int_price_, _new_size_, _new_buysell_, _new_order_level_indicator_);
  }
}

void PriceBasedAggressiveProRataTrading::PlaceIntOrderAndLog(int _int_price_, int _size_, TradeType_t _buysell_,
                                                             char _order_level_indicator_) {
  double _price_ = dep_market_view_.GetDoublePx(_int_price_);
  PlaceOrderAndLog(_price_, _int_price_, _size_, _buysell_, _order_level_indicator_);
}

void PriceBasedAggressiveProRataTrading::PlaceOrderAndLog(double _price_, int _int_price_, int _size_,
                                                          TradeType_t _buysell_, char _order_level_indicator_) {
  order_manager_.SendTrade(_price_, _int_price_, _size_, _buysell_, _order_level_indicator_);
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
  {
    DBGLOG_TIME_CLASS_FUNC << "Sending " << dep_market_view_.secname() << " " << _order_level_indicator_ << " "
                           << ((_buysell_ == kTradeTypeBuy) ? 'B' : ((_buysell_ == kTradeTypeSell) ? 'S' : 'N'))
                           << " Order at px " << _price_ << " size: " << _size_ << " position " << my_position_
                           << " ebp_t: "
                           << (target_price_ - best_nonself_bid_price_) / dep_market_view_.min_price_increment()
                           << " thresh_t: "
                           << current_tradevarset_.l1bid_place_ / dep_market_view_.min_price_increment()
                           << " IntPx: " << best_nonself_bid_int_price_ << " tMktSz: " << best_nonself_bid_size_
                           << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                           << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_
                           << " StdevSpreadFactor: " << stdev_scaled_capped_in_ticks_ << DBGLOG_ENDL_FLUSH;
    p_base_model_math_->ShowIndicatorValues();
  }
}

void PriceBasedAggressiveProRataTrading::LogCancelOrder(double _price_, int canceled_size_,
                                                        const std::string& _cancel_string_) {
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "Canceled " << canceled_size_ << " " << _cancel_string_ << " " << _price_ << " ebp_t: "
                           << (target_price_ - best_nonself_bid_price_ + bestbid_queue_hysterisis_) /
                                  dep_market_view_.min_price_increment()
                           << " thresh_t: " << current_tradevarset_.l1bid_keep_ / dep_market_view_.min_price_increment()
                           << " tMktSz: " << best_nonself_bid_size_ << " mkt: " << best_nonself_bid_size_ << " @ "
                           << best_nonself_bid_price_ << " X " << best_nonself_ask_price_ << " @ "
                           << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
  }
}

/**
 * Place best bids if there are already no orders
 */
void PriceBasedAggressiveProRataTrading::PlaceBestBid() {
  /*
   * Need to consider the size reduction due to execution too
   */

  // Not considering prices above best ( not needed in case of Pro-Rata prods as they are 1 ticks)
  int bid_size_cancel_requested = order_manager_.SumBidSizeCancelRequested(best_nonself_bid_int_price_);
  int conf_unconf_size = (order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_) +
                          order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_));

  int size_at_best = order_manager_.GetTotalBidSizeOrderedAtIntPx(best_nonself_bid_int_price_);
  int orders_at_best = order_manager_.GetNumBidOrdersAtIntPx(best_nonself_bid_int_price_);

  int total_size_to_place = ComputeSizeToPlace(best_nonself_bid_size_, kTradeTypeBuy, my_position_);
  int total_new_size_to_place = total_size_to_place - conf_unconf_size;

  bool place_order = total_new_size_to_place > 0 && orders_at_best < param_set_.max_unit_size_at_level_ &&
                     top_bid_place_ && !placed_bids_this_round_;

  // in case we improved, we would not want to cancel best-level orders
  // if their size is not more than unit trade-size
  bool cancel_order = ((total_size_to_place - size_at_best < 0) || !top_bid_keep_) && !placed_bids_this_round_;

  // top_bid_keep_&& size_at_best <= total_size_to_place;
  bool spread_condition = stdev_ <= param_set_.low_stdev_lvl_ ||
                          // Don't place any new orders in inside market if the spread is too wide
                          (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_place_);

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << dep_market_view_.secname() << " BestBid: " << best_nonself_bid_int_price_
                                << " BestAsk: " << best_nonself_ask_int_price_ << " ESizeAtBest: " << size_at_best
                                << " Order: " << orders_at_best << " New_Size_Place_Val: " << total_new_size_to_place
                                << " Conf_Unconf_Size: " << conf_unconf_size << " Total_Size: " << total_size_to_place
                                << " Cxl: " << cancel_order << " Place: " << place_order << " Spd: " << spread_condition
                                << DBGLOG_ENDL_FLUSH;
  }

  if (place_order && spread_condition) {
    {
      int num_orders_to_place = total_new_size_to_place / current_tradevarset_.l1bid_trade_size_;
      for (auto num_order = 0; num_order < num_orders_to_place && num_order < param_set_.max_unit_size_at_level_;
           num_order++) {
        PlaceOrderAndLog(best_nonself_bid_price_, best_nonself_bid_int_price_, current_tradevarset_.l1bid_trade_size_,
                         kTradeTypeBuy, 'B');
      }
    }
  } else if (cancel_order) {
    auto canceled_size = 0;
    auto size_to_cancel = total_size_to_place - size_at_best + bid_size_cancel_requested;

    // cancel only when size to cancel is higher than size already canceled
    size_to_cancel = size_to_cancel > 0 ? 0 : -size_to_cancel;

    if (!top_bid_keep_) {
      // If top bid-keep is false then cancel everything
      canceled_size = order_manager_.CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_);
    } else {
      // Cancel if l1-book has decreased
      // TODO  We are rounding of the cancellation size to nearest uts-multiple values
      // Can be more flexible here

      if (param_set_.allow_modify_orders_) {
        canceled_size = order_manager_.CancelOrModifyBidOrdersAboveIntPrice(
            best_nonself_bid_int_price_, size_to_cancel,
            param_set_.max_fraction_uts_size_to_modify_ * param_set_.unit_trade_size_);
      } else {
        canceled_size = order_manager_.CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_, size_to_cancel);
      }
    }

    if (true /*canceled_size_ > 0 && */) {
      canceled_bids_this_round_ = true;
      if (/*canceled_size_ > 0 && */ dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " Canceled B EqAboveIntPrice: " << best_nonself_bid_int_price_
                                    << " Size_Canceled: " << canceled_size << " Requested_Size: " << size_to_cancel
                                    << " New_Size_Place_Val: " << total_new_size_to_place
                                    << " Conf_Unconf_Size: " << conf_unconf_size
                                    << " Total_Size: " << total_size_to_place << " BK: " << top_bid_keep_
                                    << DBGLOG_ENDL_FLUSH;
      }
    }
  }
}

void PriceBasedAggressiveProRataTrading::PlaceBestAsk() {
  int ask_size_cancel_requested = order_manager_.SumAskSizeCancelRequested(best_nonself_ask_int_price_);
  int conf_unconf_size = (order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_) +
                          order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_));

  int size_at_best = order_manager_.GetTotalAskSizeOrderedAtIntPx(best_nonself_ask_int_price_);
  int orders_at_best = order_manager_.GetNumAskOrdersAtIntPx(best_nonself_ask_int_price_);

  int total_size_to_place = ComputeSizeToPlace(best_nonself_ask_size_, kTradeTypeSell, my_position_);

  // only place Ask orders when there is not enough active unconfirmed order at or above best_nonself_ask_int_price_
  //      and no confirmed order at or above the best price

  int total_new_size_to_place = total_size_to_place - conf_unconf_size;

  bool place_order = total_new_size_to_place > 0 && orders_at_best < param_set_.max_unit_size_at_level_ &&
                     top_ask_place_ && !placed_asks_this_round_;
  // in case we improved, we would not want to cancel best-level orders
  // if their size is not more than unit trade-size

  bool cancel_order = ((total_size_to_place - size_at_best < 0) || !top_ask_keep_) && !placed_asks_this_round_;

  bool spread_condition = stdev_ <= param_set_.low_stdev_lvl_ ||
                          // Don't place any new orders in inside market if the spread is too wide
                          (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_place_);

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << dep_market_view_.secname() << " BestBid" << best_nonself_bid_int_price_
                                << " BestAsk: " << best_nonself_ask_int_price_ << " ESizeAtBest: " << size_at_best
                                << " Order: " << orders_at_best << " New_Size_Place_Val: " << total_new_size_to_place
                                << " Conf_Unconf_Size: " << conf_unconf_size << " Total_Size: " << total_size_to_place
                                << " Cxl: " << cancel_order << " Place: " << place_order << " Spd: " << spread_condition
                                << DBGLOG_ENDL_FLUSH;
  }

  if (place_order && spread_condition) {
    int num_orders_to_place = total_new_size_to_place / current_tradevarset_.l1ask_trade_size_;

    // Place multiple orders with same size
    for (auto i = 0; i < num_orders_to_place && i < param_set_.max_unit_size_at_level_; i++) {
      PlaceOrderAndLog(best_nonself_ask_price_, best_nonself_ask_int_price_, current_tradevarset_.l1ask_trade_size_,
                       kTradeTypeSell, 'B');
    }

  } else if (cancel_order) {
    int canceled_size = 0;
    int size_to_cancel = total_size_to_place - size_at_best + ask_size_cancel_requested;

    // cancel only if size to cancel is higher than size already canceled
    size_to_cancel = size_to_cancel > 0 ? 0 : -size_to_cancel;

    if (!top_ask_keep_) {
      canceled_size = order_manager_.CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_);
    } else {
      // Cancel if l1-book has decreased
      // TODO  We are rounding of the cancellation size to nearest uts-multiple values
      // Can be more flexible here

      if (param_set_.allow_modify_orders_) {
        canceled_size = order_manager_.CancelOrModifyAskOrdersEqAboveIntPirce(
            best_nonself_ask_int_price_, size_to_cancel,
            param_set_.max_fraction_uts_size_to_modify_ * param_set_.unit_trade_size_);
      } else {
        canceled_size = order_manager_.CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_, size_to_cancel);
      }
    }
    if (true /*canceled_size > 0*/) {
      canceled_asks_this_round_ = true;
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " Canceled S EqAboveIntPrice: " << best_nonself_ask_int_price_
                                    << " Size_Canceled: " << canceled_size << " Requested_Size: " << size_to_cancel
                                    << " New_Size_Place_Val: " << total_new_size_to_place
                                    << " Conf_Unconf_Size: " << conf_unconf_size
                                    << " Total_Size: " << total_size_to_place << " AK: " << top_ask_keep_
                                    << DBGLOG_ENDL_FLUSH;
      }
    }
  }
}

/**
 * Changing the logic of typical place-cancel. Following are the two major changes
 * a) Given the overall target is to not get filled for more than 1 UTS and
 *    There are three kind of fills- FIFO, pro-rata, pro-rata 2nd round,
 *    i)   For getting fills based on FIFO,  we just need maximum of one order on non-best level
 *    ii)  Pro-rata doesn't need non-best level orders
 *    iii) In case of ZT, we would get fill for maximum 1 size  for each extra order for 3rd type
 *         In case of GE's 3rd round of allocation would benefit first order.. mostly
 * b) cancel from far
 *
 */
void PriceBasedAggressiveProRataTrading::PlaceCancelNonBestLevels() {
  if (getflat_due_to_regime_indicator_) {
    return;
  }
  // compute them less frequently
  if (exec_logic_indicators_helper_->place_nonbest()) {
    PlaceNonBestLevelBids();
    PlaceNonBestLevelAsks();

    CancelNonBestLevelBids();
    CancelNonBestLevelAsks();

  } else {
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "Cancelling all non best level orders as stdev is high" << DBGLOG_ENDL_FLUSH;
    }
    order_manager_.CancelBidsEqBelowIntPrice(best_nonself_bid_int_price_ - 1);
    order_manager_.CancelAsksEqBelowIntPrice(best_nonself_ask_int_price_ + 1);
  }
}

void PriceBasedAggressiveProRataTrading::PlaceNonBestLevelBids() {
  int t_position_ = my_position_;

  const int t_worst_case_long_position_ =
      (param_set_.read_explicit_max_long_position_ ? param_set_.explicit_worst_case_long_position_
                                                   : param_set_.worst_case_position_);

  // placing supporting orders to num_non_best_bid_levels_monitored_
  if (my_position_ <= (param_set_.max_position_ / 2) || param_set_.ignore_max_pos_check_for_non_best_) {
    // position is not very long
    unsigned int start_index = 1;

    start_index = std::max(start_index, param_set_.min_distance_for_non_best_);

    if (param_set_.min_size_ahead_for_non_best_ > 0) {
      unsigned int size_ahead = 0;
      auto i = 0u;
      for (; size_ahead <= param_set_.min_size_ahead_for_non_best_ && i <= param_set_.max_distance_for_non_best_; i++) {
        size_ahead += dep_market_view_.bid_size(i);
      }
      start_index = std::max(start_index, i);
    }

    for (unsigned int level_index = start_index;
         level_index < (start_index + param_set_.num_non_best_bid_levels_monitored_ - 1);
         level_index++) {  // check levels from 2 to param_set_.num_non_best_bid_levels_monitored_

      if ((int)(t_position_ + order_manager_.SumBidSizes() + (2 * param_set_.unit_trade_size_)) <
          t_worst_case_long_position_) {
        // placing orders still keeps the total active orders within
        // worst_case_position_ then place orders at this level

        int _this_bid_int_price_ = (best_nonself_bid_int_price_ - level_index);
        if (order_manager_.GetTotalBidSizeOrderedAtIntPx(_this_bid_int_price_) == 0) {
          // if no orders at this level
          SendTradeAndLog(dep_market_view_.GetDoublePx(_this_bid_int_price_), _this_bid_int_price_,
                          position_tradevarset_map_[P2TV_zero_idx_].l1bid_trade_size_, kTradeTypeBuy, 'S',
                          "SupportingSendTrade");
        }
      }
    }
  }
}

void PriceBasedAggressiveProRataTrading::PlaceNonBestLevelAsks() {
  int t_position_ = my_position_;

  const int t_worst_case_short_position_ =
      (param_set_.read_explicit_max_short_position_ ? param_set_.explicit_worst_case_short_position_
                                                    : param_set_.worst_case_position_);

  if (my_position_ >= (-param_set_.max_position_ / 2) || param_set_.ignore_max_pos_check_for_non_best_) {
    // position is not too short

    unsigned int start_index = 1;
    start_index = std::max(start_index, param_set_.min_distance_for_non_best_);
    if (param_set_.min_size_ahead_for_non_best_ > 0) {
      unsigned int size_ahead = 0;
      auto i = 0u;
      for (; size_ahead <= param_set_.min_size_ahead_for_non_best_ && i <= param_set_.max_distance_for_non_best_; i++) {
        size_ahead += dep_market_view_.bid_size(i);
      }
      start_index = std::max(start_index, i);
    }

    for (unsigned int level_index = start_index;
         level_index < (start_index + param_set_.num_non_best_ask_levels_monitored_ - 1);
         level_index++) {  // check levels from 2 to param_set_.num_non_best_ask_levels_monitored_

      if ((int)(-t_position_ + order_manager_.SumAskSizes() + (2 * param_set_.unit_trade_size_)) <
          t_worst_case_short_position_) {  // placing orders still keeps the total active orders within
        // worst_case_position_ then place orders at this level

        int this_ask_int_price = best_nonself_ask_int_price_ + level_index;
        if (order_manager_.GetTotalAskSizeOrderedAtIntPx(this_ask_int_price) == 0) {
          // if no orders at this level
          SendTradeAndLog(dep_market_view_.GetDoublePx(this_ask_int_price), this_ask_int_price,
                          position_tradevarset_map_[P2TV_zero_idx_].l1ask_trade_size_, kTradeTypeSell, 'S',
                          "SupportingSendTrade");
        }
      }
    }
  }
}

void PriceBasedAggressiveProRataTrading::CancelNonBestLevelBids() {
  int t_position_ = my_position_;

  const int t_worst_case_long_position_ =
      (param_set_.read_explicit_max_long_position_ ? param_set_.explicit_worst_case_long_position_
                                                   : param_set_.worst_case_position_);

  // cancel orders that get us to worst_case_position_ or more

  int order_vec_top_bid_index_ = order_manager_.GetOrderVecTopBidIndex();
  int order_vec_bottom_bid_index_ = order_manager_.GetOrderVecBottomBidIndex();
  auto& bid_order_vec = order_manager_.BidOrderVec();

  // taking max as it can be -ve in case current position is more than wpos and
  // go into error case even if we dont have any existing orders in mkt

  int _max_size_resting_bids_ = std::max(0, t_worst_case_long_position_ - std::max(0, t_position_));

  auto t_sum_bid_size = order_manager_.SumBidSizes();

  if (t_sum_bid_size > _max_size_resting_bids_) {
    // Only if we are already not crossing wpos thresholds for placing the orders

    if (order_vec_top_bid_index_ != -1) {
      auto size_canceled_so_far = 0;
      for (auto order_vec_index = order_vec_bottom_bid_index_; order_vec_index <= order_vec_top_bid_index_;
           order_vec_index++) {
        // the following code makes this not look at top level orders
        // hence top level orders are not affected by worst_case_position
        if (order_manager_.GetBidIntPrice(order_vec_index) >= best_nonself_bid_int_price_) continue;

        /**
         *
         * we iterate through all levels to see if we can  keep 1 order at non-best level and be below wpos
         *
         */

        // only look to cancel non-first orders in the vector
        if (bid_order_vec[order_vec_index].size() > 1u) {
          for (auto i = (int)bid_order_vec[order_vec_index].size() - 1; i >= 1; i--) {
            auto this_order = bid_order_vec[order_vec_index][i];

            // Try canceling the order
            if (order_manager_.Cancel(*this_order)) {
              LogCancelOrder(this_order->price(), this_order->size_remaining(), "SubBest");
            }

            if (this_order->canceled()) {
              size_canceled_so_far += this_order->size_remaining();
            }

            // if remaining bid size is less than max-resting-size then exit
            if (t_sum_bid_size - size_canceled_so_far <= _max_size_resting_bids_) {
              break;
            }
          }
        }
      }

      // Even now if remaining size is more than allowed by wpos
      if (t_sum_bid_size - size_canceled_so_far > _max_size_resting_bids_) {
        for (auto order_vec_index = order_vec_bottom_bid_index_; order_vec_index <= order_vec_top_bid_index_;
             order_vec_index++) {
          // the following code makes this not look at top level orders
          // hence top level orders are not affected by worst_case_position
          if (order_manager_.GetBidIntPrice(order_vec_index) >= best_nonself_bid_int_price_) continue;

          if (bid_order_vec[order_vec_index].size() > 0u) {
            auto this_order = bid_order_vec[order_vec_index][0];

            if (order_manager_.Cancel(*this_order)) {
              LogCancelOrder(this_order->price(), this_order->size_remaining(), "SubBestFirstOrder");
            }

            if (this_order->canceled()) {
              size_canceled_so_far += this_order->size_remaining();
            }

            // if remaining bid size is less than max-resting-size then exit
            if (t_sum_bid_size - size_canceled_so_far <= _max_size_resting_bids_) {
              break;
            }
          }
        }
      }
    } else {
      std::stringstream st;
      st << "SumBidSize NonZero: " << t_sum_bid_size << " While top_bid_index -1 ";

      auto unconfirmed_ordercount = order_manager_.SumBidUnConfirmedSizes();
      st << "Only Unconfirmed orders " << unconfirmed_ordercount;

      st << " MaxRestingBids: " << _max_size_resting_bids_;

      // Print in the logfile
      DBGLOG_TIME_CLASS_FUNC << st.str() << DBGLOG_ENDL_FLUSH;

      if (unconfirmed_ordercount != t_sum_bid_size) {
        // exit in case unconfirmed size is not equal to total size as there are no confirmed orders
        st << " SumBidSize != Unconfirmed size in case no confirmed orders are present";
        ExitVerbose(kExitErrorCodeZeroValue, st.str().c_str());
      }
    }
  }
}

/**
 *
 */
void PriceBasedAggressiveProRataTrading::CancelNonBestLevelAsks() {
  int t_position_ = my_position_;

  const int t_worst_case_short_position_ =
      (param_set_.read_explicit_max_short_position_ ? param_set_.explicit_worst_case_short_position_
                                                    : param_set_.worst_case_position_);

  // std::vector < std::vector < BaseOrder * > > & ask_order_vec_ = order_manager_.AskOrderVec ( );
  int order_vec_top_ask_index_ = order_manager_.GetOrderVecTopAskIndex();
  int order_vec_bottom_ask_index_ = order_manager_.GetOrderVecBottomAskIndex();

  auto& ask_order_vec = order_manager_.AskOrderVec();

  // taking max as it can be -ve in case current position is more than wpos and
  // go into error case even if we dont have any existing orders in mkt
  int _max_size_resting_asks_ = std::max(0, t_worst_case_short_position_ + std::min(0, t_position_));

  auto t_sum_ask_size = order_manager_.SumAskSizes();

  if (t_sum_ask_size > _max_size_resting_asks_) {
    // Only if we are already not crossing wpos thresholds for placing the orders

    if (order_vec_top_ask_index_ != -1) {
      auto size_canceled_so_far = 0;
      for (auto order_vec_index = order_vec_bottom_ask_index_; order_vec_index <= order_vec_top_ask_index_;
           order_vec_index++) {
        // the following code makes this not look at top level orders
        // hence top level orders are not affected by worst_case_position
        if (order_manager_.GetAskIntPrice(order_vec_index) <= best_nonself_ask_int_price_) continue;

        /**
         *
         * we iterate through all levels to see if we can  keep 1 order at non-best level and be below wpos
         *
         */

        // only look to cancel non-first orders in the vector
        if (ask_order_vec[order_vec_index].size() > 1u) {
          for (auto i = (int)ask_order_vec[order_vec_index].size() - 1; i >= 1; i--) {
            auto this_order = ask_order_vec[order_vec_index][i];

            // Try canceling the order
            if (order_manager_.Cancel(*this_order)) {
              LogCancelOrder(this_order->price(), this_order->size_remaining(), "SubBest");
            }

            if (this_order->canceled()) {
              size_canceled_so_far += this_order->size_remaining();
            }

            // if remaining bid size is less than max-resting-size then exit
            if (t_sum_ask_size - size_canceled_so_far <= _max_size_resting_asks_) {
              break;
            }
          }
        }
      }

      // Even now if remaining size is more than allowed by wpos
      if (t_sum_ask_size - size_canceled_so_far > _max_size_resting_asks_) {
        for (auto order_vec_index = order_vec_bottom_ask_index_; order_vec_index <= order_vec_top_ask_index_;
             order_vec_index++) {
          // the following code makes this not look at top level orders
          // hence top level orders are not affected by worst_case_position
          if (order_manager_.GetAskIntPrice(order_vec_index) <= best_nonself_ask_int_price_) continue;

          if (ask_order_vec[order_vec_index].size() > 0u) {
            auto this_order = ask_order_vec[order_vec_index][0];

            if (order_manager_.Cancel(*this_order)) {
              LogCancelOrder(this_order->price(), this_order->size_remaining(), "SubBestFirstOrder");
            }

            if (this_order->canceled()) {
              size_canceled_so_far += this_order->size_remaining();
            }

            // if remaining bid size is less than max-resting-size then exit
            if (t_sum_ask_size - size_canceled_so_far <= _max_size_resting_asks_) {
              break;
            }
          }
        }
      }
    } else {
      std::stringstream st;
      st << "SumAskSize NonZero: " << t_sum_ask_size << " While top_ask_index -1";

      auto unconfirmed_ordercount = order_manager_.SumAskUnConfirmedSizes();
      st << "Only Unconfirmed orders " << unconfirmed_ordercount;
      st << " MaxRestingBids: " << _max_size_resting_asks_;

      // Print in the logfile
      DBGLOG_TIME_CLASS_FUNC << st.str() << DBGLOG_ENDL_FLUSH;

      if (unconfirmed_ordercount != t_sum_ask_size) {
        // exit in case unconfirmed size is not equal to total size as there are no confirmed orders
        st << " SumBidSize != Unconfirmed size in case no confirmed orders are present";
        ExitVerbose(kExitErrorCodeZeroValue, st.str().c_str());
      }
    }
  }
}

void PriceBasedAggressiveProRataTrading::TradeVarSetLogic(int t_position) {
  current_position_tradevarset_map_index_ = P2TV_zero_idx_;
  // bidside

  if (map_pos_increment_ > 1)  // only when param_set_.max_position_ > MAX_POS_MAP_SIZE
  {
    if (t_position > 0)
      current_position_tradevarset_map_index_ += std::min(MAX_POS_MAP_SIZE, (abs(t_position) / map_pos_increment_));
    else
      current_position_tradevarset_map_index_ -= std::min(MAX_POS_MAP_SIZE, (abs(t_position) / map_pos_increment_));
  } else {
    if (t_position > 0)
      current_position_tradevarset_map_index_ += std::min(MAX_POS_MAP_SIZE, abs(t_position));
    else
      current_position_tradevarset_map_index_ -= std::min(MAX_POS_MAP_SIZE, abs(t_position));
  }

  current_position_tradevarset_map_index_ = std::max(0u, current_position_tradevarset_map_index_);
  current_tradevarset_ = position_tradevarset_map_[current_position_tradevarset_map_index_];

  current_bid_size_to_place_ = current_tradevarset_.l1bid_trade_size_;
  current_ask_size_to_place_ = current_tradevarset_.l1ask_trade_size_;

  if (param_set_.max_position_ >= param_set_.unit_trade_size_) {
    // while placing orders we can check with current uts-max pos

    if ((current_tradevarset_.l1bid_trade_size_ + t_position) > param_set_.max_position_) {
      current_tradevarset_.l1bid_trade_size_ = MathUtils::GetFlooredMultipleOf(
          std::max(0, param_set_.max_position_ - t_position), dep_market_view_.min_order_size());
    }

    if ((current_tradevarset_.l1ask_trade_size_ - t_position) > param_set_.max_position_) {
      current_tradevarset_.l1ask_trade_size_ = MathUtils::GetFlooredMultipleOf(
          std::max(0, param_set_.max_position_ + t_position), dep_market_view_.min_order_size());
    }

  } else {
    // compute the l1bid/l1ask trade sizes based on current market sizes
    ComputeSizeToPlace(best_nonself_bid_size_, kTradeTypeBuy, t_position);
    ComputeSizeToPlace(best_nonself_ask_size_, kTradeTypeSell, t_position);
  }

  if (t_position >= param_set_.max_position_) {
    current_tradevarset_.l1bid_trade_size_ = 0;
  }

  if (t_position <= -param_set_.max_position_) {
    current_tradevarset_.l1ask_trade_size_ = 0;
  }

  if (param_set_.read_max_global_position_) {
    if ((my_global_position_ >= param_set_.max_global_position_) &&
        (t_position >= 0)) {  // All queries are together too long
      current_tradevarset_.l1bid_trade_size_ = 0;
      int canceled_bid_size_ = order_manager_.CancelBidsAtIntPrice(best_nonself_bid_int_price_);

      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " GlobalMaxPosition = " << param_set_.max_global_position_
                               << " my_global_position_ =  " << my_global_position_
                               << " canceled_bid_size_ = " << canceled_bid_size_ << " @ " << best_nonself_bid_int_price_
                               << DBGLOG_ENDL_FLUSH;
      }
    } else {
      if ((my_global_position_ <= -param_set_.max_global_position_) &&
          (t_position <= 0)) {  // All queries are together too short
        current_tradevarset_.l1ask_trade_size_ = 0;
        int canceled_ask_size_ = order_manager_.CancelAsksAtIntPrice(best_nonself_ask_int_price_);

        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " GlobalMaxPosition = " << param_set_.max_global_position_
                                 << " my_global_position_ =  " << my_global_position_
                                 << " canceled_ask_size_ = " << canceled_ask_size_ << " @ "
                                 << best_nonself_ask_int_price_ << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  if (param_set_.read_max_security_position_) {
    if (current_risk_mapped_to_product_position_ >=
        param_set_.max_security_position_) {  //  too long in same underlying
      current_tradevarset_.l1bid_trade_size_ = 0;
    } else {
      if (current_risk_mapped_to_product_position_ <=
          -param_set_.max_security_position_) {  //  too short in same underlying
        current_tradevarset_.l1ask_trade_size_ = 0;
      }
    }
  }

  // This is meant to decrease MUR based on volumes
  if (param_set_.read_scale_max_pos_) {
    if ((current_tradevarset_.l1bid_trade_size_ + t_position > volume_adj_max_pos_) &&
        (t_position >= 0)) {  // Query is too long
      current_tradevarset_.l1bid_trade_size_ = std::max(0, volume_adj_max_pos_ - t_position);
    } else {
      if ((current_tradevarset_.l1ask_trade_size_ - t_position > volume_adj_max_pos_) &&
          (t_position <= 0)) {  // Query is too short
        current_tradevarset_.l1ask_trade_size_ = std::max(0, volume_adj_max_pos_ + t_position);
      }
    }
  }

  ModifyThresholdsAsPerVolatility();
  ModifyThresholdsAsPerModelStdev();
  ModifyThresholdsAsPerPreGetFlat();

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << " newpos: " << t_position << " gpos: " << my_global_position_ << " stddpfac "
                           << stdev_scaled_capped_in_ticks_ << " mapidx " << current_position_tradevarset_map_index_
                           << ' ' << ToString(current_tradevarset_).c_str() << DBGLOG_ENDL_FLUSH;
  }

  if (current_tradevarset_.l1bid_trade_size_ < param_set_.min_allowed_unit_trade_size_) {
    current_tradevarset_.l1bid_trade_size_ = 0;
  }

  if (current_tradevarset_.l1ask_trade_size_ < param_set_.min_allowed_unit_trade_size_) {
    current_tradevarset_.l1ask_trade_size_ = 0;
  }
}
}
