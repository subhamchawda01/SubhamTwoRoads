/**
   \file MarketAdapterCode/nse_market_per_security.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/nse_market_per_security.hpp"

namespace HFSAT {

NSEMarketPerSecurity::NSEMarketPerSecurity() : order_id_map_(), queued_order_id_map_() {}

void NSEMarketPerSecurity::Clear() {
  order_id_map_.clear();
  queued_order_id_map_.clear();
}

}  // HFSAT
