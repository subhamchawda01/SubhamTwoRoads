// =====================================================================================
//
//       Filename:  update_date.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  08/25/2015 01:01:17 PM
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

#include <iostream>
#include <cstdlib>
#include "dvccode/CommonTradeUtils/date_time.hpp"

int main(int argc, char *argv[]) {
  if (argc < 4) {
    std::cerr << "Usage : " << argv[0] << " InputDate_YYYYMMDD P/N( Previous/Next ) W/A( Work/All ) NumTimes"
              << std::endl;
    exit(-1);
  }

  int32_t input_date = atoi(argv[1]);
  char search_type = std::string("P") == std::string(argv[2]) ? 'P' : 'N';
  char day_type = std::string("W") == std::string(argv[3]) ? 'W' : 'A';

  int32_t num_of_times = 1;

  if (argc >= 5) {
    num_of_times = atoi(argv[4]);
  }

  if ('P' == search_type) {
    if ('W' == day_type) {
      for (int32_t counter = 0; counter < num_of_times; counter++) {
        input_date = HFSAT::DateTime::CalcPrevWeekDay(input_date);
      }

    } else {
      for (int32_t counter = 0; counter < num_of_times; counter++) {
        input_date = HFSAT::DateTime::CalcPrevDay(input_date);
      }
    }

  } else {
    if ('W' == day_type) {
      for (int32_t counter = 0; counter < num_of_times; counter++) {
        input_date = HFSAT::DateTime::CalcNextWeekDay(input_date);
      }

    } else {
      for (int32_t counter = 0; counter < num_of_times; counter++) {
        input_date = HFSAT::DateTime::CalcNextDay(input_date);
      }
    }
  }

  std::cout << input_date << "\n";

  return EXIT_SUCCESS;
}  // ----------  end of function main  ----------
