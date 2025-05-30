/**
   \file MarketAdapter/nse_market_per_security.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/MarketAdapter/base_market_order_manager.hpp"

namespace HFSAT {

struct NSEOrder {
  double price;
  int size;
  TradeType_t buysell_;

  // Information in String
  std::string ToString() {
    std::stringstream st;
    st << "Price: " << price << " size: " << size;
    return st.str();
  }
};

class NSEMarketPerSecurity {
 protected:
 public:

  // We don't need to maintain two maps (bid and ask) since then we will be doing lot of recurring things.
  // Instead we can add buysell field to Order Struct directly.

  std::map<int64_t, NSEOrder*> order_id_map_;
  std::map<int64_t, NSEOrder*> queued_order_id_map_;


  NSEMarketPerSecurity();

  void Clear();
};
}
