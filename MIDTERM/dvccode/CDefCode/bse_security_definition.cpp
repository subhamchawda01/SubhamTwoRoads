#include "dvccode/CDef/bse_security_definition.hpp"
#include "dvccode/CDef/common_security_definition_structs.hpp"
#include "dvccode/CDef/currency_convertor.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/Utils/bse_utils.hpp"
#include "dvccode/Utils/holiday_manager.hpp"

#include <boost/date_time/gregorian/gregorian.hpp>
#include <cstring>
#include <string>
#include <time.h>

namespace HFSAT {

#define SPARE_BSE_FOLDER_PREFIX "/spare/local/tradeinfo/BSE_Files/"
#define BSE_DATASOURCE_FILE_BACKUP_PATH "/spare/local/tradeinfo/BSE_Files/DataExchFile/"
#define BSE_DATASOURCE_TO_EXCH_SYM_FILE "datasource_exchsymbol.txt"
const boost::gregorian::date_duration one_day_duration_(1);
#define BSE_CONTRACT_MASTER_FILE_PREFIX "BSE_Files/BSEContractMaster/BFX_CO"

BSESecurityDefinitions* BSESecurityDefinitions::uniqueinstance_ = nullptr;

BSESecurityDefinitions::BSESecurityDefinitions(int date) : intdate_(date), max_exch_sym_id_(0) {
  CurrencyConvertor::SetUniqueInstance(date);
  PopulateExpiryMap();
  PopulateCurrencyExpiryMap();
  LoadInstrumentParams();
  ReadWriteDataSourceSymbolToExchSymbolMap();
}

void BSESecurityDefinitions::LoadInstrumentParams() {
  std::string contract_master_file_absolute_path_(
      "/spare/local/tradeinfo/BSE_Files/BSEContractMaster/bse_contract_master_futures");
  if (FileUtils::ExistsAndReadable(contract_master_file_absolute_path_)) {
    std::ifstream infile(contract_master_file_absolute_path_, std::ifstream::in);
    std::vector<std::string> params;

    if (infile.is_open()) {
      std::string line;
      while (infile.good()) {
        getline(infile, line);
        if (line.empty()) {
          break;
        }

        std::istringstream ss(line);
        params.clear();

        std::string field;
        while (getline(ss, field, ',')) {
          params.push_back(field);
        }

        if (params.size() >= 19) {
          BSEInstrumentParams inst_param;
          inst_param.instrument_code = std::atoi(params[0].c_str());
          inst_param.instrument_type = BSEInstrumentType::BSE_INVALID;
          if (params[6] == "FUTCUR") {
            inst_param.instrument_type = BSEInstrumentType::BSE_CURFUT;
          } else if (params[6] == "OPTCUR") {
            inst_param.instrument_type = BSEInstrumentType::BSE_CUROPT;
          } else if (params[6] == "FUTIRD") {
            inst_param.instrument_type = BSEInstrumentType::BSE_IRDFUT;
          } else if (params[6] == "FUTIRT") {
            inst_param.instrument_type = BSEInstrumentType::BSE_IRTFUT;
          }
          std::strcpy(inst_param.underlying, params[2].c_str());

          int day = std::atoi(params[18].substr(8, 2).c_str());
          int year = std::atoi(params[18].substr(0, 4).c_str());
          std::string month = params[18].substr(5, 2);
          int mon = std::atoi(month.c_str());
          /*if (month == "JAN") {
            mon = 1;
          } else if (month == "FEB") {
            mon = 2;
          } else if (month == "MAR") {
            mon = 3;
          } else if (month == "APR") {
            mon = 4;
          } else if (month == "MAY") {
            mon = 5;
          } else if (month == "JUN") {
            mon = 6;
          } else if (month == "JUL") {
            mon = 7;
          } else if (month == "AUG") {
            mon = 8;
          } else if (month == "SEP") {
            mon = 9;
          } else if (month == "OCT") {
            mon = 10;
          } else if (month == "NOV") {
            mon = 11;
          } else if (month == "DEC") {
            mon = 12;
          }*/
          inst_param.expiry = year * 10000 + mon * 100 + day;
          inst_param.strike_price = std::atoi(params[10].c_str()) / double(10000000);
          inst_param.option_type = BSEOptionType::OPT_INVALID;
          if (params[7] == "CE") {
            inst_param.option_type = BSEOptionType::OPT_CE;
          } else if (params[7] == "PE") {
            inst_param.option_type = BSEOptionType::OPT_PE;
          }
          inst_param.precision = std::atoi(params[9].c_str());
          inst_param.min_lot_size = std::atoi(params[11].c_str());
          inst_param.min_px_inc = std::atoi(params[13].c_str()) / double(10000000);
          inst_param.lot_size_multiplier = std::atoi(params[14].c_str());

          std::strcpy(inst_param.name, params[3].c_str());
          inst_param.last_close =
              0;  // [pjain] : In New Master Files, its value is is column 16. But it varies across master
                  // files for the same instrument. So for the time being I have removed column 16
                  // from the processed contract master file.

          instrument_code_2_params_[inst_param.instrument_code] = instrument_params_.size();
          instrument_params_.push_back(inst_param);
          /*std::cout << inst_param.ToString();
          std::cout << "Shortcode : " << GetShortcodeFromInstrumentCode(inst_param.instrument_code) << "\n";
          std::cout << "Data Source Symbol : " << GetDataSourceSymbolFromInstrumentCode(inst_param.instrument_code)
                    << "\n";*/
        }
      }
    }
  } else {
    std::cerr << typeid(*this).name() << ':' << __func__ << " "
              << " ERROR : Contract maste file " << contract_master_file_absolute_path_
              << " does not exist or readable.\n";
  }
}

void BSESecurityDefinitions::ReadWriteDataSourceSymbolToExchSymbolMap() {
  std::string mapping_filepath = std::string(SPARE_BSE_FOLDER_PREFIX) + std::string(BSE_DATASOURCE_TO_EXCH_SYM_FILE);
  // std::cout << "Datasource Exchange Symbol File Path : " << mapping_filepath << "\n";

  // 1. Read File and Fill Internal Data Structures "exch_sym_2_datasource_sym_" and "datasource_sym_2_exch_sym_"
  if (FileUtils::ExistsAndReadable(mapping_filepath)) {
    std::ifstream infile(mapping_filepath, std::ifstream::in);
    std::vector<std::string> params;

    if (infile.is_open()) {
      std::string line;
      while (infile.good()) {
        getline(infile, line);
        if (line.empty()) {
          break;
        }

        std::istringstream ss(line);
        params.clear();

        std::string field;
        while (getline(ss, field, ' ')) {
          params.push_back(field);
        }

        if (params.size() >= 2) {
          const std::string& exch_sym = params[0];
          const std::string& data_source_sym = params[1];
          // std::cout << "Exch Symbol : " << exch_sym << " Data Source Sym : " << data_source_sym << "\n";
          exch_sym_2_datasource_sym_[exch_sym] = data_source_sym;
          datasource_sym_2_exch_sym_[data_source_sym] = exch_sym;
          long long int exch_sym_id = 0;
          size_t last_index = exch_sym.find_last_not_of("0123456789");
          std::string id = exch_sym.substr(last_index + 1);
          if (!id.empty()) {
            exch_sym_id = std::atoll(id.c_str());
            if (exch_sym_id > max_exch_sym_id_) {
              max_exch_sym_id_ = exch_sym_id;
            }
          }
        }
      }
    }
    infile.close();
  } else {
    std::cerr << typeid(*this).name() << ':' << __func__ << " "
              << "BSE Datasource Symbol to Exchange Symbol file not exist/readable at location " << mapping_filepath
              << ". Exiting\n";
    exit(1);
  }

  // 2. Parse the fields of contract master file and see if some new datasource symbol is added. If its is added, backup
  // the contents of datasourcesymboltoexchsymbol file before modifying.
  bool is_backup_done = false;
  for (BSEInstrumentParams& param : instrument_params_) {
    if (param.instrument_type == BSEInstrumentType::BSE_CURFUT /*||
          param.instrument_type == BSEInstrumentType::BSE_IRDFUT ||
          param.instrument_type == BSEInstrumentType::BSE_IRTFUT*/) {
      const std::string data_source_sym = GetDataSourceSymbolFromInstrumentCode(param.instrument_code);
      if (!IsDataSourceSymbolValid(data_source_sym)) {
        if (!is_backup_done) {
          time_t time_midnight = HFSAT::DateTime::GetTimeMidnightUTC(intdate_);
          std::string backup_filepath = std::string(BSE_DATASOURCE_FILE_BACKUP_PATH) +
                                        std::string(BSE_DATASOURCE_TO_EXCH_SYM_FILE) + "." +
                                        std::to_string(time_midnight);
          // std::cout << "Backup File Path : " << backup_filepath << "\n";
          if (!BSE_UTILS::BackupFile(mapping_filepath, backup_filepath)) {
            std::cerr << typeid(*this).name() << ':' << __func__ << " "
                      << "Could not backup " << mapping_filepath << " to file " << backup_filepath << " . Exiting.\n";
            exit(1);
          }
          is_backup_done = true;
        }
        std::string new_exch_sym = "BSE_" + std::to_string(++max_exch_sym_id_);
        if (new_exch_sym.size() > 16) {
          std::cerr << typeid(*this).name() << ':' << __func__ << " "
                    << "Exch Symbol Size going beyond 16 chars. Exiting.\n";
          exit(1);
        }
        exch_sym_2_datasource_sym_[new_exch_sym] = data_source_sym;
        datasource_sym_2_exch_sym_[data_source_sym] = new_exch_sym;
        // std::cout << "NEW : Exch Symbol : " << new_exch_sym << " Data Source Sym : " << data_source_sym << "\n";
      }
    }
  }

  // 3. Update the data source to exch symbol mapping file in case of new data source symbols.
  if (is_backup_done) {
    std::ofstream outfile(mapping_filepath, std::ofstream::out);
    if (outfile.is_open()) {
      for (auto iter = exch_sym_2_datasource_sym_.begin(); iter != exch_sym_2_datasource_sym_.end(); iter++) {
        outfile << iter->first << " " << iter->second << "\n";
      }
      outfile.close();
    } else {
      std::cerr << typeid(*this).name() << ':' << __func__ << " "
                << "Could not write new Data Source Symbol : Exch Symbol mapping.";
    }
  }
}

void BSESecurityDefinitions::AddContractSpecifications(ShortcodeContractSpecificationMap& contract_spec_map) {
  for (BSEInstrumentParams& param : instrument_params_) {
    if (param.instrument_type == BSEInstrumentType::BSE_CURFUT /*||
        param.instrument_type == BSEInstrumentType::BSE_IRDFUT ||
        param.instrument_type == BSEInstrumentType::BSE_IRTFUT*/) {
      if (IsExpiryValid(param.expiry, param.instrument_type)) {
        const std::string& shortcode = GetShortcodeFromInstrumentCode(param.instrument_code);
        contract_spec_map[shortcode] = ContractSpecification(
            param.min_px_inc, param.lot_size_multiplier * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD),
            kExchSourceBSE, param.min_lot_size);
        shortcode_2_instrument_code[shortcode] = param.instrument_code;
        /*std::cout << "Contract Spec Map:\nShortcode : " << GetShortcodeFromInstrumentCode(param.instrument_code) <<
           "\n"
                  << "Min Px Inc : " << param.min_px_inc << "\n"
                  << "n2d : " << param.lot_size * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD) << "\n"
                  << "Lot Size : " << param.lot_size << "\n";*/
      }
    }
  }
}

