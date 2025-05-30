#ifndef _INDICATOR_CUSTOM_PRICE_AGGREGATOR_H
#define _INDICATOR_CUSTOM_PRICE_AGGREGATOR_H

#include "tradeengine/Indicator/BasePrice.hpp"

class CustomPriceAggregator {
  HFSAT::SecurityMarketView* smv_;
  HFSAT::Watch& watch_;
  HFSAT::DebugLogger& dbglogger_;
  std::map<std::string, std::string>* key_val_map_;
  std::vector<BasePrice*> price_vector_;

 public:
  CustomPriceAggregator(HFSAT::SecurityMarketView* smv, HFSAT::Watch& _watch_, HFSAT::DebugLogger& _dbglogger_,
                        std::map<std::string, std::string>* key_val_map);

  virtual ~CustomPriceAggregator();

  void CreatePriceVector();
  void GetPrice(double& bid_price, double& ask_price);
};

#endif  // _INDICATOR_CUSTOM_PRICE_AGGREGATOR_H
