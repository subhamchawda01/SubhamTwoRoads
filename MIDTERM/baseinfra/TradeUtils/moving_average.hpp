/**
    \file Indicators/time_decayed_trade_info_manager.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include <map>
#include "dvccode/CDef/math_utils.hpp"

namespace HFSAT {

/// Class that takes a decay factor
/// and encapsulates the logic to compute moving average

class MovingAverage {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  bool is_initialized_;

  int trend_history_msecs_;

  double last_val_;
  double current_val_;

  /// internal variables for detecting that a new decay zone has passed
  /// As in we break the time to fadeoff into pages, where items in each page are decayed similarly
  int last_new_page_msecs_;
  int page_width_msecs_;
  /// internal variables for decay computation
  double decay_page_factor_;
  std::vector<double> decay_vector_;
  double inv_decay_sum_;
  std::vector<double> decay_vector_sums_;

 public:

  double moving_avg_;
  
  void InitializeValues() {
    moving_avg_ = current_val_;
    last_val_ = current_val_;
    last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  }

  MovingAverage(DebugLogger& _dbglogger_, const Watch& _watch_,double _fractional_seconds_)
  : dbglogger_(_dbglogger_),
    watch_(_watch_),
    is_initialized_(false) {
      SetTimeDecayWeights(_fractional_seconds_);
  }

  inline void CalculateValue(const double& new_value) {
    if (!is_initialized_) {
      is_initialized_ = true;
      current_val_ = new_value;
      InitializeValues();
    }
    else {
      current_val_ = new_value;

      if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
        moving_avg_ += inv_decay_sum_ * (current_val_ - last_val_);
      } else {
        int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
        if (num_pages_to_add_ >= (int)decay_vector_.size()) {
          InitializeValues();
        } else {
          if (num_pages_to_add_ == 1) {
            moving_avg_ = (current_val_ * inv_decay_sum_) + (moving_avg_ * decay_vector_[1]);
          } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
            moving_avg_ = (current_val_ * inv_decay_sum_) +
                          (last_val_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                          (moving_avg_ * decay_vector_[num_pages_to_add_]);
          }
          last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
        }
      }
      last_val_ = current_val_;
    }
  }

 protected:
  void SetTimeDecayWeights(double _fractional_seconds_) {
    trend_history_msecs_ = (std::max(20, (int)round(1000 * _fractional_seconds_)));

    ///< here number of samples are not required to be very high and hence the decaylength target is just 20
    const unsigned int kDecayLength = 20;
    const unsigned int kMinPageWidth = 10;
    ///< keeping kMaxPageWidth low makes the number_fadeoffs_ pretty high and
    const unsigned int kMaxPageWidth = 200;
    /// hence keeps lots of sample points
    page_width_msecs_ = std::min(kMaxPageWidth, std::max(kMinPageWidth, (trend_history_msecs_ / kDecayLength)));

    int number_fadeoffs_ = std::max(1, (int)ceil(trend_history_msecs_ / page_width_msecs_));

    decay_page_factor_ = MathUtils::CalcDecayFactor(number_fadeoffs_);

    decay_vector_.resize(2 * number_fadeoffs_);
    decay_vector_sums_.resize(2 * number_fadeoffs_);

    for (auto i = 0u; i < decay_vector_.size(); i++) {
      decay_vector_[i] = pow(decay_page_factor_, (int)i);
    }

    decay_vector_sums_[0] = 0;
    for (unsigned int i = 1; i < decay_vector_sums_.size(); i++) {
      decay_vector_sums_[i] = decay_vector_sums_[i - 1] + decay_vector_[i];
    }

    inv_decay_sum_ = (1 - decay_page_factor_);
  }
};
}
