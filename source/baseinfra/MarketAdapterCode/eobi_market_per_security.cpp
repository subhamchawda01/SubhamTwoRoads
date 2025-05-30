#include "baseinfra/MarketAdapter/eobi_market_per_security.hpp"

namespace HFSAT {

EOBIMarketPerSecurity::EOBIMarketPerSecurity() : order_id_bids_map_(), order_id_asks_map_() {}

void EOBIMarketPerSecurity::Clear() {
  order_id_bids_map_.clear();
  order_id_asks_map_.clear();
}

}  // HFSAT
