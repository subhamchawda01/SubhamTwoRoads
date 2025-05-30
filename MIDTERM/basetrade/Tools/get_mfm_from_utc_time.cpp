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
  if (argc < 2) {
    fprintf(stderr, "USAGE: unixtimestamp [date=TODAY]\n");
    exit(0);
  }

  char unix_timestamp_[1024] = "";
  strcpy(unix_timestamp_, argv[1]);

  char *t_secs_ = strtok(unix_timestamp_, ".\n");
  char *t_usecs_ = strtok(NULL, ".\n");

  int tv_secs_ = (t_secs_) ? atoi(t_secs_) : 0;
  int tv_usecs_ = (t_usecs_) ? atoi(t_usecs_) : 0;

  int tradingdate_ = (argc >= 3) ? (atoi(argv[2])) : (HFSAT::DateTime::GetCurrentIsoDateLocal());

  time_t last_midnight_sec_ = HFSAT::DateTime::GetTimeMidnightUTC(tradingdate_);
  int msecs_from_midnight_ = (((int)tv_secs_ - (int)last_midnight_sec_) * 1000) + (int)(tv_usecs_ / 1000);

  std::cout << msecs_from_midnight_ << std::endl;

  return 0;
}
