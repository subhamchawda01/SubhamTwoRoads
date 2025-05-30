/**
    \file dvccode/ExternalData/external_data_live_listener.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_EXTERNALDATA_EXTERNAL_DATA_LIVE_LISTENER_H
#define BASE_EXTERNALDATA_EXTERNAL_DATA_LIVE_LISTENER_H

#include <sys/time.h>
#include "dvccode/CDef/ttime.hpp"

namespace HFSAT {

class ExternalDataLiveListener {
 protected:
  ttime_t next_event_timestamp_; /**< timestamp of the next event to be sent to listeners */

 public:
  virtual ~ExternalDataLiveListener() {}

  /// meant for live_data_sources, so that one can select on them
  virtual int socket_file_descriptor() const = 0;

  /// looks at events in file or in socket to see what is the earliest event that is of interest, and sets variable
  /// next_event_timestamp_. when called there is data in multicast_receiver_socket_ to process, a lot of data need not
  /// translate to events ... when no event turn _hasevents_ = false
  virtual void ComputeEarliestDataTimestamp(bool& rw_hasevents_) = 0;

  /// processes all events it can read. Typically there should be some data already read in the buffer.
  virtual void ProcessAllEvents() = 0;

  /// processes all events it has till the time _endtime_.
  /// Typically there should be at least 1 event.
  /// Also in Historical fetch the next event after the specified time and thus precompute next_event_timestamp_
  virtual void ProcessEventsTill(const ttime_t r_endtime_) = 0;

  inline const ttime_t next_event_timestamp() const { return next_event_timestamp_; }
};

typedef ExternalDataLiveListener* ExternalDataLiveListenerPtr;
typedef std::vector<ExternalDataLiveListener*> ExternalDataLiveListenerPtrVec;
typedef std::vector<ExternalDataLiveListener*>::iterator ExternalDataLiveListenerPtrVecIter;
typedef std::vector<ExternalDataLiveListener*>::const_iterator ExternalDataLiveListenerPtrVecConstIter;

inline bool ExternalDataLiveListenerPtrLess(const ExternalDataLiveListenerPtr& ed1,
                                            const ExternalDataLiveListenerPtr& ed2) {
  return (ed1->next_event_timestamp() < ed2->next_event_timestamp());
}

inline bool ExternalDataLiveListenerPtrGreater(const ExternalDataLiveListenerPtr& ed1,
                                               const ExternalDataLiveListenerPtr& ed2) {
  return (ed2->next_event_timestamp() < ed1->next_event_timestamp());
}
}

#endif  // BASE_EXTERNALDATA_EXTERNAL_DATA_LIVE_LISTENER_H
