/**
    \file dvccode/ExternalData/external_data_listener.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#pragma once

#include <sys/time.h>

namespace HFSAT {

class ExternalDataListenerListener {
 protected:
  std::vector<std::pair<int, ExternalDataListenerListener*>> external_data_listener_vec_;

 public:
  ExternalDataListenerListener() {}
  virtual ~ExternalDataListenerListener() {}
  virtual void SetSecId(int sec_id_) {}
};
}
