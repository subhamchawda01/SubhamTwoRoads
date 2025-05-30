/**
    \file Tools/get_mfm_from_utc_time.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

*/
#include <iostream>
#include <stdlib.h>
#include <string.h>

#include "dvccode/CommonTradeUtils/date_time.hpp"

/// input arguments : unix_timestamp
int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "USAGE: YYYYMMDD TIME(IST_914) \n");
    exit(0);
  }

  std::cout << " UTC_HHMM : " << HFSAT::DateTime::GetUTCHHMMFromTZHHMM(atoi(argv[1]), atoi(argv[2] + 4), argv[2]) << std::endl;
  std::cout << " UTC_HHMMSS : " << HFSAT::DateTime::GetUTCSecondsFromMidnightFromTZHHMM(atoi(argv[1]), atoi(argv[2] + 4), argv[2]) << std::endl;
  std::cout << " EST_HHMMSS : " << HFSAT::DateTime::GetESTSecondsFromMidnightFromTZHHMM(atoi(argv[1]), atoi(argv[2] + 4), argv[2]) << std::endl;
  return 0;
}
