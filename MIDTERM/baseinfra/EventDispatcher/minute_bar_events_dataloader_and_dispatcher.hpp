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
//        					Old Madras Road, Near Garden City College,
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
#include <stdlib.h>
#include <ctime>
#include "dvccode/CDef/mbar_events.hpp"
#include "baseinfra/EventDispatcher/minute_bar_data_filenamer.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp" 
#include "baseinfra/EventDispatcher/minute_bar_events_listener.hpp"

namespace hftrap {
namespace eventdispatcher {

// TODO - Fork out a separate file for cpp later
class MinuteBarDataLoaderAndDispatcher {
 private:
  // Not concerned about latency, Moreover Assuming total events/times would be less than 100k
  std::map<int32_t, std::vector<hftrap::defines::MbarEvent>*> time_to_events_map_;
  std::map<std::string, int32_t> unique_instruments_map_;  // unique instrument map to its inst id
  int32_t start_date_;
  int32_t end_date_;

  MinuteBarDataLoaderAndDispatcher() : time_to_events_map_(), unique_instruments_map_(){}

  MinuteBarDataLoaderAndDispatcher(MinuteBarDataLoaderAndDispatcher const& disabled_copy_constructor) = delete;

 public:
  // Don't expect to have more than one dispatcher in the lifespace of program, do we ?
  static MinuteBarDataLoaderAndDispatcher& GetUniqueInstance() {
    static MinuteBarDataLoaderAndDispatcher unique_instance;
    return unique_instance;
  }
  // will be called as needed to load data for required products
  bool AddData(std::string const& underlying, char const& segment, int32_t const& start_date, int32_t const& end_date,
               std::string expiry_list = "ALL") {
    start_date_ = start_date;
    end_date_ = end_date;
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
    std::cout << input_datafile << std::endl;
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
      if (NSE_MEDIUMTERM_MBAR_DATA_FORMAT_TOKENS != tokens.size()) continue;

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

  // Function to back-adjust data
  void BackAdjust( std::string index_file, std::string outfile, bool include_header = false ) {

    std::map<int32_t, double> adjustments_map;  // Map from time to adjustment ratio, all timestamps prior to this will have this adj factor
    std::vector<int> keys_vec;                  // Contains the adjustments_map keys(timestamps)
    bool is_expiry = false;                     // true on expiry day and set false to the very next bar after expiry day
    bool cache_fut1_data = false;               // flag set on expiry day so that last fut_1 traded price is recorded
    hftrap::defines::MbarEvent last_event_0;    // FUT_0
    hftrap::defines::MbarEvent last_event_1;    // FUT_1


    // Get the adjustment ratios map ready
    for (auto& map_itr : time_to_events_map_) {
      std::vector<hftrap::defines::MbarEvent>& this_event_vec = *(time_to_events_map_[map_itr.first]);
      time_t t = map_itr.first;
      struct tm *tm = localtime(&t);
      char date[20];
      strftime( date, sizeof(date), "%Y%m%d", tm );
      uint32_t date_ = std::atoi( date );

      for (uint32_t event_counter = 0; event_counter < this_event_vec.size(); event_counter++) {
        hftrap::defines::MbarEvent& event_ = this_event_vec[event_counter];
 	//std::cout << event_.instrument << std::endl;
	if (event_.instrument[strlen(event_.instrument) - 2] != '_') {
		//std::cout << "The event is -> " << event_.instrument << std::endl;
		continue;
        }
	if (!(event_.instrument[strlen(event_.instrument) - 1] == '0' || event_.instrument[strlen(event_.instrument) - 1] == '1'))
		 continue;
	

        // Case when we are looking at expiry..
        if( date_ == event_.expiry_date && ( event_.instrument[strlen(event_.instrument) - 1] == '0' ) && ( event_.instrument[strlen(event_.instrument) - 2] == '_' )) {
          is_expiry = true;
          cache_fut1_data = true;
          if( event_.instrument[strlen(event_.instrument) - 1] == '0' && ( event_.instrument[strlen(event_.instrument) - 2] == '_' ))
            //std::cout << "FUT0 is " << event_.instrument << std::endl;
            last_event_0 = event_;
        }
        // Cache FUT_1 data on the day after expiry
        if( cache_fut1_data && ( event_.instrument[strlen(event_.instrument) - 1] == '1' ) && ( event_.instrument[strlen(event_.instrument) - 2] == '_' )) {
          last_event_1 = event_;
        }
        // Case when we are on the first bar after expiry
        if( ( date_ != event_.expiry_date ) && is_expiry && cache_fut1_data && ( event_.instrument[strlen(event_.instrument) - 1] == '0' ) && ( event_.instrument[strlen(event_.instrument) - 2] == '_' )) {
          is_expiry = false;
          cache_fut1_data = false;
          double adj_factor = last_event_1.close_price / last_event_0.close_price;
          adjustments_map.insert( std::make_pair( event_.event_time, adj_factor ) );
          keys_vec.push_back( event_.event_time );
        }
      }
    }
    // adjustments_map ready at this point


    // ADD CODE HERE FOR CORPORATE EVENTS
    std::ifstream corp_index_file_;
    corp_index_file_.open( index_file );
    std::string line;
    std::string underlying = std::string(last_event_0.instrument).substr(0, std::string(last_event_0.instrument).find_first_of("_"));
    std::ofstream outfile_;
    outfile_.open( outfile + "_temp" );
    //outfile_ << "#ADJUSTED DATA FOR " << underlying << " Generated from " << start_date_ << " to " << end_date_ << '\n';
    while( getline( corp_index_file_, line ) ) {

      std::vector<std::string> corp_tokens_;
      HFSAT::PerishableStringTokenizer::StringSplit( line, ',', corp_tokens_ );
      if( corp_tokens_[3] != underlying ) {
        continue;
      }
      std::cout << "#" << line << std::endl;

      // Get the adjustment factor when there is a corporate event
      // Special handling for termiantion reintroduction
      std::string adj_type = corp_tokens_[5];

      double adj_factor = -1;
      if( adj_type == "Termination_Reintroduction" ) {
        if( corp_tokens_[7] == "X" ) {
          std::cerr << "INVALID ADJUSTMENT FACTOR IN INDEX FILE WHEN HANDLING TERMINATION REINTRODUCTION..." << std::endl;
        }
        adj_factor = std::atof( corp_tokens_[ 7 ].c_str() );
      }
      else {
        if( corp_tokens_[4] != "X" ) {
          adj_factor = std::atof( corp_tokens_[4].c_str() );
        }
      }

      if( adj_factor == -1 ) {
        continue;
      }

      std::string ex_date = corp_tokens_[2];
      outfile_ << "#Processing " << adj_type << " on " << atoi( ex_date.c_str() ) << " with adjustment factor " << adj_factor << '\n';
      struct tm timeinfo = {0};
   
      int start_date = atoi( ex_date.c_str() );
      timeinfo.tm_year = (start_date / 10000) - 1900;
      timeinfo.tm_mon = ((start_date / 100) % 100) - 1;
      timeinfo.tm_mday = start_date % 100;

      // Start Time
      time_t corp_ex_time = mktime(&timeinfo);
      //std::cout << "Ex-Date is " << ex_date << std::endl;
      /*
      int year = std::atoi( ex_date.substr( 0, 4 ).c_str() );
      int month = std::atoi( ex_date.substr( 4, 2 ).c_str() );
      int day = std::atoi( ex_date.substr( 6, 2 ).c_str() );
      int hour, min, sec = 0;
      time_t rawtime;
      time ( &rawtime );
      struct tm timeinfo = {0} ;
      timeinfo = localtime ( &rawtime );
      std::cout << "Curr time is " << timeinfo.tm_year << '\t' << timeinfo.tm_mon << '\t' << timeinfo.tm_mday << std::endl;
      std::cout << year << '\t' << month << '\t' << day << std::endl;
      timeinfo.tm_year   = year - 1900;
      timeinfo.tm_mon    = month - 1;    //months since January - [0,11]
      timeinfo.tm_mday   = day;          //day of the month - [1,31] 
      timeinfo.tm_hour   = hour;         //hours since midnight - [0,23]
      timeinfo.tm_min    = min;          //minutes after the hour - [0,59]
      timeinfo.tm_sec    = sec;          //seconds after the minute - [0,59]
      std::cout << "New time is " << timeinfo.tm_year << '\t' << timeinfo.tm_mon << '\t' << timeinfo.tm_mday << std::endl;
      */
      //int corp_ex_time = mktime( &timeinfo );
      std::cout << "Corp ex time is : " << corp_ex_time << std::endl;
      adjustments_map.insert( std::make_pair( corp_ex_time, adj_factor ) );
      std::cout << "Corporate adjustment has taken place for " << underlying << " on " << ex_date.substr( 0, 8 ) << " with an adjustment of " << adj_factor << std::endl;
    }
 
    // Points where the back-adjustment has to be done have been identified
    // Now get the cumulative back adjustment ratio and update the map
    typedef std::map<int32_t, double>::iterator it_;
    for( it_ iterator = adjustments_map.begin(); iterator != adjustments_map.end(); iterator++ ) {
      //std::cout << iterator->first << '\t' << iterator->second << std::endl;
      for( auto i : adjustments_map ) {
        if( i.first > iterator->first ) {
          adjustments_map[iterator->first] *= i.second; 
        }
      }
    }
    // adjustments map contains the cumulative back adjustment ratios


    int last_timestamp = -1;                               // temp var to store last key(timestamp) from adjustments map
    double last_adj_factor = -1;                           // temp var to store last value(adj factor) from adjustments map
    int final_timestamp = adjustments_map.rbegin()->first; // last expiry time so that further data is not back adjusted
    if( include_header ) {
      std::time_t cur_time = std::time(nullptr);
      outfile_ << "#This data has been generated on: "<< std::asctime(std::localtime(&cur_time));
      outfile_ << "#TIMESTAMP\tSCRIP\t\tSTART_TIME\tEND_TIME\tEXPIRY\t\tOPEN\tCLOSE\tLOW\tHIGH\tVOLUME\tN_TRADES  ADJ_FACTOR\n";
    }
    for (auto& map_itr : time_to_events_map_) {
      std::vector<hftrap::defines::MbarEvent>& this_event_vec = *(time_to_events_map_[map_itr.first]);
      for (uint32_t event_counter = 0; event_counter < this_event_vec.size(); event_counter++) {
        hftrap::defines::MbarEvent& event_ = this_event_vec[event_counter];
        // Only back-adjust FUT0 data, let's handle other data later
        if( event_.instrument[strlen(event_.instrument) - 1] == '0' && event_.instrument[strlen(event_.instrument) - 2] == '_' ) {
          // Case when no need to back adjust
        //std::cout<<"Event time"<<event_.event_time<<"\tLast timestamp"<<last_timestamp<<std::endl;  
	if( event_.event_time >= final_timestamp ) {
            last_adj_factor = 1.0;
          }
          // back adjust with the value from the map
          
          else if( event_.event_time >= last_timestamp ) {
            for( auto i : adjustments_map ) {
              if( i.first > last_timestamp ) {
                last_adj_factor = i.second;
                last_timestamp = i.first;
                break;
		//std::cout<< "Lastadj" << last_adj_factor << " ";
              }
            }
	    //std::cout<<"\n";
          }
          event_.open_price *= last_adj_factor;
          event_.high_price *= last_adj_factor;
          event_.low_price *= last_adj_factor;
          event_.close_price *= last_adj_factor;

          //std::cout << event_.event_time << '\t'<< last_adj_factor << '\t' << event_.close_price << '\t' << event_.expiry_date <<std::endl;
          outfile_ << event_.event_time << '\t' << event_.instrument << '\t' << event_.first_trade_time << '\t' << 
                       event_.last_trade_time << '\t' << event_.expiry_date << '\t' << event_.open_price << '\t' << 
                       event_.close_price << '\t' << event_.low_price << '\t' << event_.high_price << '\t' << 
                       event_.total_volume << '\t' << event_.no_of_trades << '\t' << last_adj_factor << '\n';

        }
      }
    }
    // Now need to rename the file to original
    std::string mv_cmd = "mv \"" + outfile + "_temp\" \"" + outfile + "\"";
    system( mv_cmd.c_str() );
  }
};
}
}
