/**
    \file dvccode/CommonTradeUtils/slow_stdev_calculator.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

     Utility to compute rolling decayed stdev value. AddValue( ) function needs be
     called by the invoking class whenever a new value needs to be added. Decay is
     done internally - half life is specified at the time of class initialization.

*/

#ifndef ROLLING_STDEV_CALCULATOR_H
#define ROLLING_STDEV_CALCULATOR_H

#include "dvccode/CommonTradeUtils/watch.hpp"

namespace HFSAT {

class RollingStdevCalculatorListener {
 public:
  virtual ~RollingStdevCalculatorListener(){};
  virtual void UpdateStdev(const double _new_value_) = 0;
};

class RollingStdevCalculator {
 public:
  RollingStdevCalculator(DebugLogger& _dbglogger_, const Watch& _watch_, double _fractional_seconds_)
      : dbglogger_(_dbglogger_),
        watch_(_watch_),
        trend_history_msecs_(std::max(100, (int)(_fractional_seconds_ * 1000))),
        my_listener_(NULL) {
    SetTimeDecayWeights();
    InitializeValues();
  }

  ~RollingStdevCalculator() {}

  void AddValue(const double _new_value_);

  void AddListener(RollingStdevCalculatorListener* listener) { my_listener_ = listener; }

 private:
  DebugLogger& dbglogger_;
  const Watch& watch_;

  double decay_page_factor_;
  std::vector<double> decay_vector_;
  std::vector<double> decay_vector_sums_;
  double inv_decay_sum_;
  double stdev_value_;
  int trend_history_msecs_;

  double moving_avg_values_;
  double moving_avg_sqr_values_;
  double last_value_recorded_;
  int last_new_page_msecs_;
  int page_width_msecs_;

  RollingStdevCalculatorListener* my_listener_;

  void SetTimeDecayWeights();
  void InitializeValues();
};
}
#endif  // ROLLING_STDEV_CALCULATOR_H
