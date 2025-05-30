/*
 * price_based_leg_trading.cpp
 *
 *  Created on: 14-May-2014
 *      Author: archit
 */

#include "dvctrade/ExecLogic/price_based_leg_trading.hpp"

namespace HFSAT {

PriceBasedLegTrading::PriceBasedLegTrading(
    DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
    SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
    MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
    const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int t_runtime_id_,
    const std::vector<std::string> _this_model_source_shortcode_vec_, SpreadTradingManager& _spread_trading_manager_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_),
      spread_trading_manager_(_spread_trading_manager_),
      ideal_spread_position_(0) {
  spread_trading_manager_.AddStmListener(this);
}

void PriceBasedLegTrading::TradingLogic() {
  // setting top level directives
  if (dbglogger_.CheckLoggingLevel(MUM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << " Last Buy: " << last_buy_msecs_ << " Last Sell: " << last_sell_msecs_
                           << " Last agg buy: " << last_agg_buy_msecs_ << " last agg sell: " << last_agg_sell_msecs_
                           << " My position: " << my_position_ << " target price: " << target_price_
                           << " best nonself bid" << best_nonself_bid_price_ << " best nonself ask "
                           << best_nonself_ask_price_ << " " << DBGLOG_ENDL_FLUSH;
    ShowParams();
  }

  int t_position_to_close_ = GetPositionToClose();

  if (t_position_to_close_ == 0) {  // nothing to be done, cancel all remaining orders
    order_manager_.CancelAllOrders();
  } else {
    // PrintFullStatus();
    double spread_threshold_adjustment_ = param_set_.spread_increase_ * (dep_market_view_.spread_increments() - 1);

    if (t_position_to_close_ > 0) {  // long hence cancel all bid orders
      order_manager_.CancelAllBidOrders();

      // TODO : check if worst_pos is needed
      if (order_manager_.SumAskSizes() > my_position_ + param_set_.worst_case_position_) {
        // shouldn't be very frequent as we cancel all bids/asks frequently
        // just to make some space in rare cases
        order_manager_.CancelAsksFromFar(param_set_.unit_trade_size_);
        // we wont be placing order greater than uts in a single round
      }

      bool done_for_this_round_ = false;
      // target_price_ will be just mkt_price_ with no signal added
      // subtrsacting spread_adj because when spread>1, ask-tgt would increase by 0.5 tick atleast
      double askside_bias_ = best_nonself_ask_price_ - target_price_ - spread_threshold_adjustment_;

      // agg after proper checks
      if ((param_set_.allowed_to_aggress_) &&
          (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_cross_) &&
          (best_nonself_ask_size_ < param_set_.max_size_to_aggress_) &&
          (best_nonself_bid_price_ - target_price_ >=
           current_tradevarset_.l1ask_aggressive_ + spread_threshold_adjustment_)) {
        int trade_size_to_place_ = MathUtils::GetFlooredMultipleOf(
            std::min(t_position_to_close_, param_set_.unit_trade_size_), dep_market_view_.min_order_size());

        int t_size_already_placed_ = order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) +
                                     order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_);

        if (trade_size_to_place_ > t_size_already_placed_) {
          // we need to agress
          order_manager_.SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                   trade_size_to_place_ - t_size_already_placed_, kTradeTypeSell, 'A');
          done_for_this_round_ = true;

          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "SendTrade S of " << trade_size_to_place_ - t_size_already_placed_ << " @ "
                                   << best_nonself_bid_price_ << " IntPx: " << best_nonself_bid_int_price_
                                   << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                   << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }

      if (!done_for_this_round_ && (askside_bias_ >= current_tradevarset_.l1ask_place_)) {
        // we should place at best
        int trade_size_to_place_ = MathUtils::GetFlooredMultipleOf(
            std::min(t_position_to_close_, param_set_.unit_trade_size_), dep_market_view_.min_order_size());

        if (askside_bias_ < current_tradevarset_.l1ask_improve_keep_) {
          // if signal is not high enough, then cancel improve trades
          // we can also have a keep threshold like the ones for canceling best orders
          order_manager_.CancelAsksAboveIntPrice(best_nonself_ask_int_price_);
        }

        // size already placed at best or above
        int t_size_already_placed_ = order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) +
                                     order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_);

        if (trade_size_to_place_ > t_size_already_placed_) {  // if the required size is not present then add more
          order_manager_.SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                   trade_size_to_place_ - t_size_already_placed_, kTradeTypeSell, 'B');
          done_for_this_round_ = true;

          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "SendTrade S of " << trade_size_to_place_ - t_size_already_placed_ << " @ "
                                   << best_nonself_ask_price_ << " IntPx: " << best_nonself_ask_int_price_
                                   << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                   << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
        // not doing anything if the placed size is more than required
      }

      if (!done_for_this_round_ && (askside_bias_ < current_tradevarset_.l1ask_place_)) {
        if (askside_bias_ < current_tradevarset_.l1ask_keep_) {
          // cancel order at or above best levels
          order_manager_.CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_);
        }

