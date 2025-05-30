/**
    \file MinuteBar/base_minute_bar_signal.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#include "baseinfra/MinuteBar/Signal/base_minute_bar_signal.hpp"

namespace HFSAT {

BaseMinuteBarSignal::BaseMinuteBarSignal(DebugLogger &dbglogger, const Watch &watch)
    : dbglogger_(dbglogger),
      watch_(watch),
      signal_value_(0),
      listeners_(),
      sid_to_smv_ptr_map_(HFSAT::SecIDMinuteBarSMVMap::GetUniqueInstance()),
      indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()) {}

void BaseMinuteBarSignal::NotifyListeners() {
  for (auto pair : listeners_) {
    auto listener = pair.first;
    auto id = pair.second;
    listener->OnSignalUpdate(id, signal_value_);
  }
}

void BaseMinuteBarSignal::SubscribeSecurityID(int security_id) {
  sid_to_smv_ptr_map_.GetSecurityMarketView(security_id)->SubscribeMinuteBars(this);
}
};
