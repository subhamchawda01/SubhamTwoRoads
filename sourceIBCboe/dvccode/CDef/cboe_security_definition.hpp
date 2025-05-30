/**
   \file dvccode/CDef/cboe_security_definition.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#pragma once

#include <set>
#include <vector>
#include <tr1/unordered_map>
#include <boost/date_time/gregorian/gregorian.hpp>
#include "dvccode/CDef/common_security_definition_structs.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/Utils/holiday_manager_utils.hpp"
#include "dvccode/Utils/cboe_spot_token_generator.hpp"
#define DOUBLE_PRECISION 0.000001

namespace HFSAT {

enum CBOEInstrumentType_t {
  CBOE_INVALID = 0,
  CBOE_IDXOPT,
  CBOE_IDXSPT,
  CBOE_COMBO,
};  

enum CBOEMonth_t {
  CBOE_INVALID_MM = 0,
  CBOE_JAN,
  CBOE_FEB,
  CBOE_MAR,
  CBOE_APR,
  CBOE_MAY,
  CBOE_JUN,
  CBOE_JUL,
  CBOE_AUG,
  CBOE_SEP,
  CBOE_OCT,
  CBOE_NOV,
  CBOE_DEC,
};

typedef struct {
  CBOEInstrumentType_t inst_type_;
  char underlying_[16];
  double last_close_;
  int lot_size_;
  double min_tick_;
  int expiry_;  // YYYYMMDD format
  // only applicable for options
  uint32_t num_itm_;  // num_otm is symmetric
  double step_value_;
} CBOEInstrumentParams_t;

// using string instead of char* because of the < comparator, which would compare char pointers
struct CBOEOPtionsIdentifier_t {
  CBOEInstrumentType_t inst_type_;
  std::string underlying_;
  int expiry_;  // YYYYMMDD format
  double strike_price_;
  std::string call_or_put_;

  // Define the == operator to compare two CBOEOPtionsIdentifier_t objects
  bool operator==(const CBOEOPtionsIdentifier_t& other) const {
    return inst_type_ == other.inst_type_ &&
           underlying_ == other.underlying_ &&
           expiry_ == other.expiry_ &&
           strike_price_ == other.strike_price_ &&
           call_or_put_ == other.call_or_put_;
  }
};

struct CBOETradingLimit_t {
  int lower_limit_;
  int upper_limit_;
  double lower_limit_fraction_;
  double upper_limit_fraction_;
  int base_limit_;
  double min_tick_size_;

  CBOETradingLimit_t(int _lower_limit_, int _upper_limit_, int _base_limit_, double _min_tick_size_) {
    lower_limit_ = _lower_limit_;
    upper_limit_ = _upper_limit_;
    base_limit_ = _base_limit_;
    min_tick_size_ = _min_tick_size_;

    // This is the initial fraction on which limits are decided. These will be used when we have
    // to increase the limits. Since that will be happen in case of FO and FO stocks for which the
    // initial limits are 10% (0.1), so keeping these values as same
    lower_limit_fraction_ = 0.1;
    upper_limit_fraction_ = 0.1;
  }
};

class CBOESecurityDefinitions {
 private:
  explicit CBOESecurityDefinitions(int t_intdate_);
  static CBOESecurityDefinitions* p_uniqueinstance_;

  static int intdate_;
  static std::vector<CBOEInstrumentParams_t*> inst_params_;
  static std::vector<CBOEOPtionsIdentifier_t> options_param_;

  static std::map<std::string, CBOETradingLimit_t*> shortcode_2_price_limit_;

  static std::map<int, uint32_t> expirydate_2_contractnumber_third_friday_;
  static std::map<int, uint32_t> daily_option_expirydate_2_contractnumber_;
  static std::map<uint32_t, int> contractnumber_2_expirydate_third_friday_;
  static std::map<uint32_t, int> daily_option_contractnumber_2_expirydate_;

  static std::map<std::string, std::string> shortcode_2_exchsymbol_;
  static std::map<std::string, std::string> exchsymbol_2_shortcode_;
  static std::map<std::string, std::string> shortcode_2_datasourcename_;
  static std::map<std::string, std::string> datasourcename_2_exchsymbol_;
  static std::map<std::string, std::string> exchsymbol_2_datasourcename_;

  static std::map<std::string, CBOEInstrumentType_t> shortcode_2_type_;
  static std::map<std::string, double> shortcode_2_commissions_;

  static std::map<std::string, double> shortcode_2_last_close_;
  static std::map<std::string, int> shortcode_2_expiry_date_;
  static std::map<std::string, double> shortcode_2_strike_price_;

 public:
  static inline CBOESecurityDefinitions& GetUniqueInstance(int t_intdate_) {
    if (p_uniqueinstance_ == NULL) {
      p_uniqueinstance_ = new CBOESecurityDefinitions(t_intdate_);
    }
    return *(p_uniqueinstance_);
  }
  
  static char GetSegmentTypeFromShortCode(std::string const& shortcode) {
    if (shortcode_2_type_.end() != shortcode_2_type_.find(shortcode)) {
      CBOEInstrumentType_t inst_type = shortcode_2_type_[shortcode];
      // std::cout<<"GetSegmentTypeFromShortCode "<<shortcode<<" "<<inst_type<<"\n";
      if (CBOE_IDXOPT == inst_type)
        return CBOE_FO_SEGMENT_MARKING;
      else if (CBOE_IDXSPT == inst_type)
	      return CBOE_IX_SEGMENT_MARKING;
      else if (CBOE_COMBO == inst_type)
        return CBOE_COMBO_SEGMENT_MARKING;
    }
      // std::cout<<shortcode<<"Not found in map"<<"\n";

    return CBOE_INVALID_SEGMENT;
  }

  static CBOEInstrumentType_t GetInstrumentTypeFromShortCode(std::string const& shortcode) {
    if (shortcode_2_type_.end() != shortcode_2_type_.find(shortcode)) {
      CBOEInstrumentType_t inst_type = shortcode_2_type_[shortcode];
      return inst_type;
    }
    return CBOE_INVALID;
  }

  static void AddDataSourceSymbolFromBhavCopy(CBOEInstrumentType_t inst_type_, std::string underlying_, int exp_date_,
                                              double strike_px_, std::string call_or_put_);

  static std::map<std::string, double>& GetShortcode2StrikePriceMap() { return shortcode_2_strike_price_; }

  static void AddCBOEContractSpecifications(ShortcodeContractSpecificationMap& t_contract_spec_map_);
  static void AddCBOECombContractSpecifications(ShortcodeContractSpecificationMap& t_contract_spec_map_,const std::string &shortcode_
    ,const std::string &exchange_symbol, const double &min_price_increment_,const double &numbers_to_dollars_,const int& min_order_size_);
  static void AddSpotIndexContractSpecifications(ShortcodeContractSpecificationMap& t_contract_spec_map_);

  static bool IsMonthlyOptionExpiry(const int yyyymmdd, char * underlying_ = NULL);
  // populate contractnumber to expiry map
  static void PopulateThirdFridayExpiryMap();
  // YYYYMMDD for ith expiry from current t_intdate_
  static int ComputeThirdFridayExpiryDate(const int expiry_number_);
  // YYYYMMDD for next contract expiry >= t_date_
  static int ComputeNextThirdFridayExpiry(const int t_date_);

  static void PopulateDailyOptionsExpiryMap();
  static bool IsDailyOptionExpiry(const int yyyymmdd, char *underlying = NULL);

  // method to read instrument params - currently from a dated file
  static void LoadInstParams(const int t_date_);

  // method to read options details from bhavcopy
  static void LoadBhavCopyParams(const int t_date_);

  // read file for mapping between exchange symbol ( ORS ) and datasourcename ( MDS )
  static void LoadDataSourceExchangeSymbolMap();
  
  // Map access for DS -> Exch Symbol and the reverse mapping is abstracted out for robustness
  static std::string ConvertDataSourceNametoExchSymbol(const std::string t_dsname_str_);
  static std::string ConvertDataSourceNametoExchSymbolSecDef(const std::string t_dsname_str_);
  static std::string ConvertExchSymboltoDataSourceName(const std::string t_exch_);

  // convert string to enum
  static CBOEInstrumentType_t StringToInst(const char* str);

  // Exchange Symbol function for CBOE
  static std::string GetExchSymbolCBOE(const std::string& _shortcode_);
  static std::string GetDatasourcenameCBOE(const std::string& _shortcode_);
  // Commission function for CBOE - returns total commissions in rupees per unit transacted
  static double GetCBOECommission(std::string _shortcode_);

  // Returns the  contract number for the expirydate
  static int GetMonthlyContractNumberFromExpiry(const int t_date_, const CBOEInstrumentType_t t_inst_type_, char* underlying);
  static int GetDailyContractNumberFromExpiry(const int t_date_, char* underlying);

  // Returns contract multiplier
  static double GetContractMultiplier(const CBOEInstrumentType_t t_inst_type_);
  // wrapper for GetContractMultiplier being called from outside class
  static double GetContractMultiplier(const std::string& _shortcode_);

  // Returns date for bhavcopy date format 13 OCT 2022 to 20221013
  static std::string GetDateInYYYYMMDD(char* _date_str_);

  // returns month enum for input month string
  static CBOEMonth_t StringToMonthNumber(const char* _cstr_);

  static bool IsOptionStrikePresentInBhavCopy(CBOEInstrumentParams_t param, double strike_price);

  // Returns expiry date from shortcode
  static int GetExpiryFromShortCode(const std::string& _shortcode_);

  // Returns Strike Price from shortcode and vice versa
  static double GetStrikePriceFromShortCode(const std::string& _shortcode_);
  static std::string GetShortcodeFromCanonical(const std::string& ticker_, int expiry_dt_, double strike_,
                                               bool is_call_);

  // Return Close Price from shortcode
  static double GetClosePriceFromShortCode(const std::string& _shortcode_);

  static bool IsOption(const std::string& _shortcode_);
  static int GetOptionType(const std::string& _shortcode_);
  static bool IsMonthlyOption(const std::string& _shortcode_);
  static bool IsDailyOption(const std::string& _shortcode_);
  static bool IsShortcode(const std::string& _shortcode_);
  static std::string GetShortCodeFromExchangeSymbol(std::string& _shortcode_);
  static CBOETradingLimit_t* GetTradingLimits(const std::string& shortcode_);
  static CBOETradingLimit_t* ChangeTradingLimits(const std::string& shortcode_, int int_px_to_cross_);

  static void split(const std::string& s, char delim, std::vector<std::string>& elems);  
};
}
