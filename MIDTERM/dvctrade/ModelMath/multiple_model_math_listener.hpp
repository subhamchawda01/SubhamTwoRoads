/**
        \file ModelMath/multiple_model_math_listener.hpp

        \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
*/
#ifndef BASE_MODELMATH_MULTIPLE_MODEL_MATH_LISTENER_H
#define BASE_MODELMATH_MULTIPLE_MODEL_MATH_LISTENER_H

namespace HFSAT {

class MultipleModelMathListener {
 public:
  virtual ~MultipleModelMathListener() {}

  virtual bool UpdateTarget(double _new_target_, double _new_sum_vars_, int _modelmath_index_ = 0,
                            int _product_index_ = 0) = 0;

  // Only called in dire circumstances like line breaks
  virtual void TargetNotReady(int modelmath_index_, int _product_index_) = 0;
};
}

#endif  // BASE_MODELMATH_MULTIPLE_MODEL_MATH_LISTENER_H
