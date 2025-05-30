/**
   \file MarketAdapter/ice_market_per_security.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include <tr1/memory>
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/MarketAdapter/base_market_order_manager.hpp"

namespace HFSAT {

struct IceOrder {
  double price_;
  int size_;
  TradeType_t buysell_;
};

class IceMarketPerSecurity {
 protected:
 public:
  std::map<int64_t, std::tr1::shared_ptr<IceOrder> > ord_map_;
  int64_t last_add_order_;

  IceMarketPerSecurity();

  void Clear();
};
}
