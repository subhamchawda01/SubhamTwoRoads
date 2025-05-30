#pragma once

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/MarketAdapter/base_market_order_manager.hpp"

namespace HFSAT {

struct EOBIOrder {
  double price;
  int size;
};

class EOBIMarketPerSecurity {
 protected:
 public:
  std::map<int64_t, EOBIOrder*> order_id_bids_map_;
  std::map<int64_t, EOBIOrder*> order_id_asks_map_;

  EOBIMarketPerSecurity();

  void Clear();
};
}