void BSESecurityDefinitions::PopulateCurrencyExpiryMap() {
  // Support for only 3 out of 12 listed currently
  // std::cout << "Expiries for Currency : \n";
  for (uint32_t contract_num = 0; contract_num < 3; contract_num++) {
    contractnumber_cf_2_expirydate_[contract_num] = ComputeExpiryDate(contract_num, true);
    /*std::cout << "Contract Num : " << contract_num << "\n"
              << "Expiry : " << contractnumber_cf_2_expirydate_[contract_num] << "\n";*/
    expirydate_2_contractnumber_cf_[contractnumber_cf_2_expirydate_[contract_num]] = contract_num;
  }
}

void BSESecurityDefinitions::PopulateExpiryMap() {
  // 3 contracts outstanding in BSE
  // std::cout << "Expiries for !Currency : \n";
  for (uint32_t contract_num = 0; contract_num < 3; contract_num++) {
    contractnumber_2_expirydate_[contract_num] = ComputeExpiryDate(contract_num);
    /*std::cout << "Contract Num : " << contract_num << "\n"
              << "Expiry : " << contractnumber_2_expirydate_[contract_num] << "\n";*/
    expirydate_2_contractnumber_[contractnumber_2_expirydate_[contract_num]] = contract_num;
  }
}

