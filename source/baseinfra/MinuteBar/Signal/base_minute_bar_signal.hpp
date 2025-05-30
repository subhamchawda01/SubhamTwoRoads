/**
    \file MinuteBar/base_minute_signal.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#pragma once

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "baseinfra/MinuteBar/minute_bar_security_market_view.hpp"
#include "baseinfra/MinuteBar/shortcode_minute_bar_smv_map.hpp"

namespace HFSAT {

class SignalListener {
 public:
  virtual ~SignalListener() {}
  virtual void OnSignalUpdate(int signal_id, double signal_value) = 0;
};

class BaseMinuteBarSignal : public MinuteBarSMVListener {
 protected:
  DebugLogger &dbglogger_;
  const Watch &watch_;
  double signal_value_;
  std::vector<std::pair<SignalListener *, int> > listeners_;
  HFSAT::SecIDMinuteBarSMVMap &sid_to_smv_ptr_map_;
  HFSAT::SecurityNameIndexer &indexer_;

 public:
  BaseMinuteBarSignal(DebugLogger &dbglogger, const Watch &watch);
  virtual ~BaseMinuteBarSignal() {}

  void SubscribeSecurityID(int security_id);

  void AddSignalListener(SignalListener *listener, int signal_id) {
    VectorUtils::UniqueVectorAdd(listeners_, std::make_pair(listener, signal_id));
  }
  void OnBarUpdate(const unsigned int security_id, const DataBar &minute_bar) override {}
  void NotifyListeners();
};
};
