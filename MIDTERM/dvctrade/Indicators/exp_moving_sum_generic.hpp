/*
 * exp_moving_sum_generic.hpp
 *
 *  Created on: Apr 13, 2015
 *      Author: archit
 */

#ifndef INDICATORSCODE_EXP_MOVING_SUM_GENERIC_HPP_
#define INDICATORSCODE_EXP_MOVING_SUM_GENERIC_HPP_

#include <vector>
#include <math.h>
#include "dvccode/CDef/math_utils.hpp"

namespace HFSAT {

class ExpMovingSumGeneric {
 protected:
  int half_life_msecs_;

  // computational variables
  double moving_sum_;

  int last_new_page_msecs_;
  int page_width_msecs_;

  double decay_page_factor_;
  std::vector<double> decay_vector_;
  bool is_ready_;

  void InitializeValues(int _new_msecs_, double _new_sample_);
  void SetTimeDecayWeights();

 public:
  ExpMovingSumGeneric(int _half_life_msecs_);
  virtual ~ExpMovingSumGeneric() {}
  void NewSample(int _new_msecs_, double _new_sample_);
  inline double GetSum() { return moving_sum_; }
};

} /* namespace HFSAT */

#endif /* INDICATORSCODE_EXP_MOVING_SUM_GENERIC_HPP_ */
