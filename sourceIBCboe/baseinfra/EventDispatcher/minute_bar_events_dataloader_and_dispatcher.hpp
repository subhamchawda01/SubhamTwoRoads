// =====================================================================================
//
//       Filename:  minute_bar_events_dataloader_and_dispatcher.hpp
//
//    Description:  A simple class to load given data and keep it ready to dispatch when required
//                  Follows a protocol for medium term data callbacks and not generalized
//
//        Version:  1.0
//        Created:  04/05/2016 03:53:32 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <sys/time.h>

#include "baseinfra/EventDispatcher/mbar_events.hpp"
#include "baseinfra/EventDispatcher/minute_bar_data_filenamer.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"  // From hfsat
#include "baseinfra/EventDispatcher/minute_bar_events_listener.hpp"

namespace hftrap {
namespace eventdispatcher {

// TODO - Fork out a separate file for cpp later
class MinuteBarDataLoaderAndDispatcher {
 private:
  // Not concerned about latency, Moreover Assuming total events/times would be less than 100k
  std::map<int32_t, std::vector<hftrap::defines::MbarEvent>*> time_to_events_map_;
  std::map<std::string, int32_t> unique_instruments_map_;  // unique instrument map to its inst id
  std::vector<hftrap::eventdispatcher::MinuteBarEventsListener*> event_listener_vec_;
  bool notify_on_last_event_;

  MinuteBarDataLoaderAndDispatcher()
      : time_to_events_map_(), unique_instruments_map_(), event_listener_vec_(), notify_on_last_event_(false) {}

  MinuteBarDataLoaderAndDispatcher(MinuteBarDataLoaderAndDispatcher const& disabled_copy_constructor) = delete;

 public:
  // Don't expect to have more than one dispatcher in the lifespace of program, do we ?
  static MinuteBarDataLoaderAndDispatcher& GetUniqueInstance() {
    static MinuteBarDataLoaderAndDispatcher unique_instance;
    return unique_instance;
  }

  void RequestNotifyOnLastEvent() { notify_on_last_event_ = true; }

  // To Add listeners
  void AddEventListeners(hftrap::eventdispatcher::MinuteBarEventsListener* minute_bar_events_listeners) {
    event_listener_vec_.push_back(minute_bar_events_listeners);
  }

  void NotifyListenersWithEvent(struct hftrap::defines::MbarEvent const& event, bool is_last_event_of_bar) {
    std::string underlying = std::string(event.instrument).substr(0, std::string(event.instrument).find_first_of("_"));
    int32_t inst_id = unique_instruments_map_[underlying];

    for (uint32_t listener_counter = 0; listener_counter < event_listener_vec_.size(); listener_counter++) {
      event_listener_vec_[listener_counter]->OnBarUpdate(inst_id, event.event_time,
                                                         event.instrument[strlen(event.instrument) - 1] == '0',
                                                         event.expiry_date, event.close_price, is_last_event_of_bar);
    }
  }

  void NotifyListenersOnAllEventsConsumed() {
    for (uint32_t listener_counter = 0; listener_counter < event_listener_vec_.size(); listener_counter++) {
      event_listener_vec_[listener_counter]->OnAllEventsConsumed();
    }
  }

