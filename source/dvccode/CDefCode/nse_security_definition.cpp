/**
   \file CDefCode/nse_security_definition.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#include <algorithm>
#include <boost/date_time/gregorian/gregorian.hpp>

#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CDef/currency_convertor.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"

#include "dvccode/Utils/exchange_names.hpp"
#include "dvccode/Utils/holiday_manager.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"

// filepaths are relative to /spare/local/tradeinfo/
#define NSE_CONTRACT_FILENAME_PREFIX "NSE_Files/ContractFiles/nse_contracts"
#define NSE_DATASOURCE_EXCHSYMBOL_FILE "NSE_Files/datasource_exchsymbol.txt"
#define NSE_INTEREST_RATE_FILE "NSE_Files/interest_rates.txt"
#define NSE_BHAVCOPY_FILENAME_PREFIX "NSE_Files/BhavCopy/fo"
#define NSE_ORDER_LIMIT_FILENAME_PREFIX "/spare/local/files/NSEFTPFiles"
#define NSE_CD_BHAVCOPY_FILENAME_PREFIX "NSE_Files/BhavCopy/cds"
#define NSE_BANNED_SECURITIES_PREFIX "NSE_Files/SecuritiesUnderBan/fo_secban_"
#define NSE_EXPIRY_FILE_PREFIX "NSE_Files/expiry_dates_"
#define NSE_LTP_FILENAME_PREFIX "NSE_Files/LTP"
#define WEEKLY_REDEFINED_DATE 20141003

namespace HFSAT {

const boost::gregorian::date_duration one_day_duration_(1);

NSESecurityDefinitions* NSESecurityDefinitions::p_uniqueinstance_ = NULL;
int NSESecurityDefinitions::intdate_ = 0;
std::vector<NSEInstrumentParams_t*> NSESecurityDefinitions::inst_params_;
std::vector<std::string> NSESecurityDefinitions::non_zero_OI_;
std::map<NSEOPtionsIdentifier_t, double> NSESecurityDefinitions::options_param_2_last_close_;
std::map<std::string, double> NSESecurityDefinitions::shortcode_2_options_last_close_;
std::map<std::string, NSETradingLimit_t*> NSESecurityDefinitions::shortcode_2_price_limit_;
std::map<std::string, int > NSESecurityDefinitions::shortcode_2_surveillance_id_;
std::vector<std::string> NSESecurityDefinitions::banned_prod_list_;
std::map<int, uint32_t> NSESecurityDefinitions::expirydate_2_contractnumber_;
std::map<int, uint32_t> NSESecurityDefinitions::expirydate_2_contractnumber_cf_;
std::map<int, uint32_t> NSESecurityDefinitions::weekly_option_expirydate_2_contractnumber_;
std::map<uint32_t, int> NSESecurityDefinitions::contractnumber_2_expirydate_;
std::map<uint32_t, int> NSESecurityDefinitions::contractnumber_cf_2_expirydate_;
std::map<uint32_t, int> NSESecurityDefinitions::weekly_option_contractnumber_2_expirydate_;

std::map<int, uint32_t> NSESecurityDefinitions::expirydate_2_contractnumber_monday_;
std::map<int, uint32_t> NSESecurityDefinitions::weekly_option_expirydate_2_contractnumber_monday_;
std::map<uint32_t, int> NSESecurityDefinitions::contractnumber_2_expirydate_monday_;
std::map<uint32_t, int> NSESecurityDefinitions::weekly_option_contractnumber_2_expirydate_monday_;

std::map<int, uint32_t> NSESecurityDefinitions::expirydate_2_contractnumber_tuesday_;
std::map<int, uint32_t> NSESecurityDefinitions::weekly_option_expirydate_2_contractnumber_tuesday_;
std::map<uint32_t, int> NSESecurityDefinitions::contractnumber_2_expirydate_tuesday_;
std::map<uint32_t, int> NSESecurityDefinitions::weekly_option_contractnumber_2_expirydate_tuesday_;

std::map<int, uint32_t> NSESecurityDefinitions::expirydate_2_contractnumber_wednesday_;
std::map<int, uint32_t> NSESecurityDefinitions::weekly_option_expirydate_2_contractnumber_wednesday_;
std::map<uint32_t, int> NSESecurityDefinitions::contractnumber_2_expirydate_wednesday_;
std::map<uint32_t, int> NSESecurityDefinitions::weekly_option_contractnumber_2_expirydate_wednesday_;

std::map<int, uint32_t> NSESecurityDefinitions::expirydate_2_contractnumber_friday_;
std::map<int, uint32_t> NSESecurityDefinitions::weekly_option_expirydate_2_contractnumber_friday_;
std::map<uint32_t, int> NSESecurityDefinitions::contractnumber_2_expirydate_friday_;
std::map<uint32_t, int> NSESecurityDefinitions::weekly_option_contractnumber_2_expirydate_friday_;

std::map<std::string, std::string> NSESecurityDefinitions::shortcode_2_exchsymbol_;
std::map<std::string, std::string> NSESecurityDefinitions::exchsymbol_2_shortcode_;
std::map<std::string, std::string> NSESecurityDefinitions::exchsymbol_2_shortcode_Weekly;
std::map<std::string, std::string> NSESecurityDefinitions::datasourcename_2_exchsymbol_;
std::map<std::string, std::string> NSESecurityDefinitions::exchsymbol_2_datasourcename_;
std::map<std::string, std::string> NSESecurityDefinitions::exchsymbol_2_datasourcesym_;
std::map<std::string, int> NSESecurityDefinitions::underlying_2_itr_num;

// For supporting all options shortcodes with non-zero OI
std::map<std::string, std::string> NSESecurityDefinitions::exchsymbol_2_datasourcename_all_;
std::map<std::string, std::string> NSESecurityDefinitions::shortcode_2_exchsymbol_all_;

std::map<std::string, NSEInstrumentType_t> NSESecurityDefinitions::shortcode_2_type_;
std::map<std::string, NSEInstrumentType_t> NSESecurityDefinitions::generic_shortcode_2_type_;
std::map<std::string, double> NSESecurityDefinitions::shortcode_2_commissions_;

std::map<std::string, bool> NSESecurityDefinitions::shortcode_2_is_spread_;
std::map<std::string, bool> NSESecurityDefinitions::exchange_symbol_2_is_spread_;

std::map<std::string, double> NSESecurityDefinitions::shortcode_2_last_close_;
std::map<std::string, int> NSESecurityDefinitions::datasource_2_openInterest_;

std::map<std::string, int> NSESecurityDefinitions::shortcode_2_expiry_date_;
std::map<std::string, double> NSESecurityDefinitions::shortcode_2_strike_price_;
std::map<std::string, double> NSESecurityDefinitions::shortcode_2_step_value_;
std::map<std::string, std::vector<NSEOptionMetaInfo>> NSESecurityDefinitions::underlying_to_options_;

std::map<std::string, std::tuple<int, char, double>> NSESecurityDefinitions::underlying_2_corporate_action_value_;

std::vector<int> expiry_dates_nifty_weekly;
std::vector<int> expiry_dates_nifty;
std::vector<int> expiry_dates_banknifty;
std::vector<int> expiry_dates_midcpnifty;
std::vector<int> expiry_dates_finnifty;

NSESecurityDefinitions::NSESecurityDefinitions(int t_intdate_) {
  CurrencyConvertor::SetUniqueInstance(t_intdate_);
  // If the date for which the NSE files are fetched is a holiday,
  // then we look back at the last working day

  while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceNSEStr, t_intdate_, true)) {
    t_intdate_ = HFSAT::DateTime::CalcPrevDay(t_intdate_);
  }

  intdate_ = t_intdate_;
  inst_params_.clear();
  LoadCorporateActions(intdate_);
  if (intdate_ > 20250101){
    LoadIndexExpiries(intdate_);
  }
  else{
    PopulateExpiryMap();
    PopulateMondayExpiryMap();
    PopulateTuesdayExpiryMap();
    PopulateWednesdayExpiryMap();
    PopulateFridayExpiryMap();
    PopulateCurrencyExpiryMap();
    PopulateWeeklyOptionsExpiryMap();
    if(intdate_<20241126){
      PopulateMondayWeeklyOptionsExpiryMap();
    }
    if(intdate_<20241127){
      PopulateTuesdayWeeklyOptionsExpiryMap();
    }
    if(intdate_<20241114){
      PopulateWednesdayWeeklyOptionsExpiryMap();
    }
  }

  //PopulateFridayExpiryMap();
  PopulateCurrencyExpiryMap();
  //PopulateFridayWeeklyOptionsExpiryMap();
  LoadBhavCopyParams(intdate_);
  LoadCDBhavCopyParams(intdate_);
  LoadInstParams(intdate_);
  LoadCMBhavCopyParams(intdate_);
  LoadDataSourceExchangeSymbolMap();
  banned_prod_list_ = LoadBannedSecurities(intdate_);
}

void NSESecurityDefinitions::LoadCorporateActions(const int t_date_) {
  std::ostringstream nse_corporate_actions_file_oss_;
  std::string nse_corporate_actions_file_name_ = "";
  int curr_expiry_date = ComputeExpiryDate(0);
  nse_corporate_actions_file_oss_ << "/spare/local/tradeinfo/NSE_Files/corporate_actions.csv";

  nse_corporate_actions_file_name_ = nse_corporate_actions_file_oss_.str();

  if (FileUtils::ExistsAndReadable(nse_corporate_actions_file_name_)) {
    std::ifstream nse_corporate_actions_file_;
    nse_corporate_actions_file_.open(nse_corporate_actions_file_name_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (nse_corporate_actions_file_.is_open()) {
      while (nse_corporate_actions_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        nse_corporate_actions_file_.getline(readline_buffer_, sizeof(readline_buffer_));

        std::vector<char*> tokens_;
        char readline_buffer_copy_[1024];
        std::string trimmed_str_;
        memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
        strcpy(readline_buffer_copy_, readline_buffer_);

        HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, ",", tokens_);
        if (tokens_.size() >= 5 && tokens_[0][0] != '#') {
          int corporate_action_date = std::atoi(tokens_[0]);
          int corp_action_expiry_date = ComputeNextExpiry(corporate_action_date);
          if (curr_expiry_date == corp_action_expiry_date && t_date_ >= corporate_action_date) {
            std::string corporate_action_type_;
            char operation_type_ = tokens_[3][0];
            double corporate_action_value_ = std::stof(tokens_[4]);
            HFSAT::PerishableStringTokenizer::TrimString(tokens_[1], trimmed_str_, ' ');
            HFSAT::PerishableStringTokenizer::TrimString(tokens_[2], corporate_action_type_, ' ');

            underlying_2_corporate_action_value_[trimmed_str_] =
                std::make_tuple(corporate_action_date, operation_type_, corporate_action_value_);
          }
        }
      }
    }
    nse_corporate_actions_file_.close();
  } else {
    std::cerr << "NSE Corporate_Action file " << nse_corporate_actions_file_name_ << " not present\n";
  }
}

void NSESecurityDefinitions::PopulateCurrencyExpiryMap() {
  // Support for only 3 out of 12 listed currently
  for (uint32_t t_ctr_ = 0; t_ctr_ < 3; t_ctr_++) {
    contractnumber_cf_2_expirydate_[t_ctr_] = ComputeExpiryDate(t_ctr_, true);
    expirydate_2_contractnumber_cf_[contractnumber_cf_2_expirydate_[t_ctr_]] = t_ctr_;
  }
}

int NSESecurityDefinitions::ComputeNextExpiryCurrency(const int t_date_) {
  int t_month_ = (t_date_ % 10000) / 100;
  int t_year_ = t_date_ / 10000;

  boost::gregorian::date first_of_month_(t_year_, t_month_, 1);
  boost::gregorian::date t_date_ctr_ = first_of_month_.end_of_month();
  boost::gregorian::date::ymd_type ymd = t_date_ctr_.year_month_day();

  int t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;

  /// we need to find three working days
  for (int i = 0; i < 3; i++) {
    while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceNSEStr, t_expiry_date_, true) ||
           HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceNSE_CDStr, t_expiry_date_, true)) {
      t_date_ctr_ -= one_day_duration_;
      ymd = t_date_ctr_.year_month_day();
      t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
    }
    if (i < 2) {
      t_date_ctr_ -= one_day_duration_;
      ymd = t_date_ctr_.year_month_day();
      t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
    }
  }

  if (t_expiry_date_ >= t_date_) {
    return t_expiry_date_;
  } else  // set date to first of next month
  {
    int t_temp_date_;
    if (t_month_ == 12) {
      t_temp_date_ = (t_year_ + 1) * 10000 + 101;
    } else {
      t_temp_date_ = t_year_ * 10000 + (t_month_ + 1) * 100 + 1;
    }
    return ComputeNextExpiryCurrency(t_temp_date_);
  }
}

void NSESecurityDefinitions::PopulateExpiryMap() {
  // 3 contracts outstanding in NSE
  for (uint32_t t_ctr_ = 0; t_ctr_ < 3; t_ctr_++) {
    contractnumber_2_expirydate_[t_ctr_] = ComputeExpiryDate(t_ctr_);
    expirydate_2_contractnumber_[contractnumber_2_expirydate_[t_ctr_]] = t_ctr_;
  }
}

int NSESecurityDefinitions::ComputeNextExpiry(const int t_date_) {
  int t_month_ = (t_date_ % 10000) / 100;
  int t_year_ = t_date_ / 10000;

  boost::gregorian::last_day_of_the_week_in_month lwdm(boost::gregorian::Thursday, t_month_);
  boost::gregorian::date l_thursday_ = lwdm.get_date(t_year_);
  boost::gregorian::date::ymd_type ymd = l_thursday_.year_month_day();
  int t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;

  while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceNSEStr, t_expiry_date_, true)) {
    l_thursday_ -= one_day_duration_;
    boost::gregorian::date::ymd_type ymd = l_thursday_.year_month_day();
    t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
  }

  if (t_expiry_date_ >= t_date_) {
    return t_expiry_date_;
  } else  // set date to first of next month
  {
    int t_temp_date_;
    if (t_month_ == 12) {
      t_temp_date_ = (t_year_ + 1) * 10000 + 101;
    } else {
      t_temp_date_ = t_year_ * 10000 + (t_month_ + 1) * 100 + 1;
    }
    return ComputeNextExpiry(t_temp_date_);
  }
}

void NSESecurityDefinitions::PopulateMondayExpiryMap() {
  // 3 contracts outstanding in NSE  FINNIFTY
  for (uint32_t t_ctr_ = 0; t_ctr_ < 3; t_ctr_++) {
    contractnumber_2_expirydate_monday_[t_ctr_] = ComputeMondayExpiryDate(t_ctr_);

    expirydate_2_contractnumber_monday_[contractnumber_2_expirydate_monday_[t_ctr_]] = t_ctr_;
  }
}


void NSESecurityDefinitions::PopulateTuesdayExpiryMap() {
  // 3 contracts outstanding in NSE  FINNIFTY
  for (uint32_t t_ctr_ = 0; t_ctr_ < 3; t_ctr_++) {
    contractnumber_2_expirydate_tuesday_[t_ctr_] = ComputeTuesdayExpiryDate(t_ctr_);

    expirydate_2_contractnumber_tuesday_[contractnumber_2_expirydate_tuesday_[t_ctr_]] = t_ctr_;
  }
}

void NSESecurityDefinitions::PopulateWednesdayExpiryMap() {
  // 3 contracts outstanding in NSE  MIDCPNIFTY
  for (uint32_t t_ctr_ = 0; t_ctr_ < 3; t_ctr_++) {
    contractnumber_2_expirydate_wednesday_[t_ctr_] = ComputeWednesdayExpiryDate(t_ctr_);
    expirydate_2_contractnumber_wednesday_[contractnumber_2_expirydate_wednesday_[t_ctr_]] = t_ctr_;
  }
}

void NSESecurityDefinitions::PopulateFridayExpiryMap() {
  // 3 contracts outstanding in NSE  NIFTYNXT50
  for (uint32_t t_ctr_ = 0; t_ctr_ < 3; t_ctr_++) {
    contractnumber_2_expirydate_friday_[t_ctr_] = ComputeFridayExpiryDate(t_ctr_);
    expirydate_2_contractnumber_friday_[contractnumber_2_expirydate_friday_[t_ctr_]] = t_ctr_;
  }
}

int NSESecurityDefinitions::ComputeMondayNextExpiry(const int t_date_) {
  int t_month_ = (t_date_ % 10000) / 100;
  int t_year_ = t_date_ / 10000;

  boost::gregorian::last_day_of_the_week_in_month lwdm(boost::gregorian::Monday, t_month_);
  boost::gregorian::date l_monday_ = lwdm.get_date(t_year_);
  boost::gregorian::date::ymd_type ymd = l_monday_.year_month_day();
  int t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;

  while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceNSEStr, t_expiry_date_, true)) {
    l_monday_ -= one_day_duration_;
    boost::gregorian::date::ymd_type ymd = l_monday_.year_month_day();
    t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
  }

  if (t_expiry_date_ >= t_date_) {
    return t_expiry_date_;
  } else  // set date to first of next month
  {
    int t_temp_date_;
    if (t_month_ == 12) {
      t_temp_date_ = (t_year_ + 1) * 10000 + 101;
    } else {
      t_temp_date_ = t_year_ * 10000 + (t_month_ + 1) * 100 + 1;
    }
    return ComputeMondayNextExpiry(t_temp_date_);
  }
}

int NSESecurityDefinitions::ComputeTuesdayNextExpiry(const int t_date_) {
  int t_month_ = (t_date_ % 10000) / 100;
  int t_year_ = t_date_ / 10000;

  boost::gregorian::last_day_of_the_week_in_month lwdm(boost::gregorian::Tuesday, t_month_);
  boost::gregorian::date l_tuesday_ = lwdm.get_date(t_year_);
  boost::gregorian::date::ymd_type ymd = l_tuesday_.year_month_day();
  int t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;

  while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceNSEStr, t_expiry_date_, true)) {
    l_tuesday_ -= one_day_duration_;
    boost::gregorian::date::ymd_type ymd = l_tuesday_.year_month_day();
    t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
  }

  if (t_expiry_date_ >= t_date_) {
    return t_expiry_date_;
  } else  // set date to first of next month
  {
    int t_temp_date_;
    if (t_month_ == 12) {
      t_temp_date_ = (t_year_ + 1) * 10000 + 101;
    } else {
      t_temp_date_ = t_year_ * 10000 + (t_month_ + 1) * 100 + 1;
    }
    return ComputeTuesdayNextExpiry(t_temp_date_);
  }
}

int NSESecurityDefinitions::ComputeWednesdayNextExpiry(const int t_date_) {
  int t_month_ = (t_date_ % 10000) / 100;
  int t_year_ = t_date_ / 10000;

  boost::gregorian::last_day_of_the_week_in_month lwdm(boost::gregorian::Wednesday, t_month_);
  boost::gregorian::date l_wednesday_ = lwdm.get_date(t_year_);
  boost::gregorian::date::ymd_type ymd = l_wednesday_.year_month_day();
  int t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;

  while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceNSEStr, t_expiry_date_, true)) {
    l_wednesday_ -= one_day_duration_;
    boost::gregorian::date::ymd_type ymd = l_wednesday_.year_month_day();
    t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
  }

  if (t_expiry_date_ >= t_date_) {
    return t_expiry_date_;
  } else  // set date to first of next month
  {
    int t_temp_date_;
    if (t_month_ == 12) {
      t_temp_date_ = (t_year_ + 1) * 10000 + 101;
    } else {
      t_temp_date_ = t_year_ * 10000 + (t_month_ + 1) * 100 + 1;
    }
    return ComputeWednesdayNextExpiry(t_temp_date_);
  }
}

int NSESecurityDefinitions::ComputeFridayNextExpiry(const int t_date_) {
  int t_month_ = (t_date_ % 10000) / 100;
  int t_year_ = t_date_ / 10000;

  boost::gregorian::last_day_of_the_week_in_month lwdm(boost::gregorian::Friday, t_month_);
  boost::gregorian::date l_friday_ = lwdm.get_date(t_year_);
  boost::gregorian::date::ymd_type ymd = l_friday_.year_month_day();
  int t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;

  while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceNSEStr, t_expiry_date_, true)) {
    l_friday_ -= one_day_duration_;
    boost::gregorian::date::ymd_type ymd = l_friday_.year_month_day();
    t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
  }

  if (t_expiry_date_ >= t_date_) {
    return t_expiry_date_;
  } else  // set date to first of next month
  {
    int t_temp_date_;
    if (t_month_ == 12) {
      t_temp_date_ = (t_year_ + 1) * 10000 + 101;
    } else {
      t_temp_date_ = t_year_ * 10000 + (t_month_ + 1) * 100 + 1;
    }
    return ComputeFridayNextExpiry(t_temp_date_);
  }
}

// HELPER IN Tradeengine calls it
int NSESecurityDefinitions::ComputePreviousExpiry(const int t_date_, const std::string& _shortcode_) {
  if (_shortcode_.find("NSE_FINNIFTY") != std::string::npos || (_shortcode_.find("MIDCPNIFTY") != std::string::npos && intdate_ < 20230426 )) {
    return ComputeTuesdayPreviousExpiry(t_date_, _shortcode_);
  } else if (_shortcode_.find("NSE_MIDCPNIFTY") != std::string::npos && intdate_ < 20230817){
    return ComputeWednesdayPreviousExpiry(t_date_, _shortcode_);
  } else if  (_shortcode_.find("NSE_MIDCPNIFTY") != std::string::npos){
    return ComputeMondayPreviousExpiry(t_date_, _shortcode_);
  } else if  (_shortcode_.find("NSE_BANKNIFTY") != std::string::npos && ((intdate_ > 20230901 && IsWeeklyOption(_shortcode_)) ||intdate_ >= 20240301)){
      return ComputeWednesdayPreviousExpiry(t_date_, _shortcode_);
  } else if  (_shortcode_.find("NSE_NIFTYNXT50") != std::string::npos ){
      return ComputeFridayPreviousExpiry(t_date_, _shortcode_);
  }
  int t_month_ = (t_date_ % 10000) / 100;
  int t_year_ = t_date_ / 10000;
  int t_first_date_prev_month_;

  bool is_currency_ = IsCurrency(_shortcode_);

  int t_first_date_curr_month_ = t_year_ * 10000 + (t_month_)*100 + 1;

  int this_month_expiry_;
  if (is_currency_) {
    this_month_expiry_ = ComputeNextExpiryCurrency(t_first_date_curr_month_);
  } else {
    this_month_expiry_ = ComputeNextExpiry(t_first_date_curr_month_);
  }

  if (this_month_expiry_ < t_date_) return this_month_expiry_;

  if (t_month_ == 1) {
    t_first_date_prev_month_ = (t_year_ - 1) * 10000 + 100 * 12 + 1;
  } else {
    t_first_date_prev_month_ = t_year_ * 10000 + (t_month_ - 1) * 100 + 1;
  }

  int prev_month_expiry_;
  if (is_currency_) {
    prev_month_expiry_ = ComputeNextExpiryCurrency(t_first_date_prev_month_);
  } else {
    prev_month_expiry_ = ComputeNextExpiry(t_first_date_prev_month_);
  }
  return prev_month_expiry_;
}

int NSESecurityDefinitions::ComputeMondayPreviousExpiry(const int t_date_, const std::string& _shortcode_) {
  int t_month_ = (t_date_ % 10000) / 100;
  int t_year_ = t_date_ / 10000;
  int t_first_date_prev_month_;

  bool is_currency_ = IsCurrency(_shortcode_);

  int t_first_date_curr_month_ = t_year_ * 10000 + (t_month_)*100 + 1;

  int this_month_expiry_;
  if (is_currency_) {
    this_month_expiry_ = ComputeNextExpiryCurrency(t_first_date_curr_month_);
  } else {
    this_month_expiry_ = ComputeMondayNextExpiry(t_first_date_curr_month_);
  }

  if (this_month_expiry_ < t_date_) return this_month_expiry_;

  if (t_month_ == 1) {
    t_first_date_prev_month_ = (t_year_ - 1) * 10000 + 100 * 12 + 1;
  } else {
    t_first_date_prev_month_ = t_year_ * 10000 + (t_month_ - 1) * 100 + 1;
  }

  int prev_month_expiry_;
  if (is_currency_) {
    prev_month_expiry_ = ComputeNextExpiryCurrency(t_first_date_prev_month_);
  } else {
    prev_month_expiry_ = ComputeMondayNextExpiry(t_first_date_prev_month_);
  }
  return prev_month_expiry_;
}

int NSESecurityDefinitions::ComputeTuesdayPreviousExpiry(const int t_date_, const std::string& _shortcode_) {
  int t_month_ = (t_date_ % 10000) / 100;
  int t_year_ = t_date_ / 10000;
  int t_first_date_prev_month_;

  bool is_currency_ = IsCurrency(_shortcode_);

  int t_first_date_curr_month_ = t_year_ * 10000 + (t_month_)*100 + 1;

  int this_month_expiry_;
  if (is_currency_) {
    this_month_expiry_ = ComputeNextExpiryCurrency(t_first_date_curr_month_);
  } else {
    this_month_expiry_ = ComputeTuesdayNextExpiry(t_first_date_curr_month_);
  }

  if (this_month_expiry_ < t_date_) return this_month_expiry_;

  if (t_month_ == 1) {
    t_first_date_prev_month_ = (t_year_ - 1) * 10000 + 100 * 12 + 1;
  } else {
    t_first_date_prev_month_ = t_year_ * 10000 + (t_month_ - 1) * 100 + 1;
  }

  int prev_month_expiry_;
  if (is_currency_) {
    prev_month_expiry_ = ComputeNextExpiryCurrency(t_first_date_prev_month_);
  } else {
    prev_month_expiry_ = ComputeTuesdayNextExpiry(t_first_date_prev_month_);
  }
  return prev_month_expiry_;
}

int NSESecurityDefinitions::ComputeWednesdayPreviousExpiry(const int t_date_, const std::string& _shortcode_) {
  int t_month_ = (t_date_ % 10000) / 100;
  int t_year_ = t_date_ / 10000;
  int t_first_date_prev_month_;

  bool is_currency_ = IsCurrency(_shortcode_);

  int t_first_date_curr_month_ = t_year_ * 10000 + (t_month_)*100 + 1;

  int this_month_expiry_;
  if (is_currency_) {
    this_month_expiry_ = ComputeNextExpiryCurrency(t_first_date_curr_month_);
  } else {
    this_month_expiry_ = ComputeWednesdayNextExpiry(t_first_date_curr_month_);
  }

  if (this_month_expiry_ < t_date_) return this_month_expiry_;

  if (t_month_ == 1) {
    t_first_date_prev_month_ = (t_year_ - 1) * 10000 + 100 * 12 + 1;
  } else {
    t_first_date_prev_month_ = t_year_ * 10000 + (t_month_ - 1) * 100 + 1;
  }

  int prev_month_expiry_;
  if (is_currency_) {
    prev_month_expiry_ = ComputeNextExpiryCurrency(t_first_date_prev_month_);
  } else {
    prev_month_expiry_ = ComputeWednesdayNextExpiry(t_first_date_prev_month_);
  }
  return prev_month_expiry_;
}

int NSESecurityDefinitions::ComputeFridayPreviousExpiry(const int t_date_, const std::string& _shortcode_) {
  int t_month_ = (t_date_ % 10000) / 100;
  int t_year_ = t_date_ / 10000;
  int t_first_date_prev_month_;

  bool is_currency_ = IsCurrency(_shortcode_);

  int t_first_date_curr_month_ = t_year_ * 10000 + (t_month_)*100 + 1;

  int this_month_expiry_;
  if (is_currency_) {
    this_month_expiry_ = ComputeNextExpiryCurrency(t_first_date_curr_month_);
  } else {
    this_month_expiry_ = ComputeFridayNextExpiry(t_first_date_curr_month_);
  }

  if (this_month_expiry_ < t_date_) return this_month_expiry_;

  if (t_month_ == 1) {
    t_first_date_prev_month_ = (t_year_ - 1) * 10000 + 100 * 12 + 1;
  } else {
    t_first_date_prev_month_ = t_year_ * 10000 + (t_month_ - 1) * 100 + 1;
  }

  int prev_month_expiry_;
  if (is_currency_) {
    prev_month_expiry_ = ComputeNextExpiryCurrency(t_first_date_prev_month_);
  } else {
    prev_month_expiry_ = ComputeFridayNextExpiry(t_first_date_prev_month_);
  }
  return prev_month_expiry_;
}

int NSESecurityDefinitions::ComputeExpiryDate(const int expiry_number_, bool is_currency_) {
  int t_expiry_number_ = expiry_number_;
  int t_temp_date_ = intdate_;
  int t_expiry_date_ = 0;
  while (t_expiry_number_ >= 0) {
    if (!is_currency_) {
      t_expiry_date_ = ComputeNextExpiry(t_temp_date_);
    } else {
      t_expiry_date_ = ComputeNextExpiryCurrency(t_temp_date_);
    }
    t_expiry_number_--;
    if (t_expiry_number_ >= 0)  // set date to 1st of next month
    {
      int t_month_ = (t_expiry_date_ % 10000) / 100;
      int t_year_ = t_expiry_date_ / 10000;
      if (t_month_ == 12) {
        t_temp_date_ = (t_year_ + 1) * 10000 + 101;
      } else {
        t_temp_date_ = t_year_ * 10000 + (t_month_ + 1) * 100 + 1;
      }
    }  // end if
  }    // end while
  return t_expiry_date_;
}

int NSESecurityDefinitions::ComputeMondayExpiryDate(const int expiry_number_, bool is_currency_) {
  int t_expiry_number_ = expiry_number_;
  int t_temp_date_ = intdate_;
  int t_expiry_date_ = 0;
  while (t_expiry_number_ >= 0) {
    if (!is_currency_) {
      t_expiry_date_ = ComputeMondayNextExpiry(t_temp_date_);
    } else {
      t_expiry_date_ = ComputeNextExpiryCurrency(t_temp_date_);
    }
    t_expiry_number_--;
    if (t_expiry_number_ >= 0)  // set date to 1st of next month
    {
      int t_month_ = (t_expiry_date_ % 10000) / 100;
      int t_year_ = t_expiry_date_ / 10000;
      if (t_month_ == 12) {
        t_temp_date_ = (t_year_ + 1) * 10000 + 101;
      } else {
        t_temp_date_ = t_year_ * 10000 + (t_month_ + 1) * 100 + 1;
      }
    }  // end if
  }    // end while
  return t_expiry_date_;
}

int NSESecurityDefinitions::ComputeTuesdayExpiryDate(const int expiry_number_, bool is_currency_) {
  int t_expiry_number_ = expiry_number_;
  int t_temp_date_ = intdate_;
  int t_expiry_date_ = 0;
  while (t_expiry_number_ >= 0) {
    if (!is_currency_) {
      t_expiry_date_ = ComputeTuesdayNextExpiry(t_temp_date_);
    } else {
      t_expiry_date_ = ComputeNextExpiryCurrency(t_temp_date_);
    }
    t_expiry_number_--;
    if (t_expiry_number_ >= 0)  // set date to 1st of next month
    {
      int t_month_ = (t_expiry_date_ % 10000) / 100;
      int t_year_ = t_expiry_date_ / 10000;
      if (t_month_ == 12) {
        t_temp_date_ = (t_year_ + 1) * 10000 + 101;
      } else {
        t_temp_date_ = t_year_ * 10000 + (t_month_ + 1) * 100 + 1;
      }
    }  // end if
  }    // end while
  return t_expiry_date_;
}

int NSESecurityDefinitions::ComputeWednesdayExpiryDate(const int expiry_number_, bool is_currency_) {
  int t_expiry_number_ = expiry_number_;
  int t_temp_date_ = intdate_;
  int t_expiry_date_ = 0;
  while (t_expiry_number_ >= 0) {
    if (!is_currency_) {
      t_expiry_date_ = ComputeWednesdayNextExpiry(t_temp_date_);
    } else {
      t_expiry_date_ = ComputeNextExpiryCurrency(t_temp_date_);
    }
    t_expiry_number_--;
    if (t_expiry_number_ >= 0)  // set date to 1st of next month
    {
      int t_month_ = (t_expiry_date_ % 10000) / 100;
      int t_year_ = t_expiry_date_ / 10000;
      if (t_month_ == 12) {
        t_temp_date_ = (t_year_ + 1) * 10000 + 101;
      } else {
        t_temp_date_ = t_year_ * 10000 + (t_month_ + 1) * 100 + 1;
      }
    }  // end if
  }    // end while
  return t_expiry_date_;
}

int NSESecurityDefinitions::ComputeFridayExpiryDate(const int expiry_number_, bool is_currency_) {
  int t_expiry_number_ = expiry_number_;
  int t_temp_date_ = intdate_;
  int t_expiry_date_ = 0;
  while (t_expiry_number_ >= 0) {
    if (!is_currency_) {
      t_expiry_date_ = ComputeFridayNextExpiry(t_temp_date_);
    } else {
      t_expiry_date_ = ComputeNextExpiryCurrency(t_temp_date_);
    }
    t_expiry_number_--;
    if (t_expiry_number_ >= 0)  // set date to 1st of next month
    {
      int t_month_ = (t_expiry_date_ % 10000) / 100;
      int t_year_ = t_expiry_date_ / 10000;
      if (t_month_ == 12) {
        t_temp_date_ = (t_year_ + 1) * 10000 + 101;
      } else {
        t_temp_date_ = t_year_ * 10000 + (t_month_ + 1) * 100 + 1;
      }
    }  // end if
  }    // end while
  return t_expiry_date_;
}

void NSESecurityDefinitions::PopulateWeeklyOptionsExpiryMap() {
  std::set<int> monthly_fo_expiries;

  // don't populate if the date is beyond the limit value as weekly counts on monthly expiry date also
  if (intdate_ < WEEKLY_REDEFINED_DATE) {
    for (uint32_t t_ctr_ = 0; t_ctr_ < 6; t_ctr_++) {
      monthly_fo_expiries.insert(ComputeExpiryDate(t_ctr_));
    }
  }

  for (uint32_t expiry_num = 0; expiry_num < 15; expiry_num++) {
    weekly_option_contractnumber_2_expirydate_[expiry_num] =
        ComputeExpiryDateWeeklyOptions(expiry_num, monthly_fo_expiries);
    weekly_option_expirydate_2_contractnumber_[weekly_option_contractnumber_2_expirydate_[expiry_num]] = expiry_num;
  }
}

int NSESecurityDefinitions::ComputeExpiryDateWeeklyOptions(const int expiry_number,
                                                           const std::set<int>& monthly_future_expiries) {
  int t_expiry_number_ = expiry_number;
  int t_temp_date_ = intdate_;
  int t_expiry_date_ = 0;
  while (t_expiry_number_ >= 0) {
    t_expiry_date_ = ComputeNextExpiryWeeklyOptions(t_temp_date_, monthly_future_expiries);
    t_expiry_number_--;
    if (t_expiry_number_ >= 0) {
      // Set to day next to nearest Thursday
      // 1. Calculate Next Thursday First (If a day is Thursday, this code returns next Thursday as the same day. This
      // assumption is very imp. If this assumption breaks, code will break.)
      boost::gregorian::date d1((t_expiry_date_ / 10000) % 10000, ((t_expiry_date_ / 100) % 100),
                                (t_expiry_date_ % 100));
      boost::gregorian::greg_weekday gw(boost::gregorian::Thursday);
      boost::gregorian::date next_thursday = next_weekday(d1, gw);
      boost::gregorian::date::ymd_type ymd = next_thursday.year_month_day();
      int date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
      // 2. Calculate Next day of Thursday
      t_temp_date_ = DateTime::CalcNextDay(date_);
    }
  }
  return t_expiry_date_;
}

int NSESecurityDefinitions::ComputeNextExpiryWeeklyOptions(const int t_date_,
                                                           const std::set<int>& monthly_future_expiries) {
  boost::gregorian::date d1((t_date_ / 10000) % 10000, ((t_date_ / 100) % 100), (t_date_ % 100));
  boost::gregorian::greg_weekday gw(boost::gregorian::Thursday);
  boost::gregorian::date next_thursday = next_weekday(d1, gw);

  boost::gregorian::date current_day = next_thursday;
  boost::gregorian::date::ymd_type ymd = current_day.year_month_day();
  int t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
  while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceNSEStr, t_expiry_date_, true)) {
    current_day -= one_day_duration_;
    boost::gregorian::date::ymd_type ymd = current_day.year_month_day();
    t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
  }

  if (t_expiry_date_ >= t_date_ && monthly_future_expiries.find(t_expiry_date_) == monthly_future_expiries.end()) {
    return t_expiry_date_;
  } else {
    boost::gregorian::date next_friday = next_thursday + boost::gregorian::date_duration(1);
    ymd = next_friday.year_month_day();
    return ComputeNextExpiryWeeklyOptions(((ymd.year * 100 + ymd.month) * 100) + ymd.day, monthly_future_expiries);
  }
}

void NSESecurityDefinitions::PopulateMondayWeeklyOptionsExpiryMap() {
  std::set<int> monthly_fo_expiries;

  if (intdate_ < WEEKLY_REDEFINED_DATE) {
    for (uint32_t t_ctr_ = 0; t_ctr_ < 6; t_ctr_++) {
      monthly_fo_expiries.insert(ComputeMondayExpiryDate(t_ctr_));
    }
  }

  for (uint32_t expiry_num = 0; expiry_num < 15; expiry_num++) {
    weekly_option_contractnumber_2_expirydate_monday_[expiry_num] =
        ComputeMondayExpiryDateWeeklyOptions(expiry_num, monthly_fo_expiries);
    weekly_option_expirydate_2_contractnumber_monday_[weekly_option_contractnumber_2_expirydate_monday_[expiry_num]] =
        expiry_num;
    //	std::cout <<"Monday" << weekly_option_contractnumber_2_expirydate_monday_[expiry_num] <<" : " << expiry_num
    //<<std::endl;
  }
}

void NSESecurityDefinitions::PopulateTuesdayWeeklyOptionsExpiryMap() {
  std::set<int> monthly_fo_expiries;

  if (intdate_ < WEEKLY_REDEFINED_DATE) {
    for (uint32_t t_ctr_ = 0; t_ctr_ < 6; t_ctr_++) {
      monthly_fo_expiries.insert(ComputeTuesdayExpiryDate(t_ctr_));
    }
  }

  for (uint32_t expiry_num = 0; expiry_num < 15; expiry_num++) {
    weekly_option_contractnumber_2_expirydate_tuesday_[expiry_num] =
        ComputeTuesdayExpiryDateWeeklyOptions(expiry_num, monthly_fo_expiries);
    weekly_option_expirydate_2_contractnumber_tuesday_[weekly_option_contractnumber_2_expirydate_tuesday_[expiry_num]] =
        expiry_num;
    // std::cout <<"TUESDAY" << weekly_option_contractnumber_2_expirydate_tuesday_[expiry_num] <<" : " << expiry_num
    //<<std::endl;
  }
}
// ONly Works For BNF Expiries.. All are all wednesday except on last week of the month which is on Thursday
void NSESecurityDefinitions::PopulateWednesdayWeeklyOptionsExpiryMap() {
  std::set<int> monthly_fo_expiries;

  if (intdate_ < WEEKLY_REDEFINED_DATE) {
    for (uint32_t t_ctr_ = 0; t_ctr_ < 6; t_ctr_++) {
      monthly_fo_expiries.insert(ComputeWednesdayExpiryDate(t_ctr_));
    }
  }
  int monthly_count = 0;
  for (uint32_t expiry_num = 0; expiry_num < 15; expiry_num++) {
      int t_date_ = ComputeWednesdayExpiryDateWeeklyOptions(expiry_num-monthly_count, monthly_fo_expiries);
      if ( expiry_num == 0  && t_date_ > ComputeNextExpiry(((intdate_))) && intdate_ < 20240301){
         t_date_ = ComputeNextExpiry(((intdate_/100) * 100) + 1 );
         monthly_count = 1;
      } 
      int t_month_ = (t_date_ % 10000) / 100;
      int t_year_ = t_date_ / 10000; 
      boost::gregorian::last_day_of_the_week_in_month lwdm(boost::gregorian::Thursday, t_month_); // Use Week Last Thursday as reference
      boost::gregorian::date l_wednesday_ = lwdm.get_date(t_year_) - one_day_duration_; // Now we on same Week Wednesday
      boost::gregorian::date::ymd_type ymd = l_wednesday_.year_month_day();
      int t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;

      while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceNSEStr, t_expiry_date_, true)) {
    	l_wednesday_ -= one_day_duration_;
    	boost::gregorian::date::ymd_type ymd = l_wednesday_.year_month_day();
    	t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
      }
      if ( t_date_ == t_expiry_date_ && intdate_ < 20240301 ){ // if Monthly Expiry
        weekly_option_contractnumber_2_expirydate_wednesday_[expiry_num] =
          ComputeNextExpiry(((ymd.year * 100 + ymd.month) * 100) + 1 );
    }else {
      weekly_option_contractnumber_2_expirydate_wednesday_[expiry_num] = t_date_; // Else Push Wednesday Expiry Date
     }
      weekly_option_expirydate_2_contractnumber_wednesday_[weekly_option_contractnumber_2_expirydate_wednesday_[expiry_num]] =
        expiry_num;
    //std::cout <<"WEDNESDAY" << weekly_option_contractnumber_2_expirydate_wednesday_[expiry_num] <<" : " << expiry_num <<std::endl;
  }
}

void NSESecurityDefinitions::PopulateFridayWeeklyOptionsExpiryMap() {
  std::set<int> monthly_fo_expiries;

  if (intdate_ < WEEKLY_REDEFINED_DATE) {
    for (uint32_t t_ctr_ = 0; t_ctr_ < 6; t_ctr_++) {
      monthly_fo_expiries.insert(ComputeFridayExpiryDate(t_ctr_));
    }
  }
  int monthly_count = 0;
  for (uint32_t expiry_num = 0; expiry_num < 15; expiry_num++) {
      int t_date_ = ComputeFridayExpiryDateWeeklyOptions(expiry_num-monthly_count, monthly_fo_expiries);
      if ( expiry_num == 0  && t_date_ > ComputeNextExpiry(((intdate_))) && intdate_ < 20240301){
         t_date_ = ComputeNextExpiry(((intdate_/100) * 100) + 1 );
         monthly_count = 1;
      } 
      int t_month_ = (t_date_ % 10000) / 100;
      int t_year_ = t_date_ / 10000; 
      boost::gregorian::last_day_of_the_week_in_month lwdm(boost::gregorian::Thursday, t_month_); // Use Week Last Thursday as reference
      boost::gregorian::date l_friday_ = lwdm.get_date(t_year_) - one_day_duration_; // Now we on same Week friday
      boost::gregorian::date::ymd_type ymd = l_friday_.year_month_day();
      int t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;

      while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceNSEStr, t_expiry_date_, true)) {
    	l_friday_ -= one_day_duration_;
    	boost::gregorian::date::ymd_type ymd = l_friday_.year_month_day();
    	t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
      }
      if ( t_date_ == t_expiry_date_ && intdate_ < 20240301 ){ // if Monthly Expiry
        weekly_option_contractnumber_2_expirydate_friday_[expiry_num] =
          ComputeNextExpiry(((ymd.year * 100 + ymd.month) * 100) + 1 );
    }else {
      weekly_option_contractnumber_2_expirydate_friday_[expiry_num] = t_date_; // Else Push friday Expiry Date
     }
      weekly_option_expirydate_2_contractnumber_friday_[weekly_option_contractnumber_2_expirydate_friday_[expiry_num]] =
        expiry_num;
    //std::cout <<"FRIDAY" << weekly_option_contractnumber_2_expirydate_friday_[expiry_num] <<" : " << expiry_num <<std::endl;
  }
}

int NSESecurityDefinitions::ComputeMondayExpiryDateWeeklyOptions(const int expiry_number,
                                                                  const std::set<int>& monthly_future_expiries) {
  int t_expiry_number_ = expiry_number;
  int t_temp_date_ = intdate_;
  int t_expiry_date_ = 0;
  while (t_expiry_number_ >= 0) {
    t_expiry_date_ = ComputeMondayNextExpiryWeeklyOptions(t_temp_date_, monthly_future_expiries);
    t_expiry_number_--;
    if (t_expiry_number_ >= 0) {
      // Set to day next to nearest Monday
      // 1. Calculate Next Monday First (If a day is Monday, this code returns next Monday as the same day. This
      // assumption is very imp. If this assumption breaks, code will break.)
      boost::gregorian::date d1((t_expiry_date_ / 10000) % 10000, ((t_expiry_date_ / 100) % 100),
                                (t_expiry_date_ % 100));
      boost::gregorian::greg_weekday gw(boost::gregorian::Monday);
      boost::gregorian::date next_monday = next_weekday(d1, gw);
      boost::gregorian::date::ymd_type ymd = next_monday.year_month_day();
      int date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
      // 2. Calculate Next day of Monday
      t_temp_date_ = DateTime::CalcNextDay(date_);
    }
  }
  return t_expiry_date_;
}

int NSESecurityDefinitions::ComputeTuesdayExpiryDateWeeklyOptions(const int expiry_number,
                                                                  const std::set<int>& monthly_future_expiries) {
  int t_expiry_number_ = expiry_number;
  int t_temp_date_ = intdate_;
  int t_expiry_date_ = 0;
  while (t_expiry_number_ >= 0) {
    t_expiry_date_ = ComputeTuesdayNextExpiryWeeklyOptions(t_temp_date_, monthly_future_expiries);
    t_expiry_number_--;
    if (t_expiry_number_ >= 0) {
      // Set to day next to nearest Tuesday
      // 1. Calculate Next Tuesday First (If a day is Tuesday, this code returns next Tuesday as the same day. This
      // assumption is very imp. If this assumption breaks, code will break.)
      boost::gregorian::date d1((t_expiry_date_ / 10000) % 10000, ((t_expiry_date_ / 100) % 100),
                                (t_expiry_date_ % 100));
      boost::gregorian::greg_weekday gw(boost::gregorian::Tuesday);
      boost::gregorian::date next_tuesday = next_weekday(d1, gw);
      boost::gregorian::date::ymd_type ymd = next_tuesday.year_month_day();
      int date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
      // 2. Calculate Next day of Tuesday
      t_temp_date_ = DateTime::CalcNextDay(date_);
    }
  }
  return t_expiry_date_;
}

int NSESecurityDefinitions::ComputeWednesdayExpiryDateWeeklyOptions(const int expiry_number,
                                                                  const std::set<int>& monthly_future_expiries) {
  int t_expiry_number_ = expiry_number;
  int t_temp_date_ = intdate_;
  int t_expiry_date_ = 0;

  while (t_expiry_number_ >= 0) {
    t_expiry_date_ = ComputeWednesdayNextExpiryWeeklyOptions(t_temp_date_, monthly_future_expiries);
    t_expiry_number_--;
    if (t_expiry_number_ >= 0) {
      // Set to day next to nearest Wednesday
      // 1. Calculate Next Wednesday First (If a day is Wednesday, this code returns next Wednesday as the same day. This
      // assumption is very imp. If this assumption breaks, code will break.)
      boost::gregorian::date d1((t_expiry_date_ / 10000) % 10000, ((t_expiry_date_ / 100) % 100),
                                (t_expiry_date_ % 100));
      boost::gregorian::greg_weekday gw(boost::gregorian::Wednesday);
      boost::gregorian::date next_wednesday = next_weekday(d1, gw);
      boost::gregorian::date::ymd_type ymd = next_wednesday.year_month_day();
      int date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
      // 2. Calculate Next day of Wednesday
      t_temp_date_ = DateTime::CalcNextDay(date_);
    }
  }
  return t_expiry_date_;
}

int NSESecurityDefinitions::ComputeFridayExpiryDateWeeklyOptions(const int expiry_number,
                                                                  const std::set<int>& monthly_future_expiries) {
  int t_expiry_number_ = expiry_number;
  int t_temp_date_ = intdate_;
  int t_expiry_date_ = 0;

  while (t_expiry_number_ >= 0) {
    t_expiry_date_ = ComputeFridayNextExpiryWeeklyOptions(t_temp_date_, monthly_future_expiries);
    t_expiry_number_--;
    if (t_expiry_number_ >= 0) {
      // Set to day next to nearest Friday
      // 1. Calculate Next friday First (If a day is friday, this code returns next friday as the same day. This
      // assumption is very imp. If this assumption breaks, code will break.)
      boost::gregorian::date d1((t_expiry_date_ / 10000) % 10000, ((t_expiry_date_ / 100) % 100),
                                (t_expiry_date_ % 100));
      boost::gregorian::greg_weekday gw(boost::gregorian::Friday);
      boost::gregorian::date next_friday = next_weekday(d1, gw);
      boost::gregorian::date::ymd_type ymd = next_friday.year_month_day();
      int date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
      // 2. Calculate Next day of friday
      t_temp_date_ = DateTime::CalcNextDay(date_);
    }
  }
  return t_expiry_date_;
}

int NSESecurityDefinitions::ComputeMondayNextExpiryWeeklyOptions(const int t_date_,
                                                                  const std::set<int>& monthly_future_expiries) {
  boost::gregorian::date d1((t_date_ / 10000) % 10000, ((t_date_ / 100) % 100), (t_date_ % 100));
  boost::gregorian::greg_weekday gw(boost::gregorian::Monday);
  boost::gregorian::date next_monday = next_weekday(d1, gw);

  boost::gregorian::date current_day = next_monday;
  boost::gregorian::date::ymd_type ymd = current_day.year_month_day();
  int t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
  while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceNSEStr, t_expiry_date_, true)) {
    current_day -= one_day_duration_;
    boost::gregorian::date::ymd_type ymd = current_day.year_month_day();
    t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
  }

  if (t_expiry_date_ >= t_date_ && monthly_future_expiries.find(t_expiry_date_) == monthly_future_expiries.end()) {
    return t_expiry_date_;
  } else {
    boost::gregorian::date next_friday = next_monday + boost::gregorian::date_duration(3);
    ymd = next_friday.year_month_day();
    return ComputeMondayNextExpiryWeeklyOptions(((ymd.year * 100 + ymd.month) * 100) + ymd.day,
                                                 monthly_future_expiries);
  }
}

int NSESecurityDefinitions::ComputeTuesdayNextExpiryWeeklyOptions(const int t_date_,
                                                                  const std::set<int>& monthly_future_expiries) {
  boost::gregorian::date d1((t_date_ / 10000) % 10000, ((t_date_ / 100) % 100), (t_date_ % 100));
  boost::gregorian::greg_weekday gw(boost::gregorian::Tuesday);
  boost::gregorian::date next_tuesday = next_weekday(d1, gw);

  boost::gregorian::date current_day = next_tuesday;
  boost::gregorian::date::ymd_type ymd = current_day.year_month_day();
  int t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
  while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceNSEStr, t_expiry_date_, true)) {
    current_day -= one_day_duration_;
    boost::gregorian::date::ymd_type ymd = current_day.year_month_day();
    t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
  }

  if (t_expiry_date_ >= t_date_ && monthly_future_expiries.find(t_expiry_date_) == monthly_future_expiries.end()) {
    return t_expiry_date_;
  } else {
    boost::gregorian::date next_friday = next_tuesday + boost::gregorian::date_duration(3);
    ymd = next_friday.year_month_day();
    return ComputeTuesdayNextExpiryWeeklyOptions(((ymd.year * 100 + ymd.month) * 100) + ymd.day,
                                                 monthly_future_expiries);
  }
}

int NSESecurityDefinitions::ComputeWednesdayNextExpiryWeeklyOptions(const int t_date_,
                                                                  const std::set<int>& monthly_future_expiries) {
  boost::gregorian::date d1((t_date_ / 10000) % 10000, ((t_date_ / 100) % 100), (t_date_ % 100));
  boost::gregorian::greg_weekday gw(boost::gregorian::Wednesday);
  boost::gregorian::date next_wednesday = next_weekday(d1, gw);

  boost::gregorian::date current_day = next_wednesday;
  boost::gregorian::date::ymd_type ymd = current_day.year_month_day();
  int t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
  while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceNSEStr, t_expiry_date_, true)) {
    current_day -= one_day_duration_;
    boost::gregorian::date::ymd_type ymd = current_day.year_month_day();
    t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
  }

  if (t_expiry_date_ >= t_date_ && monthly_future_expiries.find(t_expiry_date_) == monthly_future_expiries.end()) {
    return t_expiry_date_;
  } else {
    boost::gregorian::date next_friday = next_wednesday + boost::gregorian::date_duration(3);
    ymd = next_friday.year_month_day();
    return ComputeWednesdayNextExpiryWeeklyOptions(((ymd.year * 100 + ymd.month) * 100) + ymd.day,
                                                 monthly_future_expiries);
  }
}

int NSESecurityDefinitions::ComputeFridayNextExpiryWeeklyOptions(const int t_date_,
                                                                  const std::set<int>& monthly_future_expiries) {
  boost::gregorian::date d1((t_date_ / 10000) % 10000, ((t_date_ / 100) % 100), (t_date_ % 100));
  boost::gregorian::greg_weekday gw(boost::gregorian::Friday);
  boost::gregorian::date next_friday = next_weekday(d1, gw);

  boost::gregorian::date current_day = next_friday;
  boost::gregorian::date::ymd_type ymd = current_day.year_month_day();
  int t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
  while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceNSEStr, t_expiry_date_, true)) {
    current_day -= one_day_duration_;
    boost::gregorian::date::ymd_type ymd = current_day.year_month_day();
    t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
  }

  if (t_expiry_date_ >= t_date_ && monthly_future_expiries.find(t_expiry_date_) == monthly_future_expiries.end()) {
    return t_expiry_date_;
  } else {
    boost::gregorian::date next_friday = next_friday + boost::gregorian::date_duration(3);
    ymd = next_friday.year_month_day();
    return ComputeFridayNextExpiryWeeklyOptions(((ymd.year * 100 + ymd.month) * 100) + ymd.day,
                                                 monthly_future_expiries);
  }
}

bool NSESecurityDefinitions::IsWeeklyOptionExpiry(const int yyyymmdd, char* underlying) {
  if (strcmp(underlying, "FINNIFTY") == 0 || (strcmp(underlying, "MIDCPNIFTY") == 0 && intdate_ < 20230426 ))
    return IsTuesdayWeeklyOptionExpiry(yyyymmdd);
  else if (strcmp(underlying, "MIDCPNIFTY") == 0 && intdate_ < 20230817 )
    return IsWednesdayWeeklyOptionExpiry(yyyymmdd);
  else if (strcmp(underlying, "MIDCPNIFTY") == 0)
    return IsMondayWeeklyOptionExpiry(yyyymmdd);
  else if (strcmp(underlying, "BANKNIFTY") == 0 && intdate_ > 20230901)
    return IsWednesdayWeeklyOptionExpiry(yyyymmdd); // Chnages in banknifty Monthly expiry
  else if (strcmp(underlying, "NIFTYNXT50") == 0)
    return IsFridayWeeklyOptionExpiry(yyyymmdd);
  return (weekly_option_expirydate_2_contractnumber_.find(yyyymmdd) !=
          weekly_option_expirydate_2_contractnumber_.end());
}

bool NSESecurityDefinitions::IsMondayWeeklyOptionExpiry(const int yyyymmdd) {
  return (weekly_option_expirydate_2_contractnumber_monday_.find(yyyymmdd) !=
          weekly_option_expirydate_2_contractnumber_monday_.end());
}

bool NSESecurityDefinitions::IsTuesdayWeeklyOptionExpiry(const int yyyymmdd) {
  return (weekly_option_expirydate_2_contractnumber_tuesday_.find(yyyymmdd) !=
          weekly_option_expirydate_2_contractnumber_tuesday_.end());
}

bool NSESecurityDefinitions::IsWednesdayWeeklyOptionExpiry(const int yyyymmdd) {
  return (weekly_option_expirydate_2_contractnumber_wednesday_.find(yyyymmdd) !=
          weekly_option_expirydate_2_contractnumber_wednesday_.end());
}

bool NSESecurityDefinitions::IsFridayWeeklyOptionExpiry(const int yyyymmdd) {
  return (weekly_option_expirydate_2_contractnumber_friday_.find(yyyymmdd) !=
          weekly_option_expirydate_2_contractnumber_friday_.end());
}

bool NSESecurityDefinitions::IsMonthlyOptionExpiry(const int yyyymmdd, char* underlying) { // No Chnages in banknifty Monthly expiry
  if (strcmp(underlying, "FINNIFTY") == 0 || (strcmp(underlying, "MIDCPNIFTY") == 0 && intdate_ < 20230426 ))
    return IsTuesdayMonthlyOptionExpiry(yyyymmdd);
  else if (strcmp(underlying, "MIDCPNIFTY") == 0 && intdate_ < 20230817 )
    return IsWednesdayMonthlyOptionExpiry(yyyymmdd);
  else if (strcmp(underlying, "MIDCPNIFTY") == 0)
    return IsMondayMonthlyOptionExpiry(yyyymmdd);
  else if (strcmp(underlying, "BANKNIFTY") == 0 && intdate_ >=20240301 )
    return IsWednesdayMonthlyOptionExpiry(yyyymmdd);
  else if (strcmp(underlying, "NIFTYNXT50") == 0)
    return IsFridayMonthlyOptionExpiry(yyyymmdd);
  else
  return (expirydate_2_contractnumber_.find(yyyymmdd) != expirydate_2_contractnumber_.end());
}

bool NSESecurityDefinitions::IsMondayMonthlyOptionExpiry(const int yyyymmdd) {
  return (expirydate_2_contractnumber_monday_.find(yyyymmdd) != expirydate_2_contractnumber_monday_.end());
}

bool NSESecurityDefinitions::IsTuesdayMonthlyOptionExpiry(const int yyyymmdd) {
  return (expirydate_2_contractnumber_tuesday_.find(yyyymmdd) != expirydate_2_contractnumber_tuesday_.end());
}

bool NSESecurityDefinitions::IsWednesdayMonthlyOptionExpiry(const int yyyymmdd) {
  return (expirydate_2_contractnumber_wednesday_.find(yyyymmdd) != expirydate_2_contractnumber_wednesday_.end());
}

bool NSESecurityDefinitions::IsFridayMonthlyOptionExpiry(const int yyyymmdd) {
  return (expirydate_2_contractnumber_friday_.find(yyyymmdd) != expirydate_2_contractnumber_friday_.end());
}

std::string NSESecurityDefinitions::GetExchSymbolNSE(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  if (shortcode_2_exchsymbol_.find(local_shc) != shortcode_2_exchsymbol_.end()) {
    return (shortcode_2_exchsymbol_[local_shc]);
  } else if (shortcode_2_exchsymbol_all_.find(local_shc) != shortcode_2_exchsymbol_all_.end()) {
    return shortcode_2_exchsymbol_all_[local_shc];
  } else {
    return "";
  }
}

double NSESecurityDefinitions::GetLastClose(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  if (shortcode_2_last_close_.find(local_shc) != shortcode_2_last_close_.end()) {
    return (shortcode_2_last_close_[local_shc]);
  } else {
    return -1;
    //    std::cerr << "Fatal error - GetLastClose called with missing instrument " << local_shc << ".Exiting.\n";
    //    exit(-1);
  }
}

double NSESecurityDefinitions::GetLastCloseForOptions(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  if (shortcode_2_options_last_close_.find(local_shc) != shortcode_2_options_last_close_.end()) {
    return shortcode_2_options_last_close_[local_shc];
  } else {
    return -1;
  }
}

NSEMonth_t NSESecurityDefinitions::StringToMonthNumber(const char* _cstr_) {
  if (!strncmp(_cstr_, "JAN", 3)) {
    return NSE_JAN;
  } else if (!strncmp(_cstr_, "FEB", 3)) {
    return NSE_FEB;
  } else if (!strncmp(_cstr_, "MAR", 3)) {
    return NSE_MAR;
  } else if (!strncmp(_cstr_, "APR", 3)) {
    return NSE_APR;
  } else if (!strncmp(_cstr_, "MAY", 3)) {
    return NSE_MAY;
  } else if (!strncmp(_cstr_, "JUN", 3)) {
    return NSE_JUN;
  } else if (!strncmp(_cstr_, "JUL", 3)) {
    return NSE_JUL;
  } else if (!strncmp(_cstr_, "AUG", 3)) {
    return NSE_AUG;
  } else if (!strncmp(_cstr_, "SEP", 3)) {
    return NSE_SEP;
  } else if (!strncmp(_cstr_, "OCT", 3)) {
    return NSE_OCT;
  } else if (!strncmp(_cstr_, "NOV", 3)) {
    return NSE_NOV;
  } else if (!strncmp(_cstr_, "DEC", 3)) {
    return NSE_DEC;
  } else
    return NSE_INVALID_MM;
}

std::string NSESecurityDefinitions::GetDateInYYYYMMDD(char* _date_str_) {
  PerishableStringTokenizer st_(_date_str_, sizeof(_date_str_));
  std::ostringstream date_in_YYYYMMDD_;
  const std::vector<const char*>& tokens_ = st_.GetTokens();
  if (tokens_.size() == 3) {
    std::string mm = "";
    NSEMonth_t this_month_ = StringToMonthNumber(tokens_[1]);

    switch (this_month_) {
      case NSE_JAN:
        mm = "01";
        break;
      case NSE_FEB:
        mm = "02";
        break;
      case NSE_MAR:
        mm = "03";
        break;
      case NSE_APR:
        mm = "04";
        break;
      case NSE_MAY:
        mm = "05";
        break;
      case NSE_JUN:
        mm = "06";
        break;
      case NSE_JUL:
        mm = "07";
        break;
      case NSE_AUG:
        mm = "08";
        break;
      case NSE_SEP:
        mm = "09";
        break;
      case NSE_OCT:
        mm = "10";
        break;
      case NSE_NOV:
        mm = "11";
        break;
      case NSE_DEC:
        mm = "12";
        break;

      default:
        std::cerr << "Invalid Month" << std::endl;
        return "";
    }
    date_in_YYYYMMDD_ << tokens_[2] << mm << tokens_[0];
  } else {
    std::cerr << "Invalid date format in Bhavcopy file" << _date_str_ << "\n";
    return "";
  }
  return date_in_YYYYMMDD_.str();
}

std::string NSESecurityDefinitions::GetDateInYYYYMMDDBhav(std::string _str_) {
  std::transform(_str_.begin(), _str_.end(), _str_.begin(), ::toupper);
  std::replace(_str_.begin(), _str_.end(), '-', ' ');
  char* _date_str_ = &_str_[0];
  ;
  PerishableStringTokenizer st_(_date_str_, sizeof(_date_str_));
  std::ostringstream date_in_YYYYMMDD_;
  const std::vector<const char*>& tokens_ = st_.GetTokens();
  if (tokens_.size() == 3) {
    std::string mm = "";
    NSEMonth_t this_month_ = StringToMonthNumber(tokens_[1]);

    switch (this_month_) {
      case NSE_JAN:
        mm = "01";
        break;
      case NSE_FEB:
        mm = "02";
        break;
      case NSE_MAR:
        mm = "03";
        break;
      case NSE_APR:
        mm = "04";
        break;
      case NSE_MAY:
        mm = "05";
        break;
      case NSE_JUN:
        mm = "06";
        break;
      case NSE_JUL:
        mm = "07";
        break;
      case NSE_AUG:
        mm = "08";
        break;
      case NSE_SEP:
        mm = "09";
        break;
      case NSE_OCT:
        mm = "10";
        break;
      case NSE_NOV:
        mm = "11";
        break;
      case NSE_DEC:
        mm = "12";
        break;

      default:
        std::cerr << "Invalid Month" << std::endl;
        return "";
    }
    date_in_YYYYMMDD_ << tokens_[2] << mm << tokens_[0];
  } else {
    std::cerr << "Invalid date format in Bhavcopy file" << _date_str_ << "\n";
    return "";
  }
  return date_in_YYYYMMDD_.str();
}

void NSESecurityDefinitions::AddtoOptionsLastClosePrice(NSEInstrumentType_t t_inst_type_, char* underlying_,
                                                        int expiry_, double strike_price_, char* call_or_put_,
                                                        std::string shortcode_) {
  NSEOPtionsIdentifier_t this_option_identifier;
  std::ostringstream t_str_oss_;
  this_option_identifier.inst_type_ = t_inst_type_;
  t_str_oss_ << underlying_;
  this_option_identifier.underlying_ = t_str_oss_.str();
  this_option_identifier.expiry_ = expiry_;
  this_option_identifier.strike_price_ = strike_price_;

  t_str_oss_.str(std::string());
  t_str_oss_ << call_or_put_;
  this_option_identifier.call_or_put_ = t_str_oss_.str();

  if (options_param_2_last_close_.end() != options_param_2_last_close_.find(this_option_identifier)) {
    shortcode_2_options_last_close_[shortcode_] = options_param_2_last_close_.find(this_option_identifier)->second;
  }
}

bool NSESecurityDefinitions::IsOptionStrikePresentInBhavCopy(NSEInstrumentParams_t param, double strike_price) {
  NSEOPtionsIdentifier_t this_option_identifier;
  std::ostringstream t_str_oss_;
  this_option_identifier.inst_type_ = param.inst_type_;
  t_str_oss_ << param.underlying_;
  this_option_identifier.underlying_ = t_str_oss_.str();
  this_option_identifier.expiry_ = param.expiry_;
  this_option_identifier.strike_price_ = strike_price;

  t_str_oss_.str(std::string());
  t_str_oss_ << "CE";
  this_option_identifier.call_or_put_ = t_str_oss_.str();
  if (options_param_2_last_close_.end() != options_param_2_last_close_.find(this_option_identifier)) {
    return true;
  }
  return false;
}

double NSESecurityDefinitions::GetNearestAvailableMultipleStrike(NSEInstrumentParams_t param, double price,
                                                                 double multiple, double upper_bound,
                                                                 double lower_bound) {
  bool strike_found = false;
  double strike_upper = std::round(price / multiple) * multiple;
  while (strike_upper < upper_bound) {
    if (IsOptionStrikePresentInBhavCopy(param, strike_upper)) {
      break;
    }
    strike_upper += multiple;
  }

  double strike_lower = std::round(price / multiple) * multiple;
  while (strike_lower > lower_bound) {
    if (IsOptionStrikePresentInBhavCopy(param, strike_lower)) {
      break;
    }
    strike_lower -= multiple;
  }

  if (!strike_found) {
    return -1;
  }

  if (fabs(strike_upper - price) < fabs(strike_lower - price)) {
    return strike_upper;
  } else {
    return strike_lower;
  }
}

NSEInstrumentType_t NSESecurityDefinitions::StringToInst(const char* str) {
  if ((strcmp(str, "IDXFUT") == 0) || (strcmp(str, "FUTIDX") == 0))
    return NSE_IDXFUT;
  else if ((strcmp(str, "STKFUT") == 0) || (strcmp(str, "FUTSTK") == 0))
    return NSE_STKFUT;
  else if ((strcmp(str, "IDXOPT") == 0) || (strcmp(str, "OPTIDX") == 0))
    return NSE_IDXOPT;
  else if ((strcmp(str, "STKOPT") == 0) || (strcmp(str, "OPTSTK") == 0))
    return NSE_STKOPT;
  else if (strcmp(str, "CURFUT") == 0)
    return NSE_CURFUT;
  else if (strcmp(str, "IRFFUT") == 0)
    return NSE_IRFFUT;
  else if (strcmp(str, "IDXSPT") == 0)
    return NSE_IDXSPT;
  else
    return NSE_CUROPT;
}

// Reads entries from NSE contract file - every line is of format
// UNDERLYING_TYPE UNDERLYING CLOSE_PX LOT_SIZE MIN_TICK EXPIRY NUM_ITM STEP_VALUE
// eg INDOPT NIFTY
void NSESecurityDefinitions::LoadInstParams(const int t_date_) {
  std::ostringstream nse_contract_specs_file_name_oss_;
  std::string nse_contract_specs_file_name_ = "";
  nse_contract_specs_file_name_oss_ << NSE_CONTRACT_FILENAME_PREFIX << "." << t_date_;
  nse_contract_specs_file_name_ = std::string("/spare/local/tradeinfo/") + nse_contract_specs_file_name_oss_.str();

  if (FileUtils::ExistsAndReadable(nse_contract_specs_file_name_)) {
    std::ifstream nse_contract_file_;
    nse_contract_file_.open(nse_contract_specs_file_name_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (nse_contract_file_.is_open()) {
      while (nse_contract_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        nse_contract_file_.getline(readline_buffer_, sizeof(readline_buffer_));
        PerishableStringTokenizer st_(readline_buffer_, sizeof(readline_buffer_));

        const std::vector<const char*>& tokens_ = st_.GetTokens();
        if (tokens_.size() >= 5 && tokens_[0][0] != '#') {
          NSEInstrumentParams_t* t_config_ = new NSEInstrumentParams_t();
          t_config_->inst_type_ = StringToInst(tokens_[0]);
          strcpy(t_config_->underlying_, tokens_[1]);
          t_config_->last_close_ = atof(tokens_[2]);
          t_config_->lot_size_ = atoi(tokens_[3]);
          t_config_->min_tick_ = atof(tokens_[4]);
          t_config_->expiry_ = atoi(tokens_[5]);
          if (t_config_->inst_type_ == NSE_IDXOPT || t_config_->inst_type_ == NSE_STKOPT ||
              t_config_->inst_type_ == NSE_CUROPT) {
            t_config_->num_itm_ = atoi(tokens_[6]);
            t_config_->step_value_ = atof(tokens_[7]);
            t_config_->step_value_prev_ = t_config_->step_value_;
            t_config_->prev_num_itm_ = t_config_->num_itm_;
            if (tokens_.size() >= 10) {
              t_config_->step_value_prev_ = atof(tokens_[8]);
              t_config_->prev_num_itm_ = atoi(tokens_[9]);
            }
            underlying_2_itr_num[t_config_->underlying_ + std::to_string(t_config_->expiry_)] = t_config_->num_itm_;
            
          }
          inst_params_.push_back(t_config_);
        }
      }  // end while
    }
    nse_contract_file_.close();
  } else {
    std::cerr << "Fatal error - could not read NSE Contract Specs file " << nse_contract_specs_file_name_
              << ".Exiting.\n";
    exit(0);
  }
}

// Reads entries from NSE bhavcopy files ddmmfo_0000.md - every line is of format
// $1 = INST_TYPE, $2= UNDERLYING, $3= EXPIRY, $4= Strike price, $5= call/put(PE/CE), $10= last close price

void NSESecurityDefinitions::AddDataSourceSymbolFromBhavCopy(NSEInstrumentType_t inst_type_, std::string underlying_,
                                                             int exp_date_, double strike_px_, std::string call_or_put_,
                                                             double last_close_px_, int _open_int_) {
  NSEOPtionsIdentifier_t* t_config_ = new NSEOPtionsIdentifier_t();
  t_config_->inst_type_ = inst_type_;
  t_config_->underlying_ = underlying_;
  t_config_->expiry_ = exp_date_;
  t_config_->strike_price_ = strike_px_;
  t_config_->call_or_put_ = call_or_put_;
  options_param_2_last_close_[*t_config_] = last_close_px_;

  // Also add to map which contains all options which have non-zero OI
  std::ostringstream datasource_sym_;

  std::ostringstream temp_strike_;
  temp_strike_ << std::fixed << std::setprecision(2) << t_config_->strike_price_;

  datasource_sym_ << "NSE_" << t_config_->underlying_ << "_" << t_config_->call_or_put_ << "_" << temp_strike_.str()
                  << "_" << t_config_->expiry_;

  datasource_2_openInterest_[datasource_sym_.str()] = _open_int_;
  if (_open_int_ != 0) {
    non_zero_OI_.push_back(datasource_sym_.str());
  }
}

void NSESecurityDefinitions::LoadBhavCopyParams(const int t_date_) {
  std::ostringstream nse_bhav_copy_file_name_oss_;
  std::string nse_bhav_copy_file_name_ = "";

  int this_date = t_date_;

  // bhavcopy files for the date D are generated by the same date_stamp unlike nse_contract files
  // get the previous business day and load the data
  this_date = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForExchange("NSE", this_date);
  // split into dd mm yy
  std::ostringstream date_string_oss_;
  date_string_oss_ << this_date;
  std::string prev_date_ = date_string_oss_.str();
  nse_bhav_copy_file_name_oss_ << NSE_BHAVCOPY_FILENAME_PREFIX << "/" << prev_date_.substr(4, 2)
                               << prev_date_.substr(2, 2) << "/" << prev_date_.substr(6, 2) << prev_date_.substr(4, 2)
                               << "fo_0000.md";
  nse_bhav_copy_file_name_ = std::string("/spare/local/tradeinfo/") + nse_bhav_copy_file_name_oss_.str();

  if (FileUtils::ExistsAndReadable(nse_bhav_copy_file_name_)) {
    std::ifstream nse_bhavcopy_file_;
    nse_bhavcopy_file_.open(nse_bhav_copy_file_name_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (nse_bhavcopy_file_.is_open()) {
      while (nse_bhavcopy_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        nse_bhavcopy_file_.getline(readline_buffer_, sizeof(readline_buffer_));

        std::vector<char*> tokens_;
        // create a copy of line read before using non-const tokenizer
        char readline_buffer_copy_[1024];
        memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
        strcpy(readline_buffer_copy_, readline_buffer_);

        HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, ",", tokens_);
        std::string expr_date_;
        std::string trimmed_str_;
        // trim the inst_type field: contains spaces
        if (tokens_.size() >= 15) HFSAT::PerishableStringTokenizer::TrimString(tokens_[1], trimmed_str_, ' ');
        if (tokens_.size() >= 15 && tokens_[0][0] != '#' &&
            ((trimmed_str_ == "OPTIDX") || (trimmed_str_ == "OPTSTK") || (trimmed_str_ == "OPTCUR"))) {
          NSEInstrumentType_t inst_type_ = StringToInst(trimmed_str_.c_str());
          HFSAT::PerishableStringTokenizer::TrimString(tokens_[2], trimmed_str_, ' ');
          std::string underlying_ = trimmed_str_;

          expr_date_ = GetDateInYYYYMMDD(tokens_[3]);
          // if invalid date, don't add this entry
          if (expr_date_ == "") continue;
          int expiry_ = std::atoi(expr_date_.c_str());
          double strike_price_ = atof(tokens_[4]);
          HFSAT::PerishableStringTokenizer::TrimString(tokens_[5], trimmed_str_, ' ');
          std::string call_or_put_ = trimmed_str_.c_str();
          double last_close_px_ = atof(tokens_[10]);
          int _open_int_ = atoi(tokens_[13]);

          AddDataSourceSymbolFromBhavCopy(inst_type_, underlying_, expiry_, strike_price_, call_or_put_, last_close_px_,
                                          _open_int_);

          if (underlying_2_corporate_action_value_.find(underlying_) != underlying_2_corporate_action_value_.end()) {
            strike_price_ = GetAdjustedValue(underlying_, atof(tokens_[4]));
            AddDataSourceSymbolFromBhavCopy(inst_type_, underlying_, expiry_, strike_price_, call_or_put_,
                                            last_close_px_, _open_int_);
          }
        }
      }  // end while
    }
    nse_bhavcopy_file_.close();
  } else {
    std::cerr << "Fatal error - could not read NSE Bhavcopy file " << nse_bhav_copy_file_name_ << ".Exiting.\n";
    exit(0);
  }
}

void NSESecurityDefinitions::LoadCMBhavCopyParams(const int t_date_) {
  std::ostringstream nse_bhav_copy_file_name_oss_;
  std::string nse_bhav_copy_file_name_ = "";
  std::vector<std::string> month_vec_ = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                         "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

  int this_date = t_date_;

  // bhavcopy files for the date D are generated by the same date_stamp unlike nse_contract files
  // get the previous business day and load the data
  this_date = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForExchange("NSE", this_date);
  // split into dd mm yy
  std::ostringstream date_string_oss_;
  date_string_oss_ << this_date;
  std::string prev_date_ = date_string_oss_.str();
  nse_bhav_copy_file_name_oss_ << "NSE_Files/Margin_Files/Exposure_Files/cm" << prev_date_.substr(6, 2)
                               << month_vec_[std::stoi(prev_date_.substr(4, 2)) - 1] << prev_date_.substr(0, 4)
                               << "bhav.csv";
  nse_bhav_copy_file_name_ = std::string("/spare/local/tradeinfo/") + nse_bhav_copy_file_name_oss_.str();
  if (FileUtils::ExistsAndReadable(nse_bhav_copy_file_name_)) {
    std::ifstream nse_bhavcopy_file_;
    nse_bhavcopy_file_.open(nse_bhav_copy_file_name_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (nse_bhavcopy_file_.is_open()) {
      while (nse_bhavcopy_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        nse_bhavcopy_file_.getline(readline_buffer_, sizeof(readline_buffer_));

        std::vector<char*> tokens_;
        // create a copy of line read before using non-const tokenizer
        char readline_buffer_copy_[1024];
        memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
        strcpy(readline_buffer_copy_, readline_buffer_);

        HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, ",", tokens_);
        std::string expr_date_;
        std::string trimmed_str_;
        // trim the inst_type field: contains spaces
        if (tokens_.size() >= 9) HFSAT::PerishableStringTokenizer::TrimString(tokens_[1], trimmed_str_, ' ');
        if (tokens_.size() >= 9 && tokens_[0][0] != '#' && (trimmed_str_ == "EQ")) {
          HFSAT::PerishableStringTokenizer::TrimString(tokens_[0], trimmed_str_, ' ');
          std::string shortcode_ = "NSE_" + trimmed_str_;
          shortcode_2_type_[shortcode_] = HFSAT::NSE_EQ;
          shortcode_2_last_close_[shortcode_] = std::stof(tokens_[5]);
          // std::cout << "sc, close:: " << shortcode_ << ", " <<  shortcode_2_last_close_[trimmed_str_] << "\n";
        }
      }  // end while
    }
    nse_bhavcopy_file_.close();
  } else {
    std::cerr << "Fatal error - could not read NSE Bhavcopy file " << nse_bhav_copy_file_name_ << ".Exiting.\n";
    exit(0);
  }
}

void NSESecurityDefinitions::LoadCDBhavCopyParams(const int t_date_) {
  std::ostringstream nse_bhav_copy_file_name_oss_;
  std::string nse_bhav_copy_file_name_ = "";

  int this_date = t_date_;

  // bhavcopy files for the date D are generated by the same date_stamp unlike nse_contract files
  // get the previous business day and load the data
  this_date = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForExchange("NSE", this_date);

  // CD specific holidays
  while (this_date == 20150219 || this_date == 20151224 || this_date == 20170221) {
    this_date = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForExchange("NSE", this_date);
  }
  // split into dd mm yy
  std::ostringstream date_string_oss_;
  date_string_oss_ << this_date;
  std::string prev_date_ = date_string_oss_.str();
  nse_bhav_copy_file_name_oss_ << NSE_CD_BHAVCOPY_FILENAME_PREFIX << "/" << prev_date_.substr(4, 2)
                               << prev_date_.substr(2, 2) << "/" << prev_date_.substr(6, 2) << prev_date_.substr(4, 2)
                               << "cd_0000.md";
  nse_bhav_copy_file_name_ = std::string("/spare/local/tradeinfo/") + nse_bhav_copy_file_name_oss_.str();

  if (FileUtils::ExistsAndReadable(nse_bhav_copy_file_name_)) {
    std::ifstream nse_bhavcopy_file_;
    nse_bhavcopy_file_.open(nse_bhav_copy_file_name_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (nse_bhavcopy_file_.is_open()) {
      while (nse_bhavcopy_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        nse_bhavcopy_file_.getline(readline_buffer_, sizeof(readline_buffer_));

        std::vector<char*> tokens_;
        // create a copy of line read before using non-const tokenizer
        char readline_buffer_copy_[1024];
        memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
        strcpy(readline_buffer_copy_, readline_buffer_);

        HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, ",", tokens_);
        std::string expr_date_;
        std::string trimmed_str_;
        // trim the inst_type field: contains spaces
        if (tokens_.size() >= 15) HFSAT::PerishableStringTokenizer::TrimString(tokens_[1], trimmed_str_, ' ');
        if (tokens_.size() >= 15 && tokens_[0][0] != '#' &&
            ((trimmed_str_ == "FUTCUR") || (trimmed_str_ == "OPTCUR") || (trimmed_str_ == "FUTIRC"))) {
          NSEInstrumentType_t inst_type_ = StringToInst(trimmed_str_.c_str());
          HFSAT::PerishableStringTokenizer::TrimString(tokens_[2], trimmed_str_, ' ');
          std::string underlying_ = trimmed_str_;

          expr_date_ = GetDateInYYYYMMDD(tokens_[3]);
          // if invalid date, don't add this entry
          if (expr_date_ == "") continue;

          int expiry_ = std::atoi(expr_date_.c_str());
          double strike_price_ = atof(tokens_[4]);
          HFSAT::PerishableStringTokenizer::TrimString(tokens_[5], trimmed_str_, ' ');
          std::string call_or_put_ = trimmed_str_.c_str();
          double last_close_px_ = atof(tokens_[10]);

          AddDataSourceSymbolFromBhavCopy(inst_type_, underlying_, expiry_, strike_price_, call_or_put_, last_close_px_,
                                          0);

          if (underlying_2_corporate_action_value_.find(underlying_) != underlying_2_corporate_action_value_.end()) {
            strike_price_ = GetAdjustedValue(underlying_, atof(tokens_[4]));
            AddDataSourceSymbolFromBhavCopy(inst_type_, underlying_, expiry_, strike_price_, call_or_put_,
                                            last_close_px_, 0);
          }
        }
      }  // end while
    }
    nse_bhavcopy_file_.close();
  } else {
    std::cerr << "Fatal error - could not read NSE Bhavcopy file " << nse_bhav_copy_file_name_ << ".Exiting.\n";
    exit(0);
  }
}

void split(const std::string& s, char delim, std::vector<std::string>& elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
}

std::string NSESecurityDefinitions::ConvertDataSourceNametoExchSymbol(const std::string t_dsname_str_) {
  if (datasourcename_2_exchsymbol_.find(t_dsname_str_) != datasourcename_2_exchsymbol_.end()) {
    return datasourcename_2_exchsymbol_[t_dsname_str_];
  } else {
    return "INVALID";
  }
}

std::string NSESecurityDefinitions::ConvertDataSourceNametoExchSymbolSecDef(const std::string t_dsname_str_) {
  std::vector<std::string> tokens1_;
  std::string data_source = std::string(t_dsname_str_);

  std::string data_source_copy = std::string(data_source);

  split(data_source_copy, '_', tokens1_);

  if (intdate_ >= USING_TBT_FROM && ((tokens1_[2] == "CE") || (tokens1_[2] == "PE"))) {
    data_source = tokens1_[0] + "_" + tokens1_[1] + "_" + tokens1_[2] + "_" + tokens1_[4] + "_" + tokens1_[3];
  }

  if (datasourcename_2_exchsymbol_.find(data_source) != datasourcename_2_exchsymbol_.end()) {
    return datasourcename_2_exchsymbol_[data_source];
  } else {
    return "INVALID";
  }
}

std::string NSESecurityDefinitions::ConvertExchSymboltoDataSourceNameGeneric(const std::string t_exchsymbol_) {
  if (exchsymbol_2_datasourcename_.find(t_exchsymbol_) != exchsymbol_2_datasourcename_.end()) {
    return exchsymbol_2_datasourcename_[t_exchsymbol_];
  } else if (exchsymbol_2_datasourcename_all_.find(t_exchsymbol_) != exchsymbol_2_datasourcename_all_.end()) {
    return exchsymbol_2_datasourcename_all_[t_exchsymbol_];
  } else {
    return "INVALID";
  }
}

// underlying and expiry for option you need NUM_ITM
int NSESecurityDefinitions::GetNumberItmForUnderlying(std::string underlying_,int expiry){
  if (underlying_2_itr_num.find(underlying_ + std::to_string(expiry)) != underlying_2_itr_num.end()) {
    return underlying_2_itr_num[underlying_ + std::to_string(expiry)];
  }
  return -1;
}

int NSESecurityDefinitions::GetOpenInterestFromExchangeSymbol(const std::string& _symbol_) {
  std::string local_shc = NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(_symbol_);
  if (datasource_2_openInterest_.end() == datasource_2_openInterest_.find(local_shc))
    return -1;
  else
    return datasource_2_openInterest_[local_shc];
}

std::string NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(const std::string t_exchsymbol_) {
  if (exchsymbol_2_datasourcename_.find(t_exchsymbol_) != exchsymbol_2_datasourcename_.end()) {
    return exchsymbol_2_datasourcename_[t_exchsymbol_];
  } else {
    return "INVALID";
  }
}

std::string NSESecurityDefinitions::GetDataSourcesymbolFromExchSymbol_(const std::string t_exchsymbol_) {
  if (exchsymbol_2_datasourcesym_.find(t_exchsymbol_) != exchsymbol_2_datasourcesym_.end()) {
    return exchsymbol_2_datasourcesym_[t_exchsymbol_];
  } else {
    return "INVALID";
  }
  std::cout << "I am in GetDataSourcesymbolFromExchSymbol_()\n";

  return "";
}

bool NSESecurityDefinitions::LoadThisExpiryContracts(const int t_expiry_, const NSEInstrumentType_t t_inst_type_,
                                                     char* underlying) { // Monthly margin
  if (t_inst_type_ == HFSAT::NSE_IDXOPT && IsWeeklyOptionExpiry(t_expiry_, underlying)) return true;
  if (t_inst_type_ == HFSAT::NSE_CURFUT || t_inst_type_ == HFSAT::NSE_CUROPT) {
    return (expirydate_2_contractnumber_cf_.find(t_expiry_) != expirydate_2_contractnumber_cf_.end());
  } else if (strcmp(underlying, "FINNIFTY") == 0 || (strcmp(underlying, "MIDCPNIFTY") == 0 && intdate_ < 20230426 )) {
    return (expirydate_2_contractnumber_tuesday_.find(t_expiry_) != expirydate_2_contractnumber_tuesday_.end());
  } else if (strcmp(underlying, "MIDCPNIFTY") == 0 && intdate_ < 20230817 ) {
    return (expirydate_2_contractnumber_wednesday_.find(t_expiry_) != expirydate_2_contractnumber_wednesday_.end());
  } else if (strcmp(underlying, "MIDCPNIFTY") == 0) {
      return (expirydate_2_contractnumber_monday_.find(t_expiry_) != expirydate_2_contractnumber_monday_.end());
  } else if (strcmp(underlying, "BANKNIFTY") == 0 && intdate_ >= 20240301){
      return (expirydate_2_contractnumber_wednesday_.find(t_expiry_) != expirydate_2_contractnumber_wednesday_.end());
  } else if (strcmp(underlying, "NIFTYNXT50") == 0) {
      return (expirydate_2_contractnumber_friday_.find(t_expiry_) != expirydate_2_contractnumber_friday_.end());
  } else {
    return (expirydate_2_contractnumber_.find(t_expiry_) != expirydate_2_contractnumber_.end());
  }
}

int NSESecurityDefinitions::GetContractNumberFromExpiry(const int t_date_, const NSEInstrumentType_t t_inst_type_,
                                                        char* underlying) { // only used By monthly ? then no changes 
  if (t_inst_type_ == HFSAT::NSE_CURFUT || t_inst_type_ == HFSAT::NSE_CUROPT) {
    if (expirydate_2_contractnumber_cf_.find(t_date_) != expirydate_2_contractnumber_cf_.end()) {
      return expirydate_2_contractnumber_cf_[t_date_];
    }
  } else if (strcmp(underlying, "FINNIFTY") == 0 || (strcmp(underlying, "MIDCPNIFTY") == 0 && intdate_ < 20230426 )) {
    if (expirydate_2_contractnumber_tuesday_.find(t_date_) != expirydate_2_contractnumber_tuesday_.end()) {
      return expirydate_2_contractnumber_tuesday_[t_date_];
    }
  } else if (strcmp(underlying, "MIDCPNIFTY") == 0 && intdate_ < 20230817 ) {
    if (expirydate_2_contractnumber_wednesday_.find(t_date_) != expirydate_2_contractnumber_wednesday_.end()) {
      return expirydate_2_contractnumber_wednesday_[t_date_];
    }
  } else if (strcmp(underlying, "MIDCPNIFTY") == 0){
    if (expirydate_2_contractnumber_monday_.find(t_date_) != expirydate_2_contractnumber_monday_.end()) {
      return expirydate_2_contractnumber_monday_[t_date_];
    }
  // } else if (strcmp(underlying, "BANKNIFTY") == 0 && intdate_ > 20230901){
  //   if (expirydate_2_contractnumber_wednesday_.find(t_date_) != expirydate_2_contractnumber_wednesday_.end()) { // it its a weekly contract
  //     return expirydate_2_contractnumber_wednesday_[t_date_];
  //   } else if (expirydate_2_contractnumber_.find(t_date_) != expirydate_2_contractnumber_.end()) {
  //       return expirydate_2_contractnumber_[t_date_]; // monthly contrac Banknifty 
  //   }
  } else if(strcmp(underlying, "BANKNIFTY") == 0 && intdate_ >= 20240301){
      if (expirydate_2_contractnumber_wednesday_.find(t_date_) != expirydate_2_contractnumber_wednesday_.end()) {
        return expirydate_2_contractnumber_wednesday_[t_date_];
      }
  } else if (strcmp(underlying, "NIFTYNXT50") == 0){
    if (expirydate_2_contractnumber_friday_.find(t_date_) != expirydate_2_contractnumber_friday_.end()) {
      return expirydate_2_contractnumber_friday_[t_date_];
    }
  } else {
    if (expirydate_2_contractnumber_.find(t_date_) != expirydate_2_contractnumber_.end()) {
      return expirydate_2_contractnumber_[t_date_];
    }
  }
  return -1;
}

int NSESecurityDefinitions::GetWeeklyContractNumberFromExpiry(const int t_date_, char* underlying) {
  if (strcmp(underlying, "FINNIFTY") == 0 || (strcmp(underlying, "MIDCPNIFTY") == 0 && intdate_ < 20230426)) {
    if (weekly_option_expirydate_2_contractnumber_tuesday_.find(t_date_) !=
        weekly_option_expirydate_2_contractnumber_tuesday_.end()) {
      return weekly_option_expirydate_2_contractnumber_tuesday_[t_date_];
    }
  } else if (strcmp(underlying, "MIDCPNIFTY") == 0  && intdate_ < 20230817){
    if (weekly_option_expirydate_2_contractnumber_wednesday_.find(t_date_) !=
        weekly_option_expirydate_2_contractnumber_wednesday_.end()) {
      return weekly_option_expirydate_2_contractnumber_wednesday_[t_date_];
    }
  } else if (strcmp(underlying, "MIDCPNIFTY") == 0) {
     if (weekly_option_expirydate_2_contractnumber_monday_.find(t_date_) !=
        weekly_option_expirydate_2_contractnumber_monday_.end()) {
      return weekly_option_expirydate_2_contractnumber_monday_[t_date_];
    }
  } else if (strcmp(underlying, "BANKNIFTY") == 0 && intdate_ > 20230901){
    if (weekly_option_expirydate_2_contractnumber_wednesday_.find(t_date_) != weekly_option_expirydate_2_contractnumber_wednesday_.end()) { // it its a weekly contract
      return weekly_option_expirydate_2_contractnumber_wednesday_[t_date_];
    }
  } else if (strcmp(underlying, "NIFTYNXT50") == 0) {
     if (weekly_option_expirydate_2_contractnumber_friday_.find(t_date_) !=
        weekly_option_expirydate_2_contractnumber_friday_.end()) {
      return weekly_option_expirydate_2_contractnumber_friday_[t_date_];
    }
  } else {
    if (weekly_option_expirydate_2_contractnumber_.find(t_date_) != weekly_option_expirydate_2_contractnumber_.end()) {
      return weekly_option_expirydate_2_contractnumber_[t_date_];
    }
  }
  std::cout << "ERROR GetWeeklyContractNumberFromExpiry :" << underlying << std::endl;
  return -1;
}

int NSESecurityDefinitions::GetExpiryFromContractNumber(const uint32_t t_number_,
                                                        const NSEInstrumentType_t t_inst_type_, char* underlying) { // No changes as used by future
  if (t_inst_type_ == HFSAT::NSE_CURFUT || t_inst_type_ == HFSAT::NSE_CUROPT) {
    if (contractnumber_cf_2_expirydate_.find(t_number_) != contractnumber_cf_2_expirydate_.end()) {
      return contractnumber_cf_2_expirydate_[t_number_];
    }
  } else if (strcmp(underlying, "FINNIFTY") == 0 || (strcmp(underlying, "MIDCPNIFTY") == 0 && intdate_ < 20230426 )) {
    if (contractnumber_2_expirydate_tuesday_.find(t_number_) != contractnumber_2_expirydate_tuesday_.end()) {
      return contractnumber_2_expirydate_tuesday_[t_number_];
    }
  } else if (strcmp(underlying, "MIDCPNIFTY") == 0 && intdate_ < 20230817) {
    if (contractnumber_2_expirydate_wednesday_.find(t_number_) != contractnumber_2_expirydate_wednesday_.end()) {
      return contractnumber_2_expirydate_wednesday_[t_number_];
    }
  } else if (strcmp(underlying, "MIDCPNIFTY") == 0) {
    if (contractnumber_2_expirydate_monday_.find(t_number_) != contractnumber_2_expirydate_monday_.end()) {
      return contractnumber_2_expirydate_monday_[t_number_];
    }
  } else if (strcmp(underlying, "BANKNIFTY") == 0 && intdate_ >=20240301) {
    if (contractnumber_2_expirydate_wednesday_.find(t_number_) != contractnumber_2_expirydate_wednesday_.end()) {
      return contractnumber_2_expirydate_wednesday_[t_number_];
    }
  } else if (strcmp(underlying, "NIFTYNXT50") == 0) {
    if (contractnumber_2_expirydate_friday_.find(t_number_) != contractnumber_2_expirydate_friday_.end()) {
      return contractnumber_2_expirydate_friday_[t_number_];
    }
  } else {
    if (contractnumber_2_expirydate_.find(t_number_) != contractnumber_2_expirydate_.end()) {
      return contractnumber_2_expirydate_[t_number_];
    }
  }

  std::cerr << "Fatal error - GetExpiryFromContractNumber called with invalid parameters " << t_number_
            << ".Exiting.\n";
  exit(0);
}

double NSESecurityDefinitions::GetContractMultiplier(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  std::map<std::string, NSEInstrumentType_t>::iterator t_type_map_iter_ = shortcode_2_type_.find(local_shc);
  if (t_type_map_iter_ != shortcode_2_type_.end()) {
    return GetContractMultiplier(t_type_map_iter_->second);
  } else {
    std::cerr << "Fatal Error - shortcode not found in shortcode_2_type map " << local_shc << ".Exiting.\n";
    exit(-1);
  }
}

double NSESecurityDefinitions::GetContractMultiplier(const NSEInstrumentType_t t_inst_type_) {
  switch (t_inst_type_) {
    case HFSAT::NSE_IDXFUT:
    case HFSAT::NSE_STKFUT:
    case HFSAT::NSE_IDXOPT:
    case HFSAT::NSE_STKOPT:
    case HFSAT::NSE_IDXSPT:
      return 1;
      break;
    case HFSAT::NSE_IRFFUT:
      return 2000;
      break;
    case HFSAT::NSE_CURFUT:
    case HFSAT::NSE_CUROPT:
      return 1000;
      break;
    case HFSAT::NSE_EQ:
      return 1;
      break;
    default:
      std::cerr << "Fatal Error - unsupported instrument type in GetContractMultiplier.Exiting. \n";
      exit(0);
  }
}

double NSESecurityDefinitions::GetNSECommission(std::string _shortcode_, int diff_exchange) {
  std::replace(_shortcode_.begin(), _shortcode_.end(), '~', '&');

  std::map<std::string, double>::iterator t_commish_map_iter_ = shortcode_2_commissions_.find(_shortcode_);
  if (t_commish_map_iter_ != shortcode_2_commissions_.end()) {
    return t_commish_map_iter_->second;
  } else {
    std::map<std::string, NSEInstrumentType_t>::iterator t_type_map_iter_ = shortcode_2_type_.find(_shortcode_);
    double t_commish_;
    if (t_type_map_iter_ != shortcode_2_type_.end()) {
      if ( diff_exchange == 1) { // 1 Corresponds to TwoRoads. default 0 is GRT
      switch (t_type_map_iter_->second) {
	case HFSAT::NSE_EQ:
          // Two Roads Commission Old BackEnd before 20230530
          std::cout << "Computing Two Roads Commission" << std::endl;
          t_commish_ =
              (0.000001 * 1.18 + 0.000001 * 1.18 + 0.000125 + 0.0000297 * 1.18 + 0.000015) * GetContractMultiplier(t_type_map_iter_->second);  //IPF charge 10/cr added
// GRT              (0 + 0.000001 * 1.18 + 0.000125 + 0.0000325 * 1.18 + 0.0000118) * GetContractMultiplier(t_type_map_iter_->second);
          shortcode_2_commissions_[_shortcode_] = t_commish_;
          return t_commish_;
        default:
          std::cerr << " GetNSECommission called with improper instrument type " << t_type_map_iter_->second
                    << ".Exiting.\n";
          exit(0);
	}
      } else if ( diff_exchange  == 2){
	switch (t_type_map_iter_->second) {
        case HFSAT::NSE_EQ:
          // GRT WITH StAMP DUTY Commission Current Backend 20230814
          std::cout << "Computing GRT Commission with StampDuty" << std::endl;
          t_commish_ =
	      (0.000001 * 1.18 + 0.000001 * 1.18 + 0.000125 + 0.0000297 * 1.18 + 0.000015) * GetContractMultiplier(t_type_map_iter_->second); //IPF charge 10/cr added
// GRT        (0 + 0.000001 * 1.18 + 0.000125 + 0.0000325 * 1.18 + 0.0000118) * GetContractMultiplier(t_type_map_iter_->second);
          shortcode_2_commissions_[_shortcode_] = t_commish_;
          return t_commish_;
        default:
          std::cerr << " GetNSECommission called with improper instrument type " << t_type_map_iter_->second
                    << ".Exiting.\n";
          exit(0);
        }
      }
      switch (t_type_map_iter_->second) {
        case HFSAT::NSE_IDXFUT:
        case HFSAT::NSE_STKFUT:
          t_commish_ = (0.000001 * 1.18 + 0.000001 * 1.18 + 0.000001 + 0.0001 + 0.0000173 * 1.18 + 0.00001) *   
                       GetContractMultiplier(t_type_map_iter_->second);  // IPF=0.000001 + Clearing=0.000001 + SEBI=10/1cr +
                                                                         // STT=.0001/0.0005 for sell side +
                                                                         // TXN=1.9/1lakh + FUND + STAMP=0 + ST=18% of
                                                                         // clearing fees
          shortcode_2_commissions_[_shortcode_] = t_commish_;
          return t_commish_;

        case HFSAT::NSE_IDXOPT:
        case HFSAT::NSE_STKOPT:
          t_commish_ = (0.000001 * 1.18 + 0.000025 * 1.18 + 0.000001 + 0.0005 + 0.0003503 * 1.18 + 0.000015) *
                       GetContractMultiplier(t_type_map_iter_->second);  // txn charge can become less than this //IPF charge 10/cr added
          shortcode_2_commissions_[_shortcode_] = t_commish_;
          return t_commish_;

        case HFSAT::NSE_CURFUT:
          t_commish_ =
              (0.000001 * 1.18 + 0.0000025 * 1.14 + 0.000002 + 0.000011 * 1.14 + 0.0000005) *  // txn charge can become less than this //IPF charge 10/cr added
              GetContractMultiplier(t_type_map_iter_->second);
          shortcode_2_commissions_[_shortcode_] = t_commish_;
          return t_commish_;

        case HFSAT::NSE_CUROPT:
          t_commish_ = (0.000001 * 1.18 + 0.00025 * 1.14 + 0.000002 + 0.0004 * 1.14 + 0.00002) *
                       GetContractMultiplier(t_type_map_iter_->second);  // txn charges can become lower than this //IPF charge 10/cr added
          shortcode_2_commissions_[_shortcode_] = t_commish_;
          return t_commish_;

        case HFSAT::NSE_IRFFUT:
          t_commish_ = (0.000001 * 1.18 + 0.0000025 * 1.14 + 0.000002 + 0.0 + 0.0000015 * 1.14 + 0.00000005) *
                       GetContractMultiplier(t_type_map_iter_->second);   			//IPF charge 10/cr added
          shortcode_2_commissions_[_shortcode_] = t_commish_;
          return t_commish_;

        //STT : 0.025% on sell side - 0.000125 each side
        //Txn Charges : 0.00325% on each side - 0.0000325 ( 1st tier ) https://archives.nseindia.com/content/circulars/FA56129.pdf 
        //SEBI charges : 10/cr - 0.000001 ( IPF ) 
        //Brokerage : 118 ( 100 + gst ) / Cr - Capped to 10L/month ( irage )  - 0.0000118 
        //Stamp Duty : Not applicable long as assuming irage charges
        case HFSAT::NSE_EQ:
          t_commish_ =
              (0.000001 * 1.18 + 0.000001 * 1.18 + 0.000125 + 0.0000297 * 1.18 + 0.000015) * GetContractMultiplier(t_type_map_iter_->second);  //IPF charge 10/cr added
          shortcode_2_commissions_[_shortcode_] = t_commish_;
          return t_commish_;
        case HFSAT::NSE_IDXSPT:
          t_commish_ = 0;
          shortcode_2_commissions_[_shortcode_] = t_commish_;
          return t_commish_;
        default:
          std::cerr << " GetNSECommission called with improper instrument type " << t_type_map_iter_->second
                    << ".Exiting.\n";
          exit(0);
      }
    }
    // Try to check if this is a MFT shortcode
    else if (shortcode_2_exchsymbol_all_.find(_shortcode_) != shortcode_2_exchsymbol_all_.end()) {
      // This is a MFT shortcode so we do not want to introduce latency here
      // Hence, we can also just find the commission for the ATM option for this underlying
      std::vector<std::string> tokens_;
      HFSAT::PerishableStringTokenizer::StringSplit(_shortcode_, '_', tokens_);
      std::string atm_shc_ = "NSE_" + tokens_[1] + "_C0" + "_A";
      return GetNSECommission(atm_shc_);
    }
  }
  std::cerr << " GetNSECommission called with improper shortcode " << _shortcode_ << ".Exiting.\n";
  exit(0);
}

// Shortcode assumed to be UNDERLYING_{FUT/P/C expiry}_{A/I/O offset}
// eg NIFTY_FUT0
// NIFTY_P0_A
// BANKNIFTY_C0_I7
void NSESecurityDefinitions::AddNSEContractSpecifications(ShortcodeContractSpecificationMap& t_contract_spec_map_) {
  PopulateOrderPriceRangeMapEQ(intdate_); // getting tick size from security file for EQ
  std::map<std::string, int> temp_lotsize_map_;
  std::vector<NSEInstrumentParams_t*>::iterator t_veciter_;
  for (t_veciter_ = inst_params_.begin(); t_veciter_ != inst_params_.end(); t_veciter_++) {
    NSEInstrumentParams_t* t_param_ = *t_veciter_;

    if (temp_lotsize_map_.find(t_param_->underlying_) == temp_lotsize_map_.end()) {
      temp_lotsize_map_[t_param_->underlying_] = t_param_->lot_size_;
    }

    // Only NIFTY and BANKNIFTY FINNIFTY currently in IDXFUT
    if (t_param_->inst_type_ == HFSAT::NSE_STKFUT || t_param_->inst_type_ == HFSAT::NSE_IRFFUT ||
        t_param_->inst_type_ == HFSAT::NSE_CURFUT ||
        (t_param_->inst_type_ == HFSAT::NSE_IDXFUT &&
         (strcmp(t_param_->underlying_, "NIFTY") == 0 || strcmp(t_param_->underlying_, "BANKNIFTY") == 0 ||
          strcmp(t_param_->underlying_, "MIDCPNIFTY") == 0 || strcmp(t_param_->underlying_, "FINNIFTY") == 0 || strcmp(t_param_->underlying_, "NIFTYNXT50") == 0))) {
      if (LoadThisExpiryContracts(t_param_->expiry_, t_param_->inst_type_, t_param_->underlying_)) {
        std::ostringstream t_futname_oss_;
        std::ostringstream t_futname_ess_;
	std::string t_shortcode_eq_ = "NSE_" + std::string(t_param_->underlying_) ;
        t_futname_oss_ << "NSE_" << t_param_->underlying_ << "_FUT"
                       << GetContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_, t_param_->underlying_);
	if (t_param_->inst_type_ == HFSAT::NSE_STKFUT) {
          t_contract_spec_map_[t_futname_oss_.str()] = ContractSpecification(
              shortcode_2_price_limit_[t_shortcode_eq_]->min_tick_size_,
              GetContractMultiplier(t_param_->inst_type_) * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
              kExchSourceNSE, t_param_->lot_size_);
          shortcode_2_price_limit_[t_futname_oss_.str()] = new NSETradingLimit_t(-1, 100000000, -1, shortcode_2_price_limit_[t_shortcode_eq_]->min_tick_size_);
	}
	else {
          t_contract_spec_map_[t_futname_oss_.str()] = ContractSpecification(
              t_param_->min_tick_,
              GetContractMultiplier(t_param_->inst_type_) * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
              kExchSourceNSE, t_param_->lot_size_);
          shortcode_2_price_limit_[t_futname_oss_.str()] = new NSETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
	}

        t_futname_ess_ << "NSE_" << t_param_->underlying_ << "_FUT_" << t_param_->expiry_;
        shortcode_2_exchsymbol_[t_futname_oss_.str()] = ConvertDataSourceNametoExchSymbolSecDef(t_futname_ess_.str());
        shortcode_2_type_[t_futname_oss_.str()] = t_param_->inst_type_;
        // store last close info per shortcode
        shortcode_2_last_close_[t_futname_oss_.str()] = t_param_->last_close_;

        shortcode_2_expiry_date_[t_futname_oss_.str()] = t_param_->expiry_;

        // spread contracts - currently only the front month - next month is supported
        // contract is added if expiry 1 is observerd to take care of issues with instruments getting
        // delisted/replaced
        if (GetContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_, t_param_->underlying_) == 1) {
          std::ostringstream t_futname_oss_;
          std::ostringstream t_futname_ess_;
          t_futname_oss_ << "NSE_" << t_param_->underlying_ << "_FUT0_1";

  	  if (t_param_->inst_type_ == HFSAT::NSE_STKFUT) {
            t_contract_spec_map_[t_futname_oss_.str()] = ContractSpecification(
                shortcode_2_price_limit_[t_shortcode_eq_]->min_tick_size_,
                GetContractMultiplier(t_param_->inst_type_) * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
                kExchSourceNSE, t_param_->lot_size_);
            shortcode_2_price_limit_[t_futname_oss_.str()] =
                new NSETradingLimit_t(-1, 100000000, -1, shortcode_2_price_limit_[t_shortcode_eq_]->min_tick_size_);
  	  }
  	  else {
            t_contract_spec_map_[t_futname_oss_.str()] = ContractSpecification(
                t_param_->min_tick_,
                GetContractMultiplier(t_param_->inst_type_) * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
                kExchSourceNSE, t_param_->lot_size_);
            shortcode_2_price_limit_[t_futname_oss_.str()] =
                new NSETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
  	  }

          t_futname_ess_ << "NSE_" << t_param_->underlying_ << "_FUT_"
                         << GetExpiryFromContractNumber(0, t_param_->inst_type_, t_param_->underlying_) << "_"
                         << GetExpiryFromContractNumber(1, t_param_->inst_type_, t_param_->underlying_);
          shortcode_2_exchsymbol_[t_futname_oss_.str()] = ConvertDataSourceNametoExchSymbolSecDef(t_futname_ess_.str());

          // TODO - correct this later - once ORS trades incorporate notional info
          shortcode_2_type_[t_futname_oss_.str()] = t_param_->inst_type_;

          shortcode_2_expiry_date_[t_futname_oss_.str()] =
              GetExpiryFromContractNumber(0, t_param_->inst_type_, t_param_->underlying_);

          shortcode_2_is_spread_[t_futname_oss_.str()] = true;
          exchange_symbol_2_is_spread_[ConvertDataSourceNametoExchSymbolSecDef(t_futname_ess_.str())] = true;
        }

        // bond futures aggregate symbol to account for change in underlying
        // [ -- 27 August 2014 ] -> 883GS2023
        // [ 28 August 2014 - 1 July 2015 ] -> 840GS2024
        // [ 1 July 2015 onwards ] -> 772GS2025
        if (t_param_->inst_type_ == HFSAT::NSE_IRFFUT &&
            ((t_param_->expiry_ <= 20140827 && strcmp(t_param_->underlying_, "883GS2023") == 0) ||
             (t_param_->expiry_ >= 20140828 && t_param_->expiry_ <= 20150630 &&
              strcmp(t_param_->underlying_, "840GS2024") == 0) ||
             (t_param_->expiry_ >= 20150701 && t_param_->expiry_ <= 20160627 &&
              strcmp(t_param_->underlying_, "772GS2025") == 0) ||
             (t_param_->expiry_ >= 20160628 && strcmp(t_param_->underlying_, "759GS2026") == 0))) {
          std::ostringstream t_futname_oss_;
          std::ostringstream t_futname_ess_;
          t_futname_oss_ << "NSE_GIND10YR_FUT"
                         << GetContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_, t_param_->underlying_);

          t_contract_spec_map_[t_futname_oss_.str()] = ContractSpecification(
              t_param_->min_tick_,
              GetContractMultiplier(t_param_->inst_type_) * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
              kExchSourceNSE, t_param_->lot_size_);

          t_futname_ess_ << "NSE_" << t_param_->underlying_ << "_FUT_" << t_param_->expiry_;
          shortcode_2_exchsymbol_[t_futname_oss_.str()] = ConvertDataSourceNametoExchSymbolSecDef(t_futname_ess_.str());

          shortcode_2_type_[t_futname_oss_.str()] = t_param_->inst_type_;
          // store last close info per shortcode
          shortcode_2_last_close_[t_futname_oss_.str()] = t_param_->last_close_;
          shortcode_2_expiry_date_[t_futname_oss_.str()] = t_param_->expiry_;
          shortcode_2_price_limit_[t_futname_oss_.str()] =
              new NSETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
        }
      }
    }

    else if (t_param_->inst_type_ == HFSAT::NSE_STKOPT || t_param_->inst_type_ == HFSAT::NSE_CUROPT ||
             (t_param_->inst_type_ == HFSAT::NSE_IDXOPT &&
              (strcmp(t_param_->underlying_, "NIFTY") == 0 || strcmp(t_param_->underlying_, "BANKNIFTY") == 0 ||
               strcmp(t_param_->underlying_, "MIDCPNIFTY") == 0 || strcmp(t_param_->underlying_, "FINNIFTY") == 0|| strcmp(t_param_->underlying_, "NIFTYNXT50") == 0))) {
      if (LoadThisExpiryContracts(t_param_->expiry_, t_param_->inst_type_, t_param_->underlying_)) {
        if (IsWeeklyOptionExpiry(t_param_->expiry_, t_param_->underlying_) && t_param_->inst_type_ == HFSAT::NSE_IDXOPT && strcmp(t_param_->underlying_, "NIFTYNXT50") != 0) {
          std::ostringstream t_optname_base_oss_;
          std::ostringstream t_temp_optname_oss_;
          std::ostringstream t_temp_optname_ess_;
          std::ostringstream t_temp_strike_price_;
          std::ostringstream t_temp_futname_oss_;

          double temp_step_value = t_param_->step_value_;
          double temp_step_value_prev = t_param_->step_value_prev_;

          if (strcmp(t_param_->underlying_, "NIFTY") == 0) {
            temp_step_value *= 2;
          } else if (strcmp(t_param_->underlying_, "BANKNIFTY") == 0) {
            temp_step_value *= 5;
          } else if (strcmp(t_param_->underlying_, "FINNIFTY") == 0) {
            temp_step_value *= 1;  // currently 100 (same as NIFTY)
          } else if (strcmp(t_param_->underlying_, "MIDCPNIFTY") == 0) {
            temp_step_value *= 1;  // currently 100 (same as NIFTY)
          } else if (strcmp(t_param_->underlying_, "NIFTYNXT50") == 0) {
            temp_step_value *= 5;  // currently 100 (same as BANKNIFTY)
          }

          t_temp_futname_oss_ << "NSE_" << t_param_->underlying_ << "_FUT0";
          shortcode_2_step_value_[t_temp_futname_oss_.str()] = temp_step_value;

          // ATM based on HFT
          // since we have two steps now, we have to calculate which one is nearest to close
          // STKOPT  YESBANK 1446.25 700     0.05    20170330        5       50.00   50.00   5
          double t_strike_ = -1;
          double t_strike_1_ = std::round(t_param_->last_close_ / temp_step_value) * temp_step_value;
          double t_strike_2_ = std::round(t_param_->last_close_ / temp_step_value_prev) * temp_step_value_prev;

          // The strikes generated from previous strike step may not be present. So we check if these strikes are
          // present in previous days's bhavcopy file. If they are present, we consider them else disregard them.
          double t_strike_2_atm_ =
              GetNearestAvailableMultipleStrike(*t_param_, t_param_->last_close_, temp_step_value_prev,
                                                t_strike_1_ + temp_step_value, t_strike_1_ - temp_step_value);

          // Calculating nearest strike to FUT price. Since 2 strike steps are there, we consider both.
          // GetNearestAvailableMultipleStrike returns -1, if strikes of required multiple are not present between upper
          // and lower bound. So simply use t_strike_1_.
          // we are always setting ATM based on current schema ! ( t_strike_2_atm_ = -1, always )
          if (t_strike_2_atm_ < 0) {
            t_strike_ = t_strike_1_;
          } else {
            if (fabs(t_strike_1_ - t_param_->last_close_) < fabs(t_strike_2_atm_ - t_param_->last_close_)) {
              t_strike_ = t_strike_1_;
            } else {
              t_strike_ = t_strike_2_atm_;
            }
          }

          double t_adjusted_strike_ = t_strike_;
          bool is_underlying_corp_action_ = false;
          bool is_underlying_corp_action_ex_date_ = false;
          if (underlying_2_corporate_action_value_.find(t_param_->underlying_) !=
              underlying_2_corporate_action_value_.end()) {
            is_underlying_corp_action_ = true;
            is_underlying_corp_action_ex_date_ =
                ((std::get<0>(underlying_2_corporate_action_value_[t_param_->underlying_]) == intdate_) ? true : false);
            t_adjusted_strike_ = GetAdjustedValue(t_param_->underlying_, t_strike_, t_param_->min_tick_);
          }

          t_optname_base_oss_ << "NSE_" << t_param_->underlying_;
          std::set<double> otm_call_itm_put_strike_price_set_;
          std::set<double, std::greater<double>> otm_put_itm_call_strike_price_set_;
          double t_atm_strike_ = t_strike_;
          // atm put/call
          {
            // this is mainly to push the adjusted strike into otm_call_itm_put or otm_put_itm_call vector
            // as per the ATM strike price which is t_strike_.
            // This is mainly done so that we can include adjusted strike into itm or otm as per the condition satisfied
            if (!is_underlying_corp_action_ex_date_) {
              // This section is for other than ex-date as we include all strikes(normal and adjusted)
              t_atm_strike_ = t_strike_;
              if (t_adjusted_strike_ > t_strike_) {
                otm_call_itm_put_strike_price_set_.insert(t_adjusted_strike_);
              } else if (t_adjusted_strike_ < t_strike_) {
                otm_put_itm_call_strike_price_set_.insert(t_adjusted_strike_);
              }
            } else {
              // This section is for corporate action ex-date as we include only adjusted strike price
              t_atm_strike_ = t_adjusted_strike_;
            }
            // ATM Put
            t_temp_optname_oss_.str(std::string());
            t_temp_optname_oss_ << t_optname_base_oss_.str() << "_P"
                                  << GetWeeklyContractNumberFromExpiry(t_param_->expiry_, t_param_->underlying_)
                                  << "_A_W";
            

            t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                t_param_->min_tick_,
                GetContractMultiplier(t_param_->inst_type_) * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
                kExchSourceNSE, t_param_->lot_size_);

            t_temp_optname_ess_.str(std::string());
            t_temp_optname_ess_ << t_optname_base_oss_.str() << "_PE_" << t_param_->expiry_ << "_" << std::fixed
                                << std::setprecision(2) << t_atm_strike_;
            shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());
            // map this shortcode to options map fetched from bhavcopy file
            t_temp_strike_price_.str(std::string());
            t_temp_strike_price_ << std::fixed << std::setprecision(2) << t_atm_strike_;

            AddtoOptionsLastClosePrice(t_param_->inst_type_, t_param_->underlying_, t_param_->expiry_, t_atm_strike_,
                                       (char*)"PE", t_temp_optname_oss_.str());
            // loaded in options_lpx

            shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
            // store last close info per shortcode -- approximate.
            shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
            shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
            shortcode_2_strike_price_[t_temp_optname_oss_.str()] = t_atm_strike_;
            shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                new NSETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);

            //--------------------

            // ATM Call
            t_temp_optname_oss_.str(std::string());
              t_temp_optname_oss_ << t_optname_base_oss_.str() << "_C"
                                  << GetWeeklyContractNumberFromExpiry(t_param_->expiry_, t_param_->underlying_)
                                  << "_A_W";
            
            t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                t_param_->min_tick_,
                GetContractMultiplier(t_param_->inst_type_) * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
                kExchSourceNSE, t_param_->lot_size_);

            t_temp_optname_ess_.str(std::string());
            t_temp_optname_ess_ << t_optname_base_oss_.str() << "_CE_" << t_param_->expiry_ << "_"
                                << std::setprecision(2) << t_atm_strike_;
            shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());

            // map this shortcode to options map fetched from bhavcopy file
            t_temp_strike_price_.str(std::string());
            t_temp_strike_price_ << std::fixed << std::setprecision(2) << t_atm_strike_;
            AddtoOptionsLastClosePrice(t_param_->inst_type_, t_param_->underlying_, t_param_->expiry_, t_atm_strike_,
                                       (char*)"CE", t_temp_optname_oss_.str());
            // loaded in options_lpx

            shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
            // store last close info per shortcode -- approximate.
            shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
            shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
            shortcode_2_strike_price_[t_temp_optname_oss_.str()] = t_atm_strike_;
            shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                new NSETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
          }
          // otm/itm options
          {
            // For OTM Call and ITM Put
            {
              int counter_1_inc = 0;
              int counter_2_inc = 0;

              double start_strike_1_inc = t_strike_1_;
              double start_strike_2_inc = t_strike_2_;

              // this is to just for initialization for the upcoming loop to work
              while (start_strike_1_inc <= t_strike_) {
                start_strike_1_inc += t_param_->step_value_;
              }
              // this is to just for initialization for the upcoming loop to work
              // here we might be ignoring first OTM which is less than first OTM based on current schema
              while (start_strike_2_inc <= t_strike_) {
                start_strike_2_inc += t_param_->step_value_prev_;
              }

              // not sure why we need this
              if (fabs(start_strike_1_inc - t_strike_) < 0.0001) {
                start_strike_1_inc += t_param_->step_value_;
              }

              // not sure why we need this
              if (fabs(start_strike_2_inc - t_strike_) < 0.0001) {
                start_strike_2_inc += t_param_->step_value_prev_;
              }

              // As of now, both start_strike_inc are just more than ATM and is of required multiple
              // we want to include all options corresponding to the current schema
              // hence we loop through till we expiry
              unsigned int t_cs_ctr_ = 1;
              if (t_strike_ < t_param_->last_close_) {
                t_cs_ctr_ = 0;
              }
              // while (t_ctr_ <= t_param_->num_itm_) {
              while (t_cs_ctr_ <= t_param_->num_itm_) {
                double option_strike_1_inc = start_strike_1_inc + t_param_->step_value_ * (counter_1_inc);
                double option_strike_2_inc = start_strike_2_inc + t_param_->step_value_prev_ * (counter_2_inc);
                double option_strike_inc = -1;
                t_cs_ctr_++;
                // The below block ensures that we are considering both strike steps in calculation the OTM_1, OTM_2
                // etc.
                if (fabs(option_strike_1_inc - option_strike_2_inc) <
                    0.0001) {  // $option_strike_1_ == $option_strike_2
                  option_strike_inc = option_strike_1_inc;
                  counter_1_inc += 1;
                  counter_2_inc += 1;
                } else if (option_strike_1_inc < option_strike_2_inc) {
                  option_strike_inc = option_strike_1_inc;
                  counter_1_inc += 1;
                } else if (option_strike_1_inc > option_strike_2_inc) {
                  option_strike_inc = option_strike_2_inc;
                  counter_2_inc += 1;
                  // This checks whether strikes generated from previous steps are present in Bhavcopy.
                  // If not present we don't consider them.
                  t_cs_ctr_--;
                  if (!IsOptionStrikePresentInBhavCopy(*t_param_, option_strike_inc)) {
                    continue;
                  }
                }
                double adjusted_option_strike_inc = option_strike_inc;
                if (is_underlying_corp_action_) {
                  adjusted_option_strike_inc =
                      GetAdjustedValue(t_param_->underlying_, option_strike_inc, t_param_->min_tick_);
                }
                if (!is_underlying_corp_action_ex_date_) {
                  // this is mainly to push the inc strike into otm_call_itm_put or otm_put_itm_call vector
                  if (option_strike_inc > t_atm_strike_) {
                    otm_call_itm_put_strike_price_set_.insert(option_strike_inc);
                  } else if (option_strike_inc < t_atm_strike_) {
                    otm_put_itm_call_strike_price_set_.insert(option_strike_inc);
                  }
                }
                // this is mainly to push the adjusted inc strike into otm_call_itm_put or otm_put_itm_call vector
                if (adjusted_option_strike_inc > t_atm_strike_) {
                  otm_call_itm_put_strike_price_set_.insert(adjusted_option_strike_inc);
                } else if (adjusted_option_strike_inc < t_atm_strike_) {
                  otm_put_itm_call_strike_price_set_.insert(adjusted_option_strike_inc);
                }
              }
            }

            // For ITM Call and OTM Put
            {
              int counter_1_dec = 0;
              int counter_2_dec = 0;

              double start_strike_1_dec = t_strike_1_;
              double start_strike_2_dec = t_strike_2_;

              // this is to just for initialization for the upcoming loop to work
              while (start_strike_1_dec >= t_strike_) {
                start_strike_1_dec -= t_param_->step_value_;
              }
              // this is to just for initialization for the upcoming loop to work
              while (start_strike_2_dec >= t_strike_) {
                start_strike_2_dec -= t_param_->step_value_prev_;
              }

              if (fabs(start_strike_1_dec - t_strike_) < 0.0001) {
                start_strike_1_dec -= t_param_->step_value_;
              }

              if (fabs(start_strike_2_dec - t_strike_) < 0.0001) {
                start_strike_2_dec -= t_param_->step_value_prev_;
              }

              // As of now, both start_strike_dec are just less than ATM and is of required multiple
              unsigned int t_ctr_ = 1;
              unsigned int t_cs_ctr_ = 1;
              if (t_strike_ > t_param_->last_close_) {
                t_cs_ctr_ = 0;
              }

              int32_t ratio = 1;

              if (strcmp(t_param_->underlying_, "NIFTY") == 0) {
                ratio = 2;
              } else if (strcmp(t_param_->underlying_, "BANKNIFTY") == 0) {
                ratio = 5;
              } else if (strcmp(t_param_->underlying_, "FINNIFTY") == 0) {
                ratio = 1;
              } else if (strcmp(t_param_->underlying_, "MIDCPNIFTY") == 0) {
                ratio = 1;
              } else if (strcmp(t_param_->underlying_, "NIFTYNXT50") == 0) {
                ratio = 1;
              }

              // while (t_ctr_ <= t_param_->num_itm_) {
              while (t_cs_ctr_ <= t_param_->num_itm_) {
                t_temp_optname_ess_.str(std::string());
                t_temp_optname_oss_.str(std::string());

                if (0 == (t_ctr_ % ratio)) {
                    t_temp_optname_oss_ << t_optname_base_oss_.str() << "_P"
                                        << GetWeeklyContractNumberFromExpiry(t_param_->expiry_, t_param_->underlying_)
                                        << "_I" << t_ctr_ / ratio << "_W";
                } else {
                    t_temp_optname_oss_ << t_optname_base_oss_.str() << "_P"
                                        << GetWeeklyContractNumberFromExpiry(t_param_->expiry_, t_param_->underlying_)
                                        << "_I" << (t_ctr_ / ratio) << "_M" << (t_ctr_ % ratio) << "_W";
                }

                double option_strike_1_dec = start_strike_1_dec - t_param_->step_value_ * (counter_1_dec);
                double option_strike_2_dec = start_strike_2_dec - t_param_->step_value_prev_ * (counter_2_dec);
                double option_strike_dec = -1;
                t_cs_ctr_++;

                if (fabs(option_strike_1_dec - option_strike_2_dec) <
                    0.0001) {  // $option_strike_1_ == $option_strike_2
                  option_strike_dec = option_strike_1_dec;
                  counter_1_dec += 1;
                  counter_2_dec += 1;
                } else if (option_strike_1_dec < option_strike_2_dec) {
                  t_cs_ctr_--;
                  option_strike_dec = option_strike_2_dec;
                  counter_2_dec += 1;
                  if (!IsOptionStrikePresentInBhavCopy(*t_param_, option_strike_dec)) {
                    continue;
                  }
                } else if (option_strike_1_dec > option_strike_2_dec) {
                  option_strike_dec = option_strike_1_dec;
                  counter_1_dec += 1;
                }

                double adjusted_option_strike_dec = option_strike_dec;
                if (is_underlying_corp_action_) {
                  adjusted_option_strike_dec =
                      GetAdjustedValue(t_param_->underlying_, option_strike_dec, t_param_->min_tick_);
                }

                if (!is_underlying_corp_action_ex_date_) {
                  // this is mainly to push the dec strike into otm_call_itm_put or otm_put_itm_call vector
                  if (option_strike_dec > t_atm_strike_) {
                    otm_call_itm_put_strike_price_set_.insert(option_strike_dec);
                  } else if (option_strike_dec < t_atm_strike_) {
                    otm_put_itm_call_strike_price_set_.insert(option_strike_dec);
                  }
                }
                // this is mainly to push the adjusted dec strike into otm_call_itm_put or otm_put_itm_call vector
                if (adjusted_option_strike_dec > t_atm_strike_) {
                  otm_call_itm_put_strike_price_set_.insert(adjusted_option_strike_dec);
                } else if (adjusted_option_strike_dec < t_atm_strike_) {
                  otm_put_itm_call_strike_price_set_.insert(adjusted_option_strike_dec);
                }
              }
            }
            //
            int32_t ratio = 1;

            if (strcmp(t_param_->underlying_, "NIFTY") == 0) {
              ratio = 2;
            } else if (strcmp(t_param_->underlying_, "BANKNIFTY") == 0) {
              ratio = 5;
            } else if (strcmp(t_param_->underlying_, "FINNIFTY") == 0) {
              ratio = 1;
            } else if (strcmp(t_param_->underlying_, "MIDCPNIFTY") == 0) {
              ratio = 1;
            } else if (strcmp(t_param_->underlying_, "NIFTYNXT50") == 0) {
              ratio = 1;
            }

            unsigned int t_ctr_ = 0;
            std::set<double>::iterator otm_call_itm_put_itr_;
            for (otm_call_itm_put_itr_ = otm_call_itm_put_strike_price_set_.begin();
                 otm_call_itm_put_itr_ != otm_call_itm_put_strike_price_set_.end(); ++otm_call_itm_put_itr_) {
              double current_strike_price_ = *otm_call_itm_put_itr_;
              // ITM PUT
              t_temp_optname_ess_.str(std::string());
              t_temp_optname_oss_.str(std::string());

              t_temp_optname_ess_ << t_optname_base_oss_.str() << "_PE_" << t_param_->expiry_ << "_"
                                  << std::setprecision(2) << current_strike_price_;

	      if (ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str()) == "INVALID") continue; // INVALID exchange sym handling
              t_ctr_++;

              if (0 == (t_ctr_ % ratio)) {
                  t_temp_optname_oss_ << t_optname_base_oss_.str() << "_P"
                                      << GetWeeklyContractNumberFromExpiry(t_param_->expiry_, t_param_->underlying_)
                                      << "_I" << t_ctr_ / ratio << "_W";

              } else {
                  t_temp_optname_oss_ << t_optname_base_oss_.str() << "_P"
                                      << GetWeeklyContractNumberFromExpiry(t_param_->expiry_, t_param_->underlying_)
                                      << "_I" << (t_ctr_ / ratio) << "_M" << (t_ctr_ % ratio) << "_W";
              }

              t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                  t_param_->min_tick_,
                  GetContractMultiplier(t_param_->inst_type_) * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
                  kExchSourceNSE, t_param_->lot_size_);

              shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                  ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());

              // map this shortcode to options map fetched from bhavcopy file
              t_temp_strike_price_.str(std::string());
              t_temp_strike_price_ << std::fixed << std::setprecision(2) << current_strike_price_;
              AddtoOptionsLastClosePrice(t_param_->inst_type_, t_param_->underlying_, t_param_->expiry_,
                                         current_strike_price_, (char*)"PE", t_temp_optname_oss_.str());
              // loaded in options_lpx

              shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
              // store last close info per shortcode -- approximate.
              shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
              shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                  new NSETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
              shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
              shortcode_2_strike_price_[t_temp_optname_oss_.str()] = current_strike_price_;

              //-------------------------
              // OTM Call
              t_temp_optname_oss_.str(std::string());
              t_temp_optname_ess_.str(std::string());

              t_temp_optname_ess_ << t_optname_base_oss_.str() << "_CE_" << t_param_->expiry_ << "_"
                                  << std::setprecision(2) << current_strike_price_;

	      if (ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str()) == "INVALID") continue; // INVALID exchange sym handling

              if (0 == (t_ctr_ % ratio)) {
                  t_temp_optname_oss_ << t_optname_base_oss_.str() << "_C"
                                      << GetWeeklyContractNumberFromExpiry(t_param_->expiry_, t_param_->underlying_)
                                      << "_O" << t_ctr_ / ratio << "_W";

              } else {
                  t_temp_optname_oss_ << t_optname_base_oss_.str() << "_C"
                                      << GetWeeklyContractNumberFromExpiry(t_param_->expiry_, t_param_->underlying_)
                                      << "_O" << (t_ctr_ / ratio) << "_M" << (t_ctr_ % ratio) << "_W";
              }
              t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                  t_param_->min_tick_,
                  GetContractMultiplier(t_param_->inst_type_) * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
                  kExchSourceNSE, t_param_->lot_size_);

              shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                  ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());

              // map this shortcode to options map fetched from bhavcopy file
              t_temp_strike_price_.str(std::string());
              t_temp_strike_price_ << std::fixed << std::setprecision(2) << current_strike_price_;
              AddtoOptionsLastClosePrice(t_param_->inst_type_, t_param_->underlying_, t_param_->expiry_,
                                         current_strike_price_, (char*)"CE", t_temp_optname_oss_.str());

              // loaded in options_lpx

              shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
              // store last close info per shortcode -- approximate.
              shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
              shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                  new NSETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
              shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
              shortcode_2_strike_price_[t_temp_optname_oss_.str()] = current_strike_price_;
            }  // end of for loop

            std::set<double, std::greater<double>>::iterator otm_put_itm_call_itr_;
            t_ctr_ = 0;
            for (otm_put_itm_call_itr_ = otm_put_itm_call_strike_price_set_.begin();
                 otm_put_itm_call_itr_ != otm_put_itm_call_strike_price_set_.end(); ++otm_put_itm_call_itr_) {
              double current_strike_price_ = *otm_put_itm_call_itr_;

              // OTM Put
              t_temp_optname_oss_.str(std::string());
              t_temp_optname_ess_.str(std::string());

              t_temp_optname_ess_ << t_optname_base_oss_.str() << "_PE_" << t_param_->expiry_ << "_"
                                  << std::setprecision(2) << current_strike_price_;

	      if (ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str()) == "INVALID") continue; // INVALID exchange sym handling
              t_ctr_++;

              if (0 == (t_ctr_ % ratio)) {
                  t_temp_optname_oss_ << t_optname_base_oss_.str() << "_P"
                                      << GetWeeklyContractNumberFromExpiry(t_param_->expiry_, t_param_->underlying_)
                                      << "_O" << t_ctr_ / ratio << "_W";

              } else {
                  t_temp_optname_oss_ << t_optname_base_oss_.str() << "_P"
                                      << GetWeeklyContractNumberFromExpiry(t_param_->expiry_, t_param_->underlying_)
                                      << "_O" << (t_ctr_ / ratio) << "_M" << (t_ctr_ % ratio) << "_W";
              }

              t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                  t_param_->min_tick_,
                  GetContractMultiplier(t_param_->inst_type_) * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
                  kExchSourceNSE, t_param_->lot_size_);

              shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                  ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());

              // map this shortcode to options map fetched from bhavcopy file
              t_temp_strike_price_.str(std::string());
              t_temp_strike_price_ << std::fixed << std::setprecision(2) << current_strike_price_;
              AddtoOptionsLastClosePrice(t_param_->inst_type_, t_param_->underlying_, t_param_->expiry_,
                                         current_strike_price_, (char*)"PE", t_temp_optname_oss_.str());
              // loaded in options_lpx

              shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
              // store last close info per shortcode -- approximate.
              shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
              shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                  new NSETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
              shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
              shortcode_2_strike_price_[t_temp_optname_oss_.str()] = current_strike_price_;

              // ITM Call
              t_temp_optname_oss_.str(std::string());
              t_temp_optname_ess_.str(std::string());

              t_temp_optname_ess_ << t_optname_base_oss_.str() << "_CE_" << t_param_->expiry_ << "_"
                                  << std::setprecision(2) << current_strike_price_;

	      if (ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str()) == "INVALID") continue; // INVALID exchange sym handling

              if (0 == (t_ctr_ % ratio)) {
                  t_temp_optname_oss_ << t_optname_base_oss_.str() << "_C"
                                      << GetWeeklyContractNumberFromExpiry(t_param_->expiry_, t_param_->underlying_)
                                      << "_I" << t_ctr_ / ratio << "_W";

              } else {
                  t_temp_optname_oss_ << t_optname_base_oss_.str() << "_C"
                                      << GetWeeklyContractNumberFromExpiry(t_param_->expiry_, t_param_->underlying_)
                                      << "_I" << (t_ctr_ / ratio) << "_M" << (t_ctr_ % ratio) << "_W";
              }

              t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                  t_param_->min_tick_,
                  GetContractMultiplier(t_param_->inst_type_) * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
                  kExchSourceNSE, t_param_->lot_size_);

              shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                  ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());

              // map this shortcode to options map fetched from bhavcopy file
              t_temp_strike_price_.str(std::string());
              t_temp_strike_price_ << std::fixed << std::setprecision(2) << current_strike_price_;
              AddtoOptionsLastClosePrice(t_param_->inst_type_, t_param_->underlying_, t_param_->expiry_,
                                         current_strike_price_, (char*)"CE", t_temp_optname_oss_.str());
              // loaded in options_lpx

              shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
              // store last close info per shortcode -- approximate.
              shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
              shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                  new NSETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
              shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
              shortcode_2_strike_price_[t_temp_optname_oss_.str()] = current_strike_price_;

            }  // end of for loop
          }
        }
        if (IsMonthlyOptionExpiry(t_param_->expiry_, t_param_->underlying_)) {
          std::ostringstream t_optname_base_oss_;
          std::ostringstream t_temp_optname_oss_;
          std::ostringstream t_temp_optname_ess_;
          std::ostringstream t_temp_strike_price_;
          std::ostringstream t_temp_futname_oss_;

          double temp_step_value = t_param_->step_value_;
          double temp_step_value_prev = t_param_->step_value_prev_;

          if (strcmp(t_param_->underlying_, "NIFTY") == 0) {
            temp_step_value *= 2;
          } else if (strcmp(t_param_->underlying_, "BANKNIFTY") == 0) {
            temp_step_value *= 5;
          } else if (strcmp(t_param_->underlying_, "FINNIFTY") == 0) {
            temp_step_value *= 1;  // currently 100 (same as NIFTY)
          } else if (strcmp(t_param_->underlying_, "MIDCPNIFTY") == 0) {
            temp_step_value *= 1;  // currently 100 (same as NIFTY)
          } else if (strcmp(t_param_->underlying_, "NIFTYNXT50") == 0) {
            temp_step_value *= 5;
          }

          t_temp_futname_oss_ << "NSE_" << t_param_->underlying_ << "_FUT0";
          shortcode_2_step_value_[t_temp_futname_oss_.str()] = temp_step_value;

          // ATM based on HFT
          // since we have two steps now, we have to calculate which one is nearest to close
          // STKOPT  YESBANK 1446.25 700     0.05    20170330        5       50.00   50.00   5
          double t_strike_ = -1;
          double t_strike_1_ = std::round(t_param_->last_close_ / temp_step_value) * temp_step_value;
          double t_strike_2_ = std::round(t_param_->last_close_ / temp_step_value_prev) * temp_step_value_prev;

          // The strikes generated from previous strike step may not be present. So we check if these strikes are
          // present in previous days's bhavcopy file. If they are present, we consider them else disregard them.
          double t_strike_2_atm_ =
              GetNearestAvailableMultipleStrike(*t_param_, t_param_->last_close_, temp_step_value_prev,
                                                t_strike_1_ + temp_step_value, t_strike_1_ - temp_step_value);
          bool using_curr_stk_scheme = true;

          // Calculating nearest strike to FUT price. Since 2 strike steps are there, we consider both.
          // GetNearestAvailableMultipleStrike returns -1, if strikes of required multiple are not present between upper
          // and lower bound. So simply use t_strike_1_.
          // we are always setting ATM based on current schema ! ( t_strike_2_atm_ = -1, always )
          if (t_strike_2_atm_ < 0) {
            t_strike_ = t_strike_1_;
          } else {
            if (fabs(t_strike_1_ - t_param_->last_close_) < fabs(t_strike_2_atm_ - t_param_->last_close_)) {
              t_strike_ = t_strike_1_;
            } else {
              using_curr_stk_scheme = false;
              t_strike_ = t_strike_2_atm_;
            }
          }

          double t_adjusted_strike_ = t_strike_;
          bool is_underlying_corp_action_ = false;
          bool is_underlying_corp_action_ex_date_ = false;
          if (underlying_2_corporate_action_value_.find(t_param_->underlying_) !=
              underlying_2_corporate_action_value_.end()) {
            is_underlying_corp_action_ = true;
            is_underlying_corp_action_ex_date_ =
                ((std::get<0>(underlying_2_corporate_action_value_[t_param_->underlying_]) == intdate_) ? true : false);
            t_adjusted_strike_ = GetAdjustedValue(t_param_->underlying_, t_strike_, t_param_->min_tick_);
          }

          t_optname_base_oss_ << "NSE_" << t_param_->underlying_;
          std::set<double> otm_call_itm_put_strike_price_set_;
          std::set<double, std::greater<double>> otm_put_itm_call_strike_price_set_;
          double t_atm_strike_ = t_strike_;
          // atm put/call
          {
            // this is mainly to push the adjusted strike into otm_call_itm_put or otm_put_itm_call vector
            // as per the ATM strike price which is t_strike_.
            // This is mainly done so that we can include adjusted strike into itm or otm as per the condition satisfied
            if (!is_underlying_corp_action_ex_date_) {
              // This section is for other than ex-date as we include all strikes(normal and adjusted)
              t_atm_strike_ = t_strike_;
              if (t_adjusted_strike_ > t_strike_) {
                otm_call_itm_put_strike_price_set_.insert(t_adjusted_strike_);
              } else if (t_adjusted_strike_ < t_strike_) {
                otm_put_itm_call_strike_price_set_.insert(t_adjusted_strike_);
              }
            } else {
              // This section is for corporate action ex-date as we include only adjusted strike price
              t_atm_strike_ = t_adjusted_strike_;
            }
            // ATM Put
            t_temp_optname_oss_.str(std::string());
              t_temp_optname_oss_ << t_optname_base_oss_.str() << "_P"
                                  << GetContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_,
                                                                 t_param_->underlying_)
                                  << "_A";
            

            t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                t_param_->min_tick_,
                GetContractMultiplier(t_param_->inst_type_) * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
                kExchSourceNSE, t_param_->lot_size_);

            t_temp_optname_ess_.str(std::string());
            t_temp_optname_ess_ << t_optname_base_oss_.str() << "_PE_" << t_param_->expiry_ << "_" << std::fixed
                                << std::setprecision(2) << t_atm_strike_;
            shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());
            // map this shortcode to options map fetched from bhavcopy file
            t_temp_strike_price_.str(std::string());
            t_temp_strike_price_ << std::fixed << std::setprecision(2) << t_atm_strike_;

            AddtoOptionsLastClosePrice(t_param_->inst_type_, t_param_->underlying_, t_param_->expiry_, t_atm_strike_,
                                       (char*)"PE", t_temp_optname_oss_.str());
            // loaded in options_lpx

            shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
            // store last close info per shortcode -- approximate.
            shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
            shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
            shortcode_2_strike_price_[t_temp_optname_oss_.str()] = t_atm_strike_;
            shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                new NSETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
            // Only adding current expiry
            if (!IsWeeklyOptionExpiry(t_param_->expiry_, t_param_->underlying_) &&
                GetContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_, t_param_->underlying_) == 0) {
              NSEOptionMetaInfo info =
                  NSEOptionMetaInfo(t_temp_optname_oss_.str(), t_atm_strike_, false, using_curr_stk_scheme, 0,
                                    (t_atm_strike_ > t_param_->last_close_),
                                    ((t_atm_strike_ - t_param_->last_close_) / t_param_->step_value_));
              HFSAT::VectorUtils::UniqueVectorAdd(underlying_to_options_[t_param_->underlying_], info);
            }

            //--------------------

            // ATM Call
            t_temp_optname_oss_.str(std::string());

              t_temp_optname_oss_ << t_optname_base_oss_.str() << "_C"
                                  << GetContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_,
                                                                 t_param_->underlying_)
                                  << "_A";
            t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                t_param_->min_tick_,
                GetContractMultiplier(t_param_->inst_type_) * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
                kExchSourceNSE, t_param_->lot_size_);

            t_temp_optname_ess_.str(std::string());
            t_temp_optname_ess_ << t_optname_base_oss_.str() << "_CE_" << t_param_->expiry_ << "_"
                                << std::setprecision(2) << t_atm_strike_;
            shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());

            // map this shortcode to options map fetched from bhavcopy file
            t_temp_strike_price_.str(std::string());
            t_temp_strike_price_ << std::fixed << std::setprecision(2) << t_atm_strike_;
            AddtoOptionsLastClosePrice(t_param_->inst_type_, t_param_->underlying_, t_param_->expiry_, t_atm_strike_,
                                       (char*)"CE", t_temp_optname_oss_.str());
            // loaded in options_lpx

            shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
            // store last close info per shortcode -- approximate.
            shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
            shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
            shortcode_2_strike_price_[t_temp_optname_oss_.str()] = t_atm_strike_;
            shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                new NSETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);

            // Only adding current expiry
            if (!IsWeeklyOptionExpiry(t_param_->expiry_, t_param_->underlying_) &&
                GetContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_, t_param_->underlying_) == 0) {
              NSEOptionMetaInfo info =
                  NSEOptionMetaInfo(t_temp_optname_oss_.str(), t_atm_strike_, true, using_curr_stk_scheme, 0,
                                    (t_param_->last_close_ > t_atm_strike_),
                                    ((t_param_->last_close_ - t_atm_strike_) / t_param_->step_value_));
              HFSAT::VectorUtils::UniqueVectorAdd(underlying_to_options_[t_param_->underlying_], info);
            }
          }
          // otm/itm options
          {
            // For OTM Call and ITM Put
            {
              int counter_1_inc = 0;
              int counter_2_inc = 0;

              double start_strike_1_inc = t_strike_1_;
              double start_strike_2_inc = t_strike_2_;

              // this is to just for initialization for the upcoming loop to work
              while (start_strike_1_inc <= t_strike_) {
                start_strike_1_inc += t_param_->step_value_;
              }
              // this is to just for initialization for the upcoming loop to work
              // here we might be ignoring first OTM which is less than first OTM based on current schema
              while (start_strike_2_inc <= t_strike_) {
                start_strike_2_inc += t_param_->step_value_prev_;
              }

              // not sure why we need this
              if (fabs(start_strike_1_inc - t_strike_) < 0.0001) {
                start_strike_1_inc += t_param_->step_value_;
              }

              // not sure why we need this
              if (fabs(start_strike_2_inc - t_strike_) < 0.0001) {
                start_strike_2_inc += t_param_->step_value_prev_;
              }

              // As of now, both start_strike_inc are just more than ATM and is of required multiple
              // we want to include all options corresponding to the current schema
              // hence we loop through till we expiry
              unsigned int t_cs_ctr_ = 1;
              if (t_strike_ < t_param_->last_close_) {
                t_cs_ctr_ = 0;
              }
              // while (t_ctr_ <= t_param_->num_itm_) {
              while (t_cs_ctr_ <= t_param_->num_itm_) {
                double option_strike_1_inc = start_strike_1_inc + t_param_->step_value_ * (counter_1_inc);
                double option_strike_2_inc = start_strike_2_inc + t_param_->step_value_prev_ * (counter_2_inc);
                double option_strike_inc = -1;
                using_curr_stk_scheme = true;
                t_cs_ctr_++;
                // The below block ensures that we are considering both strike steps in calculation the OTM_1, OTM_2
                // etc.
                if (fabs(option_strike_1_inc - option_strike_2_inc) <
                    0.0001) {  // $option_strike_1_ == $option_strike_2
                  option_strike_inc = option_strike_1_inc;
                  counter_1_inc += 1;
                  counter_2_inc += 1;
                } else if (option_strike_1_inc < option_strike_2_inc) {
                  option_strike_inc = option_strike_1_inc;
                  counter_1_inc += 1;
                } else if (option_strike_1_inc > option_strike_2_inc) {
                  option_strike_inc = option_strike_2_inc;
                  counter_2_inc += 1;
                  // This checks whether strikes generated from previous steps are present in Bhavcopy.
                  // If not present we don't consider them.
                  t_cs_ctr_--;
                  if (!IsOptionStrikePresentInBhavCopy(*t_param_, option_strike_inc)) {
                    continue;
                  }
                  using_curr_stk_scheme = false;
                }
                double adjusted_option_strike_inc = option_strike_inc;
                if (is_underlying_corp_action_) {
                  adjusted_option_strike_inc =
                      GetAdjustedValue(t_param_->underlying_, option_strike_inc, t_param_->min_tick_);
                }
                if (!is_underlying_corp_action_ex_date_) {
                  // this is mainly to push the inc strike into otm_call_itm_put or otm_put_itm_call vector
                  if (option_strike_inc > t_atm_strike_) {
                    otm_call_itm_put_strike_price_set_.insert(option_strike_inc);
                  } else if (option_strike_inc < t_atm_strike_) {
                    otm_put_itm_call_strike_price_set_.insert(option_strike_inc);
                  }
                }
                // this is mainly to push the adjusted inc strike into otm_call_itm_put or otm_put_itm_call vector
                if (adjusted_option_strike_inc > t_atm_strike_) {
                  otm_call_itm_put_strike_price_set_.insert(adjusted_option_strike_inc);
                } else if (adjusted_option_strike_inc < t_atm_strike_) {
                  otm_put_itm_call_strike_price_set_.insert(adjusted_option_strike_inc);
                }
              }
            }

            // For ITM Call and OTM Put
            {
              int counter_1_dec = 0;
              int counter_2_dec = 0;

              double start_strike_1_dec = t_strike_1_;
              double start_strike_2_dec = t_strike_2_;

              // this is to just for initialization for the upcoming loop to work
              while (start_strike_1_dec >= t_strike_) {
                start_strike_1_dec -= t_param_->step_value_;
              }
              // this is to just for initialization for the upcoming loop to work
              while (start_strike_2_dec >= t_strike_) {
                start_strike_2_dec -= t_param_->step_value_prev_;
              }

              if (fabs(start_strike_1_dec - t_strike_) < 0.0001) {
                start_strike_1_dec -= t_param_->step_value_;
              }

              if (fabs(start_strike_2_dec - t_strike_) < 0.0001) {
                start_strike_2_dec -= t_param_->step_value_prev_;
              }

              // As of now, both start_strike_dec are just less than ATM and is of required multiple
              unsigned int t_ctr_ = 1;
              unsigned int t_cs_ctr_ = 1;
              if (t_strike_ > t_param_->last_close_) {
                t_cs_ctr_ = 0;
              }

              int32_t ratio = 1;

              if (strcmp(t_param_->underlying_, "NIFTY") == 0) {
                ratio = 2;
              } else if (strcmp(t_param_->underlying_, "BANKNIFTY") == 0) {
                ratio = 5;
              } else if (strcmp(t_param_->underlying_, "FINNIFTY") == 0) {
                ratio = 1;
              } else if (strcmp(t_param_->underlying_, "MIDCPNIFTY") == 0) {
                ratio = 1;
              } else if (strcmp(t_param_->underlying_, "NIFTYNXT50") == 0) {
                ratio = 1;
              }

              // while (t_ctr_ <= t_param_->num_itm_) {
              while (t_cs_ctr_ <= t_param_->num_itm_) {
                t_temp_optname_ess_.str(std::string());
                t_temp_optname_oss_.str(std::string());

                if (0 == (t_ctr_ % ratio)) {
                    t_temp_optname_oss_ << t_optname_base_oss_.str() << "_P"
                                        << GetContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_,
                                                                       t_param_->underlying_)
                                        << "_I" << t_ctr_ / ratio;

                } else {
                    t_temp_optname_oss_ << t_optname_base_oss_.str() << "_P"
                                        << GetContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_,
                                                                       t_param_->underlying_)
                                        << "_I" << (t_ctr_ / ratio) << "_M" << (t_ctr_ % ratio);
                }

                double option_strike_1_dec = start_strike_1_dec - t_param_->step_value_ * (counter_1_dec);
                double option_strike_2_dec = start_strike_2_dec - t_param_->step_value_prev_ * (counter_2_dec);
                double option_strike_dec = -1;
                using_curr_stk_scheme = true;
                t_cs_ctr_++;

                if (fabs(option_strike_1_dec - option_strike_2_dec) <
                    0.0001) {  // $option_strike_1_ == $option_strike_2
                  option_strike_dec = option_strike_1_dec;
                  counter_1_dec += 1;
                  counter_2_dec += 1;
                } else if (option_strike_1_dec < option_strike_2_dec) {
                  t_cs_ctr_--;
                  option_strike_dec = option_strike_2_dec;
                  counter_2_dec += 1;
                  if (!IsOptionStrikePresentInBhavCopy(*t_param_, option_strike_dec)) {
                    continue;
                  }
                  using_curr_stk_scheme = false;
                } else if (option_strike_1_dec > option_strike_2_dec) {
                  option_strike_dec = option_strike_1_dec;
                  counter_1_dec += 1;
                }

                double adjusted_option_strike_dec = option_strike_dec;
                if (is_underlying_corp_action_) {
                  adjusted_option_strike_dec =
                      GetAdjustedValue(t_param_->underlying_, option_strike_dec, t_param_->min_tick_);
                }

                if (!is_underlying_corp_action_ex_date_) {
                  // this is mainly to push the dec strike into otm_call_itm_put or otm_put_itm_call vector
                  if (option_strike_dec > t_atm_strike_) {
                    otm_call_itm_put_strike_price_set_.insert(option_strike_dec);
                  } else if (option_strike_dec < t_atm_strike_) {
                    otm_put_itm_call_strike_price_set_.insert(option_strike_dec);
                  }
                }
                // this is mainly to push the adjusted dec strike into otm_call_itm_put or otm_put_itm_call vector
                if (adjusted_option_strike_dec > t_atm_strike_) {
                  otm_call_itm_put_strike_price_set_.insert(adjusted_option_strike_dec);
                } else if (adjusted_option_strike_dec < t_atm_strike_) {
                  otm_put_itm_call_strike_price_set_.insert(adjusted_option_strike_dec);
                }
              }
            }
            //
            int32_t ratio = 1;

            if (strcmp(t_param_->underlying_, "NIFTY") == 0) {
              ratio = 2;
            } else if (strcmp(t_param_->underlying_, "BANKNIFTY") == 0) {
              ratio = 5;
            } else if (strcmp(t_param_->underlying_, "FINNIFTY") == 0) {
              ratio = 1;
            } else if (strcmp(t_param_->underlying_, "MIDCPNIFTY") == 0) {
              ratio = 1;
            } else if (strcmp(t_param_->underlying_, "NIFTYNXT50") == 0) {
                ratio = 1;
              }


            unsigned int t_ctr_ = 0;
            std::set<double>::iterator otm_call_itm_put_itr_;
            for (otm_call_itm_put_itr_ = otm_call_itm_put_strike_price_set_.begin();
                 otm_call_itm_put_itr_ != otm_call_itm_put_strike_price_set_.end(); ++otm_call_itm_put_itr_) {
              double current_strike_price_ = *otm_call_itm_put_itr_;
              // ITM PUT
              t_temp_optname_ess_.str(std::string());
              t_temp_optname_oss_.str(std::string());

              t_temp_optname_ess_ << t_optname_base_oss_.str() << "_PE_" << t_param_->expiry_ << "_"
                                  << std::setprecision(2) << current_strike_price_;

	      if (ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str()) == "INVALID") continue; // INVALID exchange sym handling
              t_ctr_++;

              if (0 == (t_ctr_ % ratio)) {
                  t_temp_optname_oss_ << t_optname_base_oss_.str() << "_P"
                                      << GetContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_,
                                                                     t_param_->underlying_)
                                      << "_I" << t_ctr_ / ratio;

              } else {
                  t_temp_optname_oss_ << t_optname_base_oss_.str() << "_P"
                                      << GetContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_,
                                                                     t_param_->underlying_)
                                      << "_I" << (t_ctr_ / ratio) << "_M" << (t_ctr_ % ratio);
              }
              t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                  t_param_->min_tick_,
                  GetContractMultiplier(t_param_->inst_type_) * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
                  kExchSourceNSE, t_param_->lot_size_);

              shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                  ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());

              // map this shortcode to options map fetched from bhavcopy file
              t_temp_strike_price_.str(std::string());
              t_temp_strike_price_ << std::fixed << std::setprecision(2) << current_strike_price_;
              AddtoOptionsLastClosePrice(t_param_->inst_type_, t_param_->underlying_, t_param_->expiry_,
                                         current_strike_price_, (char*)"PE", t_temp_optname_oss_.str());
              // loaded in options_lpx

              shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
              // store last close info per shortcode -- approximate.
              shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
              shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                  new NSETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
              shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
              shortcode_2_strike_price_[t_temp_optname_oss_.str()] = current_strike_price_;

              // Only adding current expiry
              if ((!IsWeeklyOptionExpiry(t_param_->expiry_, t_param_->underlying_)) &&
                  (GetContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_, t_param_->underlying_) == 0) &&
                  (0 == (t_ctr_ % ratio))) {
                NSEOptionMetaInfo info = NSEOptionMetaInfo(
                    t_temp_optname_oss_.str(), current_strike_price_, false, using_curr_stk_scheme, t_ctr_, true,
                    ((current_strike_price_ - t_param_->last_close_) / (t_param_->step_value_)));
                HFSAT::VectorUtils::UniqueVectorAdd(underlying_to_options_[t_param_->underlying_], info);
              }

              //-------------------------
              // OTM Call
              t_temp_optname_oss_.str(std::string());
              t_temp_optname_ess_.str(std::string());

              t_temp_optname_ess_ << t_optname_base_oss_.str() << "_CE_" << t_param_->expiry_ << "_"
                                  << std::setprecision(2) << current_strike_price_;

	      if (ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str()) == "INVALID") continue; // INVALID exchange sym handling

              if (0 == (t_ctr_ % ratio)) {
                  t_temp_optname_oss_ << t_optname_base_oss_.str() << "_C"
                                      << GetContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_,
                                                                     t_param_->underlying_)
                                      << "_O" << t_ctr_ / ratio;

              } else {
                  t_temp_optname_oss_ << t_optname_base_oss_.str() << "_C"
                                      << GetContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_,
                                                                     t_param_->underlying_)
                                      << "_O" << (t_ctr_ / ratio) << "_M" << (t_ctr_ % ratio);
              }

              t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                  t_param_->min_tick_,
                  GetContractMultiplier(t_param_->inst_type_) * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
                  kExchSourceNSE, t_param_->lot_size_);

              shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                  ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());

              // map this shortcode to options map fetched from bhavcopy file
              t_temp_strike_price_.str(std::string());
              t_temp_strike_price_ << std::fixed << std::setprecision(2) << current_strike_price_;
              AddtoOptionsLastClosePrice(t_param_->inst_type_, t_param_->underlying_, t_param_->expiry_,
                                         current_strike_price_, (char*)"CE", t_temp_optname_oss_.str());

              // loaded in options_lpx

              shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
              // store last close info per shortcode -- approximate.
              shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
              shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                  new NSETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
              shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
              shortcode_2_strike_price_[t_temp_optname_oss_.str()] = current_strike_price_;

              if ((!IsWeeklyOptionExpiry(t_param_->expiry_, t_param_->underlying_)) &&
                  (GetContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_, t_param_->underlying_) == 0) &&
                  (0 == (t_ctr_ % ratio))) {
                NSEOptionMetaInfo info = NSEOptionMetaInfo(
                    t_temp_optname_oss_.str(), current_strike_price_, true, using_curr_stk_scheme, t_ctr_, false,
                    ((t_param_->last_close_ - current_strike_price_) / (t_param_->step_value_)));
                HFSAT::VectorUtils::UniqueVectorAdd(underlying_to_options_[t_param_->underlying_], info);
              }
            }  // end of for loop

            std::set<double, std::greater<double>>::iterator otm_put_itm_call_itr_;
            t_ctr_ = 0;
            for (otm_put_itm_call_itr_ = otm_put_itm_call_strike_price_set_.begin();
                 otm_put_itm_call_itr_ != otm_put_itm_call_strike_price_set_.end(); ++otm_put_itm_call_itr_) {
              double current_strike_price_ = *otm_put_itm_call_itr_;

              // OTM Put
              t_temp_optname_oss_.str(std::string());
              t_temp_optname_ess_.str(std::string());

              t_temp_optname_ess_ << t_optname_base_oss_.str() << "_PE_" << t_param_->expiry_ << "_"
                                  << std::setprecision(2) << current_strike_price_;

	      if (ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str()) == "INVALID") continue; // INVALID exchange sym handling
              t_ctr_++;

              if (0 == (t_ctr_ % ratio)) {
                  t_temp_optname_oss_ << t_optname_base_oss_.str() << "_P"
                                      << GetContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_,
                                                                     t_param_->underlying_)
                                      << "_O" << t_ctr_ / ratio;

              } else {
                  t_temp_optname_oss_ << t_optname_base_oss_.str() << "_P"
                                      << GetContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_,
                                                                     t_param_->underlying_)
                                      << "_O" << (t_ctr_ / ratio) << "_M" << (t_ctr_ % ratio);
              }
              t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                  t_param_->min_tick_,
                  GetContractMultiplier(t_param_->inst_type_) * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
                  kExchSourceNSE, t_param_->lot_size_);

              shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                  ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());

              // map this shortcode to options map fetched from bhavcopy file
              t_temp_strike_price_.str(std::string());
              t_temp_strike_price_ << std::fixed << std::setprecision(2) << current_strike_price_;
              AddtoOptionsLastClosePrice(t_param_->inst_type_, t_param_->underlying_, t_param_->expiry_,
                                         current_strike_price_, (char*)"PE", t_temp_optname_oss_.str());
              // loaded in options_lpx

              shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
              // store last close info per shortcode -- approximate.
              shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
              shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                  new NSETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
              shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
              shortcode_2_strike_price_[t_temp_optname_oss_.str()] = current_strike_price_;

              // Only adding current expiry
              if ((!IsWeeklyOptionExpiry(t_param_->expiry_, t_param_->underlying_)) &&
                  (GetContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_, t_param_->underlying_) == 0) &&
                  (0 == (t_ctr_ % ratio))) {
                NSEOptionMetaInfo info = NSEOptionMetaInfo(
                    t_temp_optname_oss_.str(), current_strike_price_, false, using_curr_stk_scheme, t_ctr_, false,
                    ((current_strike_price_ - t_param_->last_close_) / (t_param_->step_value_)));
                HFSAT::VectorUtils::UniqueVectorAdd(underlying_to_options_[t_param_->underlying_], info);
              }
              //---------------------------

              // ITM Call
              t_temp_optname_oss_.str(std::string());
              t_temp_optname_ess_.str(std::string());

              t_temp_optname_ess_ << t_optname_base_oss_.str() << "_CE_" << t_param_->expiry_ << "_"
                                  << std::setprecision(2) << current_strike_price_;

	      if (ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str()) == "INVALID") continue; // INVALID exchange sym handling

              if (0 == (t_ctr_ % ratio)) {
                  t_temp_optname_oss_ << t_optname_base_oss_.str() << "_C"
                                      << GetContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_,
                                                                     t_param_->underlying_)
                                      << "_I" << t_ctr_ / ratio;

              } else {
                  t_temp_optname_oss_ << t_optname_base_oss_.str() << "_C"
                                      << GetContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_,
                                                                     t_param_->underlying_)
                                      << "_I" << (t_ctr_ / ratio) << "_M" << (t_ctr_ % ratio);
              }

              t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                  t_param_->min_tick_,
                  GetContractMultiplier(t_param_->inst_type_) * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
                  kExchSourceNSE, t_param_->lot_size_);

              shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                  ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());

              // map this shortcode to options map fetched from bhavcopy file
              t_temp_strike_price_.str(std::string());
              t_temp_strike_price_ << std::fixed << std::setprecision(2) << current_strike_price_;
              AddtoOptionsLastClosePrice(t_param_->inst_type_, t_param_->underlying_, t_param_->expiry_,
                                         current_strike_price_, (char*)"CE", t_temp_optname_oss_.str());
              // loaded in options_lpx

              shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
              // store last close info per shortcode -- approximate.
              shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
              shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                  new NSETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
              shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
              shortcode_2_strike_price_[t_temp_optname_oss_.str()] = current_strike_price_;

              // Only adding current expiry
              if ((!IsWeeklyOptionExpiry(t_param_->expiry_, t_param_->underlying_)) &&
                  (GetContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_, t_param_->underlying_) == 0) &&
                  (0 == (t_ctr_ % ratio))) {
                NSEOptionMetaInfo info = NSEOptionMetaInfo(
                    t_temp_optname_oss_.str(), current_strike_price_, true, using_curr_stk_scheme, t_ctr_, true,
                    ((t_param_->last_close_ - current_strike_price_) / (t_param_->step_value_)));
                HFSAT::VectorUtils::UniqueVectorAdd(underlying_to_options_[t_param_->underlying_], info);
              }
            }  // end of for loop
          }
        }
      }
    }
  }

  for (auto i : shortcode_2_exchsymbol_) {
    if(i.first.find("_W") != std::string::npos){
      if (exchsymbol_2_shortcode_.find(i.second) == exchsymbol_2_shortcode_.end())
        exchsymbol_2_shortcode_[i.second] = i.first;
      else
        exchsymbol_2_shortcode_Weekly[i.second] = i.first;
    } else{
      if (exchsymbol_2_shortcode_.find(i.second) != exchsymbol_2_shortcode_.end()){
        exchsymbol_2_shortcode_Weekly[i.second] = exchsymbol_2_shortcode_[i.second];
      }
      exchsymbol_2_shortcode_[i.second] = i.first;
    }
  }

  std::map<std::string, int> temp_id_map_;
  for (auto datasource_sym_ : non_zero_OI_) {
    std::string exch_sym_ = ConvertDataSourceNametoExchSymbol(datasource_sym_);

    if (exchsymbol_2_shortcode_.find(exch_sym_) == exchsymbol_2_shortcode_.end()) {
      std::vector<std::string> tokens_;
      HFSAT::PerishableStringTokenizer::StringSplit(datasource_sym_, '_', tokens_);
      std::string ticker_ = tokens_[1];
      char underlying_[50];
      strcpy(underlying_, ticker_.c_str());
      std::string opt_type_ = tokens_[2];
      int expiry_ = atoi(tokens_[4].c_str());

      int lotsize_;
      if (temp_lotsize_map_.find(ticker_) != temp_lotsize_map_.end()) {
        lotsize_ = temp_lotsize_map_[ticker_];
      } else {
        continue;
      }

      int contract_num_ = GetContractNumberFromExpiry(expiry_, HFSAT::NSEInstrumentType_t::NSE_STKOPT, underlying_);
      if (contract_num_ < 0) {
        continue;
      }

      if (temp_id_map_.find(ticker_) == temp_id_map_.end()) {
        temp_id_map_[ticker_] = 0;
      } else {
        temp_id_map_[ticker_] += 1;
      }

      std::ostringstream shc_;
      shc_ << "NSE_" << ticker_ << "_" << opt_type_.substr(0, 1) << contract_num_ << "_"
           << "S" << temp_id_map_[ticker_];

      if (shortcode_2_exchsymbol_all_.find(shc_.str()) == shortcode_2_exchsymbol_all_.end()) {
        shortcode_2_exchsymbol_all_[shc_.str()] = exch_sym_;
      }

      if (ticker_ == "NIFTY" || ticker_ == "BANKNIFTY" || ticker_ == "FINNIFTY" || ticker_ == "MIDCPNIFTY"|| ticker_ == "NIFTYNXT50") {
        t_contract_spec_map_[shc_.str()] =
            ContractSpecification(0.05,
                                  GetContractMultiplier(HFSAT::NSEInstrumentType_t::NSE_IDXOPT) *
                                      CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
                                  kExchSourceNSE, lotsize_);
        generic_shortcode_2_type_[shc_.str()] = NSEInstrumentType_t::NSE_IDXOPT;
      } else {
        t_contract_spec_map_[shc_.str()] =
            ContractSpecification(0.05,
                                  GetContractMultiplier(HFSAT::NSEInstrumentType_t::NSE_STKOPT) *
                                      CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
                                  kExchSourceNSE, lotsize_);
        generic_shortcode_2_type_[shc_.str()] = NSEInstrumentType_t::NSE_STKOPT;
      }
      shortcode_2_price_limit_[shc_.str()] = new NSETradingLimit_t(-1, 100000000, -1, 0.05);
      shortcode_2_expiry_date_[shc_.str()] = expiry_;
    }
  }

  AddEquityContractSpecifications(t_contract_spec_map_);
  AddSpotIndexContractSpecifications(t_contract_spec_map_);
  PopulateOrderPriceRangeMapFut(intdate_);
  PopulateOrderPriceRangeMapOpt(intdate_);
}

bool endsWith(const std::string &mainStr, const std::string &toMatch) {
    if (mainStr.size() >= toMatch.size() &&
        mainStr.compare(mainStr.size() - toMatch.size(), toMatch.size(), toMatch) == 0)
      return true;
    else
      return false;
  }

void NSESecurityDefinitions::AddSpotIndexContractSpecifications(
    ShortcodeContractSpecificationMap& t_contract_spec_map_) {
  // std::cout << "NSESecurityDefinitions::AddSpotIndexContractSpecifications" << std::endl;
  std::unordered_map<std::string, int> spot_index_2_token_map_ =
      HFSAT::SpotTokenGenerator::GetUniqueInstance().GetSpotIndexToTokenMap();

  std::string spot_index_shc_;
  int token_;
  for (auto i = spot_index_2_token_map_.begin(); i != spot_index_2_token_map_.end(); i++) {
    spot_index_shc_ = i->first;
    std::transform(spot_index_shc_.begin(), spot_index_shc_.end(), spot_index_shc_.begin(), ::toupper);
    // std::cout << i->first << " : " << i->second << " => " << spot_index_shc_ << " " << spot_index_shc_ << std::endl;
    token_ = i->second;

    if (t_contract_spec_map_.find(spot_index_shc_) == t_contract_spec_map_.end()) {
      t_contract_spec_map_[spot_index_shc_] = ContractSpecification(
          0.05, GetContractMultiplier(HFSAT::NSE_EQ) * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
          kExchSourceNSE, 1);

      std::ostringstream t_exch_sym_oss_;
      t_exch_sym_oss_ << "NSE_IDX" << token_;

      char exchange_symbol[16];
      memset(exchange_symbol, '\0', sizeof(char) * 16);
      std::memcpy(exchange_symbol, t_exch_sym_oss_.str().c_str(),
                  std::min<unsigned int>(15, strlen(t_exch_sym_oss_.str().c_str())));

      // std::cout << "Add String: " << t_exch_sym_oss_.str() << " : " << exchange_symbol << std::endl;
      shortcode_2_exchsymbol_[spot_index_shc_] = exchange_symbol;
      exchsymbol_2_datasourcename_[exchange_symbol] = spot_index_shc_;
      datasourcename_2_exchsymbol_[spot_index_shc_] = exchange_symbol;
      exchsymbol_2_shortcode_[exchange_symbol] = spot_index_shc_;
      shortcode_2_expiry_date_[spot_index_shc_] = intdate_;
      shortcode_2_price_limit_[spot_index_shc_] = new NSETradingLimit_t(-1, 100000000, -1, 0.05);

      shortcode_2_type_[spot_index_shc_] = HFSAT::NSE_IDXSPT;
      // store last close info per shortcode -- approximate.
      shortcode_2_last_close_[spot_index_shc_] = -1;
    }
  }
}

void NSESecurityDefinitions::AddEquityContractSpecifications(ShortcodeContractSpecificationMap& t_contract_spec_map_) {
  std::ostringstream nse_eq_file_name_str;
  nse_eq_file_name_str << NSE_REF_DATA_FILE_LOCATION << "nse_eq_" << intdate_ << "_contracts.txt";

  std::ifstream nse_ref_file;

  nse_ref_file.open(nse_eq_file_name_str.str().c_str(), std::ifstream::in);

  if (!(nse_ref_file.is_open())) {
    std::cerr << "Cannot open file " << nse_eq_file_name_str.str() << "\n";
    return;
  }

  char line[1024];
  while (!nse_ref_file.eof()) {
    bzero(line, 1024);
    nse_ref_file.getline(line, 1024);
    if (strlen(line) == 0 || strstr(line, "#") != NULL) continue;
    HFSAT::PerishableStringTokenizer st_(line, 1024);
    const std::vector<const char*>& tokens_ = st_.GetTokens();
    if (tokens_.size() < 4) {
      std::cerr << "Ignoring Malformatted line in file " << nse_eq_file_name_str.str() << "\n";
      continue;
    }

    std::string symbol(tokens_[3]);
    std::string shortcode = std::string("NSE_") + symbol;

    if (t_contract_spec_map_.find(shortcode) == t_contract_spec_map_.end()) {
      t_contract_spec_map_[shortcode] = ContractSpecification(
          shortcode_2_price_limit_[shortcode]->min_tick_size_, GetContractMultiplier(HFSAT::NSE_EQ) * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
          kExchSourceNSE, 1);

      char exchange_symbol[16];
      memset(exchange_symbol, '\0', sizeof(char) * 16);
      std::memcpy(exchange_symbol, shortcode.c_str(), std::min<unsigned int>(15, strlen(shortcode.c_str())));

      // these maps are irrelavant for cash markets. We populate it to prevent common accessor functions from
      // failing
      shortcode_2_exchsymbol_[shortcode] = exchange_symbol;
      exchsymbol_2_datasourcename_[shortcode] = shortcode;
      datasourcename_2_exchsymbol_[shortcode] = exchange_symbol;
      shortcode_2_expiry_date_[shortcode] = intdate_;

      shortcode_2_type_[shortcode] = HFSAT::NSE_EQ;
      // store last close info per shortcode -- approximate.
      shortcode_2_last_close_[shortcode] = -1;
    } else {
      t_contract_spec_map_[shortcode].min_price_increment_ = shortcode_2_price_limit_[shortcode]->min_tick_size_;
    }
  }
  nse_ref_file.close();
}
// file format NSE0 NIFTY_CE_20140730_8050
// file format NSE1 NIFTY_FUT_20140730
void NSESecurityDefinitions::LoadDataSourceExchangeSymbolMap() {
  std::string nse_datasource_exchsymbol_filepath_ =
      std::string("/spare/local/tradeinfo/") + NSE_DATASOURCE_EXCHSYMBOL_FILE;
  if (FileUtils::ExistsAndReadable(nse_datasource_exchsymbol_filepath_)) {
    std::ifstream nse_data_exch_file_;
    nse_data_exch_file_.open(nse_datasource_exchsymbol_filepath_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    std::ostringstream t_oss_;
    t_oss_ << intdate_;
    boost::gregorian::date d_today_(boost::gregorian::from_undelimited_string(t_oss_.str()));
    if (nse_data_exch_file_.is_open()) {
      while (nse_data_exch_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        nse_data_exch_file_.getline(readline_buffer_, sizeof(readline_buffer_));
        PerishableStringTokenizer st_(readline_buffer_, sizeof(readline_buffer_));

        const std::vector<const char*>& tokens_ = st_.GetTokens();
        
        // Ensure we have exactly 2 tokens and the first token is not a comment
        if (tokens_.size() == 2 && tokens_[0][0] != '#') {
          std::string datasource_name = tokens_[1];  // Convert const char* to std::string
      
          if ( intdate_ < 20241211 ) {
            // Handle GMRINFRA Replacement
            size_t pos = datasource_name.find("NSE_GMRAIRPORT");
            if (pos != std::string::npos) {
                datasource_name.replace(pos, std::string("NSE_GMRAIRPORT").length(), "NSE_GMRINFRA");
            }
          }
      
          // Convert `std::string` to a modifiable `char*` for the tokenizer
          std::vector<char> datasource_name_cstr(datasource_name.begin(), datasource_name.end());
          datasource_name_cstr.push_back('\0');  // Ensure null-termination
      
          std::vector<char*> tokens1_;
          PerishableStringTokenizer::NonConstStringTokenizer(datasource_name_cstr.data(), "_", tokens1_);
      
          if (tokens1_.size() >= 4) {
            try {
              boost::gregorian::date d_expiry(boost::gregorian::from_undelimited_string(tokens1_[3]));
  
              // Check expiry conditions
              if (d_expiry >= d_today_ && d_expiry - boost::gregorian::months(4) <= d_today_) {
                if (intdate_ >= USING_TBT_FROM &&
                    (strncmp(tokens1_[2], "CE", 2) == 0 || strncmp(tokens1_[2], "PE", 2) == 0)) {

                    // Ensure tokens1_[4] exists before using it
                    std::string extra_token = (tokens1_.size() > 4) ? ("_" + std::string(tokens1_[4])) : "";

                    std::string data_source_name = std::string(tokens1_[0]) + "_" + std::string(tokens1_[1]) + "_" +
                                                   std::string(tokens1_[2]) + extra_token + "_" + std::string(tokens1_[3]);

                    std::string data_source_sym = std::string(tokens1_[0]) + "_" + std::string(tokens1_[1]) + "_" +
                                                  std::string(tokens1_[2]) + "_" + std::string(tokens1_[3]) + extra_token;

                    exchsymbol_2_datasourcesym_[tokens_[0]] = data_source_sym;
                    datasourcename_2_exchsymbol_[data_source_name] = tokens_[0];
                    exchsymbol_2_datasourcename_[tokens_[0]] = data_source_name;
                } else {
                    datasourcename_2_exchsymbol_[datasource_name] = tokens_[0];
                    exchsymbol_2_datasourcename_[tokens_[0]] = datasource_name;
                }
              }
            } catch (const std::exception& e) {
                std::cerr << "Invalid date format in datasource: " << datasource_name << " (" << e.what() << ")" << std::endl;
            }
          } else {
              datasourcename_2_exchsymbol_[datasource_name] = tokens_[0];
              exchsymbol_2_datasourcename_[tokens_[0]] = datasource_name;
          }
        }
      }
    }
    nse_data_exch_file_.close();
  } else {
    std::cerr << "Fatal error - could not read NSE DataSource ExchSymbol file " << nse_datasource_exchsymbol_filepath_
              << ".Exiting.\n";
    exit(0);
  }
}

int NSESecurityDefinitions::GetExpiryFromShortCode(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  if (shortcode_2_expiry_date_.end() == shortcode_2_expiry_date_.find(local_shc))
    return -1;
  else
    return shortcode_2_expiry_date_[local_shc];
}

double NSESecurityDefinitions::GetStrikePriceFromShortCode(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  if (IsOption(local_shc))
    return shortcode_2_strike_price_[local_shc];
  else
    return 0;
}

double NSESecurityDefinitions::GetStrikePriceFromShortCodeGeneric(const std::string& _shortcode_) {
  double hft_strike_ = GetStrikePriceFromShortCode(_shortcode_);
  if (hft_strike_ == 0) {
    // Could be MFT shortcode, hence we now need to get its datasource symbol
    if (shortcode_2_exchsymbol_all_.find(_shortcode_) != shortcode_2_exchsymbol_all_.end()) {
      std::string exch_sym_ = shortcode_2_exchsymbol_all_[_shortcode_];
      std::string datasource_sym_ = exchsymbol_2_datasourcename_[exch_sym_];
      std::vector<std::string> tokens_;
      HFSAT::PerishableStringTokenizer::StringSplit(datasource_sym_, '_', tokens_);
      return atof(tokens_[3].c_str());
    }
    // Unable to get strike for this shortcode
    else {
      return 0;
    }
  } else {
    return hft_strike_;
  }
}

// TODO::ADD SUPPORT FOR WEEKLY OPTIONS(DONE BUT REVISIT)
// DONE::SUPPORT ADDED IN THE FORM OF NEW FUNCTION GetWeeklyShortcodeFromCanonical
// Function to get shortcode given the canonical form..
// This function would be quite latency sensitive and mainly for the use of Mid-Term
std::string NSESecurityDefinitions::GetShortcodeFromCanonical(const std::string& ticker_, int expiry_dt_,
                                                              double strike_, bool is_call_) {
  std::ostringstream datasource_sym_;
  std::ostringstream temp_strike_;
  temp_strike_ << std::fixed << std::setprecision(2) << strike_;
  std::string temp_type_;
  if (is_call_) {
    temp_type_ = "CE";
  } else {
    temp_type_ = "PE";
  }

  datasource_sym_ << "NSE_" << ticker_ << "_" << temp_type_ << "_" << temp_strike_.str() << "_" << expiry_dt_;

  std::string exch_sym_ = "";
  if (datasourcename_2_exchsymbol_.find(datasource_sym_.str()) != datasourcename_2_exchsymbol_.end()) {
    exch_sym_ = datasourcename_2_exchsymbol_[datasource_sym_.str()];
  } else {
    return "INVALID";
  }

  if (exchsymbol_2_shortcode_.find(exch_sym_) != exchsymbol_2_shortcode_.end()) {
    return exchsymbol_2_shortcode_[exch_sym_];
  }

  return "INVALID";
}

std::string NSESecurityDefinitions::GetWeeklyShortcodeFromCanonical(const std::string& ticker_, int expiry_dt_,
                                                              double strike_, bool is_call_) {
  std::ostringstream datasource_sym_;
  std::ostringstream temp_strike_;
  temp_strike_ << std::fixed << std::setprecision(2) << strike_;
  std::string temp_type_;
  if (is_call_) {
    temp_type_ = "CE";
  } else {
    temp_type_ = "PE";
  }

  datasource_sym_ << "NSE_" << ticker_ << "_" << temp_type_ << "_" << temp_strike_.str() << "_" << expiry_dt_;

  // std::cout << datasource_sym_.str() << "\n";
  std::string exch_sym_ = "";
  if (datasourcename_2_exchsymbol_.find(datasource_sym_.str()) != datasourcename_2_exchsymbol_.end()) {
    exch_sym_ = datasourcename_2_exchsymbol_[datasource_sym_.str()];
  } else {
    return "INVALID";
  }

  if (exchsymbol_2_shortcode_Weekly.find(exch_sym_) != exchsymbol_2_shortcode_Weekly.end()) {
    return exchsymbol_2_shortcode_Weekly[exch_sym_];
  }

  return "INVALID";
}

// simply ( FutPrice - Strike  )
double NSESecurityDefinitions::GetMoneynessFromShortCode(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::string futures_ = GetFutureShortcodeFromOptionShortCode(_shortcode_);
  if (IsOption(local_shc))
    return (shortcode_2_strike_price_[local_shc] - GetLastClose(futures_));
  else
    return 0;
}

double NSESecurityDefinitions::GetStepValueFromShortCode(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  if (IsFuture(local_shc))
    return shortcode_2_step_value_[local_shc];
  else
    return 0;
}

double NSESecurityDefinitions::GetClosePriceFromShortCode(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  if (IsShortcode(local_shc))
    return shortcode_2_last_close_[local_shc];
  else
    return 0;
}

// Only Changes if Req will be in Throttle and OI BarData As shortcode is Used
std::string NSESecurityDefinitions::GetShortCodeFromExchangeSymbol(const std::string exch_symbol) {
  // std::cout << "NSESecurityDefinitions::GetShortCodeFromExchangeSymbol: exch = "
  //	    << exch_symbol << std::endl;
  std::map<std::string, std::string>::iterator tmp_map_itr = exchsymbol_2_shortcode_.find(exch_symbol);
  if (tmp_map_itr != exchsymbol_2_shortcode_.end()) {
    return tmp_map_itr->second;
  } else {
    return "INVALID";
  }
}

// GetWeekly Shortcode For monthly Expires 
std::string NSESecurityDefinitions::GetWeeklyShortCodeFromSymbol(std::string& exch_symbol){
  std::map<std::string, std::string>::iterator tmp_map_itr = exchsymbol_2_shortcode_Weekly.find(exch_symbol);
  if (tmp_map_itr != exchsymbol_2_shortcode_Weekly.end()) {
    return tmp_map_itr->second;
  }
  tmp_map_itr = exchsymbol_2_shortcode_.find(exch_symbol);
  if (tmp_map_itr != exchsymbol_2_shortcode_.end()) {
    return tmp_map_itr->second;
  } else {
    return "INVALID";
  }
}

// GetWeekly Shortcode For monthly Expires From ShortCode
std::string NSESecurityDefinitions::GetWeeklyShortCodeFromMonthlyShortcode(std::string& _shortcode_){
  std::string exch_sym = GetExchSymbolNSE(_shortcode_);
  return GetWeeklyShortCodeFromSymbol(exch_sym);
}
std::string NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(const std::string& _shortcode_) {
  // validity of shc is checked elsewhere along with rest all, hence simply depending of structure of shc for now
  std::vector<std::string> tokens1_;
  split(_shortcode_, '_', tokens1_);
  if (tokens1_.size() == 4) {
    std::string future_shortcode_ = tokens1_[0] + "_" + tokens1_[1] + "_FUT" + tokens1_[2].substr(1);
    return future_shortcode_;
  } else {
    return "INVALID_SHORTCODE";
  }
}

bool NSESecurityDefinitions::IsOption(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  if (shortcode_2_type_.end() == shortcode_2_type_.find(local_shc)) return false;
  NSEInstrumentType_t inst_type = shortcode_2_type_[local_shc];
  return ((inst_type == NSE_IDXOPT) || (inst_type == NSE_STKOPT) || (inst_type == NSE_CUROPT));
}

bool NSESecurityDefinitions::IsHiddenOrderAvailable(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  if (shortcode_2_type_.end() == shortcode_2_type_.find(local_shc)) return false;
  NSEInstrumentType_t inst_type = shortcode_2_type_[local_shc];
  return ((inst_type == NSE_CUROPT) || (inst_type == NSE_CURFUT) || (inst_type == NSE_EQ));
}

bool NSESecurityDefinitions::IsFuture(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  if (shortcode_2_type_.end() == shortcode_2_type_.find(local_shc)) return false;
  NSEInstrumentType_t inst_type = shortcode_2_type_[local_shc];
  if ((inst_type == NSE_IDXFUT) || (inst_type == NSE_STKFUT) || (inst_type == NSE_CURFUT))
    return true;
  else
    return false;
}

bool NSESecurityDefinitions::IsEquity(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  if (shortcode_2_type_.end() == shortcode_2_type_.find(local_shc)) return false;
  NSEInstrumentType_t inst_type = shortcode_2_type_[local_shc];
  if (inst_type == NSE_EQ)
    return true;
  else
    return false;
}

bool NSESecurityDefinitions::IsSpotIndex(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  if (shortcode_2_type_.end() == shortcode_2_type_.find(local_shc)) return false;
  NSEInstrumentType_t inst_type = shortcode_2_type_[local_shc];
  if (inst_type == NSE_IDXSPT)
    return true;
  else
    return false;
}

bool NSESecurityDefinitions::IsWeeklyOption(const std::string& _shortcode_) {
  char underlying_[50];
  strcpy(underlying_, _shortcode_.c_str());
  if (IsOption(_shortcode_)) {
    int expiry = GetExpiryFromShortCode(_shortcode_);
    if (expiry == -1)
      return false;
    else if (IsWeeklyOptionExpiry(expiry, underlying_))
      return true;
  }
  return false;
}

bool NSESecurityDefinitions::IsCurrency(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  if (shortcode_2_type_.end() == shortcode_2_type_.find(local_shc)) return false;
  NSEInstrumentType_t inst_type = shortcode_2_type_[local_shc];
  if ((inst_type == NSE_CUROPT) || (inst_type == NSE_CURFUT))
    return true;
  else
    return false;
}

int NSESecurityDefinitions::GetOptionType(const std::string& _shortcode_) {
  if (IsOption(_shortcode_)) {
    std::string future_shortcode_;
    std::vector<std::string> tokens1_;
    split(_shortcode_, '_', tokens1_);
    if (tokens1_[2][0] == 'P')  // In Greek's Caculation Exec, put is enumerated as -1 while call as +1
      return -1;
    else
      return 1;
  } else
    return '0';
}

bool NSESecurityDefinitions::IsShortcode(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  return (shortcode_2_exchsymbol_.find(local_shc) != shortcode_2_exchsymbol_.end()) ||
         (shortcode_2_exchsymbol_all_.find(local_shc) != shortcode_2_exchsymbol_all_.end());
}

double NSESecurityDefinitions::GetInterestRate(const int t_date_) {
  double interest_rate_ = 0.0675;  // Default interest rate value

  std::string nse_interest_rate_file_absolute_path_ = std::string("/spare/local/tradeinfo/") + NSE_INTEREST_RATE_FILE;
  if (FileUtils::ExistsAndReadable(nse_interest_rate_file_absolute_path_)) {
    std::ifstream nse_interest_rate_file_;
    nse_interest_rate_file_.open(nse_interest_rate_file_absolute_path_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (nse_interest_rate_file_.is_open()) {
      while (nse_interest_rate_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        nse_interest_rate_file_.getline(readline_buffer_, sizeof(readline_buffer_));
        PerishableStringTokenizer st_(readline_buffer_, sizeof(readline_buffer_));

        const std::vector<const char*>& tokens_ = st_.GetTokens();
        if (tokens_.size() == 2) {
          int date_ = atoi(tokens_[0]);
          if (date_ <= t_date_) {
            interest_rate_ = atof(tokens_[1]);
            continue;
          } else
            break;
        }
      }
    }
    nse_interest_rate_file_.close();
  }
  return interest_rate_;
}

std::string NSESecurityDefinitions::GetShortCodeFromStrikePrice(double strike_price_,
                                                                const std::string& _fut_shortcode_, int option_type_) {
  double step_value_ = GetStepValueFromShortCode(_fut_shortcode_);
  double close_price_ = GetClosePriceFromShortCode(_fut_shortcode_);

  if ((step_value_ == 0) || (close_price_ == 0)) return "";

  double atm_strike_ = std::round(close_price_ / step_value_) * step_value_;
  int num_increment_ = (strike_price_ - atm_strike_) / step_value_;

  std::ostringstream t_optname_base_oss_;
  std::string local_shc = _fut_shortcode_;

  std::vector<std::string> tokens1_;
  split(_fut_shortcode_, '_', tokens1_);

  std::string num_increment_string_ = std::to_string(std::abs(num_increment_));

  std::string future_shortcode_;

  if (option_type_ == 1) {
    if (num_increment_ < 0)
      future_shortcode_ =
          tokens1_[0] + "_" + tokens1_[1] + "_C" + tokens1_[2].substr(3, 4) + "_I" + num_increment_string_;
    else if (num_increment_ > 0)
      future_shortcode_ =
          tokens1_[0] + "_" + tokens1_[1] + "_C" + tokens1_[2].substr(3, 4) + "_O" + num_increment_string_;
    else
      future_shortcode_ = tokens1_[0] + "_" + tokens1_[1] + "_C" + tokens1_[2].substr(3, 4) + "_A";
  } else {
    if (num_increment_ < 0)
      future_shortcode_ =
          tokens1_[0] + "_" + tokens1_[1] + "_P" + tokens1_[2].substr(3, 4) + "_O" + num_increment_string_;
    else if (num_increment_ > 0)
      future_shortcode_ =
          tokens1_[0] + "_" + tokens1_[1] + "_P" + tokens1_[2].substr(3, 4) + "_I" + num_increment_string_;
    else
      future_shortcode_ = tokens1_[0] + "_" + tokens1_[1] + "_P" + tokens1_[2].substr(3, 4) + "_A";
  }

  return future_shortcode_;
}

std::vector<std::string> NSESecurityDefinitions::GetShortCodeListForStepValue(double step_value_, const std::string& _underlying_, int num_strikes_, bool is_weekly_) {
  std::vector<std::string> shc_list_;
  std::string shc_ =  "NSE_" + _underlying_ + "_C0_A";
  if(is_weekly_) {
    shc_ = shc_ + "_W";
  }
  int expiry_ = GetExpiryFromShortCode(shc_);
  int close_price_ = GetClosePriceFromShortCode("NSE_" + _underlying_ + "_FUT0");
  int atm_stk_ = std::round(close_price_ / step_value_) * step_value_;

  
  std::string shc_ce_ = GetShortcodeFromCanonical(_underlying_, expiry_, atm_stk_, 1);
  if((is_weekly_) && (shc_ce_.find("_W") == std::string::npos)) {
    shc_ce_ = shc_ce_ + "_W";
  }
  
  std::string shc_pe_ = GetShortcodeFromCanonical(_underlying_, expiry_, atm_stk_, 0);
  if((is_weekly_) && (shc_pe_.find("_W") == std::string::npos)) {
    shc_pe_ = shc_pe_ + "_W";
  }


  VectorUtils::UniqueVectorAdd(shc_list_, shc_ce_);
  VectorUtils::UniqueVectorAdd(shc_list_, shc_pe_);

  for (int i=1 ; i < num_strikes_; i++) {
    int stk1_ = atm_stk_ + step_value_*i;
    int stk2_ = atm_stk_ - step_value_*i;

    shc_ce_ = GetShortcodeFromCanonical(_underlying_, expiry_, stk1_, 1);
    if((is_weekly_) && (shc_ce_.find("_W") == std::string::npos)) {
      shc_ce_ = shc_ce_ + "_W";
    }
    shc_pe_ = GetShortcodeFromCanonical(_underlying_, expiry_, stk1_, 0);
    if((is_weekly_) && (shc_pe_.find("_W") == std::string::npos)) {
      shc_pe_ = shc_pe_ + "_W";
    }
    VectorUtils::UniqueVectorAdd(shc_list_, shc_ce_);
    VectorUtils::UniqueVectorAdd(shc_list_, shc_pe_);

    shc_ce_ = GetShortcodeFromCanonical(_underlying_, expiry_, stk2_, 1);
    if((is_weekly_) && (shc_ce_.find("_W") == std::string::npos)) {
      shc_ce_ = shc_ce_ + "_W";
    }
    shc_pe_ = GetShortcodeFromCanonical(_underlying_, expiry_, stk2_, 0);
    if((is_weekly_) && (shc_pe_.find("_W") == std::string::npos)) {
      shc_pe_ = shc_pe_ + "_W";
    }
    VectorUtils::UniqueVectorAdd(shc_list_, shc_ce_);
    VectorUtils::UniqueVectorAdd(shc_list_, shc_pe_);
  }
  
  return shc_list_;
}

std::string NSESecurityDefinitions::GetOppositeContractShc(const std::string& _shortcode_) {
  std::stringstream ss(_shortcode_);
  std::string item;
  std::vector<std::string> tokens_;

  while (std::getline(ss, item, '_')) {
    tokens_.push_back(item);
  }

  if (tokens_[2][0] == 'C')
    tokens_[2][0] = 'P';
  else
    tokens_[2][0] = 'C';

  if (tokens_[3][0] == 'I')
    tokens_[3][0] = 'O';
  else if (tokens_[3][0] == 'O')
    tokens_[3][0] = 'I';

  return tokens_[0] + "_" + tokens_[1] + "_" + tokens_[2] + "_" + tokens_[3];
}

std::vector<std::string> NSESecurityDefinitions::LoadOptionsList(const std::string& r_dep_shortcode_,
                                                                 int max_contract_number_) {
  std::stringstream ss(r_dep_shortcode_);
  std::string item;
  std::vector<std::string> tokens_;
  std::vector<std::string> slist_;

  while (std::getline(ss, item, '_')) {
    tokens_.push_back(item);
  }

  std::string t_shc_CA = tokens_[0] + "_" + tokens_[1] + "_C0" + "_A";
  std::string t_shc_PA = tokens_[0] + "_" + tokens_[1] + "_P0" + "_A";
  slist_.push_back(t_shc_CA);
  slist_.push_back(t_shc_PA);

  for (int i = 1; i <= max_contract_number_; i++) {
    std::string t_shc_CI = tokens_[0] + "_" + tokens_[1] + "_C0_I" + std::to_string(i);
    std::string t_shc_CO = tokens_[0] + "_" + tokens_[1] + "_C0_O" + std::to_string(i);
    std::string t_shc_PI = tokens_[0] + "_" + tokens_[1] + "_P0_I" + std::to_string(i);
    std::string t_shc_PO = tokens_[0] + "_" + tokens_[1] + "_P0_O" + std::to_string(i);
    slist_.push_back(t_shc_CI);
    slist_.push_back(t_shc_CO);
    slist_.push_back(t_shc_PI);
    slist_.push_back(t_shc_PO);
  }
  return slist_;
}

std::vector<std::string> NSESecurityDefinitions::LoadBannedSecurities(const int t_date_) {
  std::string banned_securities_file_absolute_path_ =
      std::string("/spare/local/tradeinfo/") + NSE_BANNED_SECURITIES_PREFIX + std::to_string(t_date_) + ".csv";
  std::vector<std::string> banned_securities_list_;
  if (FileUtils::ExistsAndReadable(banned_securities_file_absolute_path_)) {
    std::ifstream banned_securities_file_;
    banned_securities_file_.open(banned_securities_file_absolute_path_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (banned_securities_file_.is_open()) {
      while (banned_securities_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        banned_securities_file_.getline(readline_buffer_, sizeof(readline_buffer_));
        PerishableStringTokenizer st_(readline_buffer_, sizeof(readline_buffer_));

        const std::vector<const char*>& tokens_ = st_.GetTokens();
        if (tokens_.size() != 1) {
          continue;
        }
        banned_securities_list_.push_back(tokens_[0]);
      }  // end while
    }
    banned_securities_file_.close();
  } else {
    std::cerr << "Banned securities file not present for NSE - " << banned_securities_file_absolute_path_ << '\n';
  }
  return banned_securities_list_;
}

bool NSESecurityDefinitions::IsShortcodeUnderBan(const std::string& shortcode_){
      for(unsigned int itr = 0; itr < banned_prod_list_.size(); itr++){
        // std::cout <<"Check Shorcode Ban" << shortcode_ <<" " << "NSE_" + banned_prod_list_[itr] <<"\n";
        if ( shortcode_ == "NSE_" + banned_prod_list_[itr] ) 
          return true;
      }
    return false;
}

int NSESecurityDefinitions::GetSurveillanceIndicator(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  if (shortcode_2_surveillance_id_.find(local_shc) != shortcode_2_surveillance_id_.end()) {
    return shortcode_2_surveillance_id_[local_shc];
  } else {
    return -1;
  }
}

// This will return any shortcode with non-zero OI for a particular underlying
std::vector<std::string> NSESecurityDefinitions::GetAllOptionShortcodesForUnderlyingGeneric(
    const std::string& underlying) {
  std::vector<std::string> option_list;
  std::string temp_underlying = "_" + underlying + "_";

  for (auto i : shortcode_2_exchsymbol_) {
    if (i.first.find(temp_underlying) != std::string::npos) {
      if (IsOption(i.first)) {
        option_list.push_back(i.first);
      }
    }
  }

  // shortcode_2_exchsymbol_all_ only contains _S* option shortcodes which are not present in HFT maps
  for (auto i : shortcode_2_exchsymbol_all_) {
    if (i.first.find(temp_underlying) != std::string::npos) {
      option_list.push_back(i.first);
    }
  }

  return option_list;
}

std::vector<std::string> NSESecurityDefinitions::GetAllOptionShortcodesForUnderlying(const std::string& underlying,
                                                                                     int moneyness, int call_only,
                                                                                     int number_of_contracts) {
  std::vector<std::string> option_list;
  int contracts_to_add = number_of_contracts;
  if (underlying_to_options_.find(underlying) != underlying_to_options_.end()) {
    std::sort(underlying_to_options_[underlying].begin(), underlying_to_options_[underlying].end());
    for (auto meta_info : underlying_to_options_[underlying]) {
      if (moneyness == 0) {
        if (call_only == 0 || (call_only == 1 && meta_info.is_call) || (call_only == -1 && (!meta_info.is_call))) {
          option_list.push_back(meta_info.shortcode);
          contracts_to_add--;
        }
      } else if (moneyness == 1 && meta_info.is_in_the_money) {
        if (call_only == 0 || (call_only == 1 && meta_info.is_call) || (call_only == -1 && (!meta_info.is_call))) {
          option_list.push_back(meta_info.shortcode);
          contracts_to_add--;
        }
      } else if (moneyness == -1 && (!meta_info.is_in_the_money)) {
        if (call_only == 0 || (call_only == 1 && meta_info.is_call) || (call_only == -1 && (!meta_info.is_call))) {
          option_list.push_back(meta_info.shortcode);
          contracts_to_add--;
        }
      }
      if (contracts_to_add == 0) break;
    }
  }
  return option_list;
}

std::vector<std::string> NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInCurrScheme(
    const std::string& underlying, int moneyness, int call_only, int number_of_contracts) {
  std::vector<std::string> option_list;
  int contracts_to_add = number_of_contracts;
  if (underlying_to_options_.find(underlying) != underlying_to_options_.end()) {
    std::sort(underlying_to_options_[underlying].begin(), underlying_to_options_[underlying].end());
    if (moneyness == 1) {
      std::reverse(underlying_to_options_[underlying].begin(), underlying_to_options_[underlying].end());
    }
    for (auto meta_info : underlying_to_options_[underlying]) {
      if (meta_info.is_curr_stk_scheme) {
        if (moneyness == 0) {
          if (call_only == 0 || (call_only == 1 && meta_info.is_call) || (call_only == -1 && (!meta_info.is_call))) {
            option_list.push_back(meta_info.shortcode);
            contracts_to_add--;
          }
        } else if (moneyness == 1 && meta_info.is_in_the_money) {
          if (call_only == 0 || (call_only == 1 && meta_info.is_call) || (call_only == -1 && (!meta_info.is_call))) {
            option_list.push_back(meta_info.shortcode);
            contracts_to_add--;
          }
        } else if (moneyness == -1 && (!meta_info.is_in_the_money)) {
          if (call_only == 0 || (call_only == 1 && meta_info.is_call) || (call_only == -1 && (!meta_info.is_call))) {
            option_list.push_back(meta_info.shortcode);
            contracts_to_add--;
          }
        }
      }
      if (contracts_to_add == 0) break;
    }
  } else {
    std::cerr << "Error no options found for this underlying " << underlying << ".Exiting.\n";
    exit(-1);
  }
  return option_list;
}

std::map<std::string, double> NSESecurityDefinitions::GetOptionsIntrinsicValue(const std::string& underlying,
                                                                               int moneyness, int call_only,
                                                                               int number_of_contracts) {
  std::map<std::string, double> option_list;
  int contracts_to_add = number_of_contracts;
  if (underlying_to_options_.find(underlying) != underlying_to_options_.end()) {
    std::sort(underlying_to_options_[underlying].begin(), underlying_to_options_[underlying].end());
    for (auto meta_info : underlying_to_options_[underlying]) {
      // if (meta_info.is_curr_stk_scheme) {
      {
        if (moneyness == 0) {
          if (call_only == 0 || (call_only == 1 && meta_info.is_call) || (call_only == -1 && (!meta_info.is_call))) {
            option_list[meta_info.shortcode] = meta_info.unbounded_intrinsic_value_;
            contracts_to_add--;
          }
        } else if (moneyness == 1 && meta_info.is_in_the_money) {
          if (call_only == 0 || (call_only == 1 && meta_info.is_call) || (call_only == -1 && (!meta_info.is_call))) {
            option_list[meta_info.shortcode] = meta_info.unbounded_intrinsic_value_;
            contracts_to_add--;
          }
        } else if (moneyness == -1 && (!meta_info.is_in_the_money)) {
          if (call_only == 0 || (call_only == 1 && meta_info.is_call) || (call_only == -1 && (!meta_info.is_call))) {
            option_list[meta_info.shortcode] = meta_info.unbounded_intrinsic_value_;
            contracts_to_add--;
          }
        }
      }
      if (contracts_to_add == 0) break;
    }
  } else {
    std::cerr << "Error no options found for this underlying " << underlying << ".Exiting.\n";
    exit(-1);
  }
  return option_list;
}

std::string NSESecurityDefinitions::GetOptionShortcodeForUnderlyingInCurrSchemeFromContractNumber(
    const std::string& underlying, int moneyness, int call_only, unsigned int contract_num_) {
  std::vector<std::string> options = GetOptionShortcodesForUnderlyingInCurrScheme(underlying, moneyness, call_only);

  if (options.size() == 0) return NULL;

  if (options[0].find("_A") != std::string::npos) {
    options.erase(options.begin());
  }

  if (options.size() < contract_num_)
    return NULL;
  else
    return options[contract_num_ - 1];
}

std::vector<std::string> NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInPrevScheme(
    const std::string& underlying, int moneyness, int call_only, int number_of_contracts) {
  std::vector<std::string> option_list;
  int contracts_to_add = number_of_contracts;
  if (underlying_to_options_.find(underlying) != underlying_to_options_.end()) {
    std::sort(underlying_to_options_[underlying].begin(), underlying_to_options_[underlying].end());
    for (auto meta_info : underlying_to_options_[underlying]) {
      if (!meta_info.is_curr_stk_scheme) {
        if (moneyness == 0) {
          if (call_only == 0 || (call_only == 1 && meta_info.is_call) || (call_only == -1 && (!meta_info.is_call))) {
            option_list.push_back(meta_info.shortcode);
            contracts_to_add--;
          }
        } else if (moneyness == 1 && meta_info.is_in_the_money) {
          if (call_only == 0 || (call_only == 1 && meta_info.is_call) || (call_only == -1 && (!meta_info.is_call))) {
            option_list.push_back(meta_info.shortcode);
            contracts_to_add--;
          }
        } else if (moneyness == -1 && (!meta_info.is_in_the_money)) {
          if (call_only == 0 || (call_only == 1 && meta_info.is_call) || (call_only == -1 && (!meta_info.is_call))) {
            option_list.push_back(meta_info.shortcode);
            contracts_to_add--;
          }
        }
      }
      if (contracts_to_add == 0) break;
    }
  }
  return option_list;
}

bool NSESecurityDefinitions::IsOptionFromCurrScheme(const std::string& shortcode) {
  std::vector<std::string> tokens;
  split(shortcode, '_', tokens);
  std::string underlying = tokens[1];

  if (underlying_to_options_.find(underlying) != underlying_to_options_.end()) {
    for (auto meta_info : underlying_to_options_[underlying]) {
      if (meta_info.shortcode == shortcode) {
        if (meta_info.is_curr_stk_scheme == true) {
          return true;
        } else {
          return false;
        }
      }
    }
  }
  std::cerr << " IsOptionFromCurrScheme called with improper shortcode " << shortcode << '\n';
  return false;
}

// simply return i-1 contract from vector
std::string NSESecurityDefinitions::GetPrevOptionInCurrentSchema(const std::string& option) {
  std::vector<std::string> tokens_;
  PerishableStringTokenizer::StringSplit(option, '_', tokens_);
  int type_ = GetOptionType(option);
  if (tokens_.size() > 1) {
    std::vector<std::string> options = GetOptionShortcodesForUnderlyingInCurrScheme(tokens_[1], 0, type_);
    auto it = std::find(options.begin(), options.end(), option);
    if (it != options.end() && it != options.begin()) {
      it--;
      return *it;
    }
  }
  return "";
}

std::string NSESecurityDefinitions::GetNextOptionInCurrentSchema(const std::string& option) {
  std::vector<std::string> tokens_;
  PerishableStringTokenizer::StringSplit(option, '_', tokens_);
  int type_ = GetOptionType(option);
  if (tokens_.size() > 1) {
    std::vector<std::string> options = GetOptionShortcodesForUnderlyingInCurrScheme(tokens_[1], 0, type_);
    auto it = std::find(options.begin(), options.end(), option);
    if (it != options.end()) {
      it++;
      if (it != options.end()) {
        return *it;
      }
    }
  }
  return "";
}

std::string NSESecurityDefinitions::GetFutureSHCInPreviousExpiry(const std::string& fut_shc_) {
  std::vector<std::string> tokens_;
  PerishableStringTokenizer::StringSplit(fut_shc_, '_', tokens_);
  if (tokens_.size() > 2) {
    int contract_number_ = (*tokens_[2].rbegin() - '0') + 1;
    tokens_[2] = tokens_[2].substr(0, tokens_[2].size() - 1);
    if (contract_number_ <= 2) {
      std::ostringstream t_futname_oss_;
      t_futname_oss_ << tokens_[0] << "_" << tokens_[1] << "_" << tokens_[2] << contract_number_;
      return t_futname_oss_.str();
    }
  }
  return "";
}

// just for futures for now
std::string NSESecurityDefinitions::GetBhavCopyToken(const int t_date_, const char* shortcode_, int token_id_,
                                                     const std::string& segment_,
                                                     std::map<std::string, std::string>& shortcode_2_token_map_) {
  // find the latest files <= date_ ( based on holodays alone ), if file doesnt exist report and exit
  std::ostringstream nse_bhav_copy_file_name_oss_;
  std::string nse_bhav_copy_file_name_ = "";
  int this_date = t_date_;
  std::string return_token_ = "";
  this_date = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForExchange("NSE", this_date);
  // bhavcopy files for the date D are generated by the same date_stamp unlike nse_contract files
  // get the previous business day and load the data
  // split into dd mm yy
  std::ostringstream date_string_oss_;
  date_string_oss_ << this_date;
  std::string prev_date_ = date_string_oss_.str();
  nse_bhav_copy_file_name_oss_ << NSE_BHAVCOPY_FILENAME_PREFIX << "/" << prev_date_.substr(4, 2)
                               << prev_date_.substr(2, 2) << "/" << prev_date_.substr(6, 2) << prev_date_.substr(4, 2)
                               << "fo_0000.md";
  nse_bhav_copy_file_name_ = std::string("/spare/local/tradeinfo/") + nse_bhav_copy_file_name_oss_.str();
  std::string expr_date_;

  if (FileUtils::ExistsAndReadable(nse_bhav_copy_file_name_)) {
    // if file exists, break the shortcode_ into instrument, symbol, strike, option_type_, expiry_date_
    // for now only handles futures ( static logic to get exchange symbol )
    //
    char futures_[40];
    strcpy(futures_, shortcode_);
    char* basename_ = strtok(futures_, "_");
    if (basename_ != NULL) {
      basename_ = strtok(NULL, "_");
    }
    // FUTIDX || FUTSTK
    std::ifstream nse_bhavcopy_file_;
    nse_bhavcopy_file_.open(nse_bhav_copy_file_name_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (nse_bhavcopy_file_.is_open()) {
      while (nse_bhavcopy_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        nse_bhavcopy_file_.getline(readline_buffer_, sizeof(readline_buffer_));

        std::vector<char*> tokens_;
        // create a copy of line read before using non-const tokenizer
        char readline_buffer_copy_[1024];
        memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
        strcpy(readline_buffer_copy_, readline_buffer_);

        HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, ",", tokens_);
        if (tokens_.size() < 15) continue;
        std::string trimmed_str1_;
        std::string trimmed_str2_;

        std::string trimmed_str3_;
        std::string trimmed_str4_;
        std::string trimmed_str5_;

        HFSAT::PerishableStringTokenizer::TrimString(tokens_[1], trimmed_str1_, ' ');
        HFSAT::PerishableStringTokenizer::TrimString(tokens_[2], trimmed_str2_, ' ');
        HFSAT::PerishableStringTokenizer::TrimString(tokens_[3], trimmed_str3_, ' ');
        HFSAT::PerishableStringTokenizer::TrimString(tokens_[4], trimmed_str4_, ' ');
        HFSAT::PerishableStringTokenizer::TrimString(tokens_[5], trimmed_str5_, ' ');

        // FUTIDX FUTIVX FUTSTK OPTIDX OPTSTK
        if (tokens_[0][0] != '#' && (trimmed_str1_.compare(0, segment_.size(), segment_) == 0) &&
            (basename_ == NULL || trimmed_str2_ == basename_) &&
            (expr_date_.empty() || (expr_date_.compare(GetDateInYYYYMMDD(tokens_[3])) == 0))) {
          if (expr_date_.empty()) {
            expr_date_ = GetDateInYYYYMMDD(tokens_[3]);
          }

          if (basename_ == NULL) {
            return_token_ = tokens_[token_id_];
            // for now returning the first hit
            nse_bhavcopy_file_.close();
            return return_token_;
          } else {
            shortcode_2_token_map_[trimmed_str2_ + "_" + trimmed_str4_ + "_" + trimmed_str5_] = tokens_[token_id_];
          }
        }
      }
      nse_bhavcopy_file_.close();
    } else {
      std::cerr << "Fatal error - could not read NSE Bhavcopy file " << nse_bhav_copy_file_name_ << ".Exiting.\n";
      exit(0);
    }
  } else {
    std::cerr << "Fatal error - could not find Bhavcopy file " << nse_bhav_copy_file_name_ << ".Exiting.\n";
    exit(0);
  }
  return return_token_;
}

// for stocks
double NSESecurityDefinitions::GetLastTradePrice(const int t_date_, const char* shortcode_) {
  std::ostringstream nse_ltp_file_name_oss_;
  std::string nse_ltp_file_name_ = "";
  int this_date = t_date_;
  double return_token_ = -1.0;
  this_date = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForExchange("NSE", this_date);
  // bhavcopy files for the date D are generated by the same date_stamp unlike nse_contract files
  // get the previous business day and load the data
  // split into dd mm yy
  std::ostringstream date_string_oss_;
  date_string_oss_ << this_date;
  std::string prev_date_ = date_string_oss_.str();
  nse_ltp_file_name_oss_ << NSE_LTP_FILENAME_PREFIX << "/" << prev_date_ << ".txt";
  nse_ltp_file_name_ = std::string("/spare/local/tradeinfo/") + nse_ltp_file_name_oss_.str();

  if (FileUtils::ExistsAndReadable(nse_ltp_file_name_)) {
    // if file exists, break the shortcode_ into instrument, symbol, strike, option_type_, expiry_date_
    // for now only handles futures ( static logic to get exchange symbol )
    //
    char futures_[40];
    strcpy(futures_, shortcode_);
    char* basename_ = strtok(futures_, "_");
    if (basename_ != NULL) {
      basename_ = strtok(NULL, "_");
    }

    // FUTIDX || FUTSTK
    std::ifstream nse_ltp_file_;
    nse_ltp_file_.open(nse_ltp_file_name_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (nse_ltp_file_.is_open()) {
      while (nse_ltp_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        nse_ltp_file_.getline(readline_buffer_, sizeof(readline_buffer_));

        std::vector<char*> tokens_;
        // create a copy of line read before using non-const tokenizer
        char readline_buffer_copy_[1024];
        memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
        strcpy(readline_buffer_copy_, readline_buffer_);

        HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, " ", tokens_);
        if (tokens_.size() != 2) continue;

        std::string trimmed_str1_;
        std::string trimmed_str2_;
        HFSAT::PerishableStringTokenizer::TrimString(tokens_[0], trimmed_str1_, ' ');
        HFSAT::PerishableStringTokenizer::TrimString(tokens_[1], trimmed_str2_, ' ');

        if (tokens_[0][0] != '#' && (trimmed_str1_ == basename_)) {
          return_token_ = atof(tokens_[1]);
          // for now returning the first hit
          nse_ltp_file_.close();
          return return_token_;
          // std::cerr << basename_ << " " << tokens_[10] << expr_date_ << "\n";
        }
      }
      nse_ltp_file_.close();
    } else {
      std::cerr << "Fatal error - could not read NSE Ltp file " << nse_ltp_file_name_ << ".Exiting.\n";
      exit(0);
    }
  } else {
    std::cerr << "Fatal error - could not find Ltp file " << nse_ltp_file_name_ << ".Exiting.\n";
    exit(0);
  }
  return return_token_;
}

NSETradingLimit_t* NSESecurityDefinitions::GetTradingLimits(const std::string& shortcode_) {
  if (shortcode_2_price_limit_.end() == shortcode_2_price_limit_.find(shortcode_)) {
    return NULL;
  }

  return shortcode_2_price_limit_[shortcode_];
}

NSETradingLimit_t* NSESecurityDefinitions::ChangeTradingLimits(const std::string& shortcode_, int int_px_to_cross_) {
  if (shortcode_2_price_limit_.end() == shortcode_2_price_limit_.find(shortcode_)) {
    return NULL;
  }

  NSETradingLimit_t* t_trading_limit_ = shortcode_2_price_limit_[shortcode_];

  if (int_px_to_cross_ < t_trading_limit_->lower_limit_) {  // Lower limit crossed
    t_trading_limit_->lower_limit_fraction_ += 0.05;
    if (IsFuture(shortcode_)) {
      t_trading_limit_->lower_limit_ =
          ceil(int((t_trading_limit_->base_limit_ -
                    t_trading_limit_->base_limit_ * t_trading_limit_->lower_limit_fraction_) *
                       5 +
                   0.5) *
                   0.2 -
               DOUBLE_PRECISION);
    } else {
      if (IsEquity(shortcode_)) {
        t_trading_limit_->lower_limit_ =
            ceil(t_trading_limit_->base_limit_ -
                 t_trading_limit_->base_limit_ * t_trading_limit_->lower_limit_fraction_ - DOUBLE_PRECISION);
      } else {
        t_trading_limit_->lower_limit_fraction_ += 0.45;
        t_trading_limit_->lower_limit_ =
            ceil(t_trading_limit_->base_limit_ -
                 t_trading_limit_->base_limit_ * t_trading_limit_->lower_limit_fraction_ - DOUBLE_PRECISION);
      }
    }
  } else {  // Upper limit crossed
    t_trading_limit_->upper_limit_fraction_ += 0.05;
    if (IsFuture(shortcode_)) {
      t_trading_limit_->upper_limit_ =
          ceil(int((t_trading_limit_->base_limit_ +
                    t_trading_limit_->base_limit_ * t_trading_limit_->upper_limit_fraction_) *
                       5 +
                   0.5) *
                   0.2 -
               DOUBLE_PRECISION);
    } else {
      if (IsEquity(shortcode_)) {
        t_trading_limit_->upper_limit_ =
            int(t_trading_limit_->base_limit_ +
                t_trading_limit_->base_limit_ * t_trading_limit_->upper_limit_fraction_ + DOUBLE_PRECISION);
      } else {
        t_trading_limit_->upper_limit_fraction_ += 0.45;
        t_trading_limit_->upper_limit_ =
            int(t_trading_limit_->base_limit_ +
                t_trading_limit_->base_limit_ * t_trading_limit_->upper_limit_fraction_ + DOUBLE_PRECISION);
      }
    }
  }
  return t_trading_limit_;
}

void NSESecurityDefinitions::PopulateOrderPriceRangeMapEQ(const int t_date_) {
  std::ostringstream nse_eq_file_oss_;
  std::string nse_eq_file_name_ = "";

  std::ostringstream date_string_oss_;
  int prev_date = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForExchange("NSE", t_date_);
  date_string_oss_ << prev_date;
  std::string this_date = date_string_oss_.str();
  nse_eq_file_oss_ << NSE_ORDER_LIMIT_FILENAME_PREFIX << "/" << this_date.substr(0, 4) << "/" << this_date.substr(4, 2)
                   << "/" << this_date.substr(6, 2) << "/security";
  nse_eq_file_name_ = nse_eq_file_oss_.str();

  if (FileUtils::ExistsAndReadable(nse_eq_file_name_)) {
    std::ifstream nse_eq_file_;
    nse_eq_file_.open(nse_eq_file_name_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (nse_eq_file_.is_open()) {
      while (nse_eq_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        nse_eq_file_.getline(readline_buffer_, sizeof(readline_buffer_));

        std::vector<char*> tokens_;

        std::istringstream sp(readline_buffer_);
        for (std::string each; std::getline(sp, each, '|');) {
          char* chr = new char[each.size() + 1];
          ;
          memcpy(chr, each.c_str(), each.size() + 1);
          tokens_.push_back(chr);
        }

        std::string trimmed_str_;

        if (tokens_.size() > 7) {
          HFSAT::PerishableStringTokenizer::TrimString(tokens_[2], trimmed_str_, ' ');
          std::string shc_;
          HFSAT::PerishableStringTokenizer::TrimString(tokens_[1], shc_, ' ');
          //std::cout << "BSE EXCLUSIVE SECURITY EXCLUDING NOW " << std::endl;
          if(shc_.size() && shc_[shc_.size()-1]=='$') continue;
          shc_ = "NSE_" + shc_;
          if (trimmed_str_ == "EQ") {
            std::string range_string_;
            HFSAT::PerishableStringTokenizer::TrimString(tokens_[6], range_string_, ' ');
            std::vector<std::string> tokens1_;
            split(range_string_, '-', tokens1_);
	    double t_min_tick_size_;
	    t_min_tick_size_ = atof(tokens_[20]) / 100 ;
            shortcode_2_price_limit_[shc_] = new NSETradingLimit_t(-1, 100000000, -1, t_min_tick_size_);
            if (tokens1_.size() == 2) {
              double t_min_price_increment_ = shortcode_2_price_limit_[shc_]->min_tick_size_;
              shortcode_2_price_limit_[shc_]->lower_limit_ =
                  int(atof(tokens1_[0].c_str()) / t_min_price_increment_ + 0.5);
              shortcode_2_price_limit_[shc_]->upper_limit_ =
                  int(atof(tokens1_[1].c_str()) / t_min_price_increment_ + 0.5);
              shortcode_2_price_limit_[shc_]->base_limit_ =
                  int((atof(tokens1_[0].c_str()) + atof(tokens1_[1].c_str())) / (2 * t_min_price_increment_) + 0.5);
              // std::cout << shc_ << " " << shortcode_2_price_limit_[shc_]->lower_limit_ << "-" <<
              // shortcode_2_price_limit_[shc_]->upper_limit_ << " " << t_min_price_increment_ << " " <<
              // shortcode_2_price_limit_[shc_]->base_limit_  << std::endl;
            }
            if (tokens_.size() > 24){
              std::string surid_;
              HFSAT::PerishableStringTokenizer::TrimString(tokens_[22], surid_, ' ');
              shortcode_2_surveillance_id_[shc_] = stoi(surid_);
          //    std::cout <<"SURID " << shc_ <<" : " <<  shortcode_2_surveillance_id_[shc_] << std::endl;
            }
	  }
        }
      }
    }
  }
}

void NSESecurityDefinitions::PopulateOrderPriceRangeMapFut(const int t_date_) {
  std::ostringstream nse_eq_file_oss_;
  std::string nse_eq_file_name_ = "";

  std::ostringstream date_string_oss_;
  int prev_date = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForExchange("NSE", t_date_);
  date_string_oss_ << prev_date;
  std::string this_date = date_string_oss_.str();
  nse_eq_file_oss_ << NSE_ORDER_LIMIT_FILENAME_PREFIX << "/" << this_date.substr(0, 4) << "/" << this_date.substr(4, 2)
                   << "/" << this_date.substr(6, 2) << "/contract";
  nse_eq_file_name_ = nse_eq_file_oss_.str();

  if (FileUtils::ExistsAndReadable(nse_eq_file_name_)) {
    std::ifstream nse_eq_file_;
    nse_eq_file_.open(nse_eq_file_name_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (nse_eq_file_.is_open()) {
      while (nse_eq_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        nse_eq_file_.getline(readline_buffer_, sizeof(readline_buffer_));

        std::vector<char*> tokens_;

        std::istringstream sp(readline_buffer_);
        for (std::string each; std::getline(sp, each, '|');) {
          char* chr = new char[each.size() + 1];
          ;
          memcpy(chr, each.c_str(), each.size() + 1);
          tokens_.push_back(chr);
        }
        std::string trimmed_str_;
        NSEMonth_t current_month_ = (NSEMonth_t)((contractnumber_2_expirydate_[0] % 10000) / 100);
        if (tokens_.size() > 4) {
          std::string fut_shc_ = tokens_[3];
          if (fut_shc_.find("FINNIFTY") != std::string::npos || (fut_shc_.find("MIDCPNIFTY") != std::string::npos && intdate_ < 20230426 )) {
            current_month_ = (NSEMonth_t)((contractnumber_2_expirydate_tuesday_[0] % 10000) / 100);
          } else if (fut_shc_.find("MIDCPNIFTY") != std::string::npos && intdate_ < 20230817 ) {
            current_month_ = (NSEMonth_t)((contractnumber_2_expirydate_wednesday_[0] % 10000) / 100);
          } else if (fut_shc_.find("MIDCPNIFTY") != std::string::npos ){
            current_month_ = (NSEMonth_t)((contractnumber_2_expirydate_monday_[0] % 10000) / 100);
          } else if (fut_shc_.find("BANKNIFTY") != std::string::npos && intdate_ >= 20240301 ) {
            current_month_ = (NSEMonth_t)((contractnumber_2_expirydate_wednesday_[0] % 10000) / 100);
          }else if (fut_shc_.find("NIFTYNXT50") != std::string::npos  ) {
            current_month_ = (NSEMonth_t)((contractnumber_2_expirydate_friday_[0] % 10000) / 100);
          }
        }

        int fut1_contract_step_ = 1;
        int fut2_contract_step_ = 1;
        if (current_month_ == NSE_DEC) {
          fut1_contract_step_ = 2;
        } else if (current_month_ == NSE_NOV) {
          fut2_contract_step_ = 2;
        }

        if (tokens_.size() > 68) {
          HFSAT::PerishableStringTokenizer::TrimString(tokens_[2], trimmed_str_, ' ');
          if (trimmed_str_.substr(0, 3) == "FUT") {
            std::string shc_;
            HFSAT::PerishableStringTokenizer::TrimString(tokens_[3], shc_, ' ');

            std::string date_string_;
            HFSAT::PerishableStringTokenizer::TrimString(tokens_[53], date_string_, ' ');
            NSEMonth_t this_month_ = StringToMonthNumber(date_string_.substr(date_string_.length() - 6, 3).c_str());

            int contract_number = -1;
            if (current_month_ == this_month_) {
              contract_number = 0;
            } else if ((current_month_ + fut1_contract_step_) % 13 == this_month_) {
              contract_number = 1;
            } else if ((current_month_ + fut1_contract_step_ + fut2_contract_step_) % 13 == this_month_) {
              contract_number = 2;
            } else {
              continue;
            }

            shc_ = "NSE_" + shc_ + "_FUT" + std::to_string(contract_number);
            if (IsShortcode(shc_)) {
              double t_min_price_increment_ = shortcode_2_price_limit_[shc_]->min_tick_size_;
              shortcode_2_price_limit_[shc_]->lower_limit_ =
                  int(atof(tokens_[42]) / (100 * t_min_price_increment_) + 0.5);
              shortcode_2_price_limit_[shc_]->upper_limit_ =
                  int(atof(tokens_[43]) / (100 * t_min_price_increment_) + 0.5);
              shortcode_2_price_limit_[shc_]->base_limit_ =
                  int(atof(tokens_[67]) / (100 * t_min_price_increment_) + 0.5);
              // std::cout << shc_ << " " << shortcode_2_price_limit_[shc_]->lower_limit_ << "-" <<
              // shortcode_2_price_limit_[shc_]->upper_limit_ << " " << t_min_price_increment_ << " " <<
              // shortcode_2_price_limit_[shc_]->base_limit_  << std::endl;
            }
          }
        }
      }
    }
  }
}

// only Populates Monthly (so not touching it.)
void NSESecurityDefinitions::PopulateOrderPriceRangeMapOpt(const int t_date_) {
  std::ostringstream nse_eq_file_oss_;
  std::string nse_eq_file_name_ = "";

  std::ostringstream date_string_oss_;
  int prev_date = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForExchange("NSE", t_date_);
  date_string_oss_ << prev_date;
  std::string this_date = date_string_oss_.str();
  nse_eq_file_oss_ << NSE_ORDER_LIMIT_FILENAME_PREFIX << "/" << this_date.substr(0, 4) << "/" << this_date.substr(4, 2)
                   << "/" << this_date.substr(6, 2) << "/contract";
  nse_eq_file_name_ = nse_eq_file_oss_.str();

  if (FileUtils::ExistsAndReadable(nse_eq_file_name_)) {
    std::ifstream nse_eq_file_;
    nse_eq_file_.open(nse_eq_file_name_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (nse_eq_file_.is_open()) {
      while (nse_eq_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        nse_eq_file_.getline(readline_buffer_, sizeof(readline_buffer_));

        std::vector<char*> tokens_;

        std::istringstream sp(readline_buffer_);
        for (std::string each; std::getline(sp, each, '|');) {
          char* chr = new char[each.size() + 1];
          ;
          memcpy(chr, each.c_str(), each.size() + 1);
          tokens_.push_back(chr);
        }

        std::string trimmed_str_;

        if (tokens_.size() > 68) {
          HFSAT::PerishableStringTokenizer::TrimString(tokens_[2], trimmed_str_, ' ');
          if (trimmed_str_.substr(0, 3) == "OPT") {
            std::string fut_shc_;
            HFSAT::PerishableStringTokenizer::TrimString(tokens_[3], fut_shc_, ' ');
            double strike_ = atof(tokens_[7]) / 100;
            bool is_call_ = (strcmp(tokens_[8], "CE") == 0) ? 1 : 0;

            std::string date_string_;
            HFSAT::PerishableStringTokenizer::TrimString(tokens_[53], date_string_, ' ');

            date_string_.erase(0, fut_shc_.size() + 2);
            std::string end_string_ = std::to_string(int(strike_)) + tokens_[8];
            date_string_ = date_string_.substr(0, date_string_.size() - end_string_.size());

            NSEMonth_t this_month_ = StringToMonthNumber(date_string_.c_str());
            NSEMonth_t current_month_ = (NSEMonth_t)((contractnumber_2_expirydate_[0] % 10000) / 100);
            if (fut_shc_.find("FINNIFTY") != std::string::npos || (fut_shc_.find("MIDCPNIFTY") != std::string::npos && intdate_ < 20230426 )) {
              current_month_ = (NSEMonth_t)((contractnumber_2_expirydate_tuesday_[0] % 10000) / 100);
            } else if (fut_shc_.find("MIDCPNIFTY") != std::string::npos && intdate_ < 20230817 ) {
              current_month_ = (NSEMonth_t)((contractnumber_2_expirydate_wednesday_[0] % 10000) / 100);
            } else if (fut_shc_.find("MIDCPNIFTY") != std::string::npos) {
              current_month_ = (NSEMonth_t)((contractnumber_2_expirydate_monday_[0] % 10000) / 100); // Only Monthy expires
            } else if (fut_shc_.find("BANKNIFTY") != std::string::npos && intdate_ >=20240301 ) {
              current_month_ = (NSEMonth_t)((contractnumber_2_expirydate_wednesday_[0] % 10000) / 100);
            } else if (fut_shc_.find("NIFTYNXT50") != std::string::npos) {
              current_month_ = (NSEMonth_t)((contractnumber_2_expirydate_friday_[0] % 10000) / 100); // Only Monthy expires
            }

            int contract_number = -1;
            if (current_month_ == this_month_) {
              contract_number = 0;
            } else if ((current_month_ + 1) % 12 == this_month_) {
              contract_number = 1;
            } else if ((current_month_ + 2) % 12 == this_month_) {
              contract_number = 2;
            } else {
              continue;
            }

            std::string shc_ =
                GetShortcodeFromCanonical(fut_shc_, contractnumber_2_expirydate_[contract_number], strike_, is_call_); // Only Monthy expires
            if (fut_shc_.find("FINNIFTY") != std::string::npos || (fut_shc_.find("MIDCPNIFTY") != std::string::npos && intdate_ < 20230426 )) {
              shc_ = GetShortcodeFromCanonical(fut_shc_, contractnumber_2_expirydate_tuesday_[contract_number], strike_,
                                               is_call_);
              //	std:: cout<< "SHORTOPTOPTFIN " << shc_ <<std::endl;
            } else if (fut_shc_.find("MIDCPNIFTY") != std::string::npos && intdate_ < 20230817 ){
              shc_ = GetShortcodeFromCanonical(fut_shc_, contractnumber_2_expirydate_wednesday_[contract_number], strike_,
                                               is_call_);
              //	std:: cout<< "SHORTOPTOPTFIN " << shc_ <<std::endl;
            } else if (fut_shc_.find("MIDCPNIFTY") != std::string::npos){
              shc_ = GetShortcodeFromCanonical(fut_shc_, contractnumber_2_expirydate_monday_[contract_number], strike_,
                                               is_call_);
              //	std:: cout<< "SHORTOPTOPTFIN " << shc_ <<std::endl;
            } else if (fut_shc_.find("BANKNIFTY") != std::string::npos && intdate_ >=20240301 ){
              shc_ = GetShortcodeFromCanonical(fut_shc_, contractnumber_2_expirydate_wednesday_[contract_number], strike_,
                                               is_call_);
	          } else if (fut_shc_.find("NIFTYNXT50") != std::string::npos){
              shc_ = GetShortcodeFromCanonical(fut_shc_, contractnumber_2_expirydate_friday_[contract_number], strike_,
                                               is_call_);
              //	std:: cout<< "SHORTOPTOPTFIN " << shc_ <<std::endl;
            } 
            if ((shc_ != "INVALID") && (IsShortcode(shc_))) {
              double t_min_price_increment_ = shortcode_2_price_limit_[shc_]->min_tick_size_;
              shortcode_2_price_limit_[shc_]->lower_limit_ =
                  int(atof(tokens_[42]) / (100 * t_min_price_increment_) + 0.5);
              shortcode_2_price_limit_[shc_]->upper_limit_ =
                  int(atof(tokens_[43]) / (100 * t_min_price_increment_) + 0.5);
              shortcode_2_price_limit_[shc_]->base_limit_ = int(
                  (shortcode_2_price_limit_[shc_]->lower_limit_ + shortcode_2_price_limit_[shc_]->upper_limit_) * 0.5);
              shortcode_2_price_limit_[shc_]->lower_limit_fraction_ = 0.5;
              shortcode_2_price_limit_[shc_]->upper_limit_fraction_ = 0.5;
              // std::cout << shc_ << " " << shortcode_2_price_limit_[shc_]->lower_limit_ << "-" <<
              // shortcode_2_price_limit_[shc_]->upper_limit_ << " " << t_min_price_increment_ << " " <<
              // shortcode_2_price_limit_[shc_]->base_limit_  << std::endl;
            }
          }
        }
      }
    }
  }
}
void NSESecurityDefinitions::LoadIndexExpiries(const int t_date_) {
  //std::cout<<"t_date_ "<<t_date_<<std::endl;
  std::ostringstream nse_contract_specs_file_name_oss_;
  std::string nse_contract_specs_file_name_ = "";
  nse_contract_specs_file_name_oss_ << NSE_CONTRACT_FILENAME_PREFIX << "." << t_date_;
  nse_contract_specs_file_name_ = std::string("/spare/local/tradeinfo/") + nse_contract_specs_file_name_oss_.str();
  //std::cout<<nse_contract_specs_file_name_<<std::endl;

  if (FileUtils::ExistsAndReadable(nse_contract_specs_file_name_)) {
    std::ifstream nse_contract_file_;
    nse_contract_file_.open(nse_contract_specs_file_name_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (nse_contract_file_.is_open()) {
      while (nse_contract_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        nse_contract_file_.getline(readline_buffer_, sizeof(readline_buffer_));
        PerishableStringTokenizer st_(readline_buffer_, sizeof(readline_buffer_));

        const std::vector<const char*>& tokens_ = st_.GetTokens();
        if (tokens_.size() >= 5 && tokens_[0][0] != '#') {
          if ((NSE_IDXFUT == StringToInst(tokens_[0]))){
            if (strcmp(tokens_[1], "NIFTY")==0){
              if (std::find(expiry_dates_nifty.begin(), expiry_dates_nifty.end(), atoi(tokens_[5])) == expiry_dates_nifty.end()){
                expiry_dates_nifty.push_back(atoi(tokens_[5]));
              }
            }
            if (strcmp(tokens_[1], "BANKNIFTY")==0){
              if (std::find(expiry_dates_banknifty.begin(), expiry_dates_banknifty.end(), atoi(tokens_[5])) == expiry_dates_banknifty.end()){
                expiry_dates_banknifty.push_back(atoi(tokens_[5]));
              }
            }
            if (strcmp(tokens_[1], "FINNIFTY")==0){
              if (std::find(expiry_dates_finnifty.begin(), expiry_dates_finnifty.end(), atoi(tokens_[5])) == expiry_dates_finnifty.end()){
                expiry_dates_finnifty.push_back(atoi(tokens_[5]));
              }
            }
            if (strcmp(tokens_[1], "MIDCPNIFTY")==0){
              if (std::find(expiry_dates_midcpnifty.begin(), expiry_dates_midcpnifty.end(), atoi(tokens_[5])) == expiry_dates_midcpnifty.end()){
                expiry_dates_midcpnifty.push_back(atoi(tokens_[5]));
              }
            }
          }
          else if ((NSE_IDXOPT == StringToInst(tokens_[0])) && strcmp(tokens_[1], "NIFTY")==0){
            if (std::find(expiry_dates_nifty_weekly.begin(), expiry_dates_nifty_weekly.end(), atoi(tokens_[5])) == expiry_dates_nifty_weekly.end()){
              expiry_dates_nifty_weekly.push_back(atoi(tokens_[5]));
            }
          }
        }
      }  // end while
    }
    nse_contract_file_.close();
  } else {
    std::cerr << "Fatal error - could not read NSE Contract Specs file " << nse_contract_specs_file_name_
              << ".Exiting.\n";
    exit(0);
  }
  std::sort(expiry_dates_nifty_weekly.begin(), expiry_dates_nifty_weekly.end());
  std::sort(expiry_dates_nifty.begin(), expiry_dates_nifty.end());
  std::sort(expiry_dates_banknifty.begin(), expiry_dates_banknifty.end());
  std::sort(expiry_dates_midcpnifty.begin(), expiry_dates_midcpnifty.end());
  std::sort(expiry_dates_finnifty.begin(), expiry_dates_finnifty.end());

  //std::cout<<"NIFTY WEEKLY"<<std::endl;
  for (size_t i = 0; i < expiry_dates_nifty_weekly.size(); ++i) {
    uint32_t contract_number = static_cast<uint32_t>(i);
    weekly_option_contractnumber_2_expirydate_[contract_number] = expiry_dates_nifty_weekly[i];
    weekly_option_expirydate_2_contractnumber_[expiry_dates_nifty_weekly[i]] = contract_number;
  //  std::cout<<weekly_option_expirydate_2_contractnumber_[expiry_dates_nifty_weekly[i]]<<" "<<weekly_option_contractnumber_2_expirydate_[contract_number]<<std::endl;
  }
  //std::cout<<"NIFTY MONTHLY"<<std::endl;
  for (size_t i = 0; i < expiry_dates_nifty.size(); ++i) {
    uint32_t contract_number = static_cast<uint32_t>(i);
    contractnumber_2_expirydate_[contract_number] = expiry_dates_nifty[i];
    expirydate_2_contractnumber_[expiry_dates_nifty[i]] = contract_number;
   // std::cout<<expirydate_2_contractnumber_[expiry_dates_nifty[i]]<<" "<<contractnumber_2_expirydate_[contract_number]<<std::endl;
    }
   // std::cout<<"BANKNIFTY MONTHLY"<<std::endl;
  for (size_t i = 0; i < expiry_dates_banknifty.size(); ++i) {
    uint32_t contract_number = static_cast<uint32_t>(i);
    
    contractnumber_2_expirydate_wednesday_[contract_number] = expiry_dates_banknifty[i];
    expirydate_2_contractnumber_wednesday_[expiry_dates_banknifty[i]] = contract_number;
  //  std::cout<<expirydate_2_contractnumber_wednesday_[expiry_dates_banknifty[i]]<<" "<< contractnumber_2_expirydate_wednesday_[contract_number]<<std::endl;
  }
 // std::cout<<"MIDCPNIFTY MONTHLY"<<std::endl;
  for (size_t i = 0; i < expiry_dates_midcpnifty.size(); ++i) {
    uint32_t contract_number = static_cast<uint32_t>(i);
    contractnumber_2_expirydate_monday_[contract_number] = expiry_dates_midcpnifty[i];
    expirydate_2_contractnumber_monday_[expiry_dates_midcpnifty[i]] = contract_number;
  //  std::cout<<expirydate_2_contractnumber_monday_[expiry_dates_midcpnifty[i]]<<" "<<contractnumber_2_expirydate_monday_[contract_number]<<std::endl;
  }
  //std::cout<<"FINNIFTY MONTHLY"<<std::endl;
  for (size_t i = 0; i < expiry_dates_finnifty.size(); ++i) {
    uint32_t contract_number = static_cast<uint32_t>(i);
    contractnumber_2_expirydate_tuesday_[contract_number] = expiry_dates_finnifty[i];
    expirydate_2_contractnumber_tuesday_[expiry_dates_finnifty[i]] = contract_number;
   // std::cout<<expirydate_2_contractnumber_tuesday_[expiry_dates_finnifty[i]]<<" "<<contractnumber_2_expirydate_tuesday_[contract_number]<<std::endl;
  }

}

// TODO: Abhishek- please implement this function if useful
/*
static void GetTopNotionalTradedContracts(bool _underlying_level_, bool _options_, unsigned _top_n_,
                                          unsigned _lookback_days_) {
  if (_options_) {  // options
    // we take T-1 future price and use options strike to define options on Tth day in terms of current step,
    // lets implement this later :)
    if (_underlying_level_) {  // we are going to look at sum(options_notional_grouped_by_underlying)
      // this should be straight fwd, we just sum up all the options notional for current expiry at underlying level

    } else {
    }
  } else {  // futures
  }
}*/
}  // namespace HFSAT
