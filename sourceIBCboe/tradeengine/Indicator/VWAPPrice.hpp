#ifndef _INDICATOR_VWAP_PRICE_H
#define _INDICATOR_VWAP_PRICE_H

#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "tradeengine/CommonInitializer/defines.hpp"
#include "tradeengine/Indicator/BasePrice.hpp"

class VWAPPrice : public BasePrice {
  int vwap_levels_;
  bool CalculateVWAPPrice();

 public:
  VWAPPrice(HFSAT::SecurityMarketView* smv, HFSAT::Watch& _watch_, HFSAT::DebugLogger& _dbglogger_, double weight_,
            int num_levels_);
  virtual ~VWAPPrice();

  virtual void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);

  virtual void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                            const HFSAT::MarketUpdateInfo& _market_update_info_);
};

#endif  //_INDICATOR_VWAP_PRICE_H
