#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <typeinfo>

#include <boost/date_time/gregorian/gregorian.hpp>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/Utils/common_files_path.hpp"
#include "dvccode/Utils/exchange_names.hpp"
#include "dvccode/Utils/holiday_manager.hpp"

namespace HFSAT {

HolidayManager *HolidayManager::uniqueinstance_ = nullptr;
const char *const kDelimiterKey = "|";
const char *const kGenericExchangeKey = "ALL";

HolidayManager &HolidayManager::GetUniqueInstance() {
  if (uniqueinstance_ == nullptr) {
    uniqueinstance_ = new HolidayManager();
  }
  return *(uniqueinstance_);
}

HolidayManager::HolidayManager() {
  LoadExchangeStartDates();
  LoadExchangeHolidays();
  LoadProductStartDates();
  LoadProductHolidays();
  LoadExceptionalNormalTradingDates();
}

void HolidayManager::LoadExchangeStartDates() {
  const std::string exchange_start_date_filepath(FILEPATH::kExchangeStartDateFile);
  if (FileUtils::ExistsAndReadable(exchange_start_date_filepath)) {
    std::ifstream infile(exchange_start_date_filepath, std::ifstream::in);

    if (infile.is_open()) {
      char line_buffer_[1024];

      while (infile.good()) {
        memset(line_buffer_, 0, 1024);
        infile.getline(line_buffer_, 1024);

        PerishableStringTokenizer string_tokenizer(line_buffer_, 1024, true);
        const std::vector<const char *> &tokens = string_tokenizer.GetTokens();
        if (tokens.size() >= 2 && IsNumber(tokens[1])) {
          exchange_start_date_[tokens[0]] = std::atoi(tokens[1]);
        } else if (tokens.size() >= 2 && !IsNumber(tokens[1])) {
          std::cerr << "Exchange Start Date " << tokens[1] << " has chars other than digits. Skipping\n";
        }
      }
    }
  } else {
    std::cerr << "File " << exchange_start_date_filepath << " not exists or not readable. Exiting\n";
    exit(1);
  }
}

void HolidayManager::LoadExchangeHolidays() {
  const std::string exchange_holidays_filepath(FILEPATH::kExchangeHolidaysFile);
  if (FileUtils::ExistsAndReadable(exchange_holidays_filepath)) {
    std::ifstream infile(exchange_holidays_filepath, std::ifstream::in);

    if (infile.is_open()) {
      char line_buffer_[1024];

      while (infile.good()) {
        memset(line_buffer_, 0, 1024);
        infile.getline(line_buffer_, 1024);

        PerishableStringTokenizer string_tokenizer(line_buffer_, 1024, true);
        const std::vector<const char *> &tokens = string_tokenizer.GetTokens();
        if (tokens.size() >= 2 && IsNumber(tokens[1])) {
          if (exchange_holidays_.find(tokens[0]) == exchange_holidays_.end()) {
            std::set<int> holidays;
            exchange_holidays_[tokens[0]] = holidays;
          }
          exchange_holidays_[tokens[0]].insert(std::atoi(tokens[1]));

          if (tokens.size() >= 3 && IsNumber(tokens[2]) && std::atoi(tokens[2]) > std::atoi(tokens[1])) {
            int next_date = std::atoi(tokens[1]);
            int end_date = std::atoi(tokens[2]);
            exchange_holidays_[tokens[0]].insert(next_date);
            exchange_holidays_[tokens[0]].insert(end_date);
            while (next_date < end_date) {
              next_date = HFSAT::DateTime::CalcNextDay(next_date);
              exchange_holidays_[tokens[0]].insert(next_date);
            }
          }

        } else if (tokens.size() >= 2 && !IsNumber(tokens[1])) {
          std::cerr << "Exchange Holiday " << tokens[1] << " has chars other than digits. Skipping\n";
        }
      }
    }
  } else {
    std::cerr << "File " << exchange_holidays_filepath << " not exists or not readable. Exiting\n";
    exit(1);
  }
}

void HolidayManager::LoadProductStartDates() {
  const std::string product_start_date_filepath(FILEPATH::kProductStartDateFile);
  if (FileUtils::ExistsAndReadable(product_start_date_filepath)) {
    std::ifstream infile(product_start_date_filepath, std::ifstream::in);

    if (infile.is_open()) {
      char line_buffer_[1024];

      while (infile.good()) {
        memset(line_buffer_, 0, 1024);
        infile.getline(line_buffer_, 1024);

        PerishableStringTokenizer string_tokenizer(line_buffer_, 1024, true);
        const std::vector<const char *> &tokens = string_tokenizer.GetTokens();

        if (!tokens.empty()) {
          ProcessProductStartDates(tokens);
        }
      }
    }
  } else {
    std::cerr << "File " << product_start_date_filepath << " not exists or not readable. Exiting\n";
    exit(1);
  }
}

void HolidayManager::LoadProductHolidays() {
  const std::string product_holidays_filepath = FILEPATH::kProductHolidaysFile;
  if (FileUtils::ExistsAndReadable(product_holidays_filepath)) {
    std::ifstream infile(product_holidays_filepath, std::ifstream::in);

    if (infile.is_open()) {
      char line_buffer_[1024];

      while (infile.good()) {
        memset(line_buffer_, 0, 1024);
        infile.getline(line_buffer_, 1024);

        PerishableStringTokenizer string_tokenizer(line_buffer_, 1024, true);
        const std::vector<const char *> &tokens = string_tokenizer.GetTokens();

        if (!tokens.empty()) {
          ProcessProductHolidays(tokens);
        }
      }
    }
  } else {
    std::cerr << "File " << product_holidays_filepath << " not exists or not readable. Exiting\n";
    exit(1);
  }
}

void HolidayManager::LoadExceptionalNormalTradingDates() {
  const std::string exceptional_normal_trading_dates_filepath(FILEPATH::kExceptionalNormalTradingDateFile);
  if (FileUtils::ExistsAndReadable(exceptional_normal_trading_dates_filepath)) {
    std::ifstream infile(exceptional_normal_trading_dates_filepath, std::ifstream::in);

    if (infile.is_open()) {
      char line_buffer_[1024];

      while (infile.good()) {
        memset(line_buffer_, 0, 1024);
        infile.getline(line_buffer_, 1024);

        PerishableStringTokenizer string_tokenizer(line_buffer_, 1024, true);
        const std::vector<const char *> &tokens = string_tokenizer.GetTokens();
        if (tokens.size() >= 2 && IsNumber(tokens[1])) {
          if (exceptional_normal_trading_dates_.find(tokens[0]) == exceptional_normal_trading_dates_.end()) {
            std::set<int> normal_trading_dates;
            exceptional_normal_trading_dates_[tokens[0]] = normal_trading_dates;
          }
          exceptional_normal_trading_dates_[tokens[0]].insert(std::atoi(tokens[1]));

          if (tokens.size() >= 3 && IsNumber(tokens[2]) && std::atoi(tokens[2]) > std::atoi(tokens[1])) {
            int next_date = std::atoi(tokens[1]);
            int end_date = std::atoi(tokens[2]);
            exceptional_normal_trading_dates_[tokens[0]].insert(next_date);
            exceptional_normal_trading_dates_[tokens[0]].insert(end_date);
            while (next_date < end_date) {
              next_date = HFSAT::DateTime::CalcNextDay(next_date);
              exceptional_normal_trading_dates_[tokens[0]].insert(next_date);
            }
          }

        } else if (tokens.size() >= 2 && !IsNumber(tokens[1])) {
          std::cerr << "Normal Trading Date " << tokens[1] << " has chars other than digits. Skipping\n";
        }
      }
    }
  } else {
    std::cerr << "File " << exceptional_normal_trading_dates_filepath << " not exists or not readable. Exiting\n";
    exit(1);
  }
}

void HolidayManager::ProcessProductStartDates(const std::vector<const char *> &fields) {
  std::set<std::string> shortcodes;
  std::set<int> start_date;
  std::vector<const char *>::const_iterator iter = fields.begin();

  // Process Shortcodes
  for (; iter != fields.end(); iter++) {
    if (!strcmp(*iter, kDelimiterKey)) {
      iter++;
      break;
    }
    shortcodes.insert(*iter);
  }

  if (shortcodes.empty()) {
    std::cerr << typeid(*this).name() << ':' << __func__ << " "
              << "No Shortcodes specified at this line.\n";
    return;
  }

  // Process Start Date
  for (; iter != fields.end(); iter++) {
    if (!strcmp(*iter, kDelimiterKey)) {
      // Precautionary. Won't be executed ever.
      break;
    }
    if (IsNumber(*iter)) {
      start_date.insert(std::atoi(*iter));

    } else {
      std::cerr << typeid(*this).name() << ':' << __func__ << " " << *iter << " does not contain all digits.\n";
    }
  }
  if (start_date.empty()) {
    std::cerr << typeid(*this).name() << ':' << __func__ << " "
              << "No Start Date specified at this line.\n";
    return;
  }
  if (start_date.size() > 1) {
    std::cerr << typeid(*this).name() << ':' << __func__ << " "
              << "Multiple Start Dates for Product: " << *(shortcodes.begin()) << "\n";
  }

  // Add to product_start_date_
  for (auto shortcode : shortcodes) {
    if (product_start_date_.find(shortcode) == product_start_date_.end()) {
      product_start_date_[shortcode] = *(start_date.begin());
    } else if (product_start_date_[shortcode] != *(start_date.begin())) {
      std::cerr << typeid(*this).name() << ':' << __func__ << " "
                << "Multiple Start Dates for Product : " << shortcode << "\n";
    }
  }
}

void HolidayManager::ProcessProductHolidays(const std::vector<const char *> &fields) {
  std::set<std::string> shortcodes;
  std::set<int> holidays;
  std::vector<const char *>::const_iterator iter = fields.begin();

  // Process Shortcodes
  for (; iter != fields.end(); iter++) {
    if (!strcmp(*iter, kDelimiterKey)) {
      iter++;
      break;
    }
    shortcodes.insert(*iter);
  }
  if (shortcodes.empty()) {
    std::cerr << typeid(*this).name() << ':' << __func__ << " "
              << "No Shortcodes specified at this line.\n";
    return;
  }

  // Process Holidays
  for (; iter != fields.end(); iter++) {
    if (!strcmp(*iter, kDelimiterKey)) {
      // Precautionary. Won't be executed ever.
      break;
    }
    if (IsNumber(*iter)) {
      holidays.insert(std::atoi(*iter));
    } else {
      std::cerr << typeid(*this).name() << ':' << __func__ << " " << *iter << " does not contain all digits.\n";
    }
  }
  if (holidays.empty()) {
    std::cerr << typeid(*this).name() << ':' << __func__ << " "
              << "No Holidays specified at this line.\n";
    return;
  }

  // Add to exchange_product_holidays_
  for (auto shortcode : shortcodes) {
    if (product_holidays_.find(shortcode) == product_holidays_.end()) {
      std::set<int> set_holidays;
      product_holidays_[shortcode] = set_holidays;
    }
    product_holidays_[shortcode].insert(holidays.begin(), holidays.end());
  }
}

bool HolidayManager::IsExceptionalNormalTradingDay(const std::string &exchange, const int yyyymmdd){
  if (exceptional_normal_trading_dates_.find(exchange) != exceptional_normal_trading_dates_.end() ) {
    if (exceptional_normal_trading_dates_[exchange].find(yyyymmdd) != exceptional_normal_trading_dates_[exchange].end()) {
      return true;
    }
  }
  return false;
}

bool HolidayManager::IsExchangeHoliday(const std::string &exchange, const int yyyymmdd, bool is_weekend_holiday) {
  if(IsExceptionalNormalTradingDay(exchange, yyyymmdd)){
    return false;
  }else if (is_weekend_holiday && IsWeekend(yyyymmdd)) {
    return true;
  }
  if (!IsExchangeValid(exchange)) {
    std::cerr << typeid(*this).name() << ':' << __func__ << " "
              << "Invalid Exchange : " << exchange << "\n";
    throw 10;
  }

  if (yyyymmdd < GetExchangeStartDate(exchange)) {
    return true;
  }

  // Check whether this day is a holiday for all exchanges
  if (exchange_holidays_.find(kGenericExchangeKey) != exchange_holidays_.end()) {
    if (exchange_holidays_[kGenericExchangeKey].find(yyyymmdd) != exchange_holidays_[kGenericExchangeKey].end()) {
      return true;
    }
  }

  if (exchange_holidays_.find(exchange) != exchange_holidays_.end()) {
    if (exchange_holidays_[exchange].find(yyyymmdd) != exchange_holidays_[exchange].end()) {
      return true;
    }
  }

  return false;
}

bool HolidayManager::IsProductHoliday(const std::string &product, const int yyyymmdd, bool is_weekend_holiday) {
  SecurityDefinitions &sec_def = SecurityDefinitions::GetUniqueInstance(yyyymmdd);
  std::string exchange;
  if (sec_def.contract_specification_map_.find(product) != sec_def.contract_specification_map_.end()) {
    exchange = ExchSourceStringForm(sec_def.contract_specification_map_[product].exch_source_);
    if (IsExchangeHoliday(exchange, yyyymmdd, is_weekend_holiday)) {
      return true;
    }
  } else if (product.substr(0, 4) == "NSE_") {
    if (sec_def.IsNSESecDefAvailable(yyyymmdd)) {
      /* [pjain] : We cannot do LoadNSESecurityDefinitions() here as it would not load the Security Definitions for
       yyyymmdd but for the date with which SecDef instance was first created. There is no support in the code to
       create unique instances of SecurityDefinitions for different dates, so we cannot verify whether the product is
       valid or not for the date yyyymmdd.
       ASSUMPTION: Valid product*/

      // sec_def.LoadNSESecurityDefinitions();
      exchange = EXCHANGE_KEYS::kExchSourceNSEStr;
      if (IsExchangeHoliday(exchange, yyyymmdd, is_weekend_holiday)) {
        return true;
      }
    } else {
      // For this date we don't have data for this exchange, so assuming it a holiday.
      return true;  // Default
    }
  } else if (product.substr(0, 4) == "BSE_") {
    if (sec_def.IsBSESecDefAvailable(yyyymmdd)) {
      /* [pjain] : We cannot do LoadBSESecurityDefinitions() here as it would not load the Security Definitions for
         yyyymmdd but for the date with which SecDef instance was first created. There is no support in the code to
         create unique instances of SecurityDefinitions for different dates, so we cannot verify whether the product is
         valid or not for the date yyyymmdd.
         ASSUMPTION: Valid product*/

      // sec_def.LoadBSEStocksSecurityDefinitions();
      exchange = EXCHANGE_KEYS::kExchSourceBSEStr;
      if (IsExchangeHoliday(exchange, yyyymmdd, is_weekend_holiday)) {
        return true;
      }
    } else {
      // For this date we don't have data for this exchange, so assuming it a holiday.
      return true;
    }
  } else if (product.substr(0, 3) == "HK_") {
    if (sec_def.IsHKSecDefAvailable(yyyymmdd)) {
      /* [pjain] : We cannot do LoadHKSecurityDefinitions() here as it would not load the Security Definitions for
         yyyymmdd but for the date with which SecDef instance was first created. There is no support in the code to
         create unique instances of SecurityDefinitions for different dates, so we cannot verify whether the product is
         valid or not for the date yyyymmdd.
         ASSUMPTION: Valid product*/

      // sec_def.LoadHKStocksSecurityDefinitions();
      exchange = EXCHANGE_KEYS::kExchSourceHONGKONGStr;
      if (IsExchangeHoliday(exchange, yyyymmdd, is_weekend_holiday)) {
        return true;
      }
    } else {
      // For this date we don't have data for this exchange, so assuming it a holiday.
      return true;
    }
  } else {
    throw 11;
  }
  const std::string &pure_basename = GetProductPureBaseName(product);
  if (yyyymmdd < GetProductSpecificStartDate(pure_basename)) {
    return true;
  }

  if (product_holidays_.find(pure_basename) != product_holidays_.end()) {
    const std::set<int> &holidays = product_holidays_[pure_basename];
    if (holidays.find(yyyymmdd) != holidays.end()) {
      return true;
    }
  }

  if(IsExceptionalNormalTradingDay(exchange, yyyymmdd)){
    return false;
  }else if (is_weekend_holiday && IsWeekend(yyyymmdd)) {
    return true;
  }

  return false;
}

bool HolidayManager::IsNumber(const std::string &str) {
  for (std::string::const_iterator iter = str.begin(); iter != str.end(); iter++) {
    if (!std::isdigit(*iter)) {
      return false;
    }
  }
  return true;
}

bool HolidayManager::IsExchangeValid(const std::string &exchange) {
  return (HFSAT::StringToExchSource(exchange) != HFSAT::kExchSourceInvalid);
}

void HolidayManager::PrintProductHolidays() {
  for (auto iter2 = product_holidays_.begin(); iter2 != product_holidays_.end(); iter2++) {
    std::cout << iter2->first << "\n";
    for (auto iter3 = iter2->second.begin(); iter3 != iter2->second.end(); iter3++) {
      std::cout << *iter3 << " ";
    }
    std::cout << "\n";
  }
  std::cout << "\n";
}

void HolidayManager::PrintProductStartDates() {
  for (auto iter = product_start_date_.begin(); iter != product_start_date_.end(); iter++) {
    std::cout << iter->first << " " << iter->second << "\n";
    std::cout << "\n";
  }
  std::cout << "\n";
}

int HolidayManager::GetExchangeStartDate(const std::string &exchange) {
  if (IsExchangeValid(exchange)) {
    if (exchange_start_date_.find(exchange) != exchange_start_date_.end()) {
      return exchange_start_date_[exchange];
    }
  } else {
    std::cerr << typeid(*this).name() << ':' << __func__ << " "
              << "Invalid Exchange : " << exchange << "\n";
    throw 10;
  }
  return 0;
}

int HolidayManager::GetProductStartDate(const std::string &product) {
  // Logic: Product Start Date is max of ExchangeStartDate and ProductSpecificStartDate. A product cannot start before
  // exchange starts, but its not necessary that when exchange starts, product also starts.

  std::string exchange = GetExchangeForProduct(product);
  const std::string &pure_basename = GetProductPureBaseName(product);
  return std::max(GetExchangeStartDate(exchange), GetProductSpecificStartDate(pure_basename));
}

std::string HolidayManager::GetExchangeForProduct(const std::string &product) {
  SecurityDefinitions &sec_def = SecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateUTC());

