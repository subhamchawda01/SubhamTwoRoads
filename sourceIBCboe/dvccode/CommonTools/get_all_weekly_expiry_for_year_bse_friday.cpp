/**
    \file Tools/calc_prev_day.cpp

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
#include "dvccode/Utils/exchange_names.hpp"
#include "dvccode/Utils/holiday_manager.hpp"

const boost::gregorian::date_duration one_day_duration_(1);

int intdate_ = 20230101;

int ComputeNextExpiryWeeklyOptions(const int t_date_) {
  boost::gregorian::date d1((t_date_ / 10000) % 10000, ((t_date_ / 100) % 100), (t_date_ % 100));
  boost::gregorian::greg_weekday gw(boost::gregorian::Friday);
  boost::gregorian::date next_friday = next_weekday(d1, gw);

  boost::gregorian::date current_day = next_friday;
  boost::gregorian::date::ymd_type ymd = current_day.year_month_day();
  int t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
  while (HFSAT::HolidayManagerNoThrow::IsExchangeHoliday(HFSAT::EXCHANGE_KEYS::kExchSourceBSEStr, t_expiry_date_, true)) {
    current_day -= one_day_duration_;
    boost::gregorian::date::ymd_type ymd = current_day.year_month_day();
    t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
  }

  if (t_expiry_date_ >= t_date_) {
    return t_expiry_date_;
  } else {
    boost::gregorian::date next_saturday = next_friday + boost::gregorian::date_duration(1);
    // std::cout << "Date: " << next_saturday << std::endl;
    ymd = next_saturday.year_month_day();
    return ComputeNextExpiryWeeklyOptions(((ymd.year * 100 + ymd.month) * 100) + ymd.day);
  }
}

int ComputeExpiryDateWeeklyOptions(const int expiry_number) {
  int t_expiry_number_ = expiry_number;
  int t_temp_date_ = intdate_;
  int t_expiry_date_ = 0;
  while (t_expiry_number_ >= 0) {
    t_expiry_date_ = ComputeNextExpiryWeeklyOptions(t_temp_date_);
    t_expiry_number_--;
    if (t_expiry_number_ >= 0) {
      boost::gregorian::date d1((t_expiry_date_ / 10000) % 10000, ((t_expiry_date_ / 100) % 100),
                                (t_expiry_date_ % 100));
      boost::gregorian::greg_weekday gw(boost::gregorian::Friday);
      boost::gregorian::date next_friday = next_weekday(d1, gw);
      boost::gregorian::date::ymd_type ymd = next_friday.year_month_day();
      int date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
      t_temp_date_ = HFSAT::DateTime::CalcNextDay(date_);
    }
  }
  return t_expiry_date_;
}

int main(int argc, char **argv) {
  int num_times_ = 1000;
  if (argc < 2) {
	std::cerr << "Usage: " << argv[0] << " YYYYMMDD" << std::endl;
    	exit(0);
  }
  int input_date_ = atoi(argv[1]);
  for (int i = 0; i < num_times_; i++) {
    int date_ = ComputeExpiryDateWeeklyOptions(i);
    if ( date_ > input_date_ ) break;
    std::cout << ComputeExpiryDateWeeklyOptions(i) << std::endl;
  }
}
