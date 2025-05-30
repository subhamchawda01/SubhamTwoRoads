/**
    \file ExternalData/minute_bar_dispatcher.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2016
     Address:
         Suite No 354, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/


#include <vector>
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "baseinfra/LoggedSources/minute_bar_filesource.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

namespace HFSAT {


class MinuteBarDispatcher {
 protected:
  MidTermFileSourcePtrVec external_data_listener_vec_;
  std::vector<hftrap::eventdispatcher::MinuteBarEventsListener*> event_listener_vec_;
  ///< only used to collect sources that are not of interest any more, so that they can be deallocated later
  MidTermFileSourcePtrVec prev_external_data_listener_vec_;

  bool notify_on_last_event_;
  std::map<std::string, int32_t> unique_instruments_map_;
  time_t last_processed_time = 0;
 public:
  /// Constructor
  MinuteBarDispatcher()
      : external_data_listener_vec_(),event_listener_vec_() , prev_external_data_listener_vec_(), notify_on_last_event_(false) {}

  ~MinuteBarDispatcher();

  static MinuteBarDispatcher& GetUniqueInstance() {
      static MinuteBarDispatcher unique_instance;
      return unique_instance;
    }

  int getUniqueId(std::string instrument);

  void RequestNotifyOnLastEvent();

  void NotifyListenersOnAllEventsConsumed();

  void DeleteSources();
  ///< call to add a source that dispatcher will need to work with
  void AddExternalDataListener(MidTermFileSource* _new_listener_);

  void SubscribeStrategy( hftrap::eventdispatcher::MinuteBarEventsListener* alpha_ );

  void NotifyListenersWithEvent(struct hftrap::defines::MbarEvent const& event, bool is_last_event_of_bar);

  ///< called from main after all the sources have been added
  void RunHist(int start_date , int end_date);//const time_t _end_time_ = time_t(DateTime::GetTimeUTC(20301231, 0)));
};
}

