/**
    \file ExternalDataCode/live_dispatcher.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include <algorithm>
#include "dvccode/ExternalData/live_dispatcher.hpp"

namespace HFSAT {

#define USING_HEAP 1  // using STL heap algorithm to get the earliest tmestamp source in historical

LiveDispatcher::~LiveDispatcher() { DeleteSources(); }

void LiveDispatcher::DeleteSources() {
  for (auto i = 0u; i < external_data_live_listener_vec_.size(); i++) {
    if (external_data_live_listener_vec_[i] != NULL) {
      delete external_data_live_listener_vec_[i];
    }
  }
  external_data_live_listener_vec_.clear();
}

void LiveDispatcher::AddExternalDataLiveListener(ExternalDataLiveListener* _new_listener_) {
  VectorUtils::UniqueVectorAdd(external_data_live_listener_vec_, _new_listener_);
}

/// In livetrading, takes the socket file descriptiors and selects on them
/// and for the ones that returns true on FD_ISSET
/// calls them to process the data they have.
/// for the ExternalDataLiveListener to know the time of the event first
/// ask them to read the data and depending on whether there is any data of interest
/// ( for instance it couldhave data but of securities that are not of interest ),
/// compute the next_event_timestamp
///
/// Once the sources have next_event_timestamp two choices here :
/// (i) sort the events based on time and based on the event time call the corresponding ExternalDataLiveListener to
/// process that event
///     and fetch the next_event_timestamp
///     and resort if the next_event_timestamp is != 0
/// (ii) all the sources that have some data in the channels, sequentially call them to process all data they have
///      and call any listeners as and when they feel a need to
/// downsides of method (ii) :
///    (a) if the time we take in processing events is very high then events from other sources could be ignored.
///    (b) events are sure to not be chronological in live and hence different in hist and live
/// Using (i) if more than 1 event source returned true
void LiveDispatcher::RunLive() {
  fd_set socketReadSet;
  int max_socket_file_descriptor_ = 0;

  FD_ZERO(&socketReadSet);

  for (auto i = 0u; i < external_data_live_listener_vec_.size(); i++) {
    FD_SET(external_data_live_listener_vec_[i]->socket_file_descriptor(), &socketReadSet);
    max_socket_file_descriptor_ =
        std::max(max_socket_file_descriptor_, external_data_live_listener_vec_[i]->socket_file_descriptor());
  }

  max_socket_file_descriptor_++;  // somehow 'select()' likes maxfd+1
  while (1) {
    int number_of_sockets_with_data_ = select(max_socket_file_descriptor_, &socketReadSet, 0, 0, NULL);
    if (number_of_sockets_with_data_ == -1) {
      return;  // break ... some error ... ?
    } else {
      if (number_of_sockets_with_data_ == 1) {  // number of sockets with any sort of data is 1
        for (auto i = 0u; i < external_data_live_listener_vec_.size(); i++) {
          if (FD_ISSET(external_data_live_listener_vec_[i]->socket_file_descriptor(), &socketReadSet) != 0) {
            bool _hasevents_ = false;
            external_data_live_listener_vec_[i]->ComputeEarliestDataTimestamp(_hasevents_);
            if (_hasevents_) {  // this sockets has data of securities that are of interest to us.
              external_data_live_listener_vec_[i]->ProcessAllEvents();
            }
            FD_SET(external_data_live_listener_vec_[i]->socket_file_descriptor(), &socketReadSet);
          } else {
            FD_SET(external_data_live_listener_vec_[i]->socket_file_descriptor(), &socketReadSet);
          }
        }
      } else {
        // more than one source has data
        ExternalDataLiveListenerPtrVec sources_with_events_currently_vec_;

        for (auto i = 0u; i < external_data_live_listener_vec_.size(); i++) {
          if (FD_ISSET(external_data_live_listener_vec_[i]->socket_file_descriptor(), &socketReadSet) != 0) {
            bool _hasevents_ = true;
            // call ComputeEarliestDataTimestamp to have the first timestamp to start sorting on
            // also checks if the event in this source is of interest or not
            external_data_live_listener_vec_[i]->ComputeEarliestDataTimestamp(_hasevents_);
            if (_hasevents_) {
              // if it has an event of interst then add it to vec
              sources_with_events_currently_vec_.push_back(external_data_live_listener_vec_[i]);
            }
          }
          FD_SET(external_data_live_listener_vec_[i]->socket_file_descriptor(), &socketReadSet);
        }

        if (!sources_with_events_currently_vec_.empty()) {
          if (sources_with_events_currently_vec_.size() == 1) {
            sources_with_events_currently_vec_.front()->ProcessAllEvents();  // Since ComputeEarliestDataTimestamp ()
                                                                             // has been called the time stamp will be
                                                                             // valid
            sources_with_events_currently_vec_
                .pop_back();  // remove the sole listener since it has already processed all the events it had
          } else {
            // TODO should USING_HEAP help or hurt given that sources_with_events_currently_vec_.size() is expected to
            // be small ?

            // for this step the next_event_timestamp should be set ( with nonzero values )
            std::sort(sources_with_events_currently_vec_.begin(), sources_with_events_currently_vec_.end(),
                      ExternalDataLiveListenerPtrLess);
            // sources_with_events_currently_vec_ is sorted coming in
            while (!sources_with_events_currently_vec_.empty()) {
              ExternalDataLiveListener* _top_edl_ = sources_with_events_currently_vec_.front();
              _top_edl_->ProcessEventsTill(sources_with_events_currently_vec_[1]->next_event_timestamp());

              const ttime_t next_event_timestamp_from_edl =
                  _top_edl_->next_event_timestamp();  // ask the source to get the next_event_timestamp ( the first
                                                      // event that is older than the specified endtime )
              bool _hasevents_ = (next_event_timestamp_from_edl.tv_sec != 0);
              sources_with_events_currently_vec_.erase(
                  sources_with_events_currently_vec_.begin());  // remove top listener since it is certain to be not the
                                                                // earliest timestamp source any more

              if (_hasevents_) {
                // if this source still has events ... insert it in the place where this_source->next_event_timestamp <=
                // the next_iter_source->next_event_timestamp
                ExternalDataLiveListenerPtrVecIter _iter_ = sources_with_events_currently_vec_.begin();
                for (; (_iter_ != sources_with_events_currently_vec_.end()) &&
                       ((*_iter_)->next_event_timestamp() < next_event_timestamp_from_edl);
                     _iter_++) {
                }
                sources_with_events_currently_vec_.insert(_iter_, _top_edl_);
              } else {
                // this source does not have any more events
                if (sources_with_events_currently_vec_.size() == 1) {
                  sources_with_events_currently_vec_.front()->ProcessAllEvents();  // Since ComputeEarliestDataTimestamp
                                                                                   // () has been called, or
                                                                                   // ProcessEventsTill has been called,
                                                                                   // the next_event_timestamp will be
                                                                                   // valid
                  sources_with_events_currently_vec_
                      .pop_back();  // remove the sole listener since it has already processed all the events it had
                  break;
                }
                if (sources_with_events_currently_vec_.empty()) {
                  break;
                }
              }
            }
          }
        }
      }
    }
  }
}

#undef USING_HEAP
}
