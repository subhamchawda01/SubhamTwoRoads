#ifndef _INDICATOR_MID_SIZE_FILTERED_PRICE_H
#define _INDICATOR_MID_SIZE_FILTERED_PRICE_H

#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "tradeengine/CommonInitializer/defines.hpp"
#include "tradeengine/Indicator/BasePrice.hpp"

class MidSizeFiltered : public BasePrice {
  int size_filter_;
  int max_levels_;
  bool CalculateMidSizeFiltered();

 public:
  MidSizeFiltered(HFSAT::SecurityMarketView* smv, HFSAT::Watch& _watch_, HFSAT::DebugLogger& _dbglogger_,
                  double weight_, int size_filter_);
  virtual ~MidSizeFiltered();

  virtual void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);

  virtual void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                            const HFSAT::MarketUpdateInfo& _market_update_info_);
};

#endif  //_INDICATOR_MID_SIZE_FILTERED_PRICE_H
