/**
   \file MarketAdapterCode/cme_market_per_security.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/cme_market_per_security.hpp"

namespace HFSAT {

CMEMarketPerSecurity::CMEMarketPerSecurity() : order_id_bids_map_(), order_id_asks_map_() {}

void CMEMarketPerSecurity::Clear() {
  order_id_bids_map_.clear();
  order_id_asks_map_.clear();
}

}  // HFSAT
