/**
   file Tools/parse_fxstreet_events.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551

*/

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "dvccode/CommonTradeUtils/date_time.hpp"

#define MAX_LINE_SIZE 32768

#define EU_HOURS_MASK_SEVERITY_ 1

struct FxstreetEconomicEvents {
  std::string event_start_time_;
  std::string country_;
  std::string event_desc_;
  std::string actual_numbers_;
  std::string consensus_numbers_;
  std::string previous_numbers_;
  std::string severity_;

  FxstreetEconomicEvents() {
    event_start_time_ = country_ = event_desc_ = actual_numbers_ = consensus_numbers_ = previous_numbers_ = severity_ =
        "";
  }

  std::string ToCSV(int date_) {
    std::ostringstream t_temp_oss_;

    t_temp_oss_ << "\"" << country_ << "\",\"" << date_ << " " << event_start_time_ << ":00\",\"" << event_desc_
                << "\",\"" << actual_numbers_ << "\",\"" << consensus_numbers_ << "\",\"" << previous_numbers_
                << "\",\"" << severity_ << "\""
                << "\n";

    return t_temp_oss_.str();
  }

} typedef FxEcoEvents;

class FxstreetEcoEventsManager {
  std::string live_events_html_filename_;
  std::string processed_events_csv_filename_;
  std::string existing_processed_events_csv_filename_;

  std::string economic_events_calendar_table_;
  std::map<std::string, FxEcoEvents> existing_events_map_;
  std::map<std::string, FxEcoEvents> live_events_map_;

  std::vector<std::string> html_table_tokens_;
  std::vector<FxEcoEvents> fxstreet_events_;

 public:
  FxstreetEcoEventsManager(std::string live_html_file_, std::string output_file_)
      : live_events_html_filename_(live_html_file_),
        processed_events_csv_filename_(output_file_),
        existing_processed_events_csv_filename_(""),
        economic_events_calendar_table_(""),
        existing_events_map_(),
        live_events_map_(),
        html_table_tokens_(),
        fxstreet_events_() {}

  FxstreetEcoEventsManager(std::string live_html_file_, std::string output_file_, std::string existing_file_)
      : live_events_html_filename_(live_html_file_),
        processed_events_csv_filename_(output_file_),
        existing_processed_events_csv_filename_(existing_file_),
        economic_events_calendar_table_(""),
        existing_events_map_(),
        live_events_map_(),
        html_table_tokens_(),
        fxstreet_events_() {}

  void dumpEconomicEvents() {}

  void loadExistingEventsSet() {}

  void extractHTMLEcoEventsCalendarTableBody() {
    std::ifstream live_events_html_file_;
    live_events_html_file_.open(live_events_html_filename_.c_str(), std::ios::in);

    if (!live_events_html_file_.is_open()) {
      std::cerr << " File : " << live_events_html_filename_ << " does not exists " << std::endl;
      exit(-1);
    }

    char line_buffer_[MAX_LINE_SIZE];
    std::string line_read_ = "";
    bool table_start_ = false;

    while (live_events_html_file_.good()) {
      memset(line_buffer_, 0, sizeof(line_buffer_));
      line_read_ = "";

      live_events_html_file_.getline(line_buffer_, sizeof(line_buffer_));
      line_read_ = line_buffer_;

      if (line_read_.find("fxecocal_FXstreetCalendar") != std::string::npos) {
        table_start_ = true;
        continue;
      }
      if (line_read_.find("</tbody>") != std::string::npos) break;

      if (table_start_) {
        economic_events_calendar_table_ += line_read_ + "\n";
      }
    }

    live_events_html_file_.close();
  }

  void parseHTMLEvents() {
    std::string word_ = "";
    std::string data_ = "";

    bool start_ = false;
    bool data_valid_ = true;

    for (unsigned int i = 0; i < economic_events_calendar_table_.length(); i++) {
      if (economic_events_calendar_table_.at(i) == '<') {
        start_ = true;
        word_ += economic_events_calendar_table_.at(i);

      } else if (economic_events_calendar_table_.at(i) == '>') {
        word_ += economic_events_calendar_table_.at(i);

        if (word_.find("<td") != std::string::npos) data_valid_ = true;

        if (word_ == "</strong>" && data_valid_) {
          html_table_tokens_.push_back(data_);
          data_ = "";
        }

        if (word_ == "</td>" && data_valid_) {
          html_table_tokens_.push_back(data_);
          data_valid_ = false;
          data_ = "";
        }

        html_table_tokens_.push_back(word_);

        if (!data_valid_) data_ = "";

        word_ = "";
        start_ = false;

      } else {
        if (start_)
          word_ += economic_events_calendar_table_.at(i);
        else {
          if (iscntrl(economic_events_calendar_table_.at(i)) || (int)economic_events_calendar_table_.at(i) == 13 ||
              economic_events_calendar_table_.at(i) == '\t' || economic_events_calendar_table_.at(i) == '\n')
            continue;

          data_ += economic_events_calendar_table_.at(i);
        }
      }
    }
  }

