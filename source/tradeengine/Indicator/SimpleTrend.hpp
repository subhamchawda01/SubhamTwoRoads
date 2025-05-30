#ifndef _INDICATOR_SIMPLE_TREND_H
#define _INDICATOR_SIMPLE_TREND_H

#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "tradeengine/Indicator/BasePrice.hpp"
#include "tradeengine/Indicator/TradeSpreadPrice.hpp"

class SimpleTrend : public BasePrice {
  double skew_factor_;
  uint64_t num_events_;
  int64_t current_time_usecs_;
  double sum_px_;
  int sum_size_;
  double trend_avg_px_;
  std::deque<TradeTimeInfo> trade_info_;

  bool pruneTradeVector();
  // bool CalculateVWAPPrice();
 public:
  SimpleTrend(HFSAT::SecurityMarketView* smv, HFSAT::Watch& _watch_, HFSAT::DebugLogger& _dbglogger_, double weight_,
              double skew_factor, int num_events);
  virtual ~SimpleTrend();

  virtual void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);

  virtual void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                            const HFSAT::MarketUpdateInfo& _market_update_info_);
};

#endif  // _INDICATOR_SIMPLE_TREND_H
