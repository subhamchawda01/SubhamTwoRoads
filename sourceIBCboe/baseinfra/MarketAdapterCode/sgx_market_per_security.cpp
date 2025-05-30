/*
 * sgx_market_per_security.cpp
 *
 *  Created on: 27-Oct-2017
 *      Author: anubhavpandey
 */

#include "baseinfra/MarketAdapter/sgx_market_per_security.hpp"

namespace HFSAT {

SGXMarketPerSecurity::SGXMarketPerSecurity() : order_id_bids_map_(), order_id_asks_map_() {}

void SGXMarketPerSecurity::Clear() {
  order_id_bids_map_.clear();
  order_id_asks_map_.clear();
}

}  // HFSAT
