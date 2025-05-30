/**
   \file dvccode/CDef/nse_security_definition.hpp

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
#include <boost/date_time/gregorian/gregorian.hpp>
#include "dvccode/CDef/common_security_definition_structs.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/Utils/holiday_manager_utils.hpp"
#include "dvccode/Utils/spot_token_generator.hpp"
#define DOUBLE_PRECISION 0.000001
// Mainly to deal with changing contract specification issues of
// Option contracts listed on NSE

// TODO - add support for spread contracts
namespace HFSAT {
enum NSEInstrumentType_t {
  NSE_INVALID = 0,
  NSE_IDXFUT,
  NSE_STKFUT,
  NSE_IDXOPT,
  NSE_STKOPT,
  NSE_IRFFUT,
  NSE_CURFUT,
  NSE_CUROPT,
  NSE_EQ,
  NSE_IDXSPT,
};

enum NSEMonth_t {
  NSE_INVALID_MM = 0,
  NSE_JAN,
  NSE_FEB,
  NSE_MAR,
  NSE_APR,
  NSE_MAY,
  NSE_JUN,
  NSE_JUL,
  NSE_AUG,
  NSE_SEP,
  NSE_OCT,
  NSE_NOV,
  NSE_DEC,
};

typedef struct {
  NSEInstrumentType_t inst_type_;
  char underlying_[16];
  double last_close_;
  int lot_size_;
  double min_tick_;
  int expiry_;  // YYYYMMDD format
  // only applicable for options
  uint32_t num_itm_;  // num_otm is symmetric
  double step_value_;
  double step_value_prev_;
  uint32_t prev_num_itm_;
} NSEInstrumentParams_t;

// using string instead of char* because of the < comparator, which would compare char pointers
struct NSEOPtionsIdentifier_t {
  NSEInstrumentType_t inst_type_;
  std::string underlying_;
  int expiry_;  // YYYYMMDD format
  double strike_price_;
  std::string call_or_put_;

  bool operator<(const NSEOPtionsIdentifier_t& param) const {
    return std::tie(inst_type_, underlying_, expiry_, strike_price_, call_or_put_) <
           std::tie(param.inst_type_, param.underlying_, param.expiry_, param.strike_price_, param.call_or_put_);
  }
};

struct NSEOptionMetaInfo {
  std::string shortcode;
  double strike;
  bool is_call;
  bool is_curr_stk_scheme;
  int strike_num;
  bool is_in_the_money;
  double unbounded_intrinsic_value_;

  NSEOptionMetaInfo(std::string shc, double strike_px, bool call, bool is_curr_sos, int stk_num, bool is_itm,
                    double in_value_) {
    shortcode = shc;
    strike = strike_px;
    is_call = call;
    is_curr_stk_scheme = is_curr_sos;
    strike_num = stk_num;
    is_in_the_money = is_itm;
    unbounded_intrinsic_value_ = in_value_;
  }

  bool operator==(const NSEOptionMetaInfo& info) { return (shortcode == info.shortcode); }

  // ascending order for call & descending order puts
  // returns calls and then puts
  bool operator<(const NSEOptionMetaInfo& info) const {
    if (is_call && info.is_call) {
      return (strike < info.strike);
    } else if (is_call && (!info.is_call)) {
      return (true);
    } else if ((!is_call) && (info.is_call)) {
      return (false);
    } else if ((!is_call) && (!info.is_call)) {
      return (strike > info.strike);
    } else {
      return true;
    }
  }
};

struct NSETradingLimit_t {
  int lower_limit_;
  int upper_limit_;
  double lower_limit_fraction_;
  double upper_limit_fraction_;
  int base_limit_;
  double min_tick_size_;

  NSETradingLimit_t(int _lower_limit_, int _upper_limit_, int _base_limit_, double _min_tick_size_) {
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

class NSESecurityDefinitions {
 private:
  explicit NSESecurityDefinitions(int t_intdate_);
  static NSESecurityDefinitions* p_uniqueinstance_;

  static int intdate_;
  static std::vector<NSEInstrumentParams_t*> inst_params_;
  // Vector to store all products with non zero OI. Populated from bhavcopy
  static std::vector<std::string> non_zero_OI_;
  static std::map<NSEOPtionsIdentifier_t, double> options_param_2_last_close_;
  static std::map<std::string, double> shortcode_2_options_last_close_;

  //
  static std::map<std::string, NSETradingLimit_t*> shortcode_2_price_limit_;
  static std::map<std::string, int > shortcode_2_surveillance_id_;
  static std::vector<std::string> banned_prod_list_;

  // Bond Futures & FO have similar dates whereas currency futures has different dates
  static std::map<int, uint32_t> expirydate_2_contractnumber_;
  static std::map<int, uint32_t> expirydate_2_contractnumber_cf_;
  static std::map<int, uint32_t> weekly_option_expirydate_2_contractnumber_;
  static std::map<uint32_t, int> contractnumber_2_expirydate_;
  static std::map<uint32_t, int> contractnumber_cf_2_expirydate_;
  static std::map<uint32_t, int> weekly_option_contractnumber_2_expirydate_;
  
  static std::map<int, uint32_t> expirydate_2_contractnumber_tuesday_;
  static std::map<int, uint32_t> weekly_option_expirydate_2_contractnumber_tuesday_;
  static std::map<uint32_t, int> contractnumber_2_expirydate_tuesday_;
  static std::map<uint32_t, int> weekly_option_contractnumber_2_expirydate_tuesday_;

  static std::map<int, uint32_t> expirydate_2_contractnumber_wednesday_;
  static std::map<int, uint32_t> weekly_option_expirydate_2_contractnumber_wednesday_;
  static std::map<uint32_t, int> contractnumber_2_expirydate_wednesday_;
  static std::map<uint32_t, int> weekly_option_contractnumber_2_expirydate_wednesday_;

  static std::map<int, uint32_t> expirydate_2_contractnumber_friday_;
  static std::map<int, uint32_t> weekly_option_expirydate_2_contractnumber_friday_;
  static std::map<uint32_t, int> contractnumber_2_expirydate_friday_;
  static std::map<uint32_t, int> weekly_option_contractnumber_2_expirydate_friday_;

  static std::map<int, uint32_t> expirydate_2_contractnumber_monday_;
  static std::map<int, uint32_t> weekly_option_expirydate_2_contractnumber_monday_;
  static std::map<uint32_t, int> contractnumber_2_expirydate_monday_;
  static std::map<uint32_t, int> weekly_option_contractnumber_2_expirydate_monday_;

  static std::map<std::string, std::string> shortcode_2_exchsymbol_;
  static std::map<std::string, std::string> datasourcename_2_exchsymbol_;
  static std::map<std::string, std::string> exchsymbol_2_datasourcename_;
  static std::map<std::string, std::string> exchsymbol_2_datasourcesym_;
  static std::map<std::string, int> underlying_2_itr_num;
  // Introducing this since 1-to-1 mapping between shortcode and exchsymbol
  static std::map<std::string, std::string> exchsymbol_2_shortcode_;
  static std::map<std::string, std::string> exchsymbol_2_shortcode_Weekly;
  // For supporting all options shortcodes with non-zero OI
  // More specific to MidTerm usage -> Accessing these can be latency sensitive
  static std::map<std::string, std::string> shortcode_2_exchsymbol_all_;
  static std::map<std::string, std::string> exchsymbol_2_datasourcename_all_;

  // For supporting Commission calculation
  static std::map<std::string, NSEInstrumentType_t> shortcode_2_type_;
  static std::map<std::string, NSEInstrumentType_t> generic_shortcode_2_type_;
  static std::map<std::string, double> shortcode_2_commissions_;

  static std::map<std::string, bool> shortcode_2_is_spread_;
  static std::map<std::string, bool> exchange_symbol_2_is_spread_;

  // shortcode to last close. Used for uts based on notional in paramset.
  static std::map<std::string, double> shortcode_2_last_close_;
  static std::map<std::string, int> shortcode_2_expiry_date_;
  static std::map<std::string, int> datasource_2_openInterest_;
  static std::map<std::string, double> shortcode_2_strike_price_;
  static std::map<std::string, double> shortcode_2_step_value_;

  // OptionMetaData
  static std::map<std::string, std::vector<NSEOptionMetaInfo>> underlying_to_options_;

  static std::map<std::string, std::tuple<int, char, double>> underlying_2_corporate_action_value_;

 public:
  static inline NSESecurityDefinitions& GetUniqueInstance(int t_intdate_) {
    if (p_uniqueinstance_ == NULL) {
      p_uniqueinstance_ = new NSESecurityDefinitions(t_intdate_);
    }
    return *(p_uniqueinstance_);
  }

  static void AddDataSourceSymbolFromBhavCopy(NSEInstrumentType_t inst_type_, std::string underlying_, int exp_date_,
                                              double strike_px_, std::string call_or_put_, double last_close_px_,
                                              int _open_int_);

  static double GetAdjustedValue(std::string underlying_, double value_, double multiple = 0.05) {
    char operation_ = std::get<1>(underlying_2_corporate_action_value_[underlying_]);
    double corporate_action_value_ = std::get<2>(underlying_2_corporate_action_value_[underlying_]);
    double adjusted_value_;
    double temp_adj_value_;
    switch (operation_) {
      case '-': {
        temp_adj_value_ = value_ - corporate_action_value_;
      } break;
      case '+': {
        temp_adj_value_ = value_ + corporate_action_value_;
      } break;
      case '/': {
        temp_adj_value_ = value_ / corporate_action_value_;
      } break;
      case '*': {
        temp_adj_value_ = value_ * corporate_action_value_;
      } break;
      default: { temp_adj_value_ = value_; } break;
    }
    adjusted_value_ = std::round(temp_adj_value_ / multiple) * multiple;
    return adjusted_value_;
  }

  static char GetSegmentTypeFromShortCode(std::string const& shortcode) {
    if (shortcode_2_type_.end() != shortcode_2_type_.find(shortcode)) {
      NSEInstrumentType_t inst_type = shortcode_2_type_[shortcode];

      if (NSE_IDXFUT == inst_type || NSE_STKFUT == inst_type || NSE_IDXOPT == inst_type || NSE_STKOPT == inst_type)
        return NSE_FO_SEGMENT_MARKING;

      else if (NSE_IRFFUT == inst_type || NSE_CURFUT == inst_type || NSE_CUROPT == inst_type)
        return NSE_CD_SEGMENT_MARKING;

      else if (NSE_EQ == inst_type)
        return NSE_EQ_SEGMENT_MARKING;
      else if (NSE_IDXSPT == inst_type)
	return NSE_IX_SEGMENT_MARKING;
    } else if (generic_shortcode_2_type_.find(shortcode) != generic_shortcode_2_type_.end()) {
      return NSE_FO_SEGMENT_MARKING;
    }
    return NSE_INVALID_SEGMENT;
  }

  static NSEInstrumentType_t GetInstrumentTypeFromShortCode(std::string const& shortcode) {
    if (shortcode_2_type_.end() != shortcode_2_type_.find(shortcode)) {
      NSEInstrumentType_t inst_type = shortcode_2_type_[shortcode];
      return inst_type;
    }
    return NSE_EQ;
  }

  static std::map<std::string, double>& GetShortcode2StrikePriceMap() { return shortcode_2_strike_price_; }

  // adds contract specs for NSE IDX/STK FUT/OPT
  static void AddNSEContractSpecifications(ShortcodeContractSpecificationMap& t_contract_spec_map_);
  
  static void AddEquityContractSpecifications(ShortcodeContractSpecificationMap& t_contract_spec_map_);
  static void AddSpotIndexContractSpecifications(ShortcodeContractSpecificationMap& t_contract_spec_map_);

  // populate contractnumber to expiry map
  static void LoadIndexExpiries(const int intdate_);
  static void PopulateExpiryMap();
  static void PopulateMondayExpiryMap();
  static void PopulateTuesdayExpiryMap();
  static void PopulateWednesdayExpiryMap();
  static void PopulateFridayExpiryMap();
  // YYYYMMDD for ith expiry from current t_intdate_
  static int ComputeExpiryDate(const int expiry_number_, bool is_currency = false);
  static int ComputeMondayExpiryDate(const int expiry_number_, bool is_currency = false);
  static int ComputeTuesdayExpiryDate(const int expiry_number_, bool is_currency = false);
  static int ComputeWednesdayExpiryDate(const int expiry_number_, bool is_currency = false);
  static int ComputeFridayExpiryDate(const int expiry_number_, bool is_currency = false);
  // YYYYMMDD for next contract expiry >= t_date_
  static int ComputeNextExpiry(const int t_date_);
  static int ComputeMondayNextExpiry(const int t_date_);
  static int ComputeTuesdayNextExpiry(const int t_date_);
  static int ComputeWednesdayNextExpiry(const int t_date_);
  static int ComputeFridayNextExpiry(const int t_date_);

  // YYYYMMDD for previous contract expiry date
  static int ComputePreviousExpiry(const int t_date_, const std::string& _shortcode_);
  static int ComputeMondayPreviousExpiry(const int t_date_, const std::string& _shortcode_);
  static int ComputeTuesdayPreviousExpiry(const int t_date_, const std::string& _shortcode_);
  static int ComputeWednesdayPreviousExpiry(const int t_date_, const std::string& _shortcode_);
  static int ComputeFridayPreviousExpiry(const int t_date_, const std::string& _shortcode_);

  // expiry functions for currency futures
  static void PopulateCurrencyExpiryMap();
  static int ComputeNextExpiryCurrency(const int t_date_);

  static void PopulateWeeklyOptionsExpiryMap();
  static void PopulateMondayWeeklyOptionsExpiryMap();
  static void PopulateTuesdayWeeklyOptionsExpiryMap();
  static void PopulateWednesdayWeeklyOptionsExpiryMap();
  static void PopulateFridayWeeklyOptionsExpiryMap();
  static int ComputeExpiryDateWeeklyOptions(const int expiry_number, const std::set<int>& monthly_future_expiries);
  static int ComputeMondayExpiryDateWeeklyOptions(const int expiry_number, const std::set<int>& monthly_future_expiries);
  static int ComputeTuesdayExpiryDateWeeklyOptions(const int expiry_number, const std::set<int>& monthly_future_expiries);
  static int ComputeWednesdayExpiryDateWeeklyOptions(const int expiry_number, const std::set<int>& monthly_future_expiries);
  static int ComputeFridayExpiryDateWeeklyOptions(const int expiry_number, const std::set<int>& monthly_future_expiries);
  static int ComputeNextExpiryWeeklyOptions(const int t_date_, const std::set<int>& monthly_future_expiries);
  static int ComputeMondayNextExpiryWeeklyOptions(const int t_date_, const std::set<int>& monthly_future_expiries);
  static int ComputeTuesdayNextExpiryWeeklyOptions(const int t_date_, const std::set<int>& monthly_future_expiries);
  static int ComputeWednesdayNextExpiryWeeklyOptions(const int t_date_, const std::set<int>& monthly_future_expiries);
  static int ComputeFridayNextExpiryWeeklyOptions(const int t_date_, const std::set<int>& monthly_future_expiries);
  static bool IsWeeklyOptionExpiry(const int yyyymmdd, char * underlying_);
  static bool IsMondayWeeklyOptionExpiry(const int yyyymmdd);
  static bool IsTuesdayWeeklyOptionExpiry(const int yyyymmdd);
  static bool IsWednesdayWeeklyOptionExpiry(const int yyyymmdd);
  static bool IsFridayWeeklyOptionExpiry(const int yyyymmdd);
  static bool IsMonthlyOptionExpiry(const int yyyymmdd, char * underlying_);
  static bool IsMondayMonthlyOptionExpiry(const int yyyymmdd);
  static bool IsTuesdayMonthlyOptionExpiry(const int yyyymmdd);
  static bool IsWednesdayMonthlyOptionExpiry(const int yyyymmdd);
  static bool IsFridayMonthlyOptionExpiry(const int yyyymmdd);
  static std::string GetWeeklyShortCodeFromMonthlyShortcode(std::string& _shortcode_);
  static std::string GetWeeklyShortCodeFromSymbol(std::string& exch_symbol);

  // method to read instrument params - currently from a dated file
  static void LoadInstParams(const int t_date_);

  // method to read options details from bhavcopy
  static void LoadBhavCopyParams(const int t_date_);
  static void LoadCMBhavCopyParams(const int t_date_);
  static void LoadCDBhavCopyParams(const int t_date_);

  // read file for mapping between exchange symbol ( ORS ) and datasourcename ( MDS )
  static void LoadDataSourceExchangeSymbolMap();

  static void LoadCorporateActions(const int t_date_);

  // Map access for DS -> Exch Symbol and the reverse mapping is abstracted out for robustness
  static std::string ConvertDataSourceNametoExchSymbol(const std::string t_dsname_str_);
  static std::string ConvertDataSourceNametoExchSymbolSecDef(const std::string t_dsname_str_);
  static int GetOpenInterestFromExchangeSymbol(const std::string& _symbol_);
  static std::string ConvertExchSymboltoDataSourceName(const std::string t_exch_);
  static std::string ConvertExchSymboltoDataSourceNameGeneric(const std::string t_exch_);
  static std::string GetDataSourcesymbolFromExchSymbol_(const std::string t_exch_);


  // convert string to enum
  static NSEInstrumentType_t StringToInst(const char* str);

  // Exchange Symbol function for NSE
  static std::string GetExchSymbolNSE(const std::string& _shortcode_);

  // Commission function for NSE - returns total commissions in rupees per unit transacted
  static double GetNSECommission(std::string _shortcode_, int diff_exchange = 0);
  // return ITM for the underlying option
  static int GetNumberItmForUnderlying(std::string underlying_,int expiry);
  // Returns true iff we are interested in contracts of the specified expiry for the specified segment
  static bool LoadThisExpiryContracts(const int t_expiry_, const NSEInstrumentType_t t_inst_type_, char* underlying);

  // Returns the  contract number for the expirydate
  static int GetContractNumberFromExpiry(const int t_date_, const NSEInstrumentType_t t_inst_type_, char* underlying);
  static int GetWeeklyContractNumberFromExpiry(const int t_date_, char* underlying);

  // Returns the expiry date for a contract number
  static int GetExpiryFromContractNumber(const uint32_t t_number_, const NSEInstrumentType_t t_inst_type_, char* underlying);

  // Returns contract multiplier
  static double GetContractMultiplier(const NSEInstrumentType_t t_inst_type_);
  // wrapper for GetContractMultiplier being called from outside class
  static double GetContractMultiplier(const std::string& _shortcode_);

  static inline bool IsSpreadShortcode(const std::string shortcode) {
    return (shortcode_2_is_spread_.find(shortcode) != shortcode_2_is_spread_.end());
  }

  static inline bool IsSpreadExchSymbol(const std::string exch_symbol) {
    return (exchange_symbol_2_is_spread_.find(exch_symbol) != exchange_symbol_2_is_spread_.end());
  }

  static inline bool IsBannedProduct(const std::string shortcode) {
    return (std::find(banned_prod_list_.begin(), banned_prod_list_.end(), shortcode) != banned_prod_list_.end());
  }

  // Returns last close price
  static double GetLastClose(const std::string& _shortcode_);

  // Returns last close price for options product
  static double GetLastCloseForOptions(const std::string& _shortcode_);

  // Returns date for bhavcopy date format 28 JUL 2016 to 20160728
  static std::string GetDateInYYYYMMDD(char* _date_str_);
  static std::string  GetDateInYYYYMMDDBhav(std::string _date_str_);

  // returns month enum for input month string
  static NSEMonth_t StringToMonthNumber(const char* _cstr_);

  // fills shortcode to options_last_close price map
  static void AddtoOptionsLastClosePrice(NSEInstrumentType_t t_inst_type_, char* underlying_, int expiry_,
                                         double strike_price_, char* call_or_put_, std::string shortcode_);

  static bool IsOptionStrikePresentInBhavCopy(NSEInstrumentParams_t param, double strike_price);
  static double GetNearestAvailableMultipleStrike(NSEInstrumentParams_t param, double price, double multiple,
                                                  double upper_bound, double lower_bound);

  // Returns expiry date from shortcode
  static int GetExpiryFromShortCode(const std::string& _shortcode_);

  // Returns Strike Price from shortcode and vice versa
  static double GetStrikePriceFromShortCode(const std::string& _shortcode_);
  static double GetStrikePriceFromShortCodeGeneric(const std::string& _shortcode_);
  static std::string GetShortcodeFromCanonical(const std::string& ticker_, int expiry_dt_, double strike_,
                                               bool is_call_);
  static std::string GetWeeklyShortcodeFromCanonical(const std::string& ticker_, int expiry_dt_, double strike_,
                                               bool is_call_);
  static double GetMoneynessFromShortCode(const std::string& _shortcode_);
  static std::string GetShortCodeFromStrikePrice(double strike_price_, const std::string& _fut_shortcode_,
                                                 int option_type_);

  static std::vector<std::string> GetShortCodeListForStepValue(double step_value_, const std::string& _underlying_,
                                                 int num_strikes_, bool _is_weekly_);

  // Return the same strike call/put for a put/call shortcode
  static std::string GetOppositeContractShc(const std::string& _shortcode_);

  // Return Step Value from underlying shortcode
  static double GetStepValueFromShortCode(const std::string& _shortcode_);

  // Return Close Price from shortcode
  static double GetClosePriceFromShortCode(const std::string& _shortcode_);

  // Give the list of shortcodes for options related to particular underlying
  static std::vector<std::string> LoadOptionsList(const std::string& r_dep_shortcode_, int max_contract_number_);

  // Load Banned securities list
  static std::vector<std::string> LoadBannedSecurities(const int t_date_);
  static bool IsShortcodeUnderBan(const std::string& shortcode_);
  static int GetSurveillanceIndicator(const std::string& _shortcode_);

  // moneyness (-1 == OTM), (0 == ALL), (1 == ITM) {ATM will be obsorbed in OTM/ITM}
  // call_only (0, all), (1, calls), (-1, puts)
  // number_of_contracts (-1, all)
  static std::vector<std::string> GetAllOptionShortcodesForUnderlyingGeneric(const std::string& underlying);

  static std::vector<std::string> GetAllOptionShortcodesForUnderlying(const std::string& underlying, int moneyness = 0,
                                                                      int call_only = 0, int number_of_contracts = -1);
  static std::map<std::string, double> GetOptionsIntrinsicValue(const std::string& underlying, int moneyness = 0,
                                                                int call_only = 0, int number_of_contracts = -1);
  static std::vector<std::string> GetOptionShortcodesForUnderlyingInCurrScheme(const std::string& underlying,
                                                                               int moneyness = 0, int call_only = 0,
                                                                               int number_of_contracts = -1);
  static std::string GetOptionShortcodeForUnderlyingInCurrSchemeFromContractNumber(const std::string& underlying,
                                                                                   int moneyness = 0, int call_only = 0,
                                                                                   unsigned int contract_num_ = 0);
  static std::vector<std::string> GetOptionShortcodesForUnderlyingInPrevScheme(const std::string& underlying,
                                                                               int moneyness = 0, int call_only = 0,
                                                                               int number_of_contracts = -1);
  static bool IsOptionFromCurrScheme(const std::string& shortcode);

  // reference point is ATM, so for O2 O1
  static std::string GetPrevOptionInCurrentSchema(const std::string& option_);
  // reference point is ATM, so for O1 O2
  static std::string GetNextOptionInCurrentSchema(const std::string& option_);

  // Return the shortcode of the given shc in previous expiry
  static std::string GetFutureSHCInPreviousExpiry(const std::string& fut_shc_);

  // To be used in calculation of greeks and implied vol
  static std::string GetFutureShortcodeFromOptionShortCode(const std::string& _shortcode_);
  static bool IsOption(const std::string& _shortcode_);
  static bool IsHiddenOrderAvailable(const std::string& _shortcode_);
  static bool IsFuture(const std::string& _shortcode_);
  static bool IsEquity(const std::string& _shortcode_);
  static bool IsSpotIndex(const std::string& _shortcode_);
  static bool IsCurrency(const std::string& _shortcode_);
  static int GetOptionType(const std::string& _shortcode_);
  static bool IsWeeklyOption(const std::string& _shortcode_);
  static double GetInterestRate(const int t_date_);
  static bool IsShortcode(const std::string& _shortcode_);
  static std::string GetShortCodeFromExchangeSymbol(const std::string _shortcode_);
  // just for fuures now
  // date to identify file
  // token number to identify column
  /*1 NORMAL?
2 INSTRUMENT
3 SYMBOL
4 EXPIRY_DT
5 STRIKE_PR
6 OPTION_TYP
7 OPEN
8 HIGH
9 LOW
10 CLOSE
11 SETTLE_PR
12 CONTRACTS
13 VAL_INLAKH
14 OPEN_INT
15 CHG_IN_OI*/
  // key to identify row ( INSTRUMENT + SYMBOL + STRIKE_PR + OPTION_TYP + EXPIRY_DT )
  // shortcode + expiry_date_ ( default == min_expiry_date )
  // three arguments, date_, shc_, token_id_
  static std::string GetBhavCopyToken(const int date_, const char* shortcode_, int token_id_,
                                      const std::string& _segment_,
                                      std::map<std::string, std::string>& shortcode_2_map_);
  static double GetLastTradePrice(const int date_, const char* shortcode_);
  static NSETradingLimit_t* GetTradingLimits(const std::string& shortcode_);
  static NSETradingLimit_t* ChangeTradingLimits(const std::string& shortcode_, int int_px_to_cross_);

  // Defines the price limits at which we can send the orders.
  // Beyond that exchange will reject our orders
  static void PopulateOrderPriceRangeMapEQ(const int t_date_);
  static void PopulateOrderPriceRangeMapFut(const int t_date_);
  static void PopulateOrderPriceRangeMapOpt(const int t_date_);

  // we want to get top notional traded for
  // a) futures
  // b) options sum(all_options_at_future_level)
  // c) options
  static void GetTopNotionalTradedContracts(bool _underlying_level_, bool _options_, unsigned _top_n_,
                                            unsigned _lookback_days_);
};
}
