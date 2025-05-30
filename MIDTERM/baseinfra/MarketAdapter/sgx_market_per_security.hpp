/*
 * sgx_market_per_security.hpp
 *
 *  Created on: 10-Oct-2017
 *      Author: toroads3
 */

#ifndef SGX_MARKET_PER_SECURITY_HPP_
#define SGX_MARKET_PER_SECURITY_HPP_

#pragma once

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/MarketAdapter/base_market_order_manager.hpp"

// This file just defines the map that has key as order_id and value as price and size
// The map is maintained for the bid and the ask sides seperately

namespace HFSAT {

struct SGXOrder {
  double price;
  int size;
};

class SGXMarketPerSecurity {
 protected:
 public:
  std::map<int64_t, SGXOrder*> order_id_bids_map_;
  std::map<int64_t, SGXOrder*> order_id_asks_map_;

  SGXMarketPerSecurity();

  void Clear();
};
}

#endif /* SGX_MARKET_PER_SECURITY_HPP_ */
