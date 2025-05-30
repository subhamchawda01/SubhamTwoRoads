/**
   \file VolatileTradingInfo/sweep_info_listener.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2016
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_VOLATILETRADINGINFO_SWEEP_INFO_LISTENER_H
#define BASE_VOLATILETRADINGINFO_SWEEP_INFO_LISTENER_H

#include <string>
#include "dvccode/CDef/defines.hpp"

namespace HFSAT {

// Sweep Info Listener
class SweepInfoListener {
 public:
  // SweepType : If sweep type is sell, it means someone sold heavily
  virtual void OnSweep(const unsigned int security_id, TradeType_t sweep_type) {}

  virtual ~SweepInfoListener(){};
};
}

#endif  // BASE_VOLATILETRADINGINFO_SWEEP_INFO_LISTENER_H