int BSESecurityDefinitions::ComputeExpiryDate(const int contract_number, bool is_currency) {
  int contract_num = contract_number;
  int temp_date = intdate_;
  int expiry_date = 0;
  while (contract_num >= 0) {
    if (!is_currency) {
      expiry_date = ComputeNextExpiry(temp_date);
    } else {
      expiry_date = ComputeNextExpiryCurrency(temp_date);
    }
    contract_num--;
    if (contract_num >= 0)  // set date to 1st of next month
    {
      int month = (expiry_date % 10000) / 100;
      int year = expiry_date / 10000;
      if (month == 12) {
        temp_date = (year + 1) * 10000 + 101;
      } else {
        temp_date = year * 10000 + (month + 1) * 100 + 1;
      }
    }  // end if
  }    // end while
  return expiry_date;
}

int BSESecurityDefinitions::ComputeNextExpiryCurrency(const int date) {
  int month = (date % 10000) / 100;
  int year = date / 10000;

  boost::gregorian::date first_of_month(year, month, 1);
  boost::gregorian::date current_date = first_of_month.end_of_month();
  boost::gregorian::date::ymd_type ymd = current_date.year_month_day();

  int expiry_date = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;

  /// we need to find three working days
  for (int i = 0; i < 3; i++) {
    while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceBSEStr, expiry_date, true)) {
      current_date -= one_day_duration_;
      ymd = current_date.year_month_day();
      expiry_date = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
    }
    if (i < 2) {
      current_date -= one_day_duration_;
      ymd = current_date.year_month_day();
      expiry_date = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
    }
  }

  if (expiry_date >= date) {
    return expiry_date;
  } else  // set date to first of next month
  {
    int temp_date;
    if (month == 12) {
      temp_date = (year + 1) * 10000 + 101;
    } else {
      temp_date = year * 10000 + (month + 1) * 100 + 1;
    }
    return ComputeNextExpiryCurrency(temp_date);
  }
}