  void removeHTMLElements() {
    if (html_table_tokens_.size() < 5) return;

    std::vector<std::string> data_elements_;

    std::string html_row_ = "";
    std::string html_col_ = "";

    for (unsigned token_counter_ = 4; token_counter_ < html_table_tokens_.size(); token_counter_++) {
      if (html_table_tokens_[token_counter_].find("<") != std::string::npos ||
          html_table_tokens_[token_counter_].find("<") != std::string::npos)
        continue;

      data_elements_.push_back(html_table_tokens_[token_counter_]);
    }

    for (unsigned int element_counter_ = 0; element_counter_ < data_elements_.size(); element_counter_ += 7) {
      FxEcoEvents fx_eco_event_struct_;

      fx_eco_event_struct_.event_start_time_ = data_elements_[element_counter_];
      fx_eco_event_struct_.country_ = data_elements_[element_counter_ + 1];
      fx_eco_event_struct_.event_desc_ = data_elements_[element_counter_ + 2];
      fx_eco_event_struct_.actual_numbers_ = data_elements_[element_counter_ + 3];
      fx_eco_event_struct_.consensus_numbers_ = data_elements_[element_counter_ + 4];
      fx_eco_event_struct_.previous_numbers_ = data_elements_[element_counter_ + 5];
      fx_eco_event_struct_.severity_ = data_elements_[element_counter_ + 6];

      fxstreet_events_.push_back(fx_eco_event_struct_);
    }

    if (data_elements_.size() % 7) {  // TODO handle multiple dates events

      std::cerr << " HTML Page Format Incorrect or Changed \n";
      std::cerr << " Tokens : " << fxstreet_events_.size() << "\n";
      exit(-1);
    }
  }

  void compareAndDumpEconomicEvents() {}

  // GBP severity 3 evetns handling
  void processEUHoursGBP() {
    std::vector<FxstreetEconomicEvents> duplicate_events_vector_;

    for (unsigned int events_counter_ = 0; events_counter_ < fxstreet_events_.size(); events_counter_++) {
      duplicate_events_vector_.push_back(fxstreet_events_[events_counter_]);  // add all events in order

      if (fxstreet_events_[events_counter_].event_start_time_.find(":") == std::string::npos) continue;

      int event_time_ = atoi((fxstreet_events_[events_counter_].event_start_time_.substr(
                                  0, fxstreet_events_[events_counter_].event_start_time_.find(":")))
                                 .c_str());

      const char *t_event_est_time_ = "EST_800";
      int t_event_utc_time_ =
          HFSAT::DateTime::GetUTCHHMMFromTZHHMM(HFSAT::DateTime::GetCurrentIsoDateLocal(), 800, t_event_est_time_);

      if (event_time_ >= (t_event_utc_time_ / 100)) continue;  // NO changes to US hours i.e after EST_800

      if ((fxstreet_events_[events_counter_].country_ == "United Kingdom") &&
          (fxstreet_events_[events_counter_].severity_ == "3")) {
        FxstreetEconomicEvents duplicate_event_struct_;

        duplicate_event_struct_.event_start_time_ = fxstreet_events_[events_counter_].event_start_time_;
        duplicate_event_struct_.country_ = "European Monetary Union";  // if the event is GBP and severity is 3 mark it
                                                                       // as effective duplicate EUR event
        duplicate_event_struct_.event_desc_ = fxstreet_events_[events_counter_].event_desc_;
        duplicate_event_struct_.actual_numbers_ = fxstreet_events_[events_counter_].actual_numbers_;
        duplicate_event_struct_.consensus_numbers_ = fxstreet_events_[events_counter_].consensus_numbers_;
        duplicate_event_struct_.previous_numbers_ = fxstreet_events_[events_counter_].previous_numbers_;
        //        duplicate_event_struct_.severity_ = fxstreet_events_[ events_counter_ ].severity_ ; //severity is 3
        duplicate_event_struct_.severity_ = "2";  // severity is 3

        duplicate_events_vector_.push_back(duplicate_event_struct_);
      }
    }

    fxstreet_events_.clear();  // clean up the events set and load an updated one

    for (unsigned int duplicate_event_counter_ = 0; duplicate_event_counter_ < duplicate_events_vector_.size();
         duplicate_event_counter_++) {
      fxstreet_events_.push_back(duplicate_events_vector_[duplicate_event_counter_]);
    }
  }

