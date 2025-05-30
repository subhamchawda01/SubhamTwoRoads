/**
   file Tools/get_our_presence_in_market_stats.cpp

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
#include <string.h>

//#include "dvccode/CDef/exchange_symbol_manager.hpp"
//#include "dvccode/CDef/security_definitions.hpp"
//#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
//
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
//#include "baseinfra/LoggedSources/mds_messages_list.hpp"
//#include "baseinfra/LoggedSources/ice_logged_message_filenamer.hpp"
//#include "baseinfra/LoggedSources/cme_logged_message_filenamer.hpp"
//#include "baseinfra/MarketAdapter/book_init_utils.hpp"
//#include "dvccode/Utils/bulk_file_reader.hpp"

#include "basetrade/BTUtils/common_mds_info_util.hpp"

#define OUTPUT_PLOTTABLE true
#define OUTPUT_SUMMARY false

void ParseCommandLineParams(const int argc, const char** argv, std::string& our_trades_filename_,
                            std::string& shortcode_, int& input_date_, int& period_, int& begin_secs_from_midnight_,
                            int& end_secs_from_midnight_) {
  // expect :
  // 1. $0 shortcode date_YYYYMMDD [ start_tm HHMM ] [ end_tm HHMM ]
  if (argc < 3) {
    std::cerr
        << "Usage: " << argv[0]
        << " our_trade_file shortcode input_date_YYYYMMDD  PERIOD(INSEC) [ start_tm_utc_hhmm ] [ end_tm_utc_hhmm ]"
        << std::endl;
    exit(0);
  } else {
    our_trades_filename_ = argv[1];
    shortcode_ = argv[2];
    input_date_ = atoi(argv[3]);
    period_ = atoi(argv[4]);

    if (period_ == 0) period_ = DELTA_TIME_IN_SECS;

    if (argc > 5) {
      begin_secs_from_midnight_ = (atoi(argv[4]) / 100) * 60 * 60 + (atoi(argv[4]) % 100) * 60;
    }
    if (argc > 6) {
      end_secs_from_midnight_ = (atoi(argv[5]) / 100) * 60 * 60 + (atoi(argv[5]) % 100) * 60;
    }
  }
}

int main(int argc, char** argv) {
  std::string our_trades_filename_ = "";
  std::string shortcode_ = "";
  int input_date_ = 20110101;
  int begin_secs_from_midnight_ = 0;
  int end_secs_from_midnight_ = 24 * 60 * 60;
  int period_ = 0;

  std::vector<int> our_trade_time_;
  std::vector<int> our_trade_volume_;

  std::vector<unsigned int> exch_mapped_volume_;
  std::vector<unsigned int> exch_mapped_trades_;
  std::vector<unsigned int> our_mapped_volume_;
  std::vector<unsigned int> our_mapped_trades_;

  int exch_start_time_;
  std::vector<int> exch_data_timestamps_;

  std::vector<unsigned int> our_total_volumes_;
  std::vector<unsigned int> exch_total_volumes_;

  ParseCommandLineParams(argc, (const char**)argv, our_trades_filename_, shortcode_, input_date_, period_,
                         begin_secs_from_midnight_, end_secs_from_midnight_);

  std::ifstream our_trades_file_;
  our_trades_file_.open(our_trades_filename_.c_str());

  if (!our_trades_file_.is_open()) {
    std::cerr << " Couldn't open trades file : " << our_trades_filename_ << std::endl;
    exit(-1);
  }

  int total_trade_volume_ = 0;

  CommonMdsInfoUtil* common_mds_info =
      new CommonMdsInfoUtil(shortcode_, input_date_, begin_secs_from_midnight_, end_secs_from_midnight_, period_);
  common_mds_info->Compute();

  exch_mapped_volume_ = common_mds_info->GetMappedVolume();
  exch_mapped_trades_ = common_mds_info->GetMappedTrades();
  exch_total_volumes_ = common_mds_info->GetMappedTotalVolume();
  total_trade_volume_ = common_mds_info->GetVolumeTraded();
  exch_data_timestamps_ = common_mds_info->GetTimeStamps();
  exch_start_time_ = common_mds_info->GetStartTime();

  std::vector<double> our_volume_growth_;
  std::vector<double> exch_volume_growth_;

  char line_buffer_[1024];

  while (our_trades_file_.good()) {
    memset(line_buffer_, 0, 1024);

    our_trades_file_.getline(line_buffer_, 1024);

    HFSAT::PerishableStringTokenizer st_(line_buffer_, sizeof(line_buffer_));
    const std::vector<const char*>& tokens_ = st_.GetTokens();

    if (tokens_.size() == 2) {
      our_trade_time_.push_back(atoi(tokens_[0]));
      our_trade_volume_.push_back(atoi(tokens_[1]));
    }
  }
  our_trades_file_.close();

  total_trade_volume_ = 0;

  if (our_trade_time_.size() < 1) {
    std::cerr << " No Trades Found : " << std::endl;
  }

  unsigned int our_trades_index_ = 0;
  int our_total_trade_volume_over_period_ = 0;
  int num_trades_ = 0;

  unsigned int frame_counter_;
  int start_time_;

  for (frame_counter_ = 0, start_time_ = exch_start_time_; frame_counter_ < exch_mapped_volume_.size();
       start_time_ = exch_data_timestamps_[frame_counter_] + 1, frame_counter_++) {
    our_total_trade_volume_over_period_ = 0;
    num_trades_ = 0;

    while (our_trades_index_ < our_trade_time_.size() && our_trade_time_[our_trades_index_] >= start_time_ &&
           our_trade_time_[our_trades_index_] <= exch_data_timestamps_[frame_counter_]) {
      our_total_trade_volume_over_period_ += our_trade_volume_[our_trades_index_];
      our_trades_index_++;
      num_trades_++;
    }

    total_trade_volume_ += our_total_trade_volume_over_period_;
    our_total_volumes_.push_back(total_trade_volume_);
    our_mapped_volume_.push_back(our_total_trade_volume_over_period_);
    our_mapped_trades_.push_back(num_trades_);
  }

  our_volume_growth_.push_back(0);
  exch_volume_growth_.push_back(0);

  for (frame_counter_ = 1; frame_counter_ < exch_mapped_volume_.size(); frame_counter_++) {
    if (our_total_volumes_[frame_counter_] == 0 || our_total_volumes_[frame_counter_ - 1] == 0) {
      our_volume_growth_.push_back(0);

    } else {
      our_volume_growth_.push_back(
          ((double)(our_total_volumes_[frame_counter_] - our_total_volumes_[frame_counter_ - 1]) /
           (double)our_total_volumes_[frame_counter_ - 1] * 100));
    }

    if (exch_total_volumes_[frame_counter_] == 0) {
      exch_volume_growth_.push_back(0);

    } else {
      exch_volume_growth_.push_back(
          ((double)(exch_total_volumes_[frame_counter_] - exch_total_volumes_[frame_counter_ - 1]) /
           (double)exch_total_volumes_[frame_counter_ - 1] * 100));
    }
  }

  printf(
      "#START_TIME\tEND_TIME\tOUR_VOL\t\tEXCH_VOL\tVRATIO\tOUR_T_VOL\tOUR_VOL_GROWTH\tEXCH_T_VOL\tEXCH_VOL_GROWTH\tOUR_"
      "TRADES\tEXCH_TRADES\tTRATIO\n");

  for (frame_counter_ = 0, start_time_ = exch_start_time_; frame_counter_ < exch_mapped_volume_.size();
       start_time_ = exch_data_timestamps_[frame_counter_] + 1, frame_counter_++) {
    printf("%d\t%d\t%8u\t%8u\t%2.4f\t%8u\t%2.2f\t%8u\t%2.2f\t%8u\t%8u\t%2.4f\n", start_time_,
           exch_data_timestamps_[frame_counter_], our_mapped_volume_[frame_counter_],
           exch_mapped_volume_[frame_counter_],
           ((double)(our_mapped_volume_[frame_counter_])) / exch_mapped_volume_[frame_counter_] * 100,
           our_total_volumes_[frame_counter_], our_volume_growth_[frame_counter_], exch_total_volumes_[frame_counter_],
           exch_volume_growth_[frame_counter_], our_mapped_trades_[frame_counter_], exch_mapped_trades_[frame_counter_],
           ((double)(our_mapped_trades_[frame_counter_])) / exch_mapped_trades_[frame_counter_] * 100);
  }

  return 0;
}
