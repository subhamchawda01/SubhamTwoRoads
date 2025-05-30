#ifndef _BSE_SECURITY_DEF_
#define _BSE_SECURITY_DEF_

/**
   \file dvccode/CDef/nse_security_definition.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 354, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "dvccode/CDef/common_security_definition_structs.hpp"

namespace HFSAT {
enum BSEInstrumentType { BSE_INVALID = 0, BSE_CURFUT, BSE_CUROPT, BSE_IRDFUT, BSE_IRTFUT };
enum BSEOptionType { OPT_INVALID = 0, OPT_CE, OPT_PE };

typedef struct {
  std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "\n========= BSE Instrument Parameters =========\n\n";
    temp_oss << "Instrument Code : " << instrument_code << "\n";
    switch (instrument_type) {
      case BSE_CURFUT:
        temp_oss << "Instrument Type : "
                 << "CURFUT"
                 << "\n";
        break;
      case BSE_CUROPT:
        temp_oss << "Instrument Type : "
                 << "CUROPT"
                 << "\n";
        break;
      case BSE_IRDFUT:
        temp_oss << "Instrument Type : "
                 << "IRDFUT"
                 << "\n";
        break;
      case BSE_IRTFUT:
        temp_oss << "Instrument Type : "
                 << "IRTFUT"
                 << "\n";
        break;
      default:
        temp_oss << "Instrument Type : "
                 << "UNKNOWN"
                 << "\n";
        break;
    }
    temp_oss << "Underlying : " << underlying << "\n";
    temp_oss << "Name : " << name << "\n";
    temp_oss << "Expiry : " << expiry << "\n";
    temp_oss << "Strike Price : " << strike_price << "\n";
    temp_oss << "Last Close : " << last_close << "\n";
    switch (option_type) {
      case OPT_CE:
        temp_oss << "Option Type : "
                 << "CE"
                 << "\n";
        break;
      case OPT_PE:
        temp_oss << "Option Type : "
                 << "PE"
                 << "\n";
        break;
      default:
        temp_oss << "Option Type : "
                 << "Not an Option"
                 << "\n";
        break;
    }
    temp_oss << "Precision : " << precision << "\n";
    temp_oss << "Min Price Inc : " << min_px_inc << "\n";
    temp_oss << "Lot Size Multiplier : " << lot_size_multiplier << "\n";
    temp_oss << "Min Lot Size : " << min_lot_size << "\n";
    temp_oss << "\n====================================\n\n";
    return temp_oss.str();
  }
  int instrument_code;
  BSEInstrumentType instrument_type;
  char underlying[12];
  char name[27];
  int expiry;  // YYYYMMDD format
  double strike_price;
  double last_close;
  BSEOptionType option_type;
  short int precision;
  double min_px_inc;
  int lot_size_multiplier;
  int min_lot_size;

} BSEInstrumentParams;

class BSESecurityDefinitions {
 public:
  static BSESecurityDefinitions& GetUniqueInstance(int date);
  void AddContractSpecifications(ShortcodeContractSpecificationMap& contract_spec_map);

  const std::string GetShortcodeFromInstrumentCode(const int instrument_code);
  const std::string GetDataSourceSymbolFromInstrumentCode(const int instrument_code);
  const std::string GetDataSourceSymbolFromShortcode(const std::string& shortcode);
  const std::string GetDataSourceSymbolFromExchSymbol(const std::string& exch_symbol);
  const std::string GetExchSymbolFromShortcode(const std::string& shortcode);
  const std::string GetExchangeSymbolFromInstumentCode(int instrument_code);
  short int GetPrecisionFromInstrumentCode(const int instrument_code);
  bool IsExpiryValid(const int expiry, const BSEInstrumentType type);
  bool IsExpiryValidForInstrument(int instrument_code);
  inline bool IsInstrumentValid(int instrument_code);
  double GetLastClose(const std::string& shortcode);
  double GetContractMultiplier(const std::string& shortcode);
  double GetBSECommission(const std::string& shortcode);

 private:
  BSESecurityDefinitions(const BSESecurityDefinitions&) : intdate_(0), max_exch_sym_id_(0) {}
  explicit BSESecurityDefinitions(int date);
  BSESecurityDefinitions& operator=(const BSESecurityDefinitions&) { return *this; }

  void LoadInstrumentParams();
  void ReadWriteDataSourceSymbolToExchSymbolMap();

  void PopulateExpiryMap();
  void PopulateCurrencyExpiryMap();
  int ComputeExpiryDate(const int contract_number, bool is_currency = false);
  int ComputeNextExpiry(const int date);
  int ComputeNextExpiryCurrency(const int date);
  int GetContractNumberFromExpiry(const int date, const BSEInstrumentType type);
  double GetLastCloseFromInstrumentCode(const int instrument_code);
  double GetContractMultiplierFromInstrumentCode(const int instrument_code);

  int GetInstrumentCodeFromShortcode(const std::string& shortcode);
  BSEInstrumentType GetInstrumentTypeFromShortcode(const std::string& shortcode);
  inline bool IsShortcodeValid(const std::string& shortcode);
  inline bool IsDataSourceSymbolValid(const std::string& data_source_symbol);
  inline bool IsExchSymbolValid(const std::string& exch_symbol);

 public:
  static BSESecurityDefinitions& GetUniqueInstance();  // Hack. Avoid Using It.

 private:
  static BSESecurityDefinitions* uniqueinstance_;
  int intdate_;
  long long int max_exch_sym_id_;
  std::vector<BSEInstrumentParams> instrument_params_;
  std::map<uint32_t, int> contractnumber_cf_2_expirydate_;
  std::map<int, uint32_t> expirydate_2_contractnumber_cf_;
  std::map<uint32_t, int> contractnumber_2_expirydate_;
  std::map<int, uint32_t> expirydate_2_contractnumber_;
  std::map<int, uint32_t> instrument_code_2_params_;
  std::map<std::string, int> shortcode_2_instrument_code;
  std::map<std::string, std::string> datasource_sym_2_exch_sym_;
  std::map<std::string, std::string> exch_sym_2_datasource_sym_;
  std::map<std::string, double> shortcode_2_commissions_;
};

inline bool BSESecurityDefinitions::IsInstrumentValid(int instrument_code) {
  return instrument_code_2_params_.find(instrument_code) != instrument_code_2_params_.end();
}

inline bool BSESecurityDefinitions::IsShortcodeValid(const std::string& shortcode) {
  return (shortcode_2_instrument_code.find(shortcode) != shortcode_2_instrument_code.end());
}

inline bool BSESecurityDefinitions::IsDataSourceSymbolValid(const std::string& data_source_symbol) {
  return (datasource_sym_2_exch_sym_.find(data_source_symbol) != datasource_sym_2_exch_sym_.end());
}

inline bool BSESecurityDefinitions::IsExchSymbolValid(const std::string& exch_symbol) {
  return (exch_sym_2_datasource_sym_.find(exch_symbol) != exch_sym_2_datasource_sym_.end());
}

inline BSESecurityDefinitions& BSESecurityDefinitions::GetUniqueInstance(int date) {
  if (uniqueinstance_ == nullptr) {
    uniqueinstance_ = new BSESecurityDefinitions(date);
  }
  return *(uniqueinstance_);
}

// This is a hack. This is required because all classes which call GetUniqueInstance() of BSESECURITYDEFINITIONS class
// do not have date with them. They rely upon the fact that the class object has already been created. AVOID USING AS
// FAR AS POSSIBLE. Always try to use this version : " BSESecurityDefinitions& GetUniqueInstance(int date) "
inline BSESecurityDefinitions& BSESecurityDefinitions::GetUniqueInstance() {
  if (uniqueinstance_ == nullptr) {
    std::cerr << "Can't create an instance of BSESecurityDefinition class without trading_date. Exiting.\n";
    exit(1);
  }
  return *(uniqueinstance_);
}
}

#endif