  if (sec_def.contract_specification_map_.find(product) != sec_def.contract_specification_map_.end()) {
    return ExchSourceStringForm(sec_def.contract_specification_map_[product].exch_source_);
  } else if (product.substr(0, 4) == "NSE_") {
    return EXCHANGE_KEYS::kExchSourceNSEStr;
  } else if (product.substr(0, 4) == "BSE_") {
    return EXCHANGE_KEYS::kExchSourceBSEStr;
  } else if (product.substr(0, 3) == "HK_") {
    return EXCHANGE_KEYS::kExchSourceHONGKONGStr;
  } else {
    return EXCHANGE_KEYS::kExchSourceINVALIDStr;
  }
}

int HolidayManager::GetProductSpecificStartDate(const std::string &pure_basename) {
  if (product_start_date_.find(pure_basename) != product_start_date_.end()) {
    return product_start_date_[pure_basename];
  }
  return 0;
}

bool HolidayManager::IsWeekend(const int yyyymmdd) {
  int year = (yyyymmdd / 10000) % 10000;
  int mon = (yyyymmdd / 100) % 100;
  int date = yyyymmdd % 100;

  boost::gregorian::date day(year, mon, date);
  std::string day_of_week_ = day.day_of_week().as_long_string();

  return (day_of_week_ == "Saturday" || day_of_week_ == "Sunday");
}

const std::string HolidayManager::GetProductPureBaseName(const std::string &product) {
  if (product.substr(0, 2) == "SP" || product.substr(0, 3) == "FLY") {
    return product;
  }
  std::size_t found = product.find_last_of("_");
  if (IsNumber(product.substr(found + 1))) {
    return product.substr(0, found);
  } else {
    return product;
  }
}

namespace HolidayManagerNoThrow {

bool IsExchangeHoliday(const std::string &exchange, const int yyyymmdd, bool is_weekend_holiday) {
  try {
    return HolidayManager::GetUniqueInstance().IsExchangeHoliday(exchange, yyyymmdd, is_weekend_holiday);
  } catch (...) {
    std::cerr << "Error in Holiday Manager. Exiting.\n";
    std::exit(1);
  }
}
bool IsProductHoliday(const std::string &product, const int yyyymmdd, bool is_weekend_holiday) {
  try {
    return HolidayManager::GetUniqueInstance().IsProductHoliday(product, yyyymmdd, is_weekend_holiday);
  } catch (...) {
    std::cerr << "Error in Holiday Manager. Exiting.\n";
    std::exit(1);
  }
}
}  // namespace HolidayManagerNoThrow
}  // namespace HFSAT
