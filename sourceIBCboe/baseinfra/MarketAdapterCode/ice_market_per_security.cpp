/**
   \file MarketAdapterCode/ice_market_per_security.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/ice_market_per_security.hpp"

namespace HFSAT {

IceMarketPerSecurity::IceMarketPerSecurity() : ord_map_(), last_add_order_() {}

void IceMarketPerSecurity::Clear() { ord_map_.clear(); }
}
