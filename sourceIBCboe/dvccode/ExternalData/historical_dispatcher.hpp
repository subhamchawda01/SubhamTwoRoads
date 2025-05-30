/**
    \file dvccode/ExternalData/historical_dispatcher.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_EXTERNALDATA_HISTORICAL_DISPATCHER_H
#define BASE_EXTERNALDATA_HISTORICAL_DISPATCHER_H

#include <vector>
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/ExternalData/external_data_listener.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
namespace HFSAT {

/// gets a list of sources
/// In Historical
/// for the ExternalDataListener to know the time of the event first
/// ask them to read the data and depending on whether there is any data of interest
/// ( for instance it could have data but of securities that are not of interest ),
/// compute the next_event_timestamp
///
/// Once the sources have next_event_timestamp two choices here :
/// (i) sort the events based on time and based on the event time call the corresponding ExternalDataListener to process
/// that event
///     and fetch the next_event_timestamp
///     and resort if the next_event_timestamp is != 0
/// (ii) all the sources that have some data in the channels, sequentially call them to process all data they have
///      and call any listeners as and when they feel a need to
/// downsides of method (ii) :
///    (a) if the time we take in processing events is very high then events from other sources could be ignored.
///    (b) events are sure to not be chronological in live and hence different in hist and live
///
/// At the end HistoricalDispatcher collects all sources in prev_external_data_listener_vec_
/// and calls delete on them
class HistoricalDispatcher {
 protected:
  ExternalDataListenerPtrVec external_data_listener_vec_;

  ///< only used to collect sources that are not of interest any more, so that they can be deallocated later
  ExternalDataListenerPtrVec prev_external_data_listener_vec_;
  bool first_event_enqueued_;

 public:
  /// Contructor
  HistoricalDispatcher()
      : external_data_listener_vec_(), prev_external_data_listener_vec_(), first_event_enqueued_(false) {}

  ~HistoricalDispatcher();

  void SeekHistFileSourcesTo(const ttime_t r_endtime_);

  void DeleteSources();

  ///< call to add a source that dispatcher will need to work with
  void AddExternalDataListener(ExternalDataListener* _new_listener_, bool smm_filesource = false);

  ///< called from main after all the sources have been added
  void RunHist(const ttime_t _end_time_ = ttime_t(DateTime::GetTimeUTC(20301231, 0), 0));
};
}
#endif  // BASE_EXTERNALDATA_HISTORICAL_DISPATCHER_H
