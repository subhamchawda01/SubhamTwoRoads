#ifndef _INDICATOR_TRADE_SPREAD_PRICE_H
#define _INDICATOR_TRADE_SPREAD_PRICE_H

#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "tradeengine/CommonInitializer/defines.hpp"
#include "tradeengine/Indicator/BasePrice.hpp"
#include <deque>

class TradeTimeInfo {
 public:
  double price_;
  int trade_size_;
  int64_t time_;
  TradeTimeInfo(double px, int trade_size, int64_t time);
  ~TradeTimeInfo() {}
};

class TradeSpreadPrice : public BasePrice {
  int64_t last_traded_usecs_;
  int64_t current_time_usecs_;
  int64_t window_;
  double last_bid_px_;
  double last_ask_px_;
  int sum_bid_size_;
  double sum_bid_spread_;
  int sum_ask_size_;
  double sum_ask_spread_;
  int vwap_levels_;
  double skew_;
  double skew_factor_;
  std::deque<TradeTimeInfo> bid_queue_;
  std::deque<TradeTimeInfo> ask_queue_;

  bool CalculateVWAPPrice();

 public:
  TradeSpreadPrice(HFSAT::SecurityMarketView* smv, HFSAT::Watch& _watch_, HFSAT::DebugLogger& _dbglogger_,
                   double weight_, int64_t window, int vwap_levels, double skew_factor);
  virtual ~TradeSpreadPrice();

  virtual void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);

  virtual void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                            const HFSAT::MarketUpdateInfo& _market_update_info_);
};

#endif  //_INDICATOR_TRADE_SPREAD_PRICE_H
