/**
    \file Tools/get_utc_hhmm_str.cpp

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

bool isValidTZ(char* c) {
  if ((strncmp(c, "EST_", 4) == 0) || (strncmp(c, "CST_", 4) == 0) || (strncmp(c, "CET_", 4) == 0) ||
      (strncmp(c, "BRT_", 4) == 0) || (strncmp(c, "UTC_", 4) == 0) || (strncmp(c, "AST_", 4) == 0))
    return true;
  return false;
}

/// TODO ADD more formats as required
/// input arguments : hhmmstr [date=TODAY]
int main(int argc, char** argv) {
  std::string shortcode_ = "";

  if (argc < 3) {
    fprintf(stderr, "USAGE: %s TO_EPOCH hhmmstr [date=TODAY]\n", argv[0]);
    fprintf(stderr, "USAGE: %s TO_UTC hhmmstr [date=TODAY]\n", argv[0]);
    fprintf(stderr, "USAGE: %s FROM_SECS epoch_secs\n", argv[0]);

    exit(0);
  }

  if (strncmp(argv[1], "FROM_SECS", 9) == 0) {
    time_t t = atoi(argv[2]);
    printf("%d\n", (int)HFSAT::DateTime::GetUTCHHMMFromTime(t));
  } else if (strncmp(argv[1], "TO_", 3) == 0) {
    unsigned int tradingdate_ = (argc >= 4) ? (atoi(argv[3])) : (HFSAT::DateTime::GetCurrentIsoDateLocal());

    if (!isValidTZ(argv[2])) {
      fprintf(stderr, "Time zone not valid. must be one of { EST_, CST_ CET_, BRT_, UTC_ } \n");
      exit(0);
    }

    if (strncmp(argv[1] + 3, "EPOCH", 4) == 0) {
      printf("%d\n", (int)HFSAT::DateTime::GetTimeFromTZHHMM(tradingdate_, atoi(argv[2] + 4), argv[2]));
    }
    if (strncmp(argv[1] + 3, "UTC", 3) == 0) {
      printf("%d\n", (int)HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(argv[2] + 4), argv[2]));
    }
  }
}