        int trade_size_to_place_ = MathUtils::GetFlooredMultipleOf(
            std::min(t_position_to_close_, param_set_.unit_trade_size_), dep_market_view_.min_order_size());

        // size already placed at or above (best_nonself_ask_int_price_ + 1)
        int t_size_already_placed_ =
            order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_ + 1) +
            order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_ + 1);

        if (trade_size_to_place_ > t_size_already_placed_) {
          order_manager_.SendTradeIntPx(best_nonself_ask_int_price_ + 1, trade_size_to_place_ - t_size_already_placed_,
                                        kTradeTypeSell, 'B');
          done_for_this_round_ = true;

          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "SendTrade S of " << trade_size_to_place_ - t_size_already_placed_ << " @ "
                                   << best_nonself_ask_price_ + dep_market_view_.min_price_increment()
                                   << " IntPx: " << best_nonself_ask_int_price_ + 1
                                   << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                   << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    } else {  // t_position_to_close_ < 0
      // short hence cancel all sell orders
      order_manager_.CancelAllAskOrders();

      // TODO : check if worst_pos if needed
      if (order_manager_.SumBidSizes() > -my_position_ + param_set_.worst_case_position_) {
        // shouldn't be very frequent as we cancel all bids/asks frequently
        // just to make some space in rare cases
        order_manager_.CancelBidsFromFar(param_set_.unit_trade_size_);
        // we wont be placing order greater than uts in a single round
      }

      bool done_for_this_round_ = false;

      double bidside_bias_ = target_price_ - best_nonself_bid_price_ - spread_threshold_adjustment_;

      // agg after proper checks
      if ((param_set_.allowed_to_aggress_) &&
          (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_cross_) &&
          (best_nonself_bid_size_ < param_set_.max_size_to_aggress_) &&
          (target_price_ - best_nonself_ask_price_ >=
           current_tradevarset_.l1bid_aggressive_ + spread_threshold_adjustment_)) {
        int trade_size_to_place_ = MathUtils::GetFlooredMultipleOf(
            std::min(-t_position_to_close_, param_set_.unit_trade_size_), dep_market_view_.min_order_size());
        // size already agressed
        int t_size_already_placed_ = order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) +
                                     order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_);

        if (trade_size_to_place_ > t_size_already_placed_) {
          order_manager_.SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                   trade_size_to_place_ - t_size_already_placed_, kTradeTypeBuy, 'A');
          done_for_this_round_ = true;

          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "GetFlatSendTrade B of " << trade_size_to_place_ - t_size_already_placed_ << " @ "
                                   << best_nonself_ask_price_ << " IntPx: " << best_nonself_ask_int_price_
                                   << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                   << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }

      // checking for stable markets
      if (!done_for_this_round_ && (bidside_bias_ >= current_tradevarset_.l1bid_place_)) {
        // we should place at best
        int trade_size_to_place_ = MathUtils::GetFlooredMultipleOf(
            std::min(-t_position_to_close_, param_set_.unit_trade_size_), dep_market_view_.min_order_size());

        if (bidside_bias_ < current_tradevarset_.l1bid_improve_keep_) {
          // if signal is not high enough, then cancel improve trades
          // we can also have a keep threshold like the ones for canceling best orders
          order_manager_.CancelBidsAboveIntPrice(best_nonself_bid_int_price_);
        }

        // size already placed at best or above
        int t_size_already_placed_ = order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) +
                                     order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_);

        if (trade_size_to_place_ > t_size_already_placed_) {  // if the required size is not present then add more
          order_manager_.SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                   trade_size_to_place_ - t_size_already_placed_, kTradeTypeBuy, 'B');
          done_for_this_round_ = true;

          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "GetFlatSendTrade B of " << trade_size_to_place_ - t_size_already_placed_ << " @ "
                                   << best_nonself_bid_price_ << " IntPx: " << best_nonself_bid_int_price_
                                   << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                   << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
        // not doing anything if the placed size is more than required
      }

      if (!done_for_this_round_ && (bidside_bias_ < current_tradevarset_.l1bid_place_)) {
        if (bidside_bias_ < current_tradevarset_.l1bid_keep_) {
          // cancel order at or above best levels
          order_manager_.CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_);
        }
        int trade_size_to_place_ = MathUtils::GetFlooredMultipleOf(
            std::min(-t_position_to_close_, param_set_.unit_trade_size_), dep_market_view_.min_order_size());
        // size already placed at or above (best_nonself_bid_int_price_ - 1)
        int t_size_already_placed_ =
            order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_ - 1) +
            order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_ - 1);

        if (trade_size_to_place_ > t_size_already_placed_) {
          order_manager_.SendTradeIntPx(best_nonself_bid_int_price_ - 1, trade_size_to_place_ - t_size_already_placed_,
                                        kTradeTypeBuy, 'B');
          done_for_this_round_ = true;

          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "GetFlatSendTrade B of " << trade_size_to_place_ - t_size_already_placed_ << " @ "
                                   << best_nonself_bid_price_ - dep_market_view_.min_price_increment()
                                   << " IntPx: " << best_nonself_bid_int_price_ - 1
                                   << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                   << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
  }
}

