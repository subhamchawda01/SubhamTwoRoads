/**
   file Tools/get_event_line.cpp

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
#include <iomanip>
#include "dvccode/CommonTradeUtils/date_time.hpp"

// Displays the formatted line corresponding to an event
int main(int argc, char **argv) {
  if (argc < 4) {
    std::cerr << " USAGE : <exec> <event-name(no spaces)> <currency> <priority> <date: yyyymmdd> <time: tz_hhmm>\n";
    exit(-1);
  }
  std::string event = argv[1];
  std::string currency = argv[2];
  int priority = atoi(argv[3]);
  int yyyymmdd = atoi(argv[4]);
  int utc_hhmm = -1;
  if ((strncmp(argv[5], "EST_", 4) == 0) || (strncmp(argv[5], "CST_", 4) == 0) || (strncmp(argv[5], "CET_", 4) == 0) ||
      (strncmp(argv[5], "BRT_", 4) == 0) || (strncmp(argv[5], "UTC_", 4) == 0) || (strncmp(argv[5], "KST_", 4) == 0) ||
      (strncmp(argv[5], "HKT_", 4) == 0) || (strncmp(argv[5], "MSK_", 4) == 0) || (strncmp(argv[5], "IST_", 4) == 0) ||
      (strncmp(argv[5], "JST_", 4) == 0) || (strncmp(argv[5], "BST_", 4) == 0) || (strncmp(argv[5], "AST_", 4) == 0)) {
    // Time was provided in a different timezone
    utc_hhmm = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(yyyymmdd, atoi(argv[5] + 4), argv[5]);
    yyyymmdd = HFSAT::DateTime::GetUTCYYMMDDFromTZHHMM(yyyymmdd, atoi(argv[5] + 4), argv[5]);
  } else {
    // The time is probably in UTC already => use directly
    utc_hhmm = atoi(argv[5]);
  }
  time_t event_time = HFSAT::DateTime::GetTimeUTC(yyyymmdd, utc_hhmm);
  // Display the line in the correct format (to be added to events file)
  std::cout << event_time << " " << currency << " " << event << " " << priority << " " << yyyymmdd << "_"
            << std::setfill('0') << std::setw(2) << (utc_hhmm / 100) << ":" << std::setfill('0') << std::setw(2)
            << (utc_hhmm % 100) << ":00_UTC\n";

  return 0;
}
