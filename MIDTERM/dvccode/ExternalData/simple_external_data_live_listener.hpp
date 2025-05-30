/**
    \file dvccode/ExternalData/simple_external_data_live_listener.hpp

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

class SimpleExternalDataLiveListener {
 public:
  virtual ~SimpleExternalDataLiveListener() {}

  /// processes all events it can read. Typically there should be some data already read in the buffer.
  virtual void ProcessAllEvents(int this_socket_fd_) = 0;
  virtual void CleanUp() {}
};

typedef SimpleExternalDataLiveListener* SimpleExternalDataLiveListenerPtr;
typedef std::vector<SimpleExternalDataLiveListener*> SimpleExternalDataLiveListenerPtrVec;
typedef std::vector<SimpleExternalDataLiveListener*>::iterator SimpleExternalDataLiveListenerPtrVecIter;
typedef std::vector<SimpleExternalDataLiveListener*>::const_iterator SimpleExternalDataLiveListenerPtrVecConstIter;
}
