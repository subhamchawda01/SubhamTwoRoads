/**
        \file OptionsUtils/implied_vol_listener.hpp

        \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
*/
#ifndef BASE_IMPLIED_VOL_LISTENER_H
#define BASE_IMPLIED_VOL_LISTENER_H

namespace HFSAT {

class ImpliedVolListener {
 public:
  virtual ~ImpliedVolListener() {}

  virtual bool UpdateIV(double _new_iv_, double _new_sum_vars_, int _modelmath_index_ = 0) = 0;
  // Only called in dire circumstances like line breaks
  virtual void IVNotReady() = 0;
};
}

#endif  // BASE_MODELMATH_MODEL_MATH_LISTENER_H
