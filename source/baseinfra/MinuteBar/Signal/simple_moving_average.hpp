/**
    \file MinuteBar/simple_moving_average.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#pragma once

#include <queue>
#include "baseinfra/MinuteBar/Signal/base_minute_bar_signal.hpp"

namespace HFSAT {

class SimpleMovingAverage : public BaseMinuteBarSignal {
 protected:
  double moving_avg_;
  int num_bar_updates_;
  int security_id_;
  unsigned int duration_;
  MinuteBarPriceType price_type_;
  std::queue<double> price_list_;

 public:
  static SimpleMovingAverage* GetUniqueInstance(DebugLogger& dbglogger, const Watch& watch,
                                                const std::vector<const char*>& tokens, MinuteBarPriceType price_type);

  SimpleMovingAverage(DebugLogger& dbglogger, const Watch& watch, std::string shortcode, int duration,
                      MinuteBarPriceType price_type);
  void OnBarUpdate(const unsigned int security_id, const DataBar& minute_bar) override;
  static std::string SignalName() { return "SimpleMovingAverage"; }
};
};
