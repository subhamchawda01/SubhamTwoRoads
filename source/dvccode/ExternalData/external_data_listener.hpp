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
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/ExternalData/external_data_listener_listener.hpp"

namespace HFSAT {

class ExternalDataListener {
 protected:
  ttime_t next_event_timestamp_; /**< timestamp of the next event to be sent to listeners */
  std::vector<ExternalDataListenerListener*> external_data_listener_listener_vec_;

 public:
  // Priority field is used to ensure that in case of same timestamps, order level feed messages are processed first
  // This ensures similar strat results while clubbing
  int priority_;
  void SetHighPriority() { priority_ = 0; }
  int GetPriority() { return priority_; }

  ExternalDataListener() : external_data_listener_listener_vec_(), priority_(1) {}

  virtual ~ExternalDataListener() {}

  /// meant for live_data_sources, so that one can select on them
  virtual int socket_file_descriptor() const = 0;

  /// reads and discards events till given ttime_t
  virtual void SeekToFirstEventAfter(const ttime_t r_endtime_, bool& rw_hasevents_) = 0;

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

  inline void NotifyExternalDataListenerListener(const unsigned int _sec_id_) {
    for (auto& t_external_data_listener_listener : external_data_listener_listener_vec_) {
      t_external_data_listener_listener->SetSecId(_sec_id_);
    }
  }

  inline void AddExternalDataListenerListener(ExternalDataListenerListener* t_external_data_listener_listener_) {
    if (t_external_data_listener_listener_ != NULL) {
      if (find(external_data_listener_listener_vec_.begin(), external_data_listener_listener_vec_.end(),
               t_external_data_listener_listener_) == external_data_listener_listener_vec_.end()) {
        external_data_listener_listener_vec_.push_back(t_external_data_listener_listener_);
      }
    }
  }
};

typedef ExternalDataListener* ExternalDataListenerPtr;
typedef std::vector<ExternalDataListener*> ExternalDataListenerPtrVec;
typedef std::vector<ExternalDataListener*>::iterator ExternalDataListenerPtrVecIter;
typedef std::vector<ExternalDataListener*>::const_iterator ExternalDataListenerPtrVecConstIter;

inline bool ExternalDataListenerPtrLess(const ExternalDataListenerPtr& ed1, const ExternalDataListenerPtr& ed2) {
  return (ed1->next_event_timestamp() < ed2->next_event_timestamp()) ||
         ((ed2->next_event_timestamp() == ed1->next_event_timestamp()) && (ed2->GetPriority() > ed1->GetPriority()));
}

inline bool ExternalDataListenerPtrGreater(const ExternalDataListenerPtr& ed1, const ExternalDataListenerPtr& ed2) {
  return (ed2->next_event_timestamp() < ed1->next_event_timestamp()) ||
         ((ed2->next_event_timestamp() == ed1->next_event_timestamp()) && (ed2->GetPriority() < ed1->GetPriority()));
}
}
