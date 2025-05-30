/**
    \file dvccode/ExternalData/external_time_listener.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_EXTERNALDATA_EXTERNAL_TIME_LISTENER_H
#define BASE_EXTERNALDATA_EXTERNAL_TIME_LISTENER_H

#include <sys/time.h>
#include "dvccode/CDef/ttime.hpp"

namespace HFSAT {

/** @brief stub, currently implemented only by Watch. All listeners of external data call
 * ExternalTimeListener::OnTimeReceived on the vector<ExternalTimeListener*> on a new time point
 */
class ExternalTimeListener {
 public:
  virtual ~ExternalTimeListener() {}
  virtual void OnTimeReceived(const ttime_t _tv_, int sec_id_ = -1) = 0;
};

class TimePeriodListener {
 public:
  virtual ~TimePeriodListener() {}
  virtual void OnTimePeriodUpdate(const int num_pages_to_add_) = 0;
};
}

#endif  // BASE_EXTERNALDATA_EXTERNAL_DATA_LISTENER_H
