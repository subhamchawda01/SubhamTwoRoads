#include "baseinfra/MinuteBar/minute_bar_data_market_view_manager.hpp"

namespace HFSAT {

MinuteBarDataMarketViewManager::MinuteBarDataMarketViewManager(DebugLogger& dbglogger, const Watch& watch,
                                                               const SecurityNameIndexer& sec_name_indexer)
    : security_market_view_map_(HFSAT::SecIDMinuteBarSMVMap::GetUniqueInstance()) {}

void MinuteBarDataMarketViewManager::OnNewBar(const unsigned int security_id, const DataBar& minute_bar) {
  MinuteBarSecurityMarketView& smv_ = *(security_market_view_map_.GetSecurityMarketView(security_id));
  if (!smv_.is_ready_) {
    smv_.is_ready_ = true;
  }

  smv_.minute_bar_ = minute_bar;

  smv_.NotifyMinuteBarListeners(security_id, minute_bar);
}
}
