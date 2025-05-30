/**
   \file CDefCode/cboe_security_definition.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#include <algorithm>
#include <boost/date_time/gregorian/gregorian.hpp>

#include "dvccode/CDef/cboe_security_definition.hpp"
#include "dvccode/CDef/currency_convertor.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"

#include "dvccode/Utils/exchange_names.hpp"
#include "dvccode/Utils/holiday_manager.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"

#define SPARE_CBOE_FOLDER_PREFIX "/spare/local/tradeinfo/"

#define CBOE_CONTRACT_FILENAME_PREFIX "CBOE_Files/ContractFiles/cboe_contracts"
#define CBOE_DATASOURCE_EXCHSYMBOL_FILE "CBOE_Files/datasource_exchsymbol.txt"

#define CBOE_FO_BHAVCOPY_FILENAME_PREFIX "CBOE_Files/BhavCopy/fo"

namespace HFSAT {

const boost::gregorian::date_duration one_day_duration_(1);

CBOESecurityDefinitions* CBOESecurityDefinitions::p_uniqueinstance_ = NULL;
int CBOESecurityDefinitions::intdate_ = 0;
std::vector<CBOEInstrumentParams_t*> CBOESecurityDefinitions::inst_params_;
std::vector<CBOEOPtionsIdentifier_t> CBOESecurityDefinitions::options_param_;
std::map<std::string, CBOETradingLimit_t*> CBOESecurityDefinitions::shortcode_2_price_limit_;
std::map<int, uint32_t> CBOESecurityDefinitions::expirydate_2_contractnumber_third_friday_;
std::map<int, uint32_t> CBOESecurityDefinitions::daily_option_expirydate_2_contractnumber_;
std::map<uint32_t, int> CBOESecurityDefinitions::contractnumber_2_expirydate_third_friday_;
std::map<uint32_t, int> CBOESecurityDefinitions::daily_option_contractnumber_2_expirydate_;
std::map<std::string, std::string> CBOESecurityDefinitions::shortcode_2_exchsymbol_;
std::map<std::string, std::string> CBOESecurityDefinitions::exchsymbol_2_shortcode_;
std::map<std::string, std::string> CBOESecurityDefinitions::shortcode_2_datasourcename_;
std::map<std::string, std::string> CBOESecurityDefinitions::datasourcename_2_exchsymbol_;
std::map<std::string, std::string> CBOESecurityDefinitions::exchsymbol_2_datasourcename_;

std::map<std::string, CBOEInstrumentType_t> CBOESecurityDefinitions::shortcode_2_type_;
std::map<std::string, double> CBOESecurityDefinitions::shortcode_2_commissions_;

std::map<std::string, double> CBOESecurityDefinitions::shortcode_2_last_close_;

std::map<std::string, int> CBOESecurityDefinitions::shortcode_2_expiry_date_;
std::map<std::string, double> CBOESecurityDefinitions::shortcode_2_strike_price_;

CBOESecurityDefinitions::CBOESecurityDefinitions(int t_intdate_) {
  CurrencyConvertor::SetUniqueInstance(t_intdate_);
  // If the date for which the CBOE files are fetched is a holiday,
  // then we look back at the last working day

  while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceCBOEStr, t_intdate_, true)) {
    t_intdate_ = HFSAT::DateTime::CalcPrevDay(t_intdate_);
  }

  intdate_ = t_intdate_;
  inst_params_.clear();
  PopulateThirdFridayExpiryMap(); 
  PopulateDailyOptionsExpiryMap();
  LoadBhavCopyParams(intdate_);
  LoadInstParams(intdate_); 
  LoadDataSourceExchangeSymbolMap();

}

bool CBOESecurityDefinitions::IsMonthlyOptionExpiry(const int yyyymmdd, char* underlying) {
   if (strcmp(underlying, "SPX") == 0 && intdate_ > 20220101) {
     return (expirydate_2_contractnumber_third_friday_.find(yyyymmdd) != expirydate_2_contractnumber_third_friday_.end());
   }
   return false;
}

void CBOESecurityDefinitions::PopulateThirdFridayExpiryMap() {
  // 3 contracts outstanding in CBOE
  for (uint32_t t_ctr_ = 0; t_ctr_ < 3; t_ctr_++) {
    contractnumber_2_expirydate_third_friday_[t_ctr_] = ComputeThirdFridayExpiryDate(t_ctr_);
    expirydate_2_contractnumber_third_friday_[contractnumber_2_expirydate_third_friday_[t_ctr_]] = t_ctr_;
  }
}

int CBOESecurityDefinitions::ComputeNextThirdFridayExpiry(const int t_date_) {
  int t_month_ = (t_date_ % 10000) / 100;
  int t_year_ = t_date_ / 10000;

  // Get the third Friday of the month
  boost::gregorian::nth_day_of_the_week_in_month third_friday(boost::gregorian::nth_day_of_the_week_in_month::third, boost::gregorian::Friday, t_month_);
  boost::gregorian::date third_friday_date_ = third_friday.get_date(t_year_);
    
  // Extract year, month, and day
  boost::gregorian::date::ymd_type ymd = third_friday_date_.year_month_day();
    
  // Format the expiry date as YYYYMMDD
  int t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;

  while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceCBOEStr, t_expiry_date_, true)) {
    third_friday_date_ -= one_day_duration_;
    boost::gregorian::date::ymd_type ymd = third_friday_date_.year_month_day();
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
    return ComputeNextThirdFridayExpiry(t_temp_date_);
  }
}


int CBOESecurityDefinitions::ComputeThirdFridayExpiryDate(const int expiry_number_) {
  int t_expiry_number_ = expiry_number_;
  int t_temp_date_ = intdate_;
  int t_expiry_date_ = 0;
  while (t_expiry_number_ >= 0) {
    t_expiry_date_ = ComputeNextThirdFridayExpiry(t_temp_date_);
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

void CBOESecurityDefinitions::PopulateDailyOptionsExpiryMap() {
  int temp_date_ = intdate_; 
  
  for (uint32_t expiry_num = 0; expiry_num < 25; expiry_num++) {
    while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceCBOEStr, temp_date_, true)) {
      temp_date_ = HFSAT::DateTime::CalcNextDay(temp_date_);
    } 
    daily_option_contractnumber_2_expirydate_[expiry_num] = temp_date_;
    daily_option_expirydate_2_contractnumber_[daily_option_contractnumber_2_expirydate_[expiry_num]] = expiry_num;
    temp_date_ = HFSAT::DateTime::CalcNextDay(temp_date_);
  }
}

bool CBOESecurityDefinitions::IsDailyOptionExpiry(const int yyyymmdd, char* underlying) {
    if (strcmp(underlying, "SPXW") == 0 && intdate_ > 20220101) {
      return (daily_option_expirydate_2_contractnumber_.find(yyyymmdd) != daily_option_expirydate_2_contractnumber_.end());
    }
    return false;
}

std::string CBOESecurityDefinitions::GetExchSymbolCBOE(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  if (shortcode_2_exchsymbol_.find(local_shc) != shortcode_2_exchsymbol_.end()) {
    return (shortcode_2_exchsymbol_[local_shc]);
  } else {
    return "";
  }
}

std::string CBOESecurityDefinitions::GetDatasourcenameCBOE(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  if (shortcode_2_datasourcename_.find(local_shc) != shortcode_2_datasourcename_.end()) {
    return (shortcode_2_datasourcename_[local_shc]);
  } else {
    return "";
  }
}

CBOEMonth_t CBOESecurityDefinitions::StringToMonthNumber(const char* _cstr_) {
  if (!strncmp(_cstr_, "JAN", 3)) {
    return CBOE_JAN;
  } else if (!strncmp(_cstr_, "FEB", 3)) {
    return CBOE_FEB;
  } else if (!strncmp(_cstr_, "MAR", 3)) {
    return CBOE_MAR;
  } else if (!strncmp(_cstr_, "APR", 3)) {
    return CBOE_APR;
  } else if (!strncmp(_cstr_, "MAY", 3)) {
    return CBOE_MAY;
  } else if (!strncmp(_cstr_, "JUN", 3)) {
    return CBOE_JUN;
  } else if (!strncmp(_cstr_, "JUL", 3)) {
    return CBOE_JUL;
  } else if (!strncmp(_cstr_, "AUG", 3)) {
    return CBOE_AUG;
  } else if (!strncmp(_cstr_, "SEP", 3)) {
    return CBOE_SEP;
  } else if (!strncmp(_cstr_, "OCT", 3)) {
    return CBOE_OCT;
  } else if (!strncmp(_cstr_, "NOV", 3)) {
    return CBOE_NOV;
  } else if (!strncmp(_cstr_, "DEC", 3)) {
    return CBOE_DEC;
  } else
    return CBOE_INVALID_MM;
}

std::string CBOESecurityDefinitions::GetDateInYYYYMMDD(char* _date_str_) {
  PerishableStringTokenizer st_(_date_str_, sizeof(_date_str_));
  std::ostringstream date_in_YYYYMMDD_;
  const std::vector<const char*>& tokens_ = st_.GetTokens();
  if (tokens_.size() == 3) {
    std::string mm = "";
    CBOEMonth_t this_month_ = StringToMonthNumber(tokens_[1]);

    switch (this_month_) {
      case CBOE_JAN:
        mm = "01";
        break;
      case CBOE_FEB:
        mm = "02";
        break;
      case CBOE_MAR:
        mm = "03";
        break;
      case CBOE_APR:
        mm = "04";
        break;
      case CBOE_MAY:
        mm = "05";
        break;
      case CBOE_JUN:
        mm = "06";
        break;
      case CBOE_JUL:
        mm = "07";
        break;
      case CBOE_AUG:
        mm = "08";
        break;
      case CBOE_SEP:
        mm = "09";
        break;
      case CBOE_OCT:
        mm = "10";
        break;
      case CBOE_NOV:
        mm = "11";
        break;
      case CBOE_DEC:
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

bool CBOESecurityDefinitions::IsOptionStrikePresentInBhavCopy(CBOEInstrumentParams_t param, double strike_price) {
  CBOEOPtionsIdentifier_t this_option_identifier;
  std::ostringstream t_str_oss_;
  this_option_identifier.inst_type_ = param.inst_type_;
  t_str_oss_ << param.underlying_;
  this_option_identifier.underlying_ = t_str_oss_.str();
  this_option_identifier.expiry_ = param.expiry_;
  this_option_identifier.strike_price_ = strike_price;

  t_str_oss_.str(std::string());
  t_str_oss_ << "CE";
  this_option_identifier.call_or_put_ = t_str_oss_.str();
  if (std::find(options_param_.begin(), options_param_.end(), this_option_identifier) != options_param_.end()) {
    return true;
  }
  return false;
}

CBOEInstrumentType_t CBOESecurityDefinitions::StringToInst(const char* str) {
  if ((strcmp(str, "IDXOPT") == 0))
    return CBOE_IDXOPT;
  else if ((strcmp(str, "IDXSPT") == 0))
    return CBOE_IDXSPT;
  else
    return CBOE_INVALID;
}

// Reads entries from CBOE contract file - every line is of format
// UNDERLYING_TYPE UNDERLYING CLOSE_PX LOT_SIZE MIN_TICK EXPIRY NUM_ITM STEP_VALUE
// eg INDOPT SPX
void CBOESecurityDefinitions::LoadInstParams(const int t_date_) { // Contract File For FO
  std::ostringstream cboe_contract_specs_file_name_oss_;
  std::string cboe_contract_specs_file_name_ = "";
  cboe_contract_specs_file_name_oss_ << CBOE_CONTRACT_FILENAME_PREFIX << "." << t_date_;
  cboe_contract_specs_file_name_ = std::string("/spare/local/tradeinfo/") + cboe_contract_specs_file_name_oss_.str();

  if (FileUtils::ExistsAndReadable(cboe_contract_specs_file_name_)) {
    std::ifstream cboe_contract_file_;
    cboe_contract_file_.open(cboe_contract_specs_file_name_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (cboe_contract_file_.is_open()) {
      while (cboe_contract_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        cboe_contract_file_.getline(readline_buffer_, sizeof(readline_buffer_));
        PerishableStringTokenizer st_(readline_buffer_, sizeof(readline_buffer_));

        const std::vector<const char*>& tokens_ = st_.GetTokens();
        if (tokens_.size() >= 5 && tokens_[0][0] != '#') {
          CBOEInstrumentParams_t* t_config_ = new CBOEInstrumentParams_t();
          t_config_->inst_type_ = StringToInst(tokens_[0]);
          strcpy(t_config_->underlying_, tokens_[1]);
          t_config_->expiry_ = atoi(tokens_[2]);
          t_config_->last_close_ = atof(tokens_[3]);
          t_config_->lot_size_ = atoi(tokens_[4]);
          t_config_->min_tick_ = atof(tokens_[5]);
          if (t_config_->inst_type_ == CBOE_IDXOPT) {
            t_config_->num_itm_ = atoi(tokens_[6]);
            t_config_->step_value_ = atof(tokens_[7]);
          }
          inst_params_.push_back(t_config_);
        }
      }  // end while
    }
    cboe_contract_file_.close();
  } else {
    std::cerr << "Fatal error - could not read CBOE Contract Specs file " << cboe_contract_specs_file_name_
              << ".Exiting.\n";
    exit(0);
  }
}

void CBOESecurityDefinitions::AddDataSourceSymbolFromBhavCopy(CBOEInstrumentType_t inst_type_, std::string underlying_,
                                                             int exp_date_, double strike_px_, std::string call_or_put_) {
  CBOEOPtionsIdentifier_t t_config_;
  t_config_.inst_type_ = inst_type_;
  t_config_.underlying_ = underlying_;
  t_config_.expiry_ = exp_date_;
  t_config_.strike_price_ = strike_px_;
  t_config_.call_or_put_ = call_or_put_;
  options_param_.push_back(t_config_);

}

void CBOESecurityDefinitions::LoadBhavCopyParams(const int t_date_) {
  std::ostringstream cboe_bhav_copy_file_name_oss_;
  std::string cboe_bhav_copy_file_name_ = "";

  int this_date = t_date_;

  // bhavcopy files for the date D are generated by the same date_stamp unlike cboe_contract files
  // get the previous business day and load the data
  this_date = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForExchange("CBOE", this_date);
  // split into dd mm yy
  std::ostringstream date_string_oss_;
  date_string_oss_ << this_date;
  std::string prev_date_ = date_string_oss_.str();
  cboe_bhav_copy_file_name_oss_ << CBOE_FO_BHAVCOPY_FILENAME_PREFIX << "/" << prev_date_.substr(4, 2)
                               << prev_date_.substr(2, 2) << "/" << prev_date_.substr(6, 2) << prev_date_.substr(4, 2)
                               << "fo_0000.md";
  cboe_bhav_copy_file_name_ = std::string("/spare/local/tradeinfo/") + cboe_bhav_copy_file_name_oss_.str();

  if (FileUtils::ExistsAndReadable(cboe_bhav_copy_file_name_)) {
    std::ifstream cboe_bhavcopy_file_;
    cboe_bhavcopy_file_.open(cboe_bhav_copy_file_name_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (cboe_bhavcopy_file_.is_open()) {
      while (cboe_bhavcopy_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        cboe_bhavcopy_file_.getline(readline_buffer_, sizeof(readline_buffer_));

        std::vector<char*> tokens_;
        // create a copy of line read before using non-const tokenizer
        char readline_buffer_copy_[1024];
        memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
        strcpy(readline_buffer_copy_, readline_buffer_);

        HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, ",", tokens_);
	if (tokens_.size() < 6) continue;
        std::string type_ = tokens_[1];
        // trim the inst_type field: contains spaces
        if (tokens_[0][0] != '#' && (type_ == "IDXOPT")) { // Only Index Options Support
          CBOEInstrumentType_t inst_type_ = StringToInst(type_.c_str());
          std::string underlying_ = tokens_[2];
          int expiry_ = std::atoi(tokens_[3]);
          double strike_price_ = atof(tokens_[4]);
          std::string call_or_put_ = tokens_[5];

          AddDataSourceSymbolFromBhavCopy(inst_type_, underlying_, expiry_, strike_price_, call_or_put_);

        }
      }  // end while
    }
    cboe_bhavcopy_file_.close();
  } else {
    std::cerr << "Fatal error - could not read CBOE Bhavcopy file " << cboe_bhav_copy_file_name_ << ".Exiting.\n";
    exit(0);
  }
}


void CBOESecurityDefinitions::split(const std::string& s, char delim, std::vector<std::string>& elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
}

std::string CBOESecurityDefinitions::ConvertDataSourceNametoExchSymbol(const std::string t_dsname_str_) {

  if (datasourcename_2_exchsymbol_.find(t_dsname_str_) != datasourcename_2_exchsymbol_.end()) {
    return datasourcename_2_exchsymbol_[t_dsname_str_];
  } else {
    return "INVALID";
  }
}

std::string CBOESecurityDefinitions::ConvertDataSourceNametoExchSymbolSecDef(const std::string t_dsname_str_) {
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


std::string CBOESecurityDefinitions::ConvertExchSymboltoDataSourceName(const std::string t_exchsymbol_) {
  if (exchsymbol_2_datasourcename_.find(t_exchsymbol_) != exchsymbol_2_datasourcename_.end()) {
    return exchsymbol_2_datasourcename_[t_exchsymbol_];
  } else {
    return "INVALID";
  }
}

int CBOESecurityDefinitions::GetMonthlyContractNumberFromExpiry(const int t_date_, const CBOEInstrumentType_t t_inst_type_, char* underlying) {
  if (strcmp(underlying, "SPX") == 0 && intdate_ > 20220101){
    if (expirydate_2_contractnumber_third_friday_.find(t_date_) != expirydate_2_contractnumber_third_friday_.end()) {
      return expirydate_2_contractnumber_third_friday_[t_date_];
    }
  }
  return -1;
}

int CBOESecurityDefinitions::GetDailyContractNumberFromExpiry(const int t_date_, char* underlying) {
  if (strcmp(underlying, "SPXW") == 0 && intdate_ > 20220101 ) {
    if (daily_option_expirydate_2_contractnumber_.find(t_date_) != daily_option_expirydate_2_contractnumber_.end()) { 
      return daily_option_expirydate_2_contractnumber_[t_date_];
    }
  }
  return -1;
}

double CBOESecurityDefinitions::GetContractMultiplier(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  std::map<std::string, CBOEInstrumentType_t>::iterator t_type_map_iter_ = shortcode_2_type_.find(local_shc);
  if (t_type_map_iter_ != shortcode_2_type_.end()) {
    return GetContractMultiplier(t_type_map_iter_->second);
  } else {
    std::cerr << "Fatal Error - shortcode not found in shortcode_2_type map " << local_shc << ".Exiting.\n";
    exit(-1);
  }
}

double CBOESecurityDefinitions::GetContractMultiplier(const CBOEInstrumentType_t t_inst_type_) {
  switch (t_inst_type_) {
    case HFSAT::CBOE_IDXOPT:
    case HFSAT::CBOE_IDXSPT:
      return 100;
      break;
    default:
      std::cerr << "Fatal Error - unsupported instrument type in GetContractMultiplier.Exiting. \n";
      exit(0);
  }
}

double CBOESecurityDefinitions::GetCBOECommission(std::string _shortcode_) {
  std::replace(_shortcode_.begin(), _shortcode_.end(), '~', '&');

  std::map<std::string, double>::iterator t_commish_map_iter_ = shortcode_2_commissions_.find(_shortcode_);
  if (t_commish_map_iter_ != shortcode_2_commissions_.end()) {
    return t_commish_map_iter_->second;
  } else {
    std::map<std::string, CBOEInstrumentType_t>::iterator t_type_map_iter_ = shortcode_2_type_.find(_shortcode_);
    double t_commish_;
    if (t_type_map_iter_ != shortcode_2_type_.end()) {
      switch (t_type_map_iter_->second) {

        case HFSAT::CBOE_IDXOPT:
        case HFSAT::CBOE_IDXSPT:
          t_commish_ = (0.01 + 0.0056) *
                       GetContractMultiplier(t_type_map_iter_->second);  // txn charge can become less than this
          shortcode_2_commissions_[_shortcode_] = t_commish_;
          return t_commish_;

        default:
          std::cerr << " GetCBOECommission called with improper instrument type " << t_type_map_iter_->second
                    << ".Exiting.\n";
          exit(0);
      }
    }
  }
  std::cerr << " GetCBOECommission called with improper shortcode " << _shortcode_ << ".Exiting.\n";
  exit(0);
}

void CBOESecurityDefinitions::AddCBOEContractSpecifications(ShortcodeContractSpecificationMap& t_contract_spec_map_) {
  std::vector<CBOEInstrumentParams_t*>::iterator t_veciter_;
  for (t_veciter_ = inst_params_.begin(); t_veciter_ != inst_params_.end(); t_veciter_++) {
    CBOEInstrumentParams_t* t_param_ = *t_veciter_;
    if ((t_param_->inst_type_ == HFSAT::CBOE_IDXOPT) && (strcmp(t_param_->underlying_, "SPXW") == 0 || strcmp(t_param_->underlying_, "SPX") == 0)) {
        if (IsDailyOptionExpiry(t_param_->expiry_, t_param_->underlying_) && t_param_->inst_type_ == HFSAT::CBOE_IDXOPT) {
          std::ostringstream t_optname_base_oss_;
          std::ostringstream t_temp_optname_oss_;
          std::ostringstream t_temp_optname_ess_;
          std::set<double> otm_call_itm_put_strike_price_set_;
          std::set<double, std::greater<double>> otm_put_itm_call_strike_price_set_;

          double temp_step_value = t_param_->step_value_;

          if (strcmp(t_param_->underlying_, "SPXW") == 0) {
            temp_step_value *= 1;
          }

          // ATM based on HFT
          // since we have two steps now, we have to calculate which one is nearest to close
          // SO  SPXW 1446.25 700     0.05    20220101        5       50.00   50.00   5
          double t_strike_ = std::round(t_param_->last_close_ / temp_step_value) * temp_step_value;;

          t_optname_base_oss_ << "CBOE_" << t_param_->underlying_;
          double t_atm_strike_ = t_strike_;
          // atm put/call
          {
            // ATM Put
            t_temp_optname_oss_.str(std::string());
            t_temp_optname_oss_ << t_optname_base_oss_.str() << "_P"
                                  << GetDailyContractNumberFromExpiry(t_param_->expiry_, t_param_->underlying_)
                                  << "_A";

            t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                t_param_->min_tick_, GetContractMultiplier(t_param_->inst_type_), kExchSourceCBOE, t_param_->lot_size_);

            t_temp_optname_ess_.str(std::string());
            t_temp_optname_ess_ << t_optname_base_oss_.str() << "_PE_" << t_param_->expiry_ << "_" << std::fixed
                                << std::setprecision(2) << t_atm_strike_;
            shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());
            exchsymbol_2_shortcode_[ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str())] =
                t_temp_optname_oss_.str();

	    shortcode_2_datasourcename_[t_temp_optname_oss_.str()] = 
		    exchsymbol_2_datasourcename_[shortcode_2_exchsymbol_[t_temp_optname_oss_.str()]];
            shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
            // store last close info per shortcode -- approximate.
            shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
            shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
            shortcode_2_strike_price_[t_temp_optname_oss_.str()] = t_atm_strike_;
            shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                new CBOETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);

            //--------------------

            // ATM Call
            t_temp_optname_oss_.str(std::string());
              t_temp_optname_oss_ << t_optname_base_oss_.str() << "_C"
                                  << GetDailyContractNumberFromExpiry(t_param_->expiry_, t_param_->underlying_)
                                  << "_A";

            t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                t_param_->min_tick_, GetContractMultiplier(t_param_->inst_type_), kExchSourceCBOE, t_param_->lot_size_);

            t_temp_optname_ess_.str(std::string());
            t_temp_optname_ess_ << t_optname_base_oss_.str() << "_CE_" << t_param_->expiry_ << "_"
                                << std::setprecision(2) << t_atm_strike_;
            shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());
            exchsymbol_2_shortcode_[ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str())] =
                t_temp_optname_oss_.str();

	    shortcode_2_datasourcename_[t_temp_optname_oss_.str()] = 
		    exchsymbol_2_datasourcename_[shortcode_2_exchsymbol_[t_temp_optname_oss_.str()]];
            shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
            // store last close info per shortcode -- approximate.
            shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
            shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
            shortcode_2_strike_price_[t_temp_optname_oss_.str()] = t_atm_strike_;
            shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                new CBOETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
          }
          // otm/itm options
          {
            // For OTM Call and ITM Put
            {
              int counter_inc = 0;

              double start_strike_inc = t_strike_;

              // this is to just for initialization for the upcoming loop to work
              while (start_strike_inc <= t_strike_) {
                start_strike_inc += t_param_->step_value_;
              }

              // As of now, both start_strike_inc are just more than ATM and is of required multiple
              // we want to include all options corresponding to the current schema
              // hence we loop through till we expiry
              unsigned int t_cs_ctr_ = 0;
              while (t_cs_ctr_ <= t_param_->num_itm_) {
                double option_strike_inc = start_strike_inc + t_param_->step_value_ * (counter_inc++);
		/*
                // This checks whether strikes generated from previous steps are present in Bhavcopy.
                // If not present we don't consider them.
                if (!IsOptionStrikePresentInBhavCopy(*t_param_, option_strike_inc)) {
                  continue;
		}
		*/
                t_cs_ctr_++;
                // this is mainly to push the inc strike into otm_call_itm_put or otm_put_itm_call vector
                if (option_strike_inc > t_atm_strike_) {
                  otm_call_itm_put_strike_price_set_.insert(option_strike_inc);
                } else if (option_strike_inc < t_atm_strike_) {
                  otm_put_itm_call_strike_price_set_.insert(option_strike_inc);
                }
              }
            }
            // For ITM Call and OTM Put
            {
              int counter_dec = 0;

              double start_strike_dec = t_strike_;

              // this is to just for initialization for the upcoming loop to work
              while (start_strike_dec >= t_strike_) {
                start_strike_dec -= t_param_->step_value_;
              }

              // As of now, both start_strike_dec are just less than ATM and is of required multiple
              unsigned int t_cs_ctr_ = 0;
              while (t_cs_ctr_ <= t_param_->num_itm_) {
                double option_strike_dec = start_strike_dec - t_param_->step_value_ * (counter_dec++);
		/*
                if (!IsOptionStrikePresentInBhavCopy(*t_param_, option_strike_dec)) 
                  continue;
		*/
                t_cs_ctr_++;

                // this is mainly to push the dec strike into otm_call_itm_put or otm_put_itm_call vector
                if (option_strike_dec > t_atm_strike_) {
                  otm_call_itm_put_strike_price_set_.insert(option_strike_dec);
                } else if (option_strike_dec < t_atm_strike_) {
                  otm_put_itm_call_strike_price_set_.insert(option_strike_dec);
                }
              }
            }
            //

            unsigned int t_ctr_ = 0;
            std::set<double>::iterator otm_call_itm_put_itr_;
            for (otm_call_itm_put_itr_ = otm_call_itm_put_strike_price_set_.begin();
                 otm_call_itm_put_itr_ != otm_call_itm_put_strike_price_set_.end(); ++otm_call_itm_put_itr_) {
              t_ctr_++;
              double current_strike_price_ = *otm_call_itm_put_itr_;
              // ITM PUT
              t_temp_optname_ess_.str(std::string());
              t_temp_optname_oss_.str(std::string());

              t_temp_optname_oss_ << t_optname_base_oss_.str() << "_P"
                                  << GetDailyContractNumberFromExpiry(t_param_->expiry_, t_param_->underlying_)
                                  << "_I" << t_ctr_;

              t_temp_optname_ess_ << t_optname_base_oss_.str() << "_PE_" << t_param_->expiry_ << "_"
                                  << std::setprecision(2) << current_strike_price_;

	      if (ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str()) == "INVALID") continue; // INVALID exchange sym handling 

              t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                  t_param_->min_tick_, GetContractMultiplier(t_param_->inst_type_), kExchSourceCBOE, t_param_->lot_size_);

              shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                  ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());
              exchsymbol_2_shortcode_[ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str())] =
                  t_temp_optname_oss_.str();

	      shortcode_2_datasourcename_[t_temp_optname_oss_.str()] = 
		    exchsymbol_2_datasourcename_[shortcode_2_exchsymbol_[t_temp_optname_oss_.str()]];
              shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
              // store last close info per shortcode -- approximate.
              shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
              shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                  new CBOETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
              shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
              shortcode_2_strike_price_[t_temp_optname_oss_.str()] = current_strike_price_;

              //-------------------------
              // OTM Call
              t_temp_optname_oss_.str(std::string());
              t_temp_optname_ess_.str(std::string());

              t_temp_optname_oss_ << t_optname_base_oss_.str() << "_C"
                                  << GetDailyContractNumberFromExpiry(t_param_->expiry_, t_param_->underlying_)
                                  << "_O" << t_ctr_;

              t_temp_optname_ess_ << t_optname_base_oss_.str() << "_CE_" << t_param_->expiry_ << "_"
                                  << std::setprecision(2) << current_strike_price_;

	      if (ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str()) == "INVALID") continue; // INVALID exchange sym handling 

              t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                  t_param_->min_tick_, GetContractMultiplier(t_param_->inst_type_), kExchSourceCBOE, t_param_->lot_size_);

              shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                  ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());
              exchsymbol_2_shortcode_[ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str())] =
                  t_temp_optname_oss_.str();

	      shortcode_2_datasourcename_[t_temp_optname_oss_.str()] = 
		    exchsymbol_2_datasourcename_[shortcode_2_exchsymbol_[t_temp_optname_oss_.str()]];
              shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
              // store last close info per shortcode -- approximate.
              shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
              shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                  new CBOETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
              shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
              shortcode_2_strike_price_[t_temp_optname_oss_.str()] = current_strike_price_;
            }  // end of for loop

            std::set<double, std::greater<double>>::iterator otm_put_itm_call_itr_;
            t_ctr_ = 0;
            for (otm_put_itm_call_itr_ = otm_put_itm_call_strike_price_set_.begin();
                 otm_put_itm_call_itr_ != otm_put_itm_call_strike_price_set_.end(); ++otm_put_itm_call_itr_) {
              t_ctr_++;
              double current_strike_price_ = *otm_put_itm_call_itr_;

              // OTM Put
              t_temp_optname_oss_.str(std::string());
              t_temp_optname_ess_.str(std::string());

              t_temp_optname_oss_ << t_optname_base_oss_.str() << "_P"
                                  << GetDailyContractNumberFromExpiry(t_param_->expiry_, t_param_->underlying_)
                                  << "_O" << t_ctr_;

              t_temp_optname_ess_ << t_optname_base_oss_.str() << "_PE_" << t_param_->expiry_ << "_"
                                  << std::setprecision(2) << current_strike_price_;

	      if (ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str()) == "INVALID") continue; // INVALID exchange sym handling 

              t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                  t_param_->min_tick_, GetContractMultiplier(t_param_->inst_type_), kExchSourceCBOE, t_param_->lot_size_);

              shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                  ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());
              exchsymbol_2_shortcode_[ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str())] =
                  t_temp_optname_oss_.str();

	      shortcode_2_datasourcename_[t_temp_optname_oss_.str()] = 
		    exchsymbol_2_datasourcename_[shortcode_2_exchsymbol_[t_temp_optname_oss_.str()]];
              shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
              // store last close info per shortcode -- approximate.
              shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
              shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                  new CBOETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
              shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
              shortcode_2_strike_price_[t_temp_optname_oss_.str()] = current_strike_price_;

              // ITM Call
              t_temp_optname_oss_.str(std::string());
              t_temp_optname_ess_.str(std::string());
              t_temp_optname_oss_ << t_optname_base_oss_.str() << "_C"
                                  << GetDailyContractNumberFromExpiry(t_param_->expiry_, t_param_->underlying_)
                                  << "_I" << t_ctr_;

              t_temp_optname_ess_ << t_optname_base_oss_.str() << "_CE_" << t_param_->expiry_ << "_"
                                  << std::setprecision(2) << current_strike_price_;

	      if (ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str()) == "INVALID") continue; // INVALID exchange sym handling 

              t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                  t_param_->min_tick_, GetContractMultiplier(t_param_->inst_type_), kExchSourceCBOE, t_param_->lot_size_);

              shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                  ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());
              exchsymbol_2_shortcode_[ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str())] =
                  t_temp_optname_oss_.str();

	      shortcode_2_datasourcename_[t_temp_optname_oss_.str()] = 
		    exchsymbol_2_datasourcename_[shortcode_2_exchsymbol_[t_temp_optname_oss_.str()]];
              shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
              // store last close info per shortcode -- approximate.
              shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
              shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                  new CBOETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
              shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
              shortcode_2_strike_price_[t_temp_optname_oss_.str()] = current_strike_price_;

            }  // end of for loop
          }
        }
        if (IsMonthlyOptionExpiry(t_param_->expiry_, t_param_->underlying_)) {
          std::ostringstream t_optname_base_oss_;
          std::ostringstream t_temp_optname_oss_;
          std::ostringstream t_temp_optname_ess_;

          double temp_step_value = t_param_->step_value_;

          if (strcmp(t_param_->underlying_, "SPX") == 0) {
            temp_step_value *= 1;
	  }

          // ATM based on HFT
          // since we have two steps now, we have to calculate which one is nearest to close
          // SO  SPX 1446.25 700     0.05    20220101        5       50.00   50.00   5
          double t_strike_ = std::round(t_param_->last_close_ / temp_step_value) * temp_step_value;

          t_optname_base_oss_ << "CBOE_" << t_param_->underlying_;
          std::set<double> otm_call_itm_put_strike_price_set_;
          std::set<double, std::greater<double>> otm_put_itm_call_strike_price_set_;
          double t_atm_strike_ = t_strike_;
          // atm put/call
          {
            // ATM Put
            t_temp_optname_oss_.str(std::string());
            t_temp_optname_oss_ << t_optname_base_oss_.str() << "_P"
                                << GetMonthlyContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_, t_param_->underlying_)
                                << "_A";

            t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                t_param_->min_tick_, GetContractMultiplier(t_param_->inst_type_), kExchSourceCBOE, t_param_->lot_size_);

            t_temp_optname_ess_.str(std::string());
            t_temp_optname_ess_ << t_optname_base_oss_.str() << "_PE_" << t_param_->expiry_ << "_" << std::fixed
                                << std::setprecision(2) << t_atm_strike_;
            shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());
            exchsymbol_2_shortcode_[ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str())] =
                t_temp_optname_oss_.str();

	    shortcode_2_datasourcename_[t_temp_optname_oss_.str()] = 
		    exchsymbol_2_datasourcename_[shortcode_2_exchsymbol_[t_temp_optname_oss_.str()]];
            shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
            // store last close info per shortcode -- approximate.
            shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
            shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
            shortcode_2_strike_price_[t_temp_optname_oss_.str()] = t_atm_strike_;
            shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                new CBOETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);

            //--------------------

            // ATM Call
            t_temp_optname_oss_.str(std::string());
            t_temp_optname_oss_ << t_optname_base_oss_.str() << "_C"
                                << GetMonthlyContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_, t_param_->underlying_)
                                << "_A";
            t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                t_param_->min_tick_, GetContractMultiplier(t_param_->inst_type_), kExchSourceCBOE, t_param_->lot_size_);

            t_temp_optname_ess_.str(std::string());
            t_temp_optname_ess_ << t_optname_base_oss_.str() << "_CE_" << t_param_->expiry_ << "_"
                                << std::setprecision(2) << t_atm_strike_;
            shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());
            exchsymbol_2_shortcode_[ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str())] =
                t_temp_optname_oss_.str();

	    shortcode_2_datasourcename_[t_temp_optname_oss_.str()] = 
		    exchsymbol_2_datasourcename_[shortcode_2_exchsymbol_[t_temp_optname_oss_.str()]];
            shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
            // store last close info per shortcode -- approximate.
            shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
            shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
            shortcode_2_strike_price_[t_temp_optname_oss_.str()] = t_atm_strike_;
            shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                new CBOETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);

          }
          // otm/itm options
          {
            // For OTM Call and ITM Put
            {
              int counter_inc = 0;

              double start_strike_inc = t_strike_;

              // this is to just for initialization for the upcoming loop to work
              while (start_strike_inc <= t_strike_) {
                start_strike_inc += t_param_->step_value_;
              }

              // As of now, both start_strike_inc are just more than ATM and is of required multiple
              // we want to include all options corresponding to the current schema
              // hence we loop through till we expiry
              unsigned int t_cs_ctr_ = 0;
              while (t_cs_ctr_ <= t_param_->num_itm_) {
                double option_strike_inc = start_strike_inc + t_param_->step_value_ * (counter_inc++);
		/*
                // This checks whether strikes generated from previous steps are present in Bhavcopy.
                // If not present we don't consider them.
                if (!IsOptionStrikePresentInBhavCopy(*t_param_, option_strike_inc)) 
                  continue;
		*/
                t_cs_ctr_++;
                // this is mainly to push the inc strike into otm_call_itm_put or otm_put_itm_call vector
                if (option_strike_inc > t_atm_strike_) {
                  otm_call_itm_put_strike_price_set_.insert(option_strike_inc);
                } else if (option_strike_inc < t_atm_strike_) {
                  otm_put_itm_call_strike_price_set_.insert(option_strike_inc);
                }
              }
            }

            // For ITM Call and OTM Put
            {
              int counter_dec = 0;

              double start_strike_dec = t_strike_;

              // this is to just for initialization for the upcoming loop to work
              while (start_strike_dec >= t_strike_) {
                start_strike_dec -= t_param_->step_value_;
              }

              // As of now, both start_strike_dec are just less than ATM and is of required multiple
              unsigned int t_cs_ctr_ = 0;
              while (t_cs_ctr_ <= t_param_->num_itm_) {
                double option_strike_dec = start_strike_dec - t_param_->step_value_ * (counter_dec++);
		/*
                if (!IsOptionStrikePresentInBhavCopy(*t_param_, option_strike_dec))
                  continue;
		*/
                t_cs_ctr_++;

                // this is mainly to push the dec strike into otm_call_itm_put or otm_put_itm_call vector
                if (option_strike_dec > t_atm_strike_) {
                  otm_call_itm_put_strike_price_set_.insert(option_strike_dec);
                } else if (option_strike_dec < t_atm_strike_) {
                  otm_put_itm_call_strike_price_set_.insert(option_strike_dec);
                }
              }
            }
            //

            unsigned int t_ctr_ = 0;
            std::set<double>::iterator otm_call_itm_put_itr_;
            for (otm_call_itm_put_itr_ = otm_call_itm_put_strike_price_set_.begin();
                 otm_call_itm_put_itr_ != otm_call_itm_put_strike_price_set_.end(); ++otm_call_itm_put_itr_) {
              t_ctr_++;
              double current_strike_price_ = *otm_call_itm_put_itr_;
              // ITM PUT
              t_temp_optname_ess_.str(std::string());
              t_temp_optname_oss_.str(std::string());

              t_temp_optname_oss_ << t_optname_base_oss_.str() << "_P"
                                  << GetMonthlyContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_, t_param_->underlying_)
                                  << "_I" << t_ctr_;

              t_temp_optname_ess_ << t_optname_base_oss_.str() << "_PE_" << t_param_->expiry_ << "_"
                                  << std::setprecision(2) << current_strike_price_;

	      if (ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str()) == "INVALID") continue; // INVALID exchange sym handling 

              t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                  t_param_->min_tick_, GetContractMultiplier(t_param_->inst_type_), kExchSourceCBOE, t_param_->lot_size_);

              shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                  ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());
              exchsymbol_2_shortcode_[ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str())] =
                  t_temp_optname_oss_.str();

	      shortcode_2_datasourcename_[t_temp_optname_oss_.str()] = 
		    exchsymbol_2_datasourcename_[shortcode_2_exchsymbol_[t_temp_optname_oss_.str()]];
              shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
              // store last close info per shortcode -- approximate.
              shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
              shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                  new CBOETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
              shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
              shortcode_2_strike_price_[t_temp_optname_oss_.str()] = current_strike_price_;

              //-------------------------
              // OTM Call
              t_temp_optname_oss_.str(std::string());
              t_temp_optname_ess_.str(std::string());

              t_temp_optname_oss_ << t_optname_base_oss_.str() << "_C"
                                  << GetMonthlyContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_, t_param_->underlying_)
                                  << "_O" << t_ctr_ ;

              t_temp_optname_ess_ << t_optname_base_oss_.str() << "_CE_" << t_param_->expiry_ << "_"
                                  << std::setprecision(2) << current_strike_price_;

	      if (ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str()) == "INVALID") continue; // INVALID exchange sym handling 

              t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                  t_param_->min_tick_, GetContractMultiplier(t_param_->inst_type_), kExchSourceCBOE, t_param_->lot_size_);

              shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                  ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());
              exchsymbol_2_shortcode_[ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str())] =
                  t_temp_optname_oss_.str();

	      shortcode_2_datasourcename_[t_temp_optname_oss_.str()] = 
		    exchsymbol_2_datasourcename_[shortcode_2_exchsymbol_[t_temp_optname_oss_.str()]];
              shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
              // store last close info per shortcode -- approximate.
              shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
              shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                  new CBOETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
              shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
              shortcode_2_strike_price_[t_temp_optname_oss_.str()] = current_strike_price_;

            }  // end of for loop

            std::set<double, std::greater<double>>::iterator otm_put_itm_call_itr_;
            t_ctr_ = 0;
            for (otm_put_itm_call_itr_ = otm_put_itm_call_strike_price_set_.begin();
                 otm_put_itm_call_itr_ != otm_put_itm_call_strike_price_set_.end(); ++otm_put_itm_call_itr_) {
              t_ctr_++;
              double current_strike_price_ = *otm_put_itm_call_itr_;

              // OTM Put
              t_temp_optname_oss_.str(std::string());
              t_temp_optname_ess_.str(std::string());

              t_temp_optname_oss_ << t_optname_base_oss_.str() << "_P"
                                  << GetMonthlyContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_, t_param_->underlying_)
                                  << "_O" << t_ctr_;

              t_temp_optname_ess_ << t_optname_base_oss_.str() << "_PE_" << t_param_->expiry_ << "_"
                                  << std::setprecision(2) << current_strike_price_;

	      if (ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str()) == "INVALID") continue; // INVALID exchange sym handling 

              t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                  t_param_->min_tick_, GetContractMultiplier(t_param_->inst_type_), kExchSourceCBOE, t_param_->lot_size_);

              shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                  ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());
              exchsymbol_2_shortcode_[ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str())] =
                  t_temp_optname_oss_.str();

	      shortcode_2_datasourcename_[t_temp_optname_oss_.str()] = 
		    exchsymbol_2_datasourcename_[shortcode_2_exchsymbol_[t_temp_optname_oss_.str()]];
              shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
              // store last close info per shortcode -- approximate.
              shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
              shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                  new CBOETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
              shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
              shortcode_2_strike_price_[t_temp_optname_oss_.str()] = current_strike_price_;

              //---------------------------

              // ITM Call
              t_temp_optname_oss_.str(std::string());
              t_temp_optname_ess_.str(std::string());
              t_temp_optname_oss_ << t_optname_base_oss_.str() << "_C"
                                  << GetMonthlyContractNumberFromExpiry(t_param_->expiry_, t_param_->inst_type_, t_param_->underlying_)
                                  << "_I" << t_ctr_;

              t_temp_optname_ess_ << t_optname_base_oss_.str() << "_CE_" << t_param_->expiry_ << "_"
                                  << std::setprecision(2) << current_strike_price_;

	      if (ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str()) == "INVALID") continue; // INVALID exchange sym handling 

              t_contract_spec_map_[t_temp_optname_oss_.str()] = ContractSpecification(
                  t_param_->min_tick_, GetContractMultiplier(t_param_->inst_type_), kExchSourceCBOE, t_param_->lot_size_);

              shortcode_2_exchsymbol_[t_temp_optname_oss_.str()] =
                  ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str());
              exchsymbol_2_shortcode_[ConvertDataSourceNametoExchSymbolSecDef(t_temp_optname_ess_.str())] =
                  t_temp_optname_oss_.str();

	      shortcode_2_datasourcename_[t_temp_optname_oss_.str()] = 
		    exchsymbol_2_datasourcename_[shortcode_2_exchsymbol_[t_temp_optname_oss_.str()]];
              shortcode_2_type_[t_temp_optname_oss_.str()] = t_param_->inst_type_;
              // store last close info per shortcode -- approximate.
              shortcode_2_last_close_[t_temp_optname_oss_.str()] = t_param_->last_close_;
              shortcode_2_price_limit_[t_temp_optname_oss_.str()] =
                  new CBOETradingLimit_t(-1, 100000000, -1, t_param_->min_tick_);
              shortcode_2_expiry_date_[t_temp_optname_oss_.str()] = t_param_->expiry_;
              shortcode_2_strike_price_[t_temp_optname_oss_.str()] = current_strike_price_;

            }  // end of for loop
          }
        }
    }
  }
  AddSpotIndexContractSpecifications(t_contract_spec_map_);
}