  // will be called as needed to load data for required products
  bool AddData(std::string const& underlying, char const& segment, int32_t const& start_date, int32_t const& end_date,
               std::string expiry_list = "ALL") {
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

    // First check whether instrument is already added
    // Ideally would have liked to use the VectorUtils class from HFSAT but that'll fetch some more
    // dependent classes
    if (unique_instruments_map_.end() != unique_instruments_map_.find(underlying)) {
      std::cerr << "Can't Add Same Instrument Multiple Times : " << underlying << " Segment : " << segment << std::endl;
      return false;
    }

    unique_instruments_map_[underlying] = unique_instruments_map_.size();

    // Get file from filenamer
    std::string input_datafile = hftrap::loggedsources::MinuteBarDataFileNamer::GetFileName(underlying, segment);

    // Check whether file can be loaded or not
    std::ifstream datafile_stream;
    datafile_stream.open(input_datafile.c_str(), std::ifstream::in);

    if (false == datafile_stream.is_open()) {
      std::cerr << "Unable To Open The DataFile For Reading Data : " << input_datafile
                << " For Given Underlying : " << underlying << " Segment : " << segment << std::endl;
      return false;
    }

    // Filtering logic
    bool are_we_filtering = false;

    // Max Expiry Can be 9
    char list_of_expiries_to_consider[9];
    for (int32_t i = 0; i < 9; i++) {
      list_of_expiries_to_consider[i] = 'N';
    }

    if (std::string("ALL") != expiry_list) {
      are_we_filtering = true;
      std::vector<std::string> list_of_expiry_strings;
      HFSAT::PerishableStringTokenizer::StringSplit(expiry_list, ',', list_of_expiry_strings);

      // copying strings to char to make comparions into char later on
      for (auto& itr : list_of_expiry_strings) {
        list_of_expiries_to_consider[atoi(itr.c_str())] = 'Y';
      }
    }

#define MAX_LINE_SIZE 1024
    // Load file
    char line_buffer[MAX_LINE_SIZE];
    int32_t number_of_events_loaded = 0;

    while (datafile_stream.good()) {
      memset((void*)line_buffer, 0, MAX_LINE_SIZE);
      datafile_stream.getline(line_buffer, MAX_LINE_SIZE);

      // Skip comments in case we are adding in datafile
      if (std::string::npos != std::string(line_buffer).find("#")) continue;

      HFSAT::PerishableStringTokenizer pst = HFSAT::PerishableStringTokenizer(line_buffer, MAX_LINE_SIZE);
      std::vector<char const*> const& tokens = pst.GetTokens();

      // Not the one we expect, must be something wrong in the datafile or could be the end of file
      if (NSE_MEDIUMTERM_MBAR_DATA_FORMAT_TOKENS != tokens.size() &&
          NSE_MEDIUMTERM_MBAR_DATA_FORMAT_TOKENS + 1 != tokens.size())
        continue;  // adjusted data files have more tokens

      time_t data_time = (time_t)(atoi(tokens[0]));

      if (data_time < start_time_of_start_day) continue;  // Skip data until given date
      if (data_time > end_time_of_end_day) {
        return (number_of_events_loaded > 0);
      }  // Break out as we have loaded all events

      // okay, we are going to ignore some expiry here
      if (true == are_we_filtering) {
        int32_t this_expiry = tokens[1][strlen(tokens[1]) - 1] - '0';
        if ('N' == list_of_expiries_to_consider[this_expiry]) continue;
      }

      // Load up a single event and put in time sorted map
      // TODO - if number of events are too high, consider moving away from map/vector based storage
      hftrap::defines::MbarEvent mbar_event;
      memset((void*)&mbar_event, 0, sizeof(hftrap::defines::MbarEvent));

      mbar_event.event_time = atoi(tokens[0]);

      if (strlen(tokens[1]) >= 32) {
        std::cerr << "One Of Our Assumption Of Max Instrument Length Has Broken Here : " << tokens[1]
                  << " With length : " << strlen(tokens[1]) << std::endl;
        exit(-1);
      }

      // copy instrument
      memcpy((void*)mbar_event.instrument, (void*)tokens[1], strlen(tokens[1]));
      mbar_event.first_trade_time = atoi(tokens[2]);
      mbar_event.last_trade_time = atoi(tokens[3]);
      mbar_event.expiry_date = atoi(tokens[4]);
      mbar_event.open_price = atof(tokens[5]);
      mbar_event.close_price = atof(tokens[6]);
      mbar_event.low_price = atof(tokens[7]);
      mbar_event.high_price = atof(tokens[8]);
      mbar_event.total_volume = atoi(tokens[9]);
      mbar_event.no_of_trades = atoi(tokens[10]);

      if (time_to_events_map_.end() == time_to_events_map_.find((int32_t)mbar_event.event_time)) {
        std::vector<hftrap::defines::MbarEvent>* new_bar_event_time_vec = new std::vector<hftrap::defines::MbarEvent>();
        time_to_events_map_[(int32_t)mbar_event.event_time] = new_bar_event_time_vec;
      }

      std::vector<hftrap::defines::MbarEvent>* this_vec = time_to_events_map_[(int32_t)mbar_event.event_time];
      this_vec->push_back(mbar_event);

      number_of_events_loaded++;
    }

    datafile_stream.close();

#undef MAX_LINE_SIZE

    return true;
  }

  // Main function to dispatch events
  void DispatchEvents() {
    for (auto& map_itr : time_to_events_map_) {
      std::vector<hftrap::defines::MbarEvent>& this_event_vec = *(time_to_events_map_[map_itr.first]);

      for (uint32_t event_counter = 0; event_counter < this_event_vec.size(); event_counter++) {
        NotifyListenersWithEvent(this_event_vec[event_counter], (event_counter != this_event_vec.size() - 1));
      }
    }

    if (true == notify_on_last_event_) {
      NotifyListenersOnAllEventsConsumed();
    }
  }
};
}
}
