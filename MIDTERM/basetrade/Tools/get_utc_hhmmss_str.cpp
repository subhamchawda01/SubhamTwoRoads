/**
    \file Tools/get_utc_hhmmss_str.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

*/

#include <iostream>
#include <stdlib.h>
#include "dvccode/CommonTradeUtils/date_time.hpp"

/// input arguments : hhmmstr [date=TODAY]
int main(int argc, char** argv) {
  std::string shortcode_ = "";

  if (argc < 2) {
    fprintf(stderr, "USAGE: hhmmstr [date=TODAY]\n");
    exit(0);
  }
  unsigned int tradingdate_ = (argc >= 3) ? (atoi(argv[2])) : (HFSAT::DateTime::GetCurrentIsoDateLocal());

  if (strncmp(argv[1], "PREV_", 5) == 0) {
    argv[1] = argv[1] + 5;
  }

  if (strlen(argv[1]) >= 6 && strncmp(argv[1] + 3, "_", 1) == 0) {
    printf("%06d", HFSAT::DateTime::GetUTCHHMMSSFromTZHHMMSS(tradingdate_, HFSAT::DateTime::GetHHMMSSTime(argv[1] + 4),
                                                             argv[1]));
  } else {
    printf("%06d", HFSAT::DateTime::GetHHMMSSTime(argv[1]));
  }
}
