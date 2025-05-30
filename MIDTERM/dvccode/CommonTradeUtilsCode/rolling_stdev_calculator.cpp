/**
    \file CommonTradeUtilsCode/slow_stdev_calculator.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551


*/
#include "dvccode/CommonTradeUtils/rolling_stdev_calculator.hpp"
#include <vector>
#define MIN_STDEV_VALUE 1.0

namespace HFSAT {

// RollingStdevCalculator::RollingStdevCalculator( DbgLogger & _dbglogger_,
// 						  const Watch & _watch_,
// 						  double _fractional_seconds_ )
//   : dbglogger_ ( _dbglogger_ ),
//     watch_ ( _watch_ ),
//     trend_history_msecs_( std::max( 100u, _fractional_seconds_ * 1000 ) ),
//     my_listener_( NULL )
// {
//   SetTimeDecayWeights( );
//   InitializeValues( );
// }

void RollingStdevCalculator::InitializeValues() {
  moving_avg_values_ = 0;
  moving_avg_sqr_values_ = 0;
  last_value_recorded_ = 0;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  stdev_value_ = MIN_STDEV_VALUE;
}

void RollingStdevCalculator::AddValue(const double _new_value_) {
  if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
    moving_avg_values_ += inv_decay_sum_ * (_new_value_ - last_value_recorded_);
    moving_avg_sqr_values_ +=
        inv_decay_sum_ * (_new_value_ * _new_value_ - last_value_recorded_ * last_value_recorded_);
    last_value_recorded_ = _new_value_;
  } else {
    int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
    if (num_pages_to_add_ >= (int)decay_vector_.size()) {
      InitializeValues();
    } else {
      if (num_pages_to_add_ == 1) {
        moving_avg_values_ = (_new_value_ * inv_decay_sum_) + (moving_avg_values_ * decay_page_factor_);
        moving_avg_sqr_values_ =
            (_new_value_ * _new_value_ * inv_decay_sum_) + (moving_avg_sqr_values_ * decay_page_factor_);
      } else {
        moving_avg_values_ = (_new_value_ * inv_decay_sum_) +
                             (last_value_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                             (moving_avg_values_ * decay_vector_[num_pages_to_add_]);
        moving_avg_sqr_values_ = (_new_value_ * _new_value_ * inv_decay_sum_) +
                                 (last_value_recorded_ * last_value_recorded_ * inv_decay_sum_ *
                                  decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                                 (moving_avg_sqr_values_ * decay_vector_[num_pages_to_add_]);
      }
      last_value_recorded_ = _new_value_;
      last_new_page_msecs_ += num_pages_to_add_ * page_width_msecs_;
    }
    double unbiased_l2_norm_ =
        std::max(MIN_STDEV_VALUE * MIN_STDEV_VALUE, moving_avg_sqr_values_ - moving_avg_values_ * moving_avg_values_);
    stdev_value_ = sqrt(unbiased_l2_norm_);
  }
  if (my_listener_) my_listener_->UpdateStdev(stdev_value_);
}

void RollingStdevCalculator::SetTimeDecayWeights() {
  const unsigned int kDecayLength = 1000;
  const unsigned int kMinPageWidth = 100;
  const unsigned int kMaxPageWidth = 2000;
  page_width_msecs_ = std::min(kMaxPageWidth, std::max(kMinPageWidth, trend_history_msecs_ / kDecayLength));

  int num_fadeoffs_ = std::max(1, (int)ceil(trend_history_msecs_ / page_width_msecs_));

  decay_page_factor_ = MathUtils::CalcDecayFactor(num_fadeoffs_);

  decay_vector_.resize(2 * num_fadeoffs_);
  decay_vector_sums_.resize(2 * num_fadeoffs_);

  for (auto i = 0u; i < decay_vector_.size(); i++) {
    decay_vector_[i] = pow(decay_page_factor_, (int)i);
  }

  decay_vector_sums_[0] = 0;
  for (unsigned int i = 1; i < decay_vector_sums_.size(); i++) {
    decay_vector_sums_[i] = decay_vector_sums_[i - 1] + decay_vector_[i];
  }

  inv_decay_sum_ = (1 - decay_page_factor_);
}
}