void PriceBasedLegTrading::PrintFullStatus() {
  DBGLOG_TIME_CLASS_FUNC_LINE << "tgt: " << target_price_ << " mkt " << best_nonself_bid_price_ << " X "
                              << best_nonself_bid_size_ << " pft: " << (target_price_ - best_nonself_bid_price_)
                              << " -- " << best_nonself_ask_price_ << " X " << best_nonself_ask_size_
                              << " pft: " << (best_nonself_ask_price_ - target_price_)
                              << " bias: " << ((target_price_ - dep_market_view_.mkt_size_weighted_price()) /
                                               dep_market_view_.min_price_increment()) << " pos: " << my_position_
                              << " ideal_spd_pos: " << ideal_spread_position_ << " smaps "
                              << order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_) << " "
                              << order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) << " "
                              << order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) << " "
                              << order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_)
                              << " opnl: " << order_manager_.base_pnl().opentrade_unrealized_pnl()
                              << " gpos: " << my_global_position_ << "\n" << ToString(current_tradevarset_)
                              << DBGLOG_ENDL_FLUSH;
}

void PriceBasedLegTrading::OnPositionChange(int t_new_position_, int position_diff_, const unsigned int _security_id_) {
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    // debug info ... Only in LiveTrading
    if (t_new_position_ != my_position_) {
      dump_inds = true;
    }
  }

  my_position_ = t_new_position_;

  TradeVarSetLogic(my_position_);  ///< since position changed the applicable tradevarset might have changed

  if (!livetrading_) {  // we were getting control here before the signal was updated
    // hence only doing getflat here in sim

    // see if trades need to be placed
    if ((!external_freeze_trading_) && (!freeze_due_to_exchange_stage_)) {
      ProcessGetFlat();  // TODO ... see if this needs to be here ... I mean not much changes in get flat conditions
                         // when the signal is updating
    }
  } else {
    // commented out temporarily ... if the nonself book has changed becasue of our own orders then we will get a
    // notification soon
    // NonSelfMarketUpdate ( ) ; ///< since orderrouting update, hence our own orders in the top levels might have
    // changed enough to move the non self level markers

    // see if trades need to be placed
    if ((!external_freeze_trading_) && (!freeze_due_to_exchange_stage_) && (!freeze_due_to_funds_reject_)) {
      ProcessGetFlat();  // TODO ... see if this needs to be here ... I mean not much changes in get flat conditions
                         // when the signal is updating

      if (!should_be_getting_flat_) {
        //              TradingLogic ( ) ;

        // skipping CallPlaceCancelNonBestLevels () and calling directly
        last_non_best_level_om_msecs_ = watch_.msecs_from_midnight();
        PlaceCancelNonBestLevels();
      } else {
        GetFlatTradingLogic();
        if (livetrading_ || (dbglogger_.CheckLoggingLevel(TRADING_INFO))) {
          LogFullStatus();
        }
      }
    }
  }
}

void PriceBasedLegTrading::TradeVarSetLogic(int t_position) {
  current_position_tradevarset_map_index_ = P2TV_zero_idx_;
  int t_position_to_close_ = GetPositionToClose();
  if (map_pos_increment_ > 1)  // only when param_set_.max_position_ > MAX_POS_MAP_SIZE
  {
    if (t_position_to_close_ > 0) {
      current_position_tradevarset_map_index_ +=
          std::min(MAX_POS_MAP_SIZE, (abs(t_position_to_close_) / map_pos_increment_));
    } else {
      current_position_tradevarset_map_index_ -=
          std::min(MAX_POS_MAP_SIZE, (abs(t_position_to_close_) / map_pos_increment_));
    }
  } else {
    if (t_position_to_close_ > 0) {
      current_position_tradevarset_map_index_ += std::min(MAX_POS_MAP_SIZE, abs(t_position_to_close_));
    } else {
      current_position_tradevarset_map_index_ -= std::min(MAX_POS_MAP_SIZE, abs(t_position_to_close_));
    }
  }

  current_tradevarset_ = position_tradevarset_map_[current_position_tradevarset_map_index_];

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << " newpos: " << my_position_ << " t_position_to_close_: " << t_position_to_close_
                           << " gpos: " << my_global_position_ << " stddpfac " << stdev_scaled_capped_in_ticks_
                           << " mapidx " << current_position_tradevarset_map_index_ << ' '
                           << ToString(current_tradevarset_).c_str() << DBGLOG_ENDL_FLUSH;
  }
}

void PriceBasedLegTrading::OnPositionUpdate(int _ideal_spread_position_) {
  ideal_spread_position_ =
      HFSAT::MathUtils::GetFlooredMultipleOf(_ideal_spread_position_, dep_market_view_.min_order_size_);
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << "my_position_: " << my_position_
                                << " ideal_spread_position_: " << ideal_spread_position_ << DBGLOG_ENDL_FLUSH;
  }
  TradeVarSetLogic(my_position_);
  UpdateTarget(target_price_, targetbias_numbers_);
}
} /* namespace HFSAT */
