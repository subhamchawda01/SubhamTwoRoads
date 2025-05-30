#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/Utils/holiday_manager.hpp"
#include "dvccode/Utils/holiday_manager_utils.hpp"

#include <iostream>

namespace HFSAT {
namespace HolidayManagerUtils {

int GetPrevBusinessDayForExchange(const std::string& exchange, const int yyyymmdd, bool is_weekend_holiday) {
  int this_YYYYMMDD = yyyymmdd;
  try {
    int start_date = HolidayManager::GetUniqueInstance().GetExchangeStartDate(exchange);
    bool is_holiday = false;
    do {
      this_YYYYMMDD = HFSAT::DateTime::CalcPrevDay(this_YYYYMMDD);
      if (this_YYYYMMDD < start_date) {
        return INVALID_DATE;
      }
      is_holiday = HolidayManager::GetUniqueInstance().IsExchangeHoliday(exchange, this_YYYYMMDD, is_weekend_holiday);

    } while (is_holiday);
  } catch (...) {
    std::cerr << "Error in Holiday Manager for exchange: " << exchange << ". Exiting.\n";
    std::exit(1);
  }
  return this_YYYYMMDD;
}
int GetPrevBusinessDayForProduct(const std::string& product, const int yyyymmdd, bool is_weekend_holiday) {
  int this_YYYYMMDD = yyyymmdd;
  try {
    int start_date = HolidayManager::GetUniqueInstance().GetProductStartDate(product);
    bool is_holiday = false;
    do {
      this_YYYYMMDD = HFSAT::DateTime::CalcPrevDay(this_YYYYMMDD);
      if (this_YYYYMMDD < start_date) {
        return INVALID_DATE;
      }
      is_holiday = HolidayManager::GetUniqueInstance().IsProductHoliday(product, this_YYYYMMDD, is_weekend_holiday);

    } while (is_holiday);
  } catch (...) {
    std::cerr << "Error in Holiday Manager for product " << product << ". Exiting.\n";
    std::exit(1);
  }

  return this_YYYYMMDD;
}
int GetNextBusinessDayForExchange(const std::string& exchange, const int yyyymmdd, bool is_weekend_holiday) {
  int this_YYYYMMDD = yyyymmdd;
  bool is_holiday = false;
  do {
    this_YYYYMMDD = HFSAT::DateTime::CalcNextDay(this_YYYYMMDD);
    try {
      is_holiday = HolidayManager::GetUniqueInstance().IsExchangeHoliday(exchange, this_YYYYMMDD, is_weekend_holiday);
    } catch (...) {
      std::cerr << "Error in Holiday Manager for exchange: " << exchange << ". Exiting.\n";
      std::exit(1);
    }
  } while (is_holiday);

  return this_YYYYMMDD;
}
int GetNextBusinessDayForProduct(const std::string& product, const int yyyymmdd, bool is_weekend_holiday) {
  int this_YYYYMMDD = yyyymmdd;
  bool is_holiday = false;
  do {
    this_YYYYMMDD = HFSAT::DateTime::CalcNextDay(this_YYYYMMDD);
    try {
      is_holiday = HolidayManager::GetUniqueInstance().IsProductHoliday(product, this_YYYYMMDD, is_weekend_holiday);
    } catch (...) {
      std::cerr << "Error in Holiday Manager for product " << product << ". Exiting.\n";
      std::exit(1);
    }
  } while (is_holiday);

  return this_YYYYMMDD;
}
}
}
