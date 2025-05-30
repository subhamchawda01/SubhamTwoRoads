/**
   \file dvccode/CDef/security_definitions.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_CDEF_SECURITY_DEFINITIONS_H
#define BASE_CDEF_SECURITY_DEFINITIONS_H

#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <math.h>  // pow

#include "dvccode/CDef/common_security_definition_structs.hpp"
#include "dvccode/CDef/currency_convertor.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/hk_stocks_security_definition.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"

#define SIXJ_TICKCHANGE_DATE 20150621
#define SIXE_TICKCHANGE_DATE 20160109
#define SIXC_TICKCHANGE_DATE 20160709
#define YT_TICKCHANGE_DATE 20171201

namespace HFSAT {

const boost::gregorian::date_duration one_day_date_duration_secdef(1);

class SecurityDefinitions {
 private:
  /// Made copy constructor private to disable
  SecurityDefinitions(const SecurityDefinitions &);
  explicit SecurityDefinitions(int t_intdate_);

  void CMESecurityDefinitions(int t_intdate_);
  void EUREXSecurityDefinitions(int t_intdate_);
  void ASXSecurityDefinitions(int t_intdate_);
  void CFESecurityDefinitions(int t_intdate_);
  void TMXSecurityDefinitions(int t_intdate_);
  void BMFSecurityDefinitions(int t_indate_);
  void LIFFEISESecurityDefintions(int t_intdate_);
  void SGXSecurityDefinitions(int t_intdate_);
  void OSESecurityDefinitions(int t_intdate_);

  void BatsSecurityDefinitions(int t_intdate_);
  void BMFEQSecurityDefinitions(int t_intdate_);

  double GetASXSpreadNumbersToDollars();
  void GetRTSContractSpecifications();

  static int tradingdate_;
  std::map<std::string, int> di_reserves_cache_;
  std::map<std::string, int> asx_reserves_cache_;

  static SecurityDefinitions *p_uniqueinstance_;
  std::vector<std::string> non_self_securities_;

 public:
  double GetASXBondPrice(double price_, int asx_term_) const;

  static ShortcodeContractSpecificationMap contract_specification_map_;
  ContractSpecification _dummy_;

  static inline SecurityDefinitions &GetUniqueInstance(int t_intdate_) {
    if (p_uniqueinstance_ == NULL) {
      p_uniqueinstance_ = new SecurityDefinitions(t_intdate_);
    }
    return *(p_uniqueinstance_);
  }

  static void RemoveUniqueInstance() {
    //    NSESecurityDefinitions::RemoveUniqueInstance();
    if (NULL != p_uniqueinstance_) {
      delete p_uniqueinstance_;
      p_uniqueinstance_ = NULL;
    }
  }

  static inline SecurityDefinitions &GetUniqueInstance() {
    if (p_uniqueinstance_ == NULL) {
      std::cerr << "please first initiate SecurityDefinitions by calling SecurityDefinitions::GetUniqueInstance ( int "
                   "t_intdate_ )\n";
      exit(1);
    }
    return *(p_uniqueinstance_);
  }
  //Check whether a security can be cancelled before confirmation or not. Currently only valid for exchanges EUREX, MICEX, BMF, SGX 
  bool inline cancel_before_conf(const char* security_name_) {
    HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
    HFSAT::ExchSource_t CancellableExchange= (GetContractExchSource(sec_name_indexer_.GetShortcodeFromId(sec_name_indexer_.GetIdFromSecname(security_name_))));	
    switch (CancellableExchange) {
      case HFSAT::kExchSourceEUREX : return true;
      case HFSAT::kExchSourceBMF : return true;
      case HFSAT::kExchSourceSGX : return true;
      case HFSAT::kExchSourceMICEX_CR : return true;
      case HFSAT::kExchSourceICE : return true;                                    
      default : return false;			 
    }		
  }

  bool IsValidContract(const std::string &_shortcode_) const {
    ShortcodeContractSpecificationMapCIter_t t_citer_ = contract_specification_map_.find(_shortcode_);
    if (t_citer_ != contract_specification_map_.end())
      return true;
    else
      return false;
  }

  static inline int GetDIReserves(const int &yyyymmdd_, const std::string &_shortcode_) {
    return GetUniqueInstance(yyyymmdd_)._GetDIReserves(yyyymmdd_, _shortcode_);
  }

  static inline int GetASXReserves(const int &yyyymmdd_, const std::string &_secname_) {
    return GetUniqueInstance(yyyymmdd_)._GetASXReserves(yyyymmdd_, _secname_);
  }

  static inline void ResetASXMpiIfNeeded(const int &yyyymmdd_, const std::string &_start_time_str_) {
    GetUniqueInstance(yyyymmdd_)
        ._ResetASXMpiIfNeeded(DateTime::GetTimeFromTZHHMMStr(yyyymmdd_, _start_time_str_.c_str()));
  }

  static inline void ResetASXMpiIfNeeded(const int &yyyymmdd_, const time_t &_start_time_) {
    GetUniqueInstance(yyyymmdd_)._ResetASXMpiIfNeeded(_start_time_);
  }

  static inline void ResetASXMpiIfNeeded(const int &yyyymmdd_, const time_t &_start_time_, const bool livetrading_) {
    GetUniqueInstance(yyyymmdd_)._ResetASXMpiIfNeeded(_start_time_, livetrading_);
  }

  //    time_t c_time_ = DateTime::GetTimeFromTZHHMMStr(tradingdate_, _start_time_str_.c_str());

  static const ContractSpecification &GetContractSpecification(const std::string &_shortcode_, int t_intdate_) {
    return GetUniqueInstance(t_intdate_)._GetContractSpecification(_shortcode_);
  }

  static double GetContractMinPriceIncrement(const std::string &_shortcode_, const int &yyyymmdd_) {
    return (GetUniqueInstance(yyyymmdd_)._GetContractSpecification(_shortcode_).min_price_increment_ *
            GetUniqueInstance(yyyymmdd_)._GetExpiryPriceFactor(_shortcode_, yyyymmdd_));
  }

  static std::vector<std::string> GetNonSelfEnabledSecurities(const int &yyyymmdd_) {
    return GetUniqueInstance(yyyymmdd_).non_self_securities_;
  }

  static double GetExpiryPriceFactor(const std::string &_shortcode_, const int &yyyymmdd_) {
    return GetUniqueInstance(yyyymmdd_)._GetExpiryPriceFactor(_shortcode_, yyyymmdd_);
  }

  static double GetContractMinPriceIncrementWithDateAlreadySet(const std::string &_shortcode_) {
    return (GetUniqueInstance(tradingdate_)._GetContractSpecification(_shortcode_).min_price_increment_ *
            GetUniqueInstance(tradingdate_)._GetExpiryPriceFactor(_shortcode_, tradingdate_));
  }

  static double GetContractNumbersToDollars(const std::string &_shortcode_, int t_intdate_) {
    if (_shortcode_.compare("SP_XT0_YT0") == 0) {
      return GetUniqueInstance(t_intdate_).GetASXSpreadNumbersToDollars();
    }
    return GetUniqueInstance(t_intdate_)._GetContractSpecification(_shortcode_).numbers_to_dollars_;
  }

  static ExchSource_t GetContractExchSource(const std::string &_shortcode_, int t_intdate_) {
    return GetUniqueInstance(t_intdate_)._GetContractSpecification(_shortcode_).exch_source_;
  }

  static ExchSource_t GetContractExchSource(const std::string &_shortcode_) {
    return GetUniqueInstance()._GetContractSpecification(_shortcode_).exch_source_;
  }

  static int GetContractMinOrderSize(const std::string &_shortcode_, int t_intdate_) {
    return GetUniqueInstance(t_intdate_)._GetContractSpecification(_shortcode_).min_order_size_;
  }

  static void GetDefinedShortcodesVec(std::vector<std::string> &_shortcode_vec_, int t_intdate_) {
    GetUniqueInstance(t_intdate_)._GetDefinedShortcodesVec(_shortcode_vec_);
  }
  static bool GetRemoveSelfOrdersFromBook(const std::string &_shortcode_, int t_intdate_) {
    return GetUniqueInstance(t_intdate_)._GetRemoveSelfOrdersFromBook(_shortcode_);
  }

  static int GetConfToMarketUpdateMsecs(const std::string &_shortcode_, int t_intdate_) {
    return GetUniqueInstance(t_intdate_)._GetConfToMarketUpdateMsecs(_shortcode_);
  }

  static bool GetTradeBeforeQuote(const std::string &_shortcode_, int t_intdate_) {
    return GetUniqueInstance(t_intdate_)._GetTradeBeforeQuote(_shortcode_);
  }

  static bool ModifyAfterPartialExecAllowed(const std::string &_shortcode_, int t_intdate_) {
    return GetUniqueInstance(t_intdate_)._ModifyAfterPartialExecAllowed(_shortcode_);
  }

  static bool IsTimeProRataSimShortcode(const std::string &_shortcode_, int t_intdate_) {
    return GetUniqueInstance(t_intdate_)._IsTimeProRataSimShortcode(_shortcode_);
  }

  static bool IsNewTimeProRataSimShortcode(const std::string &_shortcode_, int t_intdate_) {
    return GetUniqueInstance(t_intdate_)._IsNewTimeProRataSimShortcode(_shortcode_);
  }

  static bool IsSimpleProRataSimShortcode(const std::string &_shortcode_, int t_intdate_) {
    return GetUniqueInstance(t_intdate_)._IsSimpleProRataSimShortcode(_shortcode_);
  }

  static bool IsSplitFIFOProRataSimShortcode(const std::string &_shortcode_, int t_intdate_) {
    return GetUniqueInstance(t_intdate_)._IsSplitFIFOProRataSimShortcode(_shortcode_);
  }

  static double GetFIFOPercentageForSimShortcode(const std::string &_shortcode_, int t_intdate_) {
    return GetUniqueInstance(t_intdate_)._GetFIFOPercentageForSimShortcode(_shortcode_);
  }

  static bool CheckIfContractSpecExists(const std::string &_shortcode_, int t_intdate_) {
    return GetUniqueInstance(t_intdate_)._CheckIfContractSpecExists(_shortcode_);
  }
  static bool IsICEProRataSimShortcodeLFI(const std::string &_shortcode_, int t_intdate_) {
    return GetUniqueInstance(t_intdate_)._IsICEProRataSimShortcodeLFI(_shortcode_);
  }

  static bool IsICEProRataSimShortcodeLFL(const std::string &_shortcode_, int t_intdate_) {
    return GetUniqueInstance(t_intdate_)._IsICEProRataSimShortcodeLFL(_shortcode_);
  }

  inline int _GetDIReserves(const int &yyyymmdd_, const std::string &_shortcode_) {
    std::ostringstream t_oss_;
    t_oss_ << _shortcode_ << yyyymmdd_;
    std::string t_di_reserves_cache_key_ = t_oss_.str();
    if (di_reserves_cache_.find(t_di_reserves_cache_key_) != di_reserves_cache_.end()) {
      return di_reserves_cache_[t_di_reserves_cache_key_];
    }

    int reserves_ = 0;
    std::string _shc_ = _shortcode_;

    if (_shc_.find("DI1") != std::string::npos)  // DI1F14 we have a specific_ticker
    {
      char _ltd_date_[10] = {0};  // changed from 8 to 10 since the sprintf line below was writing memory to a location
                                  // it should not have written

      const std::string ExchMonthCode("FGHJKMNQUVXZ");
      const boost::gregorian::date_duration one_day_date_duration(1);

      int ltd_mm = ExchMonthCode.find(_shc_[3]);
      ltd_mm++;                                    // for DI1F17 it will 1
      int ltd_yy = atoi(_shc_.substr(4).c_str());  // for DI1F17 it will be 17

      if (sprintf(_ltd_date_, "20%02d%02d01", ltd_yy, ltd_mm) > 0)  // 20170101
      {
        std::stringstream ss;
        ss << yyyymmdd_;

        boost::gregorian::date sdate_ =
            boost::gregorian::from_undelimited_string(ss.str());  // my compiler  doesnt know to_string ?
        boost::gregorian::date edate_ = boost::gregorian::from_undelimited_string(_ltd_date_);

        std::string str_edate_ = boost::gregorian::to_iso_string(edate_);
        std::string str_sdate_ = boost::gregorian::to_iso_string(sdate_);

        // In case 20170101 is holiday go to next business day
        while (IsDIHoliday(std::atoi(str_edate_.c_str()), true)) {
          edate_ = edate_ + one_day_date_duration;
          str_edate_ = boost::gregorian::to_iso_string(edate_);
        }

        while (sdate_ < edate_) {
          str_sdate_ = boost::gregorian::to_iso_string(sdate_);
          if (IsDIHoliday(std::atoi(str_sdate_.c_str()), true)) {
          } else {
            reserves_++;
          }
          sdate_ = sdate_ + one_day_date_duration;
        }
        di_reserves_cache_[t_di_reserves_cache_key_] = reserves_;
        return reserves_;
      }
    }
    di_reserves_cache_[t_di_reserves_cache_key_] = -1;
    return -1;
  }

  inline int _GetASXReserves(const int &yyyymmdd_, const std::string &_secname_) {
    const char ASXMonthCodes[] = {
        'F',  // January
        'G',  // February
        'H',  // March
        'J',  // April
        'K',  // May
        'M',  // June
        'N',  // July
        'Q',  // August
        'U',  // September
        'V',  // October
        'X',  // November
        'Z'   // December
    };
    if (asx_reserves_cache_.find(_secname_) != asx_reserves_cache_.end()) {
      return asx_reserves_cache_[_secname_];
    }

    int reserves_ = 0;
    std::string _sec_name_ = _secname_;

    // Comparing only the initial letters for ASX products since these can exists as substring
    // of other secname also (like YT in NSE_CENTURYTEX)
    if (_sec_name_.find("XT") == 0 || _sec_name_.find("YT") == 0 || _sec_name_.find("IR") == 0) {
      // XTH6-YTH6 => use values for XT201603
      if (_sec_name_.find("-") != std::string::npos) {
        std::stringstream outright_name;
        char month = _sec_name_[2];
        outright_name << _sec_name_.substr(0, 2) << "201" << _sec_name_[3];
        for (int ctr = 0; ctr < 12; ctr++) {
          if (ASXMonthCodes[ctr] == month) {
            if (ctr < 9) {
              outright_name << "0" << (ctr + 1);
            } else {
              outright_name << (ctr + 1);
            }
            break;
          }
        }
        _sec_name_ = outright_name.str();
      }
      char _ltd_date_[8] = {0};

      const boost::gregorian::date_duration one_day_date_duration(1);

      int ltd_mm = atoi(_sec_name_.substr(6, 2).c_str());  // for XT201407 it will 7
      int ltd_yy = atoi(_sec_name_.substr(2, 4).c_str());  // for XT201407 it will be 2014

      ExchangeSymbolManager::SetUniqueInstance(yyyymmdd_);
      int _tmp_date_ = ExchangeSymbolManager::GetASXLastTradingDateYYYYMM_(_sec_name_.substr(0, 2), ltd_mm, ltd_yy);

      sprintf(_ltd_date_, "%d", _tmp_date_);

      std::stringstream ss;
      ss << yyyymmdd_;

      boost::gregorian::date sdate_ =
          boost::gregorian::from_undelimited_string(ss.str());  // my compiler doesnt know to_string ?
      boost::gregorian::date edate_ = boost::gregorian::from_undelimited_string(_ltd_date_);

      std::string str_edate_ = boost::gregorian::to_iso_string(edate_);
      std::string str_sdate_ = boost::gregorian::to_iso_string(sdate_);

      while (IsASXHoliday(std::atoi(str_edate_.c_str()), true))

      {
        edate_ = edate_ - one_day_date_duration;
        str_edate_ = boost::gregorian::to_iso_string(edate_);
      }

      while (sdate_ < edate_) {
        str_sdate_ = boost::gregorian::to_iso_string(sdate_);
        if (IsASXHoliday(std::atoi(str_sdate_.c_str()), true)) {
        } else {
          reserves_++;
        }
        sdate_ = sdate_ + one_day_date_duration;
      }
      asx_reserves_cache_[_secname_] = reserves_;
      return reserves_;
    }
    asx_reserves_cache_[_secname_] = -1;
    return -1;
  }

 protected:
  // expiry based min price inccrement changes
  inline bool IsCMEExchangeDate(const std::string &_pure_basename_, boost::gregorian::date &d1) {
    return ((d1.day_of_week() != boost::gregorian::Saturday) && (d1.day_of_week() != boost::gregorian::Sunday));
  }

  // check if expiry month
  bool IsCMEMonth(std::string &_pure_basename_, const int &yyyymmdd_) {
    int this_month_ = (yyyymmdd_ / 100) % 100;
    if (_pure_basename_ == "GE") {
      return ((this_month_ + 1) % 3 == 0);
    }
    return false;
  }

  // fetch last trading date, only used for GE, 2day prior to 3rd Wed
  int GetCMELastTradingDateYYYYMM(const std::string &_pure_basename_, const int &yyyymmdd_) {
    int this_month_ = (yyyymmdd_ / 100) % 100;
    int this_year_ = (yyyymmdd_ / 10000);

    if (_pure_basename_ == "GE") {
      boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::third,
                                                         boost::gregorian::Wednesday, this_month_);

      boost::gregorian::date d1 = ndm.get_date(this_year_);

      for (auto i = 0u; i < 2; i++) {
        do {
          d1 -= one_day_date_duration_secdef;
        } while (!IsCMEExchangeDate(_pure_basename_, d1));
      }

      d1 -= one_day_date_duration_secdef;

      return YYYYMMDD_from_date(d1);
    }

    // dummy
    return 20171231;
  }

  // check if GE_0 has become the nearest expiry contract
  bool IsCMEFrontMonth(const std::string &_pure_basename_, const int &yyyymmdd_) {
    int this_month_ = (yyyymmdd_ / 100) % 100;

    if (_pure_basename_ == "GE") return (this_month_ % 3 == 0);

    // dummy
    return false;
  }

  // fetch the last date for front end contract
  int _GetFrontMonthContractExpiryDate(const std::string &_pure_basename_, const int &yyyymmdd_) {
    if (IsCMEFrontMonth(_pure_basename_, yyyymmdd_)) {
      return GetCMELastTradingDateYYYYMM(_pure_basename_, yyyymmdd_);
    }

    return yyyymmdd_;
  }

  bool IsBMFMonth(const std::string &_pure_basename_, const int _this_month_) {
    if (_pure_basename_ == "DI1") return true;

    return false;  // only using for DI
  }

  void SetToNextBMFMonth(const std::string &_pure_basename_, int &_next_bmf_month_, int &_next_bmf_year_) {
    if (_pure_basename_ == "DI1") {
      if (_next_bmf_month_ == 12) {
        _next_bmf_month_ = 1;
        _next_bmf_year_++;

      } else {
        _next_bmf_month_++;
      }

      return;
    }
  }

  std::string GetDIFrontMonthSymobol(int _trading_date_, int _expiry_no_) {
    const char CMEMonthCode[] = {
        'F',  // January
        'G',  // February
        'H',  // March
        'J',  // April
        'K',  // May
        'M',  // June
        'N',  // July
        'Q',  // August
        'U',  // September
        'V',  // October
        'X',  // November
        'Z'   // December
    };

    int current_min_last_trading_date_mm = (_trading_date_ / 100) % 100;
    int current_min_last_trading_date_yy = (_trading_date_ / 10000) % 100;

    while (_expiry_no_ >= 0) {
      if (current_min_last_trading_date_mm == 12) {
        current_min_last_trading_date_mm = 1;
        current_min_last_trading_date_yy++;
      } else {
        current_min_last_trading_date_mm++;
      }
      _expiry_no_--;
    }

    std::stringstream ss;
    ss << "DI1" << CMEMonthCode[current_min_last_trading_date_mm - 1] << current_min_last_trading_date_yy;
    return ss.str();
  }

  // get price factor, currently only used for GE
  double _GetExpiryPriceFactor(const std::string &_shortcode_, const int &yyyymmdd_) {
    if (_shortcode_.find("SP_GE_0") != std::string::npos) {  // min price increment of GE spread behave like min price
                                                             // increment of GE contracts present as leg
      std::string leg_ = "GE_0";
      if (IsCMEMonth(leg_, yyyymmdd_)) {
        // check if we are past rollover date with GE becoming nearest expiring contract
        if (yyyymmdd_ > GetCMELastTradingDateYYYYMM(leg_, yyyymmdd_)) {
          // TODO currently only being for GE
          return 0.5;
        }
      }

      if (IsCMEFrontMonth(leg_, yyyymmdd_)) {
        // check if GE_0 still being the front contract on expiry month
        if (yyyymmdd_ <= _GetFrontMonthContractExpiryDate(leg_, yyyymmdd_)) {
          return 0.5;
        }
      }

      // default
      return 1;
    }

    std::string _pure_basename_ = _shortcode_.substr(0, _shortcode_.find("_"));

    std::string _expiry_ = _shortcode_.substr(_shortcode_.find("_") + 1);

    if (_pure_basename_ == "GE" && _expiry_ == "0") {
      if (IsCMEMonth(_pure_basename_, yyyymmdd_)) {
        // check if we are past rollover date with GE becoming nearest expiring contract
        if (yyyymmdd_ > GetCMELastTradingDateYYYYMM(_pure_basename_, yyyymmdd_)) {
          // TODO currently only being for GE
          return 0.5;
        }
      }

      if (IsCMEFrontMonth(_pure_basename_, yyyymmdd_)) {
        // check if GE_0 still being the front contract on expiry month
        if (yyyymmdd_ <= _GetFrontMonthContractExpiryDate(_pure_basename_, yyyymmdd_)) {
          return 0.5;
        }
      }

      // default
      return 1;
    }
    _pure_basename_ = _shortcode_.substr(0, 3);
    return 1.0;
  }

  inline unsigned int YYYYMMDD_from_date(const boost::gregorian::date &d1) {
    boost::gregorian::date::ymd_type ymd = d1.year_month_day();
    return (((ymd.year * 100 + ymd.month) * 100) + ymd.day);
  }

  const ContractSpecification &_GetContractSpecification(const std::string &_shortcode_) const {
    std::string _this_shortcode_ = _shortcode_;
    std::replace(_this_shortcode_.begin(), _this_shortcode_.end(), '~', '&');

    ShortcodeContractSpecificationMapCIter_t t_citer_ = contract_specification_map_.find(_this_shortcode_);
    if (t_citer_ != contract_specification_map_.end()) {
      return t_citer_->second;
    }

    std::ostringstream t_temp_oss_;
    t_temp_oss_ << " " << _this_shortcode_;
    ExitVerbose(kGetContractSpecificationMissingCode, t_temp_oss_.str().c_str());
    return _dummy_;
  }

  const void _GetDefinedShortcodesVec(std::vector<std::string> &_shortcode_vec_) {
    ShortcodeContractSpecificationMapCIter_t t_citer_ = contract_specification_map_.begin();
    while (t_citer_ != contract_specification_map_.end()) {
      _shortcode_vec_.push_back(t_citer_->first);
      t_citer_++;
    }
  }
  bool _CheckIfContractSpecExists(const std::string &_shortcode_) {
    std::string _this_shortcode_ = _shortcode_;
    ShortcodeContractSpecificationMapCIter_t t_citer_ = contract_specification_map_.find(_this_shortcode_);
    if (t_citer_ != contract_specification_map_.end()) {
      return true;
    }
    return false;
  }

  bool IsDIHoliday(int yyyymmdd, bool is_weekend_holiday = false);
  bool IsASXHoliday(int yyyymmdd, bool is_weekend_holiday = false);
  void GetNonSelfEnabledSecurities();
  void ResetDI1MPI();
  void _ResetASXMpiIfNeeded(const time_t &_start_time_ = 0, const bool livetrading_ = false);
  bool _GetRemoveSelfOrdersFromBook(const std::string &_shortcode_);
  int _GetConfToMarketUpdateMsecs(const std::string &_shortcode_);
  bool _IsTimeProRataSimShortcode(const std::string &_shortcode_);
  bool _IsNewTimeProRataSimShortcode(const std::string &_shortcode_);
  bool _IsSimpleProRataSimShortcode(const std::string &_shortcode_);
  bool _IsSplitFIFOProRataSimShortcode(const std::string &_shortcode_);
  bool _IsICEProRataSimShortcodeLFI(const std::string &_shortcode_);
  bool _IsICEProRataSimShortcodeLFL(const std::string &_shortcode_);
  bool _IsICESimShortcode_(const std::string &_shortcode_);
  double _GetFIFOPercentageForSimShortcode(const std::string &_shortcode_);

  bool _GetTradeBeforeQuote(const std::string &_shortcode_);
  bool _ModifyAfterPartialExecAllowed(const std::string &_shortcode_);

  bool IsQuincyFeedAvailableForShortcode(const std::string &_shortcode_);

 public:
  static void LoadBSESecurityDefinitions();
  static void LoadNSESecurityDefinitions() {
    if (IsNSESecDefAvailable()) {
      HFSAT::NSESecurityDefinitions nsd_ = HFSAT::NSESecurityDefinitions::GetUniqueInstance(tradingdate_);
      nsd_.AddNSEContractSpecifications(contract_specification_map_);
    }
  }

  static void LoadHKStocksSecurityDefinitions() {
    if (IsHKSecDefAvailable()) {
      HFSAT::HKStocksSecurityDefinitions hksecdef_ =
          HFSAT::HKStocksSecurityDefinitions::GetUniqueInstance(tradingdate_);
      hksecdef_.AddHKStocksContractSpecifications(contract_specification_map_);
    }
  }

  void GetAllSecurities(std::vector<std::string> *shclist_) {
    for (auto const &element : contract_specification_map_) {
      shclist_->push_back(element.first);
    }
  }

  static inline bool IsNSESecDefAvailable(const int trading_date) {
    return ((trading_date > 20140930) && (!((trading_date >= 20150401) && (trading_date <= 20150910))));
  }

  static inline bool IsHKSecDefAvailable(const int trading_date) {
    return ((trading_date >= 20150803) && (trading_date <= 20151030));
  }

  static inline bool IsBSESecDefAvailable(const int trading_date) { return false; }

 private:
  static inline bool IsHKSecDefAvailable() { return IsHKSecDefAvailable(tradingdate_); }
  static inline bool IsNSESecDefAvailable() { return IsNSESecDefAvailable(tradingdate_); }
};
}  // namespace HFSAT

#endif  // BASE_CDEF_SECURITY_DEFINITIONS_H