void CBOESecurityDefinitions::AddCBOECombContractSpecifications(ShortcodeContractSpecificationMap& t_contract_spec_map_,const std::string &shortcode_
    ,const std::string &exchange_symbol, const double &min_price_increment_
    ,const double &numbers_to_dollars_,const int& min_order_size_)
{
  std::string combo_shc_=shortcode_;
  std::transform(combo_shc_.begin(), combo_shc_.end(), combo_shc_.begin(), ::toupper);
  if (t_contract_spec_map_.find(combo_shc_) == t_contract_spec_map_.end()) {
    t_contract_spec_map_[combo_shc_] = ContractSpecification(
        min_price_increment_, numbers_to_dollars_, kExchSourceCBOE, min_order_size_);

    //std::cout << "Add String: " << spot_index_shc_ << " : " << exchange_symbol << std::endl;
    shortcode_2_exchsymbol_[combo_shc_] = exchange_symbol;
    exchsymbol_2_datasourcename_[exchange_symbol] = combo_shc_;
    datasourcename_2_exchsymbol_[combo_shc_] = exchange_symbol;
    exchsymbol_2_shortcode_[exchange_symbol] = combo_shc_;
    shortcode_2_expiry_date_[combo_shc_] = intdate_;
    shortcode_2_price_limit_[combo_shc_] = new CBOETradingLimit_t(-1, 100000000, -1, 0.01); //What to add here (Sanskar)

    shortcode_2_type_[combo_shc_] = HFSAT::CBOE_COMBO;
    shortcode_2_last_close_[combo_shc_] = -1; //Do not have it as of now (Sanskar) 
  }
}

