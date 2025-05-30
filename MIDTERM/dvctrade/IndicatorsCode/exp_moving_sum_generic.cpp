/*
 * exp_moving_sum_generic.cpp
 *
 *  Created on: Apr 13, 2015
 *      Author: archit
 */

#include "dvctrade/Indicators/exp_moving_sum_generic.hpp"

namespace HFSAT {

ExpMovingSumGeneric::ExpMovingSumGeneric(int _half_life_msecs_)
    : half_life_msecs_(_half_life_msecs_),
      moving_sum_(0.0),
      last_new_page_msecs_(0),
      page_width_msecs_(500),
      decay_page_factor_(0.95),
      is_ready_(false) {
  SetTimeDecayWeights();
}

void ExpMovingSumGeneric::NewSample(int _new_msecs_, double _new_sample_) {
  if (!is_ready_) {
    // just to cover the case of first call
    InitializeValues(_new_msecs_, _new_sample_);
    is_ready_ = true;
  } else {
    int num_pages_to_add_ = (int)floor((_new_msecs_ - last_new_page_msecs_) / page_width_msecs_);
    if (num_pages_to_add_ >= (int)decay_vector_.size()) {
      InitializeValues(_new_msecs_, _new_sample_);
    } else {
      moving_sum_ *= decay_vector_[num_pages_to_add_];
      moving_sum_ += _new_sample_;
      last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
    }
  }
}

void ExpMovingSumGeneric::InitializeValues(int _new_msecs_, double _new_sample_) {
  moving_sum_ = _new_sample_;
  last_new_page_msecs_ = _new_msecs_ - _new_msecs_ % page_width_msecs_;
}

void ExpMovingSumGeneric::SetTimeDecayWeights() {
  const unsigned int kDecayLength =
      20;  ///< here number of samples are not required to be very high and hence the decaylength target is just 20
  const unsigned int kMinPageWidth = 10;
  const unsigned int kMaxPageWidth =
      200;  ///< keeping kMaxPageWidth low makes the number_fadeoffs_ pretty high and hence keeps lots of sample points
  page_width_msecs_ = std::min(kMaxPageWidth, std::max(kMinPageWidth, (half_life_msecs_ / kDecayLength)));

  int number_fadeoffs_ = std::max(1, (int)ceil(half_life_msecs_ / page_width_msecs_));

  decay_page_factor_ = MathUtils::CalcDecayFactor(number_fadeoffs_);

  decay_vector_.resize(2 * number_fadeoffs_);
  for (auto i = 0u; i < decay_vector_.size(); i++) {
    decay_vector_[i] = pow(decay_page_factor_, (int)i);
  }
}

} /* namespace HFSAT */
