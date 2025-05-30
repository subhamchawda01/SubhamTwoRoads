/**
    \file ExecLogicCode/trade_based_agrressive_trading.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/ExecLogic/trade_based_aggressive_trading.hpp"
// exec_logic_code / defines.hpp was empty

namespace HFSAT {

TradeBasedAggressiveTrading::TradeBasedAggressiveTrading(
    DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
    SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
    MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
    const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int t_runtime_id_,
    const std::vector<std::string> _this_model_source_shortcode_vec_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_) {
  _dep_market_view_.ComputeTradeWPrice();
}

void TradeBasedAggressiveTrading::TradingLogic() {
  // setting top level directives
  top_bid_place_ = false;
  top_bid_keep_ = false;
  top_bid_improve_ = false;
  top_ask_lift_ = false;
  // if based on current risk level or if trading is stopped ... check if we have any allowance to place orders at top
  // level
  if (current_tradevarset_.l1bid_trade_size_ > 0) {
    // first check for cooloff_interval
    if ((last_buy_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_)) {
      // check if the margin of buying
      // i.e. ( targetbias_numbers_ )
      // exceeds the threshold current_tradevarset_.l1bid_place_
      if ((targetbias_numbers_ >= current_tradevarset_.l1bid_place_) &&
          (param_set_.place_on_trade_update_implied_quote_ || !dep_market_view_.trade_update_implied_quote())) {
        top_bid_place_ = true;
        top_bid_keep_ = true;
      } else {  // signal is not strong enough for placing bids at trade_price - 1
        // check if we should retain exisiting bids due to place_in_line
        if ((targetbias_numbers_ >= current_tradevarset_.l1bid_keep_)) {
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

  if (current_tradevarset_.l1ask_trade_size_ > 0) {
    // first check for cooloff_interval
    if ((last_sell_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_)) {
      if ((-targetbias_numbers_ >= current_tradevarset_.l1ask_place_) &&
          (param_set_.place_on_trade_update_implied_quote_ || !dep_market_view_.trade_update_implied_quote())) {
        top_ask_place_ = true;
        top_ask_keep_ = true;
      } else {
        // signal not strog enough to place limit orders at the best ask level
        if ((-targetbias_numbers_ >=
             current_tradevarset_.l1ask_keep_)) {  // but with place in line effect enough to keep the live order there
          top_ask_keep_ = true;
        } else {
          top_ask_keep_ = false;
        }
      }
    }
  }

  // After setting top-level directives ...
  // get to order placing or canceling part

  double best_bid_tradew_price_ =
      dep_market_view_.GetPriceFromType(HFSAT::kPriceTypeTradeWPrice, dep_market_view_.market_update_info_) -
      dep_market_view_.min_price_increment();
  double best_ask_tradew_price_ =
      dep_market_view_.GetPriceFromType(HFSAT::kPriceTypeTradeWPrice, dep_market_view_.market_update_info_) +
      dep_market_view_.min_price_increment();

  int best_bid_tradew_int_price_ = dep_market_view_.GetIntPx(best_bid_tradew_price_);
  int best_ask_tradew_int_price_ = dep_market_view_.GetIntPx(best_ask_tradew_price_);

  {  // only come here if no aggressive or improve bid orders sent this cycle
    if (top_bid_place_) {
      // only place Bid orders when there is no active unconfirmed order and no confirmed order at or above the best
      // price
      if ((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_bid_tradew_int_price_) == 0) &&
          (order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_bid_tradew_int_price_) == 0)) {
        order_manager_.SendTrade(best_bid_tradew_price_, best_bid_tradew_int_price_,
                                 current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'B');
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "SendTrade B of " << current_tradevarset_.l1bid_trade_size_ << " @ "
                                 << best_bid_tradew_int_price_
                                 << " tgt_bias: " << targetbias_numbers_ / dep_market_view_.min_price_increment()
                                 << " thresh_t: "
                                 << current_tradevarset_.l1bid_place_ / dep_market_view_.min_price_increment()
                                 << " Int Px: " << best_nonself_bid_int_price_ << " tMktSz: " << best_nonself_bid_size_
                                 << " Mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << "  ---  "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
        }
      }
    } else {
      if (!top_bid_keep_) {  // cancel all orders at best_bid_tradew_int_price_
        int canceled_size_ = order_manager_.CancelBidsEqAboveIntPrice(best_bid_tradew_int_price_);
        if (canceled_size_ > 0) {
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
          {
            DBGLOG_TIME << "Canceled B of " << canceled_size_ << " @ " << best_bid_tradew_int_price_
                        << " tgt_bias: " << targetbias_numbers_ / dep_market_view_.min_price_increment()
                        << " thresh_t: " << current_tradevarset_.l1bid_keep_ / dep_market_view_.min_price_increment()
                        << " tMktSz: " << best_nonself_bid_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
  }

  {  // only come here if no aggressive or improve ask orders sent this cycle
    if (top_ask_place_) {
      // only place Ask orders when there is no active unconfirmed order and no confirmed order at or above the best
      // price
      if ((order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_ask_tradew_int_price_) == 0) &&
          (order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_ask_tradew_int_price_) == 0)) {
        order_manager_.SendTrade(best_ask_tradew_price_, best_ask_tradew_int_price_,
                                 current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'B');
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "SendTrade S of " << current_tradevarset_.l1ask_trade_size_ << " @ "
                                 << best_ask_tradew_int_price_
                                 << " tgt_bias: " << targetbias_numbers_ / dep_market_view_.min_price_increment()
                                 << " thresh_t: "
                                 << current_tradevarset_.l1ask_place_ / dep_market_view_.min_price_increment()
                                 << " Int Px: " << best_nonself_ask_int_price_ << " tMktSz: " << best_nonself_ask_size_
                                 << " Mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_ask_price_ << "  ---  "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
        }
      }
    } else {
      if (!top_ask_keep_) {  // cancel all orders at best_ask_tradew_int_price_
        int canceled_size_ = order_manager_.CancelAsksEqAboveIntPrice(best_ask_tradew_int_price_);
        if (canceled_size_ > 0) {
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
          {
            DBGLOG_TIME << "Canceled B of " << canceled_size_ << " @ " << best_ask_tradew_int_price_
                        << " tgt_bias: " << targetbias_numbers_ / dep_market_view_.min_price_increment()
                        << " thresh_t: " << current_tradevarset_.l1ask_keep_ / dep_market_view_.min_price_increment()
                        << " tMktSz: " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
  }

  if (!top_ask_keep_) {  // No best ask , place a non-best ask
    if ((order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_ask_tradew_int_price_ + 1) == 0) &&
        (order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_ask_tradew_int_price_ + 1) == 0)) {
      order_manager_.SendTrade(best_ask_tradew_price_ + dep_market_view_.min_price_increment(),
                               best_ask_tradew_int_price_ + 1, current_tradevarset_.l1ask_trade_size_, kTradeTypeSell,
                               'S');
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
      {
        DBGLOG_TIME_CLASS_FUNC << "SendTrade S of " << current_tradevarset_.l1ask_trade_size_ << " @ "
                               << (best_ask_tradew_int_price_ + 1)
                               << " tgt_bias: " << targetbias_numbers_ / dep_market_view_.min_price_increment()
                               << " thresh_t: "
                               << current_tradevarset_.l1ask_place_ / dep_market_view_.min_price_increment()
                               << " Int Px: " << best_nonself_ask_int_price_ << " tMktSz: " << best_nonself_ask_size_
                               << " Mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_ask_price_ << "  ---  "
                               << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
      }
    }
  }
  if (!top_bid_keep_) {  // No best bid , place a non-best bid
    if ((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_bid_tradew_int_price_ - 1) == 0) &&
        (order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_bid_tradew_int_price_ - 1) == 0)) {
      order_manager_.SendTrade(best_bid_tradew_price_ - dep_market_view_.min_price_increment(),
                               best_bid_tradew_int_price_ - 1, current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy,
                               'S');
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
      {
        DBGLOG_TIME_CLASS_FUNC << "SendTrade B of " << current_tradevarset_.l1bid_trade_size_ << " @ "
                               << (best_bid_tradew_int_price_ - 1)
                               << " tgt_bias: " << targetbias_numbers_ / dep_market_view_.min_price_increment()
                               << " thresh_t: "
                               << current_tradevarset_.l1bid_place_ / dep_market_view_.min_price_increment()
                               << " Int Px: " << best_nonself_bid_int_price_ << " tMktSz: " << best_nonself_bid_size_
                               << " Mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << "  ---  "
                               << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
      }
    }
  }
}

void TradeBasedAggressiveTrading::PrintFullStatus() {
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
