/**
        \file ModelMath/model_math_listener.hpp

        \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
*/
#ifndef BASE_MODELMATH_CANCEL_MODEL_LISTENER_H
#define BASE_MODELMATH_CANCEL_MODEL_LISTENER_H

namespace HFSAT {

class CancelModelListener {
 public:
  virtual ~CancelModelListener() {}

  virtual void UpdateCancelSignal(double predicted_class_prob_, int bid_or_ask) {}
};
}

#endif  // BASE_MODELMATH_CANCEL_MODEL_LISTENER_H
