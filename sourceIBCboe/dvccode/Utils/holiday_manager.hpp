#ifndef _HOLIDAY_MANAGER_
#define _HOLIDAY_MANAGER_

#include <map>
#include <set>
#include <string>
#include <vector>

#define INVALID_DATE -1

namespace HFSAT {

namespace HolidayManagerNoThrow {

// These functions do not throw error, but exits in case of error. Internally uses the api's of HolidayManager class.
bool IsExchangeHoliday(const std::string& exchange, const int yyyymmdd, bool is_weekend_holiday = false);
bool IsProductHoliday(const std::string& product, const int yyyymmdd, bool is_weekend_holiday = false);
}  // namespace HolidayManagerNoThrow

class HolidayManager {
 public:
  static HolidayManager& GetUniqueInstance();

  // These 2 api's "THROW" in case of error. Always call these api's inside try-catch block and Handle the error.
  bool IsExchangeHoliday(const std::string& exchange, const int yyyymmdd, bool is_weekend_holiday = false);
  bool IsProductHoliday(const std::string& product, const int yyyymmdd, bool is_weekend_holiday = false);

  int GetExchangeStartDate(const std::string& exchange);
  int GetProductStartDate(const std::string& product);

  bool IsWeekend(const int yyyymmdd);
  bool IsExceptionalNormalTradingDay(const std::string &exchange, const int yyyymmdd);

 private:
  HolidayManager();
  HolidayManager(const HolidayManager&) {}

  void LoadExchangeStartDates();
  void LoadProductStartDates();
  void LoadExchangeHolidays();
  void LoadProductHolidays();
  void LoadExceptionalNormalTradingDates();
  void ProcessProductStartDates(const std::vector<const char*>& fields);
  void ProcessProductHolidays(const std::vector<const char*>& fields);
  int GetProductSpecificStartDate(const std::string& pure_basename);
  bool IsExchangeValid(const std::string& exchange);
  bool IsNumber(const std::string& str);
  const std::string GetProductPureBaseName(const std::string& product);
  std::string GetExchangeForProduct(const std::string& product);

  void PrintProductHolidays();
  void PrintProductStartDates();

 private:
  static HolidayManager* uniqueinstance_;
  std::map<std::string, int> exchange_start_date_;
  std::map<std::string, std::set<int> > exchange_holidays_;
  std::map<std::string, int> product_start_date_;
  std::map<std::string, std::set<int> > product_holidays_;
  std::map<std::string, std::set<int> > exceptional_normal_trading_dates_;
};
}  // namespace HFSAT

#endif