int BSESecurityDefinitions::ComputeNextExpiry(const int date) {
  int month = (date % 10000) / 100;
  int year = date / 10000;

  boost::gregorian::last_day_of_the_week_in_month lwdm(boost::gregorian::Thursday, month);
  boost::gregorian::date l_thursday = lwdm.get_date(year);
  boost::gregorian::date::ymd_type ymd = l_thursday.year_month_day();
  int expiry_date = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
  while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceBSEStr, expiry_date, true)) {
    l_thursday -= one_day_duration_;
    boost::gregorian::date::ymd_type ymd = l_thursday.year_month_day();
    expiry_date = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
  }

  if (expiry_date >= date) {
    return expiry_date;
  } else  // set date to first of next month
  {
    int temp_date;
    if (month == 12) {
      temp_date = (year + 1) * 10000 + 101;
    } else {
      temp_date = year * 10000 + (month + 1) * 100 + 1;
    }
    return ComputeNextExpiry(temp_date);
  }
}

int BSESecurityDefinitions::GetContractNumberFromExpiry(const int date, const BSEInstrumentType type) {
  if (type == HFSAT::BSEInstrumentType::BSE_CURFUT || type == HFSAT::BSEInstrumentType::BSE_CUROPT) {
    if (expirydate_2_contractnumber_cf_.find(date) != expirydate_2_contractnumber_cf_.end()) {
      return expirydate_2_contractnumber_cf_[date];
    }
  } else {
    if (expirydate_2_contractnumber_.find(date) != expirydate_2_contractnumber_.end()) {
      return expirydate_2_contractnumber_[date];
    }
  }

  std::cerr << typeid(*this).name() << ':' << __func__ << " "
            << "Fatal error - GetContractNumberFromExpiry called with invalid parameters " << date << " ";
  exit(0);
}

bool BSESecurityDefinitions::IsExpiryValid(const int expiry, const BSEInstrumentType type) {
  if (type == HFSAT::BSEInstrumentType::BSE_CURFUT || type == HFSAT::BSEInstrumentType::BSE_CUROPT) {
    return (expirydate_2_contractnumber_cf_.find(expiry) != expirydate_2_contractnumber_cf_.end());
  } else {
    return (expirydate_2_contractnumber_.find(expiry) != expirydate_2_contractnumber_.end());
  }
}

