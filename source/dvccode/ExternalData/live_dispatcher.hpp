/**
    \file dvccode/ExternalData/live_dispatcher.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_EXTERNALDATA_LIVE_DISPATCHER_H
#define BASE_EXTERNALDATA_LIVE_DISPATCHER_H

#include <vector>
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/ExternalData/external_data_live_listener.hpp"

namespace HFSAT {

/// Livetrading, takes the socket file descriptiors and selects on them
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
class LiveDispatcher {
 protected:
  ExternalDataLiveListenerPtrVec external_data_live_listener_vec_;

 public:
  /// Contructor
  LiveDispatcher() : external_data_live_listener_vec_() {}

  ~LiveDispatcher();

  void DeleteSources();

  void AddExternalDataLiveListener(
      ExternalDataLiveListener* _new_listener_);  ///< call to add a source that dispatcher will need to work with

  /// called from main after all the sources have been added
  void RunLive();
};
}
#endif  // BASE_EXTERNALDATA_LIVE_DISPATCHER_H
