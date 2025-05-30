#ifndef _INDICATOR_TRADE_PRICE_H
#define _INDICATOR_TRADE_PRICE_H

#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "tradeengine/CommonInitializer/defines.hpp"
#include "tradeengine/Indicator/BasePrice.hpp"

class TradePrice : public BasePrice {
  int64_t last_traded_usecs_;
  int64_t current_time_usecs_;
  int64_t last_trade_mid_price_cutoff_usecs_;
  int vwap_levels_;
  bool CalculateVWAPPrice();

 public:
  TradePrice(HFSAT::SecurityMarketView* smv, HFSAT::Watch& _watch_, HFSAT::DebugLogger& _dbglogger_, double weight_,
             int64_t last_trade_mid_price_cutoff_usecs, int vwap_levels);
  virtual ~TradePrice();

  virtual void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);

  virtual void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                            const HFSAT::MarketUpdateInfo& _market_update_info_);
};

#endif  //_INDICATOR_TRADE_PRICE_H
