/**
    \file dvccode/CDef/currency_convertor.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_CDEF_CURRENCY_CONVERTOR_H
#define BASE_CDEF_CURRENCY_CONVERTOR_H

#include <iostream>
#include <map>
#include "dvccode/CDef/error_utils.hpp"

#define CURRENCY_RATES_REPORTS_DIR "/spare/local/files/CurrencyData/"

namespace HFSAT {

typedef enum {
  kCurrencyUSD,
  kCurrencyBRL,
  kCurrencyEUR,
  kCurrencyJPY,
  kCurrencyHKD,
  kCurrencySGD,
  kCurrencyMXN,
  kCurrencyINR,
  kCurrencyGBP,
  kCurrencyCAD,
  kCurrencyRUB,
  kCurrencyAUD,
  kCurrencyCHF,
  kCurrencyKRW,
  kCurrencyMAX
} Currency_t;

struct CurrencyPair_t {
  const Currency_t base_;
  const Currency_t quote_;

  CurrencyPair_t(const Currency_t& _base_, const Currency_t& _quote_) : base_(_base_), quote_(_quote_) {}

  inline bool operator<(const CurrencyPair_t& r2) const {
    return ((base_ < r2.base_) || ((base_ == r2.base_) && (quote_ < r2.quote_)));
  }

  inline bool operator==(const CurrencyPair_t& r2) const { return ((base_ == r2.base_) && (quote_ == r2.quote_)); }

  inline bool operator!=(const CurrencyPair_t& r2) const { return ((base_ != r2.base_) || (quote_ != r2.quote_)); }

  CurrencyPair_t Invert() const { return CurrencyPair_t(quote_, base_); }
};

class CurrencyConvertor {
  typedef std::map<CurrencyPair_t, double> OneIn;
  typedef std::map<CurrencyPair_t, double>::iterator OneInIter_t;
  typedef std::map<CurrencyPair_t, double>::const_iterator OneInCiter_t;

 private:
  /// Added to disable copy constructor
  CurrencyConvertor(const CurrencyConvertor&);
  static CurrencyConvertor* p_uniqueinstance_;
  OneIn one_in_;
  int date_;

 public:
  CurrencyConvertor(int _YYYYMMDD_) : one_in_(), date_(_YYYYMMDD_) { LoadOfflineCurrencyData(); }

  static std::string GetStringFromEnum(Currency_t e_val_) {
    if (e_val_ == kCurrencyUSD) {
      return (std::string("USDUSD"));
    } else if (e_val_ == kCurrencyBRL) {
      return (std::string("BRLUSD"));
    } else if (e_val_ == kCurrencyEUR) {
      return (std::string("EURUSD"));
    } else if (e_val_ == kCurrencyJPY) {
      return (std::string("JPYUSD"));
    } else if (e_val_ == kCurrencyHKD) {
      return (std::string("HKDUSD"));
    } else if (e_val_ == kCurrencySGD) {
      return (std::string("SGDUSD"));
    } else if (e_val_ == kCurrencyMXN) {
      return (std::string("MXNUSD"));
    } else if (e_val_ == kCurrencyINR) {
      return (std::string("INRUSD"));
    } else if (e_val_ == kCurrencyGBP) {
      return (std::string("GBPUSD"));
    } else if (e_val_ == kCurrencyCAD) {
      return (std::string("CADUSD"));
    } else if (e_val_ == kCurrencyRUB) {
      return (std::string("RUBUSD"));
    } else if (e_val_ == kCurrencyAUD) {
      return (std::string("AUDUSD"));
    } else if (e_val_ == kCurrencyCHF) {
      return (std::string("CHFUSD"));
    } else if (e_val_ == kCurrencyKRW) {
      return (std::string("KRWUSD"));
    } else {
      return (std::string("INVALID"));
    }
  }

  static inline void SetUniqueInstance(int _YYYYMMDD_) {
    // for safety calling this function from exchange_symbol_manager & security_definitions,
    // if these 2 classes are not used which is rare, we have to call this func from the main function of the exec
    if (p_uniqueinstance_ == NULL) {
      p_uniqueinstance_ = new CurrencyConvertor(_YYYYMMDD_);
    } else if (GetUniqueInstance().date_ != _YYYYMMDD_) {
      std::cerr << "CurrencyConvertor::SetUniqueInstance already called earlier with date: "
                << GetUniqueInstance().date_ << ". While this date is: " << _YYYYMMDD_ << std::endl;
    }
  }

  static void RemoveInstance() {
    if (NULL != p_uniqueinstance_) {
      delete p_uniqueinstance_;
      p_uniqueinstance_ = NULL;
    }
  }

  static inline CurrencyConvertor& GetUniqueInstance() {
    if (p_uniqueinstance_ == NULL) {
      ExitVerbose(kExitErrorCodeGeneral, "please call CurrencyConvertor::SetUniqueInstance first");
    }
    return *p_uniqueinstance_;
  }

  static inline double Convert(const Currency_t _base_, const Currency_t _quote_) {
    return Convert(CurrencyPair_t(_base_, _quote_));
  }
  static inline double Convert(const CurrencyPair_t& _cpair_) { return GetUniqueInstance()._Convert(_cpair_); }

 protected:
  /// Called by CurrencyConvertor::Convert
  inline double _Convert(const CurrencyPair_t& _cpair_) const {
    if ((_cpair_.base_ == kCurrencyINR) || (_cpair_.quote_ == kCurrencyINR)) {
      return 1.00;
    }

    OneInCiter_t _citer_ = one_in_.find(_cpair_);
    if (_citer_ != one_in_.end()) {
      return _citer_->second;
    }
    return 1.00;
  }

  void LoadOfflineCurrencyData();
  bool LoadCurrencyDataFromReports();
};

Currency_t GetCurrencyFromString(const char* cc_str_);
CurrencyPair_t GetCurrencyPairFromString(const char* cc_str_);
}

#endif  // BASE_CDEF_CURRENCY_CONVERTOR_H
