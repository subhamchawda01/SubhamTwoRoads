/**
   \file MinuteBar/minute_bar_order_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#pragma once

#include "baseinfra/OrderRouting/base_order_manager.hpp"
#include "baseinfra/MinuteBar/minute_bar_security_market_view.hpp"

namespace HFSAT {

class MinuteBarOrderManager : public BaseOrderManager {
  HFSAT::MinuteBarSecurityMarketView& minute_smv_;

 public:
  MinuteBarOrderManager(DebugLogger& dbglogger, const Watch& watch, SecurityNameIndexer& sec_name_indexer,
                        BaseTrader& sim_trader, MinuteBarSecurityMarketView& minute_smv, int runtime_id,
                        const bool live_trading, int first_client_order_seq);

  ~MinuteBarOrderManager() {}

  void Buy(int size);
  void Sell(int size);
};

typedef std::map<std::pair<int, int>, MinuteBarOrderManager*> StrategySecurityPairToMinuteBarOrderManager;

///< stored here as a static sid to smv* map so that Indicators can use this directly
static inline StrategySecurityPairToMinuteBarOrderManager& sid_to_minute_bar_order_manager_map() {
  static StrategySecurityPairToMinuteBarOrderManager sid_to_minute_bar_om_map;
  return sid_to_minute_bar_om_map;
}
}
