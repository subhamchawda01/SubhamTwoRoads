/**
    \file Indicators/stdev_l1_bias.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_STDEV_CALCULATOR_H
#define BASE_INDICATORS_STDEV_CALCULATOR_H

namespace HFSAT {

/// Class returning exponentially time-decaying stdev
class StdevCalculator {
 protected:
  // variables
  DebugLogger& dbglogger_;
  const Watch& watch_;
  int trend_history_msecs_;
  double ticksize_;

  // computational variables
  double moving_avg_val_;
  double moving_avg_val_square_;
  double last_val_recorded_;
  double stdev_;

  int last_new_page_msecs_;
  int page_width_msecs_;

  double decay_page_factor_;
  std::vector<double> decay_vector_;
  double inv_decay_sum_;
  std::vector<double> decay_vector_sums_;

  bool initialized_;
  //double start_tstamp_;

  // functions
 public:
  StdevCalculator(DebugLogger& _dbglogger_, const Watch& _watch_, double _fractional_seconds_, double ticksize_);

  ~StdevCalculator() {}

  double AddCurrentValue(double current_val_);

  // listener interface
  double GetStdev() { return stdev_; }

 protected:
  void SetTimeDecayWeights();
  void InitializeValues(double current_val_);
};
}

#endif  // BASE_INDICATORS_STDEV_CALCULATOR_H
