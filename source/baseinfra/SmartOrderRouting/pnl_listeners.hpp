/**
    \file SmartOrderRouting/pnl_listeners.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_SMARTORDERROUTING_PNL_LISTENERS_H
#define BASE_SMARTORDERROUTING_PNL_LISTENERS_H

#include "dvccode/CDef/defines.hpp"

namespace HFSAT {

/** @brief interface for listeners of change in global_pnl_ .. PromPNL**/
class GlobalPNLChangeListener {
 public:
  virtual ~GlobalPNLChangeListener(){};
  virtual void OnGlobalPNLChange(double _new_PNL_) = 0;
};
}
#endif  // BASE_SMARTORDERROUTING_PNL_LISTENERS_H