void CBOESecurityDefinitions::AddSpotIndexContractSpecifications(ShortcodeContractSpecificationMap& t_contract_spec_map_) { // Add Spot shortcode handling below
  std::unordered_map<std::string, int> spot_index_2_token_map_ =
  HFSAT::CBOESpotTokenGenerator::GetUniqueInstance().GetSpotIndexToTokenMap();

  std::string spot_index_shc_;
  int token_;
  for (auto i = spot_index_2_token_map_.begin(); i != spot_index_2_token_map_.end(); i++) {
    spot_index_shc_ = i->first;
    std::transform(spot_index_shc_.begin(), spot_index_shc_.end(), spot_index_shc_.begin(), ::toupper);
    //std::cout << i->first << " : " << i->second << " => " << spot_index_shc_ << " " << spot_index_shc_ << std::endl;
    token_ = i->second;

    if (t_contract_spec_map_.find(spot_index_shc_) == t_contract_spec_map_.end()) {
      t_contract_spec_map_[spot_index_shc_] = ContractSpecification(
          0.01, GetContractMultiplier(HFSAT::CBOE_IDXSPT), kExchSourceCBOE, 1);

      std::ostringstream t_exch_sym_oss_;
      t_exch_sym_oss_ << "CBOE_IDX" << token_;

      char exchange_symbol[16];
      memset(exchange_symbol, '\0', sizeof(char) * 16);
      std::memcpy(exchange_symbol, t_exch_sym_oss_.str().c_str(),
                  std::min<unsigned int>(15, strlen(t_exch_sym_oss_.str().c_str())));

      //std::cout << "Add String: " << spot_index_shc_ << " : " << exchange_symbol << std::endl;
      shortcode_2_exchsymbol_[spot_index_shc_] = exchange_symbol;
      exchsymbol_2_datasourcename_[exchange_symbol] = spot_index_shc_;
      datasourcename_2_exchsymbol_[spot_index_shc_] = exchange_symbol;
      exchsymbol_2_shortcode_[exchange_symbol] = spot_index_shc_;
      shortcode_2_expiry_date_[spot_index_shc_] = intdate_;
      shortcode_2_price_limit_[spot_index_shc_] = new CBOETradingLimit_t(-1, 100000000, -1, 0.01);

      shortcode_2_type_[spot_index_shc_] = HFSAT::CBOE_IDXSPT;
      shortcode_2_last_close_[spot_index_shc_] = -1;
    }
  }

}

