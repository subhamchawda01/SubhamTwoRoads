/**
   \file MarketAdapter/cme_market_per_security.hpp

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

struct CMEOrder {
  double price;
  int size;

  // Information in String
  std::string ToString() {
    std::stringstream st;
    st << "Price: " << price << " size: " << size;
    return st.str();
  }
};

class CMEMarketPerSecurity {
 protected:
 public:
  std::map<int64_t, CMEOrder*> order_id_bids_map_;
  std::map<int64_t, CMEOrder*> order_id_asks_map_;

  CMEMarketPerSecurity();

  void Clear();
};
}