bool BSESecurityDefinitions::IsExpiryValidForInstrument(int instrument_code) {
  if (IsInstrumentValid(instrument_code)) {
    return IsExpiryValid(instrument_params_[instrument_code_2_params_[instrument_code]].expiry,
                         instrument_params_[instrument_code_2_params_[instrument_code]].instrument_type);
  }
  std::cerr << typeid(*this).name() << ':' << __func__ << " "
            << "Instrument " << instrument_code << "Invalid\n";
  return false;
}

const std::string BSESecurityDefinitions::GetShortcodeFromInstrumentCode(const int instrument_code) {
  if (instrument_code_2_params_.find(instrument_code) == instrument_code_2_params_.end() ||
      instrument_params_.size() < (instrument_code_2_params_[instrument_code] + 1)) {
    return "";
  }
  BSEInstrumentParams param = instrument_params_[instrument_code_2_params_[instrument_code]];
  std::ostringstream shortcode(std::string(""));
  // FIXME: Only Implemented for futures
  if (IsExpiryValid(param.expiry, param.instrument_type)) {
    if (param.instrument_type == BSEInstrumentType::BSE_CURFUT /*||
	          param.instrument_type == BSEInstrumentType::BSE_IRDFUT ||
	          param.instrument_type == BSEInstrumentType::BSE_IRTFUT*/) {
      shortcode << "BSE_" << param.underlying << "_FUT"
                << GetContractNumberFromExpiry(param.expiry, param.instrument_type);
    }
  }

  return shortcode.str();
}

const std::string BSESecurityDefinitions::GetDataSourceSymbolFromInstrumentCode(const int instrument_code) {
  if (instrument_code_2_params_.find(instrument_code) == instrument_code_2_params_.end() ||
      instrument_params_.size() < (instrument_code_2_params_[instrument_code] + 1)) {
    std::cerr << typeid(*this).name() << ':' << __func__ << " "
              << "Instrument Code " << instrument_code << " invalid. Exiting.";
    exit(1);
  }
  BSEInstrumentParams param = instrument_params_[instrument_code_2_params_[instrument_code]];
  std::ostringstream data_source_symbol;

  // FIXME: Only Implemented for futures
  if (param.instrument_type == BSEInstrumentType::BSE_CURFUT /*||
	        param.instrument_type == BSEInstrumentType::BSE_IRDFUT ||
	        param.instrument_type == BSEInstrumentType::BSE_IRTFUT*/) {
    data_source_symbol << "BSE_" << param.underlying << "_FUT_" << param.expiry;
  }

  return data_source_symbol.str();
}

short int BSESecurityDefinitions::GetPrecisionFromInstrumentCode(const int instrument_code) {
  int precision = 0;
  if (IsInstrumentValid(instrument_code)) {
    precision = instrument_params_[instrument_code_2_params_[instrument_code]].precision;
  }
  return precision;
}

const std::string BSESecurityDefinitions::GetDataSourceSymbolFromShortcode(const std::string& shortcode) {
  int instrument_code = GetInstrumentCodeFromShortcode(shortcode);
  if (IsInstrumentValid(instrument_code)) {
    return (GetDataSourceSymbolFromInstrumentCode(instrument_code));
  } else {
    std::cerr << typeid(*this).name() << ':' << __func__ << " "
              << "Instrument with Code " << instrument_code << "not present\n ";
    exit(1);
  }
}

const std::string BSESecurityDefinitions::GetDataSourceSymbolFromExchSymbol(const std::string& exch_symbol) {
  if (IsExchSymbolValid(exch_symbol)) {
    return exch_sym_2_datasource_sym_[exch_symbol];
  } else {
    std::cerr << typeid(*this).name() << ':' << __func__ << " "
              << "Exchange Symbol " << exch_symbol << " invalid. Exiting.\n";
    exit(1);
  }
}

const std::string BSESecurityDefinitions::GetExchSymbolFromShortcode(const std::string& shortcode) {
  const std::string& data_source_symbol = GetDataSourceSymbolFromShortcode(shortcode);
  if (IsDataSourceSymbolValid(data_source_symbol)) {
    return datasource_sym_2_exch_sym_[data_source_symbol];
  } else {
    std::cerr << typeid(*this).name() << ':' << __func__ << " "
              << "Data Source Symbol " << data_source_symbol << " not present. Exiting.\n";
    exit(1);
  }
}

