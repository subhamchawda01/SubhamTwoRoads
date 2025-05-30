#ifndef _HOLIDAY_MANAGER_UTILS_
#define _HOLIDAY_MANAGER_UTILS_
#include "dvccode/Utils/holiday_manager.hpp"

namespace HFSAT {
namespace HolidayManagerUtils {
int GetPrevBusinessDayForExchange(const std::string& exchange, const int yyyymmdd, bool is_weekend_holiday = true);
int GetPrevBusinessDayForProduct(const std::string& product, const int yyyymmdd, bool is_weekend_holiday = true);
int GetNextBusinessDayForExchange(const std::string& exchange, const int yyyymmdd, bool is_weekend_holiday = true);
int GetNextBusinessDayForProduct(const std::string& product, const int yyyymmdd, bool is_weekend_holiday = true);
}
}

#endif