  // TODO if liffe requires GBP severity 1, add an additional check for filter
  void processEUHoursFilter() {
    std::vector<FxstreetEconomicEvents> filtered_events_set_;

    for (unsigned int events_counter_ = 0; events_counter_ < fxstreet_events_.size(); events_counter_++) {
      if (fxstreet_events_[events_counter_].event_start_time_.find(":") == std::string::npos) continue;

      int event_time_ = atoi((fxstreet_events_[events_counter_].event_start_time_.substr(
                                  0, fxstreet_events_[events_counter_].event_start_time_.find(":")))
                                 .c_str());

      const char *t_event_est_time_ = "EST_800";
      int t_event_utc_time_ =
          HFSAT::DateTime::GetUTCHHMMFromTZHHMM(HFSAT::DateTime::GetCurrentIsoDateLocal(), 800, t_event_est_time_);

      const char *as_end_cet_time_ = "CET_800";
      int as_end_utc_time_ =
          HFSAT::DateTime::GetUTCHHMMFromTZHHMM(HFSAT::DateTime::GetCurrentIsoDateLocal(), 800, as_end_cet_time_);

      if (event_time_ >= (t_event_utc_time_ / 100) ||
          event_time_ <=
              (as_end_utc_time_ / 100)) {  // NO changes to US or AS events i.e. after EST_800 or before CET_800

        filtered_events_set_.push_back(fxstreet_events_[events_counter_]);

      } else {
        // MASK EU_HOURS_MASK_SEVERITY_ events - Only applies to EU hours
        if (atoi((fxstreet_events_[events_counter_].severity_).c_str()) > EU_HOURS_MASK_SEVERITY_) {
          // change severity of events from 3->2
          if (fxstreet_events_[events_counter_].severity_ == "3") {
            fxstreet_events_[events_counter_].severity_ = "2";
          }

          filtered_events_set_.push_back(fxstreet_events_[events_counter_]);

        } else if ("United Kingdom" == fxstreet_events_[events_counter_].country_) {
          filtered_events_set_.push_back(fxstreet_events_[events_counter_]);
        }
      }
    }

    fxstreet_events_.clear();

    for (unsigned int filtered_event_counter_ = 0; filtered_event_counter_ < filtered_events_set_.size();
         filtered_event_counter_++) {
      fxstreet_events_.push_back(filtered_events_set_[filtered_event_counter_]);
    }
  }

  void dumpCSVData() {
    std::string header_ = "\"country\",\"date\",\"name\",\"actual\",\"consensus\",\"previous\",\"volatility\"\n";

    int this_yyyymmdd_ = HFSAT::DateTime::GetCurrentIsoDateLocal();

    std::ofstream csv_output_file_;
    csv_output_file_.open(processed_events_csv_filename_.c_str(), std::ios::out);

    if (!csv_output_file_.is_open()) {
      std::cerr << " File : " << processed_events_csv_filename_ << " Does not exists \n";
      exit(1);
    }

    csv_output_file_.write(header_.c_str(), header_.length());  // dump header

    for (unsigned int events_counter_ = 0; events_counter_ < fxstreet_events_.size(); events_counter_++) {
      csv_output_file_.write(
          fxstreet_events_[events_counter_].ToCSV(this_yyyymmdd_).c_str(),
          fxstreet_events_[events_counter_].ToCSV(this_yyyymmdd_).length());  // TODO multiple dates to be handled
    }

    csv_output_file_.close();
  }

  void dumpRawHTMLCalendar() { std::cout << economic_events_calendar_table_ << "\n"; }
};

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << " USAGE : <exec> <live_events.html> <output_file.csv> <OPT : existing_file.csv> \n";
    exit(-1);
  }

  std::string live_events_html_filename_ = argv[1];
  std::string processed_events_csv_filename_ = argv[2];

  std::string existing_processed_events_csv_filename_ = "";

  // TODO arg3 will be used to handle existing csv file to check if any new events has been added
  /*  if( argc > 3 ){

      existing_processed_events_csv_filename_ = argv[3] ;
      FxstreetEcoEventsManager

      }
  */

  FxstreetEcoEventsManager fx_eco_event_manager_(live_events_html_filename_, processed_events_csv_filename_);

  fx_eco_event_manager_.extractHTMLEcoEventsCalendarTableBody();
  fx_eco_event_manager_.parseHTMLEvents();
  fx_eco_event_manager_.removeHTMLElements();
  fx_eco_event_manager_.processEUHoursGBP();
  fx_eco_event_manager_.processEUHoursFilter();
  fx_eco_event_manager_.dumpCSVData();

  return 0;
}
