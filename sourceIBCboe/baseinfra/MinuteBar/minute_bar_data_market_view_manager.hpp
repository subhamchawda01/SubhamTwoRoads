#pragma once

#include "dvccode/CDef/data_bar_struct.hpp"
#include "baseinfra/MinuteBar/minute_bar_security_market_view.hpp"
#include "baseinfra/MinuteBar/shortcode_minute_bar_smv_map.hpp"

namespace HFSAT {

class MinuteBarDataListener {
 public:
  virtual void OnNewBar(const unsigned int security_id, const DataBar& minute_bar) = 0;

  virtual ~MinuteBarDataListener(){};
};

class MinuteBarDataMarketViewManager : public MinuteBarDataListener {
  HFSAT::SecIDMinuteBarSMVMap& security_market_view_map_;

 public:
  MinuteBarDataMarketViewManager(DebugLogger& dbglogger, const Watch& watch,
                                 const SecurityNameIndexer& sec_name_indexer);

  void OnNewBar(const unsigned int security_id, const DataBar& minute_bar);
};
}