// file format CBOE0 SPX_CE_20220101_8050
void CBOESecurityDefinitions::LoadDataSourceExchangeSymbolMap() {
  std::string cboe_datasource_exchsymbol_filepath_ =
      std::string("/spare/local/tradeinfo/") + CBOE_DATASOURCE_EXCHSYMBOL_FILE;
  if (FileUtils::ExistsAndReadable(cboe_datasource_exchsymbol_filepath_)) {
    std::ifstream cboe_data_exch_file_;
    cboe_data_exch_file_.open(cboe_datasource_exchsymbol_filepath_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    std::ostringstream t_oss_;
    t_oss_ << intdate_;
    boost::gregorian::date d_today_(boost::gregorian::from_undelimited_string(t_oss_.str()));
    if (cboe_data_exch_file_.is_open()) {
      while (cboe_data_exch_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        cboe_data_exch_file_.getline(readline_buffer_, sizeof(readline_buffer_));
        PerishableStringTokenizer st_(readline_buffer_, sizeof(readline_buffer_));

        const std::vector<const char*>& tokens_ = st_.GetTokens();
        if (tokens_.size() == 2 && tokens_[0][0] != '#') {
          // file size can grow very large with time - we only need to load a small
          // sucboet of instruments - currently ones with expiry <= 3 months
          char datasource_name_[CBOE_DOTEX_OFFLINE_SYMBOL_LENGTH];
          strncpy(datasource_name_, tokens_[1], CBOE_DOTEX_OFFLINE_SYMBOL_LENGTH);
          std::vector<char*> tokens1_;
          PerishableStringTokenizer::NonConstStringTokenizer(datasource_name_, "_", tokens1_);
          if (tokens1_.size() >= 4) {
            boost::gregorian::date d_expiry_(boost::gregorian::from_undelimited_string(tokens1_[3]));

            if (d_expiry_ >= d_today_ && d_expiry_ - boost::gregorian::months(4) <= d_today_) {
              if (intdate_ >= USING_TBT_FROM &&
                  (strncmp(tokens1_[2], "CE", 2) == 0 || strncmp(tokens1_[2], "PE", 2) == 0)) {
                std::string data_source_name = std::string(tokens1_[0]) + "_" + std::string(tokens1_[1]) + "_" +
                                               std::string(tokens1_[2]) + "_" + std::string(tokens1_[4]) + "_" +
                                               std::string(tokens1_[3]);
                datasourcename_2_exchsymbol_[data_source_name] = tokens_[0];
                exchsymbol_2_datasourcename_[tokens_[0]] = data_source_name;
              } else {
                datasourcename_2_exchsymbol_[tokens_[1]] = tokens_[0];
                exchsymbol_2_datasourcename_[tokens_[0]] = tokens_[1];
              }
            }
          } else {
            datasourcename_2_exchsymbol_[tokens_[1]] = tokens_[0];
            exchsymbol_2_datasourcename_[tokens_[0]] = tokens_[1];
          }
        }
      }
    }
    cboe_data_exch_file_.close();
  } else {
    std::cerr << "Fatal error - could not read CBOE DataSource ExchSymbol file " << cboe_datasource_exchsymbol_filepath_
              << ".Exiting.\n";
    exit(0);
  }
}

int CBOESecurityDefinitions::GetExpiryFromShortCode(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  if (shortcode_2_expiry_date_.end() == shortcode_2_expiry_date_.find(local_shc))
    return -1;
  else
    return shortcode_2_expiry_date_[local_shc];
}

double CBOESecurityDefinitions::GetStrikePriceFromShortCode(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  if (IsOption(local_shc))
    return shortcode_2_strike_price_[local_shc];
  else
    return 0;
}

std::string CBOESecurityDefinitions::GetShortcodeFromCanonical(const std::string& ticker_, int expiry_dt_,
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

  datasource_sym_ << "CBOE_" << ticker_ << "_" << temp_type_ << "_" << temp_strike_.str() << "_" << expiry_dt_;

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


double CBOESecurityDefinitions::GetClosePriceFromShortCode(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  if (IsShortcode(local_shc))
    return shortcode_2_last_close_[local_shc];
  else
    return 0;
}

std::string CBOESecurityDefinitions::GetShortCodeFromExchangeSymbol(std::string& exch_symbol) {
  std::map<std::string, std::string>::iterator tmp_map_itr = exchsymbol_2_shortcode_.find(exch_symbol);
  if (tmp_map_itr != exchsymbol_2_shortcode_.end()) {
    return tmp_map_itr->second;
  } else {
    return "INVALID";
  }
}

bool CBOESecurityDefinitions::IsOption(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  if (shortcode_2_type_.end() == shortcode_2_type_.find(local_shc)) return false;
  CBOEInstrumentType_t inst_type = shortcode_2_type_[local_shc];
  return ((inst_type == CBOE_IDXOPT));
}


bool CBOESecurityDefinitions::IsMonthlyOption(const std::string& _shortcode_) {
  if (IsOption(_shortcode_)) {
    int expiry = GetExpiryFromShortCode(_shortcode_);
    if (expiry == -1)
      return false;
    else if (IsMonthlyOptionExpiry(expiry))
      return true;
  }
  return false;
}
bool CBOESecurityDefinitions::IsDailyOption(const std::string& _shortcode_) {
  char underlying_[50];
  strcpy(underlying_, _shortcode_.c_str());
  if (IsOption(_shortcode_)) {
    
    int expiry = GetExpiryFromShortCode(_shortcode_);
    if (expiry == -1)
      return false;
    else if (IsDailyOptionExpiry(expiry, underlying_))
      return true;
  }
  return false;
}


int CBOESecurityDefinitions::GetOptionType(const std::string& _shortcode_) {
  if (IsOption(_shortcode_)) {
    std::vector<std::string> tokens1_;
    split(_shortcode_, '_', tokens1_);
    if (tokens1_[2][0] == 'P')  // In Greek's Caculation Exec, put is enumerated as -1 while call as +1
      return -1;
    else
      return 1;
  } else
    return '0';
}

bool CBOESecurityDefinitions::IsShortcode(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  return (shortcode_2_exchsymbol_.find(local_shc) != shortcode_2_exchsymbol_.end()); 
}

CBOETradingLimit_t* CBOESecurityDefinitions::GetTradingLimits(const std::string& shortcode_) {
  if (shortcode_2_price_limit_.end() == shortcode_2_price_limit_.find(shortcode_)) {
    return NULL;
  }
  return shortcode_2_price_limit_[shortcode_];
}

CBOETradingLimit_t* CBOESecurityDefinitions::ChangeTradingLimits(const std::string& shortcode_, int int_px_to_cross_) {
  if (shortcode_2_price_limit_.end() == shortcode_2_price_limit_.find(shortcode_)) {
    return NULL;
  }

  CBOETradingLimit_t* t_trading_limit_ = shortcode_2_price_limit_[shortcode_];

  if (int_px_to_cross_ < t_trading_limit_->lower_limit_) {  // Lower limit crossed
    t_trading_limit_->lower_limit_fraction_ += 0.45;
    t_trading_limit_->lower_limit_ =
        ceil(t_trading_limit_->base_limit_ -
             t_trading_limit_->base_limit_ * t_trading_limit_->lower_limit_fraction_ - DOUBLE_PRECISION);
  } else {  // Upper limit crossed
    t_trading_limit_->upper_limit_fraction_ += 0.45;
    t_trading_limit_->upper_limit_ =
        int(t_trading_limit_->base_limit_ +
            t_trading_limit_->base_limit_ * t_trading_limit_->upper_limit_fraction_ + DOUBLE_PRECISION);
  }
  return t_trading_limit_;
}
}  // namespace HFSAT
