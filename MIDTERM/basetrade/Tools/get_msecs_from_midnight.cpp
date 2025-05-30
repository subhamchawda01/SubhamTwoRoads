/**
    \file Tools/get_msecs_from midnight.cpp

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

  if ((strncmp(argv[1], "EST_", 4) == 0) || (strncmp(argv[1], "CST_", 4) == 0) || (strncmp(argv[1], "CET_", 4) == 0) ||
      (strncmp(argv[1], "BRT_", 4) == 0) || (strncmp(argv[1], "UTC_", 4) == 0) || (strncmp(argv[1], "KST_", 4) == 0) ||
      (strncmp(argv[1], "HKT_", 4) == 0) || (strncmp(argv[1], "IST_", 4) == 0) || (strncmp(argv[1], "JST_", 4) == 0) ||
      (strncmp(argv[1], "MSK_", 4) == 0) || (strncmp(argv[1], "PAR_", 4) == 0) || (strncmp(argv[1], "AMS_", 4) == 0) ||
      (strncmp(argv[1], "LON_", 4) == 0) || (strncmp(argv[1], "AST_", 4) == 0) || (strncmp(argv[1], "BST_", 4) == 0)) {
    printf("%d", HFSAT::GetMsecsFromMidnightFromHHMM(
                     HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(argv[1] + 4), argv[1])));
  } else {
    printf("%d", atoi(argv[1]));
  }
}
