#ifndef _GENRIC_L1_DATA_MARKET_VIEW_MANAGER_
#define _GENRIC_L1_DATA_MARKET_VIEW_MANAGER_

#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "baseinfra/MDSMessages/mds_message_listener.hpp"

namespace HFSAT {

class GenericL1DataMarketViewManager : public L1DataListener, public BaseMarketViewManager {
 public:
  GenericL1DataMarketViewManager(DebugLogger& dbglogger, const Watch& watch,
                                 const SecurityNameIndexer& sec_name_indexer,
                                 const std::vector<SecurityMarketView*>& security_market_view_map);

  void OnL1New(const unsigned int security_id, const GenericL1DataStruct& l1_data);
  void OnTrade(const unsigned int security_id, const GenericL1DataStruct& l1_data);

  void SetTimeToSkipUntilFirstEvent(const ttime_t start_time);
};
}

#endif
