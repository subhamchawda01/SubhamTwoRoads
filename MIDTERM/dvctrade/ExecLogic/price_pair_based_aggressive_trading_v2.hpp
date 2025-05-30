/**
    \file ExecLogic/price_pair_based_aggressive_trading_v2.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_EXECLOGIC_PRICE_PAIR_BASED_AGGRESSIVE_TRADING_V2_H
#define BASE_EXECLOGIC_PRICE_PAIR_BASED_AGGRESSIVE_TRADING_V2_H

#include "dvctrade/ExecLogic/base_trading.hpp"

namespace HFSAT {

/// Class to trade with a target price that is very dependant on top level nonself marketprice
///
/// the base price moves sharply with market price and so does the target price
/// the targetprice is thus compared to bid price and ask price ( for getting an opinion to place trades )
/// and not midprice or a price that is much more stable than market weighted price

class PricePairBasedAggressiveTradingV2 : public virtual BaseTrading {
 public:
  static void CollectORSShortCodes(DebugLogger& _dbglogger_, const std::string& r_dep_shortcode_,
                                   std::vector<std::string>& source_shortcode_vec_,
                                   std::vector<std::string>& ors_source_needed_vec_);

 protected:
 public:
  PricePairBasedAggressiveTradingV2(DebugLogger& _dbglogger_, const Watch& _watch_,
                                    const SecurityMarketView& _dep_market_view_,
                                    const SecurityMarketView& _indep_market_view_, SmartOrderManager& _order_manager_,
                                    const std::string& _paramfilename_, const bool _livetrading_,
                                    MulticastSenderSocket* _p_strategy_param_sender_socket_,
                                    EconomicEventsManager& t_economic_events_manager_,
                                    const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_,
                                    const int t_runtime_id_,
                                    const std::vector<std::string> _this_model_source_shortcode_vec_);

  ~PricePairBasedAggressiveTradingV2(){};  ///< destructor not made virtual ... please do so when making child classes

  static std::string StrategyName() {
    return "PricePairBasedAggressiveTradingV2";
  }  ///< used in trade_init to check which strategy is to be initialized

  void OnMarketUpdate(const unsigned int security_id, const MarketUpdateInfo& market_update_info) {
    if (security_id == dep_sec_id_) {
      BaseTrading::OnMarketUpdate(security_id, market_update_info);
    }
    CallTradingLogic();
  }

  void OnTradePrint(const unsigned int security_id, const TradePrintInfo& trade_print_info,
                    const MarketUpdateInfo& market_update_info) {
    mini_top_bid_agg_place_ = false;
    mini_top_ask_agg_place_ = false;
    mini_top_bid_keep_ = true;
    mini_top_ask_keep_ = true;

    if (security_id == dep_sec_id_) {
      BaseTrading::OnTradePrint(security_id, trade_print_info, market_update_info);
    }

    if ((security_id == indep_sec_id_) && CanTrade()) {
      if (trade_print_info.buysell_ == kTradeTypeBuy) {
        if (param_set_.mini_agg_logic_ && (market_update_info.bestask_int_price_ >
                                           (best_nonself_ask_int_price_ + param_set_.mini_agg_sweep_margin_))) {
          mini_top_bid_agg_place_ = true;
          // Assuming price quotes are same for both shortcodes
          // and same tick size
          agg_buy_int_price_ = market_update_info.bestask_int_price_ - param_set_.mini_agg_sweep_margin_ - 1;
          agg_buy_price_ = dep_market_view_.GetDoublePx(agg_buy_int_price_);
        }
        if (param_set_.mini_top_keep_logic_ && (market_update_info.bestask_int_price_ > best_nonself_ask_int_price_)) {
          mini_top_ask_keep_ = false;
          last_mini_top_ask_cancel_msecs_ = watch_.msecs_from_midnight();
        } else {
          mini_top_ask_keep_ = true;
          last_mini_top_ask_cancel_msecs_ = -1;
        }
        OrderPlacingLogic();
      } else if (trade_print_info.buysell_ == kTradeTypeSell) {
        if (param_set_.mini_agg_logic_ && (market_update_info.bestbid_int_price_ <
                                           (best_nonself_bid_int_price_ - param_set_.mini_agg_sweep_margin_))) {
          mini_top_ask_agg_place_ = true;
          // Assuming price quotes are same for both shortcodes
          // and same tick size
          agg_sell_int_price_ = market_update_info.bestbid_int_price_ + param_set_.mini_agg_sweep_margin_ + 1;
          agg_sell_price_ = dep_market_view_.GetDoublePx(agg_sell_int_price_);
        }
        if (param_set_.mini_top_keep_logic_ && (market_update_info.bestbid_int_price_ < best_nonself_bid_int_price_)) {
          mini_top_bid_keep_ = false;
          last_mini_top_bid_cancel_msecs_ = watch_.msecs_from_midnight();
        } else {
          mini_top_bid_keep_ = true;
          last_mini_top_bid_cancel_msecs_ = -1;
        }
        OrderPlacingLogic();
      }
    }

    if ((security_id == dep_sec_id_) || (security_id == indep_sec_id_)) {
      CallTradingLogic();
    }
  }

 protected:
  const SecurityMarketView& indep_market_view_;
  PriceType_t indep_price_type_;
  unsigned int indep_sec_id_;
  unsigned int dep_sec_id_;
  bool mini_top_bid_agg_place_;
  bool mini_top_bid_place_;
  bool mini_top_bid_keep_;
  unsigned int last_mini_top_bid_cancel_msecs_;
  bool mini_top_ask_agg_place_;
  bool mini_top_ask_place_;
  bool mini_top_ask_keep_;
  unsigned int last_mini_top_ask_cancel_msecs_;
  double agg_buy_price_;
  int agg_buy_int_price_;
  double agg_sell_price_;
  int agg_sell_int_price_;
  std::vector<unsigned int> ioc_orders_sent_;

  void TradingLogic();  ///< All the strategy based trade execution is written here
  void OrderPlacingLogic();
  inline bool CanTrade() {
    if (is_ready_) {
      if ((!external_freeze_trading_) && (!freeze_due_to_exchange_stage_) && (!freeze_due_to_funds_reject_)) {
        if ((!should_be_getting_flat_) && (throttle_manager_->allowed_through_throttle(watch_.msecs_from_midnight()))) {
          return true;
        }
      }
    }
    return false;
  }
  void CallTradingLogic() {
    if (CanTrade()) {
      TradingLogic();
    }
  }

  void PrintFullStatus();

  void ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool _conservative_close_) {
    BaseTrading::ReportResults(trades_writer_, _conservative_close_);
    DBGLOG_TIME_CLASS_FUNC << "IOC Orders Sent Timestamps" << DBGLOG_ENDL_FLUSH;
    for (unsigned int i = 0; i < ioc_orders_sent_.size(); i++) {
      DBGLOG_TIME_CLASS_FUNC << ioc_orders_sent_[i] << DBGLOG_ENDL_FLUSH;
    }
  }
};
}
#endif  // BASE_EXECLOGIC_PRICE_PAIR_BASED_AGGRESSIVE_TRADING_V2_H
