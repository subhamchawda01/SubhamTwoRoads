/**
   \file MarketAdapter/base_market_order_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include <vector>

#include "baseinfra/MarketAdapter/market_orders_view.hpp"

namespace HFSAT {

class BaseMarketOrderManager {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;

  const std::vector<MarketOrdersView*> market_orders_view_map_;

 public:
  BaseMarketOrderManager(DebugLogger& t_dbglogger_, const Watch& t_watch_, SecurityNameIndexer& t_sec_name_indexer_,
                         const std::vector<MarketOrdersView*>& t_market_orders_view_map_);

  ~BaseMarketOrderManager();
};
}
