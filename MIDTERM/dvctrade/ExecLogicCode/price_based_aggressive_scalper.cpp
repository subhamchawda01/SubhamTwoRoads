/**
   \file ExecLogicCode/price_based_aggressive_scalper.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/ExecLogic/price_based_aggressive_scalper.hpp"
// exec_logic_code / defines.hpp was empty

namespace HFSAT {

PriceBasedAggressiveScalper::PriceBasedAggressiveScalper(
    DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
    SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
    MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
    MarketUpdateManager& t_market_update_manager_, const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_,
    const int t_runtime_id_, const std::vector<std::string> _this_model_source_shortcode_vec_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_) {
  // forcing symm scaling for scalper execlogics
  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    param_set_vec_[i].increase_thresholds_symm_ = true;
  }
  param_set_.increase_thresholds_symm_ = true;
}

void PriceBasedAggressiveScalper::TradingLogic() {
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    dbglogger_ << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__ << " tgt: " << target_price_
               << " ExpBidPft@ " << best_nonself_bid_price_ << ' ' << (target_price_ - best_nonself_bid_price_)
               << " ExpAskPft@ " << best_nonself_ask_price_ << ' ' << (best_nonself_ask_price_ - target_price_)
               << " signalbias: " << ((target_price_ - dep_market_view_.mkt_size_weighted_price()) /
                                      dep_market_view_.min_price_increment())
               << " pos: " << my_position_ << '\n';
    dbglogger_ << " best_nonself bid " << best_nonself_bid_price_ << " l1 bid size "
               << current_tradevarset_.l1bid_trade_size_ << " bid place " << current_tradevarset_.l1bid_place_ << '\n';
    dbglogger_ << " best nonself ask " << best_nonself_ask_price_ << " l1 ask size "
               << current_tradevarset_.l1ask_trade_size_ << " ask place " << current_tradevarset_.l1ask_place_ << '\n';
    dbglogger_.CheckToFlushBuffer();
  }

  // if ( order_manager_.base_pnl ( ) .opentrade_unrealized_pnl ( ) > 0 )
  //   {
  // 	dbglogger_ << "target_price_ = " << target_price_;
  //   }

  /// analogous to stoploss values
  if (order_manager_.base_pnl().opentrade_unrealized_pnl() > 0) {
    if (my_position_ > 0) {
      target_price_ -=
          param_set_.pclose_factor_ * (order_manager_.base_pnl().opentrade_unrealized_pnl() / abs(my_position_));
    } else if (my_position_ < 0) {
      target_price_ +=
          param_set_.pclose_factor_ * (order_manager_.base_pnl().opentrade_unrealized_pnl() / abs(my_position_));
    }
  }

  // if ( order_manager_.base_pnl ( ) .opentrade_unrealized_pnl ( ) > 0 )
  //   {
  // 	dbglogger_ << " pclose-adjusted target_price_ = " << target_price_ << "\n";
  // 	dbglogger_.CheckToFlushBuffer ( );
  //   }

  // setting top level directives
  top_bid_place_ = false;
  top_bid_keep_ = false;
  top_bid_improve_ = false;
  top_ask_lift_ = false;
  bid_improve_keep_ = false;

  // if based on current risk level or if trading is stopped ... check if we have any allowance to place orders at top
  // level
  if (current_tradevarset_.l1bid_trade_size_ >
      0) {  // check if the margin of buying at the price ( best_nonself_bid_price_ )
    // i.e. ( target_price_ - best_nonself_bid_price_ )
    // exceeds the threshold current_tradevarset_.l1bid_place_
    if (target_price_ - best_nonself_bid_price_ >= current_tradevarset_.l1bid_place_ &&
        (watch_.msecs_from_midnight() - last_buy_msecs_ > param_set_.cooloff_interval_) &&
        (!param_set_.enforce_min_cooloff_ || (watch_.msecs_from_midnight() - last_exec_msecs_ > 10)) &&
        (!dep_market_view_.trade_update_implied_quote())) {
      top_bid_place_ = true;
      top_bid_keep_ = true;
    } else {  // signal is not strong enough for placing bids at best_nonself_bid_price_
      // check if we should retain exisiting bids due to place_in_line

      // TODO : perhaps we should make place_in_line effect bestbid_queue_hysterisis_ smarter
      //        for instance when short term volatility in the market is very high
      //        then being high in the queue should count for less.
      if ((target_price_ - best_nonself_bid_price_ + bestbid_queue_hysterisis_) >= current_tradevarset_.l1bid_keep_) {
        top_bid_keep_ = true;
      } else {
        top_bid_keep_ = false;
      }
    }
    if (((dep_market_view_.spread_increments() > 1) &&
         (target_price_ - best_nonself_ask_price_) >= current_tradevarset_.l1bid_improve_keep_)) {
      bid_improve_keep_ = true;
    } else {
      bid_improve_keep_ = false;
    }
  }

  /// adjust bid_keep and bid_place based on open trade interval and size at level
  if (best_nonself_bid_size_ >= param_set_.safe_cancel_size_) {
    top_bid_keep_ = true;
  }

  if (watch_.msecs_from_midnight() - last_flip_msecs_ > param_set_.high_time_limit_ && my_position_ < 0) {
    top_bid_keep_ = true;
    top_bid_place_ = true;
  }

  if (watch_.msecs_from_midnight() - last_flip_msecs_ > param_set_.moderate_time_limit_ && my_position_ > 0) {
    top_bid_keep_ = false;
    top_bid_place_ = false;
  }

  top_ask_place_ = false;
  top_ask_keep_ = false;
  top_ask_improve_ = false;
  top_bid_hit_ = false;
  ask_improve_keep_ = false;
  // if based on current risk level or if trading is stopped ... check if we have any allowance to place orders at top
  // level
  if (current_tradevarset_.l1ask_trade_size_ >
      0) {  // check if the margin of placing limit ask orders at the price ( best_nonself_ask_price_ )
    // i.e. ( best_nonself_ask_price_ - target_price_ )
    //      exceeds the threshold current_tradevarset_.l1ask_place_
    if (best_nonself_ask_price_ - target_price_ >= current_tradevarset_.l1ask_place_ &&
        (watch_.msecs_from_midnight() - last_sell_msecs_ > param_set_.cooloff_interval_) &&
        (!param_set_.enforce_min_cooloff_ || (watch_.msecs_from_midnight() - last_exec_msecs_ > 10)) &&
        (!dep_market_view_.trade_update_implied_quote()))  ///-1 for consistency with earlier semantics
    {
      top_ask_place_ = true;
      top_ask_keep_ = true;
    } else {
      // signal not strog enough to place limit orders at the best ask level
      if ((best_nonself_ask_price_ - target_price_ + bestask_queue_hysterisis_) >=
          current_tradevarset_.l1ask_keep_) {  // but with place in line effect enough to keep the live order there
        top_ask_keep_ = true;
      } else {
        top_ask_keep_ = false;
      }
    }

    if (((dep_market_view_.spread_increments() > 1) &&
         (best_nonself_ask_price_ - target_price_) >= current_tradevarset_.l1ask_improve_keep_)) {
      ask_improve_keep_ = true;
    } else {
      ask_improve_keep_ = false;
    }
  }

  /// adjust bid_keep and bid_place based on open trade interval and size at level
  if (best_nonself_ask_size_ >= param_set_.safe_cancel_size_) {
    top_ask_keep_ = true;
  }

  if (watch_.msecs_from_midnight() - last_flip_msecs_ > param_set_.high_time_limit_ && my_position_ > 0) {
    top_ask_keep_ = true;
    top_ask_place_ = true;
  }

  if (watch_.msecs_from_midnight() - last_flip_msecs_ > param_set_.moderate_time_limit_ && my_position_ < 0) {
    top_ask_keep_ = false;
    top_ask_place_ = false;
  }

  // After setting top-level directives ...

  // Active BID order management
  placed_bids_this_round_ = false;
  if (!placed_bids_this_round_) {  // only come here if no aggressive or improve bid orders sent this cycle
    if (top_bid_place_) {
      // only place best level Bid orders when there is no active unconfirmed order at or above
      // best_nonself_bid_int_price_
      //      and no confirmed order at or above the best price
      if ((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_) == 0) &&
          (order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) == 0)) {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          dbglogger_ << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__ << " SendTrade B of "
                     << current_tradevarset_.l1bid_trade_size_ << " @ " << best_nonself_bid_price_
                     << " ebp_t: " << (target_price_ - best_nonself_bid_price_) / dep_market_view_.min_price_increment()
                     << " thresh_t: " << current_tradevarset_.l1bid_place_ / dep_market_view_.min_price_increment()
                     << " Int Px: " << best_nonself_bid_int_price_ << " Mkt: " << best_nonself_bid_size_ << " @ "
                     << best_nonself_bid_price_ << "  ---  " << best_nonself_ask_price_ << " @ "
                     << best_nonself_ask_size_ << " Stdev: " << stdev_scaled_capped_in_ticks_ << '\n';
          dbglogger_.CheckToFlushBuffer();
        }
        order_manager_.SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                 current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'B');
        placed_bids_this_round_ = true;
      } else {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          dbglogger_ << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__
                     << " Not sending trade despite signal because order already exists "
                     << " BidUnconfirmedEqAbove " << best_nonself_bid_int_price_ << " is "
                     << order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_)
                     << " BidConfirmedAbove " << best_nonself_bid_int_price_ << " is "
                     << order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) << '\n';
          dbglogger_.CheckToFlushBuffer();
        }
      }
    } else {
      if (!top_bid_keep_)  // && order_manager_.intpx_2_sum_bid_confirmed( best_nonself_bid_int_price_ ) > 0 )
      {                    // cancel all orders at best_nonself_bid_price_
        int cancelled_size_ = order_manager_.CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_);
        if (cancelled_size_ > 0) {
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            dbglogger_ << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__
                       << " CancelTrade B EqAbove: " << best_nonself_bid_price_ << " ebp_t: "
                       << (target_price_ - best_nonself_bid_price_ + bestbid_queue_hysterisis_) /
                              dep_market_view_.min_price_increment()
                       << " thresh_t: " << current_tradevarset_.l1bid_keep_ / dep_market_view_.min_price_increment()
                       << " tMktSz: " << best_nonself_bid_size_ << '\n';
            dbglogger_.CheckToFlushBuffer();
          }
        }
      } else {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          dbglogger_ << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__
                     << " not cancelling because top_bid_keep = " << (top_bid_keep_ ? 'Y' : 'N') << " at price  "
                     << best_nonself_bid_int_price_ << '\n';
          dbglogger_.CheckToFlushBuffer();
        }
      }
    }

    if ((dep_market_view_.spread_increments() > 1) && (!bid_improve_keep_)) {
      int cancelled_size_ = order_manager_.CancelBidsAboveIntPrice(best_nonself_bid_int_price_);
      if (cancelled_size_ > 0) {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " Cancelled Improve Bids above priceL " << best_nonself_bid_int_price_ << " ebp_t: "
                                 << (target_price_ - best_nonself_bid_price_ + bestbid_queue_hysterisis_) /
                                        dep_market_view_.min_price_increment()
                                 << " thresh_t: "
                                 << current_tradevarset_.l1bid_keep_ / dep_market_view_.min_price_increment()
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  // Active ASK order management
  placed_asks_this_round_ = false;
  if (!placed_asks_this_round_) {
    if (top_ask_place_) {
      // only place Ask orders when there is no active unconfirmed order at or above best_nonself_ask_int_price_
      //      and no confirmed order at or above the best price
      if ((order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_) == 0) &&
          (order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) == 0)) {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          dbglogger_ << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__ << " SendTrade S of "
                     << current_tradevarset_.l1ask_trade_size_ << " @ " << best_nonself_ask_price_
                     << " eap_t: " << (best_nonself_ask_price_ - target_price_) / dep_market_view_.min_price_increment()
                     << " thresh_t: " << current_tradevarset_.l1ask_place_ / dep_market_view_.min_price_increment()
                     << " Int Px: " << best_nonself_ask_int_price_ << " Mkt: " << best_nonself_bid_size_ << " @ "
                     << best_nonself_bid_price_ << " ---- " << best_nonself_ask_price_ << " @ "
                     << best_nonself_ask_size_ << " Stdev: " << stdev_scaled_capped_in_ticks_ << '\n';
          dbglogger_.CheckToFlushBuffer();
        }
        order_manager_.SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                 current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'B');
        placed_asks_this_round_ = true;
      } else {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          dbglogger_ << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__
                     << " Not sending trade despite signal because order already exists "
                     << " AskUnconfirmedEqAbove " << best_nonself_ask_int_price_ << " is "
                     << order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_)
                     << " AskConfirmedAbove " << best_nonself_ask_int_price_ << " is "
                     << order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) << '\n';
          dbglogger_.CheckToFlushBuffer();
        }
      }
    } else {
      if (!top_ask_keep_)  // && order_manager_.intpx_2_sum_ask_confirmed( best_nonself_ask_int_price_ ) > 0 )
      {                    // cancel all orders at best_nonself_ask_price_
        int cancelled_size_ = order_manager_.CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_);
        if (cancelled_size_ > 0) {
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            dbglogger_ << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__ << " CancelTrade S EqAbove "
                       << best_nonself_ask_price_ << " eap_t: "
                       << (best_nonself_ask_price_ - target_price_ + bestask_queue_hysterisis_) /
                              dep_market_view_.min_price_increment()
                       << " thresh_t: " << current_tradevarset_.l1ask_keep_ / dep_market_view_.min_price_increment()
                       << " tMktSz: " << best_nonself_ask_size_ << '\n';
            dbglogger_.CheckToFlushBuffer();
          }
        }
      } else {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          dbglogger_ << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__
                     << " not cancelling because top_ask_keep = " << (top_ask_keep_ ? 'Y' : 'N') << " at price  "
                     << best_nonself_ask_int_price_ << '\n';
          dbglogger_.CheckToFlushBuffer();
        }
      }
    }

    if ((dep_market_view_.spread_increments() > 1) && (!ask_improve_keep_)) {
      int cancelled_size_ = order_manager_.CancelAsksAboveIntPrice(best_nonself_ask_int_price_);
      if (cancelled_size_ > 0) {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " Cancelled Improve Asks above price: " << best_nonself_ask_int_price_
                                 << " Size: " << cancelled_size_ << " eap_t: "
                                 << (best_nonself_ask_price_ - target_price_ + bestask_queue_hysterisis_) /
                                        dep_market_view_.min_price_increment()
                                 << " thresh_t: "
                                 << current_tradevarset_.l1ask_improve_keep_ / dep_market_view_.min_price_increment()
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  //    if ( ( ! placed_bids_this_round_ ) && ( ! placed_asks_this_round_ ) )
  {
    if (non_best_level_order_management_counter_ >= 5)  // NON_BEST_LEVEL_ORDER_MANAGEMENT_FREQUENCY )
    {
      non_best_level_order_management_counter_ = 0;
      // non best level order management
      PlaceCancelNonBestLevels();
    } else {
      non_best_level_order_management_counter_++;
    }
  }
}

void PriceBasedAggressiveScalper::PrintFullStatus() {
  DBGLOG_TIME << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__ << " tgt: " << target_price_
              << " ExpBidPft@ " << best_nonself_bid_price_ << ' ' << (target_price_ - best_nonself_bid_price_)
              << " ExpAskPft@ " << best_nonself_ask_price_ << ' ' << (best_nonself_ask_price_ - target_price_)
              << " signalbias: "
              << ((target_price_ - dep_market_view_.mkt_size_weighted_price()) / dep_market_view_.min_price_increment())
              << " pos: " << my_position_ << '\n';

  dbglogger_ << " best_nonself bid " << best_nonself_bid_price_ << " l1 bid size "
             << current_tradevarset_.l1bid_trade_size_ << " bid place " << current_tradevarset_.l1bid_place_ << '\n';
  dbglogger_ << " best nonself ask " << best_nonself_ask_price_ << " l1 ask size "
             << current_tradevarset_.l1ask_trade_size_ << " ask place " << current_tradevarset_.l1ask_place_ << '\n';
  dbglogger_.CheckToFlushBuffer();
}
}
