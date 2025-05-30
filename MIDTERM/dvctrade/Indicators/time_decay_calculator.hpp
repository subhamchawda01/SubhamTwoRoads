#pragma once

#include <math.h> // for pow
#include "dvccode/CDef/math_utils.hpp" // for CalcDecayFactor

namespace HFSAT{

class TimeDecayCalculator {
 public:
  int trend_history_msecs_;
  int last_new_page_msecs_;
  int page_width_msecs_;

  double decay_page_factor_;
  std::vector<double> decay_vector_;
  double inv_decay_sum_;
  std::vector<double> decay_vector_sums_;

  TimeDecayCalculator ( int g_trend_history_msecs_, int g_last_new_page_msecs_, int g_page_width_msecs_, double g_decay_page_factor_, double g_inv_decay_sum_ )
      :
      trend_history_msecs_ ( g_trend_history_msecs_ ),
      last_new_page_msecs_ ( g_last_new_page_msecs_ ),
      page_width_msecs_ ( g_page_width_msecs_ ),
      decay_page_factor_ ( g_decay_page_factor_ ),
      inv_decay_sum_ ( g_inv_decay_sum_ )
  {
    
  }

  TimeDecayCalculator ( int g_trend_history_msecs_, int g_last_new_page_msecs_, int g_page_width_msecs_, double g_decay_page_factor_ )
      :
      trend_history_msecs_ ( g_trend_history_msecs_ ),
      last_new_page_msecs_ ( g_last_new_page_msecs_ ),
      page_width_msecs_ ( g_page_width_msecs_ ),
      decay_page_factor_ ( g_decay_page_factor_ ),
      inv_decay_sum_ ( 1 - g_decay_page_factor_ )
  {
    
  }

  void SetTimeDecayWeights()
  {
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
    
    int power_index = 0;
    for (auto & t_decay_vector_ : decay_vector_ ) {
      t_decay_vector_ = pow(decay_page_factor_, power_index);
      power_index ++;
    }
    
    decay_vector_sums_[0] = 0;
    for (auto i = 1u; i < decay_vector_sums_.size(); i++) {
      decay_vector_sums_[i] = decay_vector_sums_[i - 1] + decay_vector_[i];
    }
    
    inv_decay_sum_ = (1 - decay_page_factor_);
  }

  void InitializeValues(const int watch_mfm_)
  {
    if ( page_width_msecs_ > 0 ) {
      last_new_page_msecs_ = watch_mfm_ - (watch_mfm_ % page_width_msecs_);
    }
  }
};
}
