/**
    \file ExternalDataCode/historical_dispatcher.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2016
     Address:
         Suite No 354, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include <algorithm>
#include "basetrade/hftrap/MinuteBarDispatcher/minute_bar_dispatcher.hpp"

namespace HFSAT {

MinuteBarDispatcher::~MinuteBarDispatcher() { DeleteSources(); }

void MinuteBarDispatcher::DeleteSources() {
  for (auto i = 0u; i < external_data_listener_vec_.size(); i++) {
    if (external_data_listener_vec_[i] != NULL) {
      delete external_data_listener_vec_[i];
    }
  }
  external_data_listener_vec_.clear();
  for (auto i = 0u; i < prev_external_data_listener_vec_.size(); i++) {
    if (prev_external_data_listener_vec_[i] != NULL) {
      delete prev_external_data_listener_vec_[i];
    }
  }
  prev_external_data_listener_vec_.clear();
}

void MinuteBarDispatcher::RequestNotifyOnLastEvent() { notify_on_last_event_ = true; }

void MinuteBarDispatcher::SubscribeStrategy(hftrap::eventdispatcher::MinuteBarEventsListener* alpha_) {
  event_listener_vec_.push_back(alpha_);
}

int MinuteBarDispatcher::getUniqueId(std::string instrument) {
  // Assigns a unique id to each instrument being added
  if (unique_instruments_map_.end() != unique_instruments_map_.find(instrument)) {
    std::cerr << "Can't Add Same Instrument Multiple Times : " << instrument << std::endl;
    return -1;
  }
  unique_instruments_map_[instrument] = unique_instruments_map_.size();
  return unique_instruments_map_[instrument];
}

void MinuteBarDispatcher::AddExternalDataListener(MidTermFileSource* _new_listener_) {
  VectorUtils::UniqueVectorAdd(external_data_listener_vec_, _new_listener_);
}

void MinuteBarDispatcher::NotifyListenersOnAllEventsConsumed() {
  for (uint32_t listener_counter = 0; listener_counter < event_listener_vec_.size(); listener_counter++) {
    event_listener_vec_[listener_counter]->OnAllEventsConsumed();
  }
}

void MinuteBarDispatcher::NotifyListenersWithEvent(struct hftrap::defines::MbarEvent const& event,
                                                   bool is_last_event_of_bar) {
  std::string underlying = std::string(event.instrument).substr(0, std::string(event.instrument).find_first_of("_"));
  int32_t inst_id = unique_instruments_map_[underlying];
  // Notifies the data to each event listener
  for (uint32_t listener_counter = 0; listener_counter < event_listener_vec_.size(); listener_counter++) {
    // std::cout<<"Inst id: "<<inst_id<<"\tTime: "<<event.event_time<<"\tIslast: "<< is_last_event_of_bar <<"\tExpiry:
    // "<<event.expiry_date<<std::endl;
    event_listener_vec_[listener_counter]->OnBarUpdate(inst_id, event.event_time,
                                                       event.instrument[strlen(event.instrument) - 1] == '0',
                                                       event.expiry_date, event.close_price, is_last_event_of_bar);
  }
}

void MinuteBarDispatcher::RunHist(const int start_date, const int end_date /*= DateTime::GetTimeUTC(20301231, 0000)*/) {
  struct tm start_timeinfo = {0};
  struct tm end_timeinfo = {0};
  start_timeinfo.tm_year = (start_date / 10000) - 1900;
  start_timeinfo.tm_mon = ((start_date / 100) % 100) - 1;
  start_timeinfo.tm_mday = start_date % 100;
  // Start Time
  time_t start_time_of_start_day = mktime(&start_timeinfo);

  end_timeinfo.tm_year = (end_date / 10000) - 1900;
  end_timeinfo.tm_mon = ((end_date / 100) % 100) - 1;
  end_timeinfo.tm_mday = end_date % 100;

  // End Time
  time_t end_time_of_end_day = mktime(&end_timeinfo);

  // Basically making end-date inclusive
  end_time_of_end_day += (24 * 3600);
  time_t start_time = start_time_of_start_day;
  time_t end_time = end_time_of_end_day;

  // Get to  the first data point from start_time
  for (MidTermFileSourcePtrVecIter _iter_ = external_data_listener_vec_.begin();
       _iter_ != external_data_listener_vec_.end();) {
    bool has_events = true;
    (*_iter_)->SeekToFirstEventAfter(start_time, has_events);

    if (!has_events) {
      prev_external_data_listener_vec_.push_back(*_iter_);
      _iter_ = external_data_listener_vec_.erase(_iter_);
    } else
      _iter_++;
  }

  time_t min_time;
  std::vector<hftrap::defines::MbarEvent> events_vec;
  std::vector<hftrap::defines::MbarEvent> new_events_vec;

  while (external_data_listener_vec_.size() > 0) {
    min_time = 10000000000;
    events_vec.clear();
    new_events_vec.clear();
    // Finds the minimum time from all the file sources
    for (MidTermFileSourcePtrVecIter _iter_ = external_data_listener_vec_.begin();
         _iter_ != external_data_listener_vec_.end();) {
      if ((*_iter_)->curr_event_timestamp() <= end_time) {
        if ((*_iter_)->curr_event_timestamp() <= min_time) min_time = (*_iter_)->curr_event_timestamp();
        _iter_++;
      } else {
        prev_external_data_listener_vec_.push_back(*_iter_);
        _iter_ = external_data_listener_vec_.erase(_iter_);
      }
    }
    if (min_time == 10000000000) break;

    // Gets the event vector for that time from all filesources and appends the vectors to events_vec
    for (MidTermFileSourcePtrVecIter _iter_ = external_data_listener_vec_.begin();
         _iter_ != external_data_listener_vec_.end();) {
      bool source_has_events = true;
      if ((*_iter_)->curr_event_timestamp() == min_time) {
        new_events_vec = (*_iter_)->GetCurrentEventVector(min_time);
        events_vec.insert(events_vec.end(), new_events_vec.begin(), new_events_vec.end());
        source_has_events = (*_iter_)->SetNextEventVector();
      }

      if (false == source_has_events) {
        prev_external_data_listener_vec_.push_back(*_iter_);
        _iter_ = external_data_listener_vec_.erase(_iter_);
      } else {
        _iter_++;
      }
    }

    //
    for (uint32_t event_counter = 0; event_counter < events_vec.size(); event_counter++) {
      NotifyListenersWithEvent(events_vec[event_counter], (event_counter != events_vec.size() - 1));
    }
  }

  if (true == notify_on_last_event_) {
    NotifyListenersOnAllEventsConsumed();
  }
}
}
