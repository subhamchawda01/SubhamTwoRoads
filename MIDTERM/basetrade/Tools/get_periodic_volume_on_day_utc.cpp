/**
   \file Tools/get_volume_on_day.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717

 */

#include "basetrade/BTUtils/common_mds_info_util.hpp"

void ParseCommandLineParams(const int argc, const char** argv, std::string& shortcode_, int& input_date_, int& period_,
                            int& begin_secs_from_midnight_, int& end_secs_from_midnight_) {
  // expect :
  // 1. $0 shortcode date_YYYYMMDD [ start_tm HHMM ] [ end_tm HHMM ]
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0]
              << " shortcode input_date_YYYYMMDD PERIOD(INSEC) [ start_tm_utc_hhmm ] [ end_tm_utc_hhmm ]" << std::endl;
    exit(0);
  } else {
    shortcode_ = argv[1];
    input_date_ = atoi(argv[2]);
    period_ = atoi(argv[3]);
    if (period_ == 0) period_ = DELTA_TIME_IN_SECS;

    if (argc > 4) {
      begin_secs_from_midnight_ = (atoi(argv[4]) / 100) * 60 * 60 + (atoi(argv[4]) % 100) * 60;
    }
    if (argc > 5) {
      end_secs_from_midnight_ = (atoi(argv[5]) / 100) * 60 * 60 + (atoi(argv[5]) % 100) * 60;
    }
  }
}

/// input arguments : input_date
int main(int argc, char** argv) {
  std::string shortcode_ = "";
  int input_date_ = 20110101;
  int begin_secs_from_midnight_ = 0;
  int end_secs_from_midnight_ = 24 * 60 * 60;
  int period_ = 0;
  ParseCommandLineParams(argc, (const char**)argv, shortcode_, input_date_, period_, begin_secs_from_midnight_,
                         end_secs_from_midnight_);

  CommonMdsInfoUtil* common_mds_info_util =
      new CommonMdsInfoUtil(shortcode_, input_date_, begin_secs_from_midnight_, end_secs_from_midnight_, period_);
  common_mds_info_util->Compute();

  std::vector<unsigned int> periodic_vols_ = common_mds_info_util->GetMappedVolume();
  std::vector<int> timestamps_ = common_mds_info_util->GetTimeStamps();

  if (periodic_vols_.size() == timestamps_.size()) {
    for (unsigned i = 0; i < timestamps_.size(); i++) {
      time_t tval_ = (time_t)timestamps_[i];
      tm* time_value = gmtime(&tval_);
      printf("%06d%s%u\n", time_value->tm_hour * 10000 + time_value->tm_min * 100 + time_value->tm_sec, " ",
             periodic_vols_[i]);
    }
  }
}
