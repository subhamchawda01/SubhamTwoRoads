/**
    \file IndicatorsCode/stdev_l1_bias.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/stdev_calculator.hpp"

namespace HFSAT {

StdevCalculator::StdevCalculator(DebugLogger& _dbglogger_, const Watch& _watch_,
                         double _fractional_seconds_, double _ticksize_)
    : dbglogger_(_dbglogger_),
      watch_(_watch_),
      trend_history_msecs_(std::max(20, (int)round(1000 * _fractional_seconds_))),
      ticksize_(_ticksize_),
      last_new_page_msecs_(0),
      page_width_msecs_(500),
      decay_page_factor_(0.95),
      inv_decay_sum_(0.05) {
  SetTimeDecayWeights();
  InitializeValues(0);
  initialized_ = false;
}

double StdevCalculator::AddCurrentValue(double current_val_) {
  if (! initialized_) {
    InitializeValues(current_val_);
    initialized_ = true;
    return stdev_;
  }
  
  if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
    moving_avg_val_ += inv_decay_sum_ * (current_val_ - last_val_recorded_);
    moving_avg_val_square_ +=
      inv_decay_sum_ * (current_val_ * current_val_ - last_val_recorded_ * last_val_recorded_);
  } else {
    int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
    if (num_pages_to_add_ >= (int)decay_vector_.size()) {
      InitializeValues(current_val_);
    } else {
      if (num_pages_to_add_ == 1) {
        moving_avg_val_ = (current_val_ * inv_decay_sum_) + (moving_avg_val_ * decay_vector_[1]);
        moving_avg_val_square_ =
          (current_val_ * current_val_ * inv_decay_sum_) + (moving_avg_val_square_ * decay_vector_[1]);
      } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
        moving_avg_val_ = (current_val_ * inv_decay_sum_) +
          (last_val_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
          (moving_avg_val_ * decay_vector_[num_pages_to_add_]);
        moving_avg_val_square_ = (current_val_ * current_val_ * inv_decay_sum_) +
          (last_val_recorded_ * last_val_recorded_ * inv_decay_sum_ *
           decay_vector_sums_[(num_pages_to_add_ - 1)]) +
          (moving_avg_val_square_ * decay_vector_[num_pages_to_add_]);
      }
      last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
    }
  }
  last_val_recorded_ = current_val_;

  if (current_val_==0) {
    DBGLOG_TIME << " stdev: " << moving_avg_val_ << " " << moving_avg_val_square_ << DBGLOG_ENDL_FLUSH;
  }
  //stdev = sqrt( expectation(y^2) - (expectation(y))^2
  stdev_ = std::sqrt(moving_avg_val_square_ - moving_avg_val_ * moving_avg_val_);
  return stdev_;
}

void StdevCalculator::SetTimeDecayWeights() {
  const unsigned int kDecayLength =
      20;  ///< here number of samples are not required to be very high and hence the decaylength target is just 20
  const unsigned int kMinPageWidth = 10;
  const unsigned int kMaxPageWidth =
      200;  ///< keeping kMaxPageWidth low makes the number_fadeoffs_ pretty high and hence keeps lots of sample points
  page_width_msecs_ = std::min(kMaxPageWidth, std::max(kMinPageWidth, (trend_history_msecs_ / kDecayLength)));

  int number_fadeoffs_ = std::max(1, (int)ceil(trend_history_msecs_ / page_width_msecs_));

  decay_page_factor_ = MathUtils::CalcDecayFactor(number_fadeoffs_);

  decay_vector_.resize(2 * number_fadeoffs_);
  decay_vector_sums_.resize(2 * number_fadeoffs_);

  for (unsigned int i = 0; i < decay_vector_.size(); i++) {
    decay_vector_[i] = pow(decay_page_factor_, (int)i);
  }

  decay_vector_sums_[0] = 0;
  for (unsigned int i = 1; i < decay_vector_sums_.size(); i++) {
    decay_vector_sums_[i] = decay_vector_sums_[i - 1] + decay_vector_[i];
  }

  inv_decay_sum_ = (1 - decay_page_factor_);
}

void StdevCalculator::InitializeValues(double current_val_) {
  moving_avg_val_ = current_val_;
  moving_avg_val_square_ = moving_avg_val_ * moving_avg_val_;
  last_val_recorded_ = current_val_;
  stdev_ = 0;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
}

}