const std::string BSESecurityDefinitions::GetExchangeSymbolFromInstumentCode(int instrument_code) {
  const std::string& shortcode = GetShortcodeFromInstrumentCode(instrument_code);
  return GetExchSymbolFromShortcode(shortcode);
}

inline int BSESecurityDefinitions::GetInstrumentCodeFromShortcode(const std::string& shortcode) {
  if (IsShortcodeValid(shortcode)) {
    return shortcode_2_instrument_code[shortcode];
  } else {
    std::cerr << typeid(*this).name() << ':' << __func__ << " "
              << "Invalid Shortcode " << shortcode << ". Exiting.\n";
    exit(1);
  }
}

double BSESecurityDefinitions::GetLastClose(const std::string& shortcode) {
  if (IsShortcodeValid(shortcode)) {
    return GetLastCloseFromInstrumentCode(shortcode_2_instrument_code[shortcode]);
  } else {
    std::cerr << typeid(*this).name() << ':' << __func__ << " "
              << "Invalid Shortcode " << shortcode << ". Exiting.\n";
    exit(1);
  }
}

double BSESecurityDefinitions::GetLastCloseFromInstrumentCode(const int instrument_code) {
  if (IsInstrumentValid(instrument_code)) {
    return instrument_params_[instrument_code_2_params_[instrument_code]].last_close;
  } else {
    std::cerr << typeid(*this).name() << ':' << __func__ << " "
              << "Instrument with Code " << instrument_code << "not present\n ";
    exit(1);
  }
}

double BSESecurityDefinitions::GetContractMultiplier(const std::string& shortcode) {
  if (IsShortcodeValid(shortcode)) {
    return GetContractMultiplierFromInstrumentCode(shortcode_2_instrument_code[shortcode]);
  } else {
    std::cerr << typeid(*this).name() << ':' << __func__ << " "
              << "Invalid Shortcode " << shortcode << ". Exiting.\n";
    exit(1);
  }
}

double BSESecurityDefinitions::GetContractMultiplierFromInstrumentCode(const int instrument_code) {
  if (IsInstrumentValid(instrument_code)) {
    return instrument_params_[instrument_code_2_params_[instrument_code]].lot_size_multiplier;
  } else {
    std::cerr << typeid(*this).name() << ':' << __func__ << " "
              << "Instrument with Code " << instrument_code << "not present\n ";
    exit(1);
  }
}

BSEInstrumentType BSESecurityDefinitions::GetInstrumentTypeFromShortcode(const std::string& shortcode) {
  int instrument_code = GetInstrumentCodeFromShortcode(shortcode);
  if (IsInstrumentValid(instrument_code)) {
    return instrument_params_[instrument_code_2_params_[instrument_code]].instrument_type;
  } else {
    std::cerr << typeid(*this).name() << ':' << __func__ << " "
              << "Instrument with Code " << instrument_code << "not present\n ";
    exit(1);
  }
}

double BSESecurityDefinitions::GetBSECommission(const std::string& shortcode) {
  if (IsShortcodeValid(shortcode)) {
    if (shortcode_2_commissions_.find(shortcode) != shortcode_2_commissions_.end()) {
      return shortcode_2_commissions_[shortcode];
    } else {
      double commission = 0;
      switch (GetInstrumentTypeFromShortcode(shortcode)) {
        case BSE_CURFUT:
          commission = 0;
          shortcode_2_commissions_[shortcode] = commission;
          break;
        case BSE_CUROPT:
          commission = 0;
          shortcode_2_commissions_[shortcode] = commission;
          break;
        case BSE_IRDFUT:
          commission = 0;
          shortcode_2_commissions_[shortcode] = commission;
          break;
        case BSE_IRTFUT:
          commission = 0;
          shortcode_2_commissions_[shortcode] = commission;
          break;
        default:
          std::cerr << typeid(*this).name() << ':' << __func__ << " "
                    << "GetBSECommission called with improper instrument type "
                    << GetInstrumentTypeFromShortcode(shortcode);
          exit(0);
          break;
      }
      return commission;
    }
  } else {
    std::cerr << typeid(*this).name() << ':' << __func__ << " "
              << "Invalid Shortcode " << shortcode << ". Exiting.\n";
    exit(1);
  }
}
}
