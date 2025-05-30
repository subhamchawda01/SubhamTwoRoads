/**
    \file Tools/calc_next_week_day.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

#include <iostream>
#include <stdlib.h>
#include "dvccode/CommonTradeUtils/date_time.hpp"

void ParseCommandLineParams(const int argc, const char **argv, int &input_date_, int &num_times_) {
  // expect :
  // 1. $calc_next_week_day input_date
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " input_date_YYYYMMDD [num_times] " << std::endl;
    exit(0);
  } else {
    input_date_ = atoi(argv[1]);
    if (argc >= 3) {
      num_times_ = std::min(1000, std::max(1, atoi(argv[2])));
    }
  }
}

int main(int argc, char **argv) {
  int input_date_ = 0;
  int num_times_ = 1;

  ParseCommandLineParams(argc, (const char **)argv, input_date_, num_times_);

  for (int i = 0; i < num_times_; i++) {
    input_date_ = HFSAT::DateTime::CalcNextWeekDay(input_date_);
  }
  printf("%d", input_date_);
}
