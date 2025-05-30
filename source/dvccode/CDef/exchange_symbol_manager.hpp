/**
   \file dvccode/CDef/exchange_symbol_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_CDEF_EXCHANGE_SYMBOL_MANAGER_H
#define BASE_CDEF_EXCHANGE_SYMBOL_MANAGER_H

#include <iostream>
#include <map>
#include <string>
#include <utility>

#include "dvccode/CDef/currency_convertor.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/error_utils.hpp"

namespace HFSAT {
#define LARGE_ES_DB_SIZE 4096
#define JGBL_LAST_TRADING_DATE_CHANGE 20151201

/** @brief to compute the exchange symbol corresponding to this shortcode
 * Only does this for one date, since in practice this will never be needed for more than one date at a time
 **/
class ExchangeSymbolManager {
 protected:
  typedef struct {
    std::string shortcode_;
    char exchange_symbol_[kSecNameLen];
  } ORpair;
  typedef std::map<std::string, const char*> ShortcodeExchangeSymbolMap;
  typedef struct { char symbol1_[kSecNameLen], symbol2_[kSecNameLen], sp_symbol_[kSecNameLen]; } SpreadSymbol;
  typedef struct {
    char symbol1_[kSecNameLen], symbol2_[kSecNameLen], symbol3_[kSecNameLen], fly_symbol_[kSecNameLen];
  } FlySymbol;

  /// fixed trading date, at one time the exchange symbol manager can only be used for one trading date.
  /// this variable will be used in loading override file
  /// and in all computations of symbols.
  const int YYYYMMDD_;
  std::vector<ORpair> orpair_vec_;
  std::vector<ORpair> vol_orpair_vec_;
  std::vector<ORpair> di_orpair_vec_;
  std::vector<ORpair> yfebm_orpair_vec_;
  std::vector<SpreadSymbol> liffe_spread_vec_;
  std::vector<FlySymbol> liffe_fly_vec_;

  std::vector<Char16_t> large_es_db_;
  size_t large_es_db_front_index_;
  ShortcodeExchangeSymbolMap shortcode_exchange_symbol_map_;

  char temp_str_global_scope_[kSecNameLen];

  std::vector<int> bmf_last_trading_dates_vec_;

 protected:
  static ExchangeSymbolManager* p_uniqueinstance_;

  ExchangeSymbolManager(const int _YYYYMMDD_);

 public:
  inline int YYYYMMDD() const { return YYYYMMDD_; }

  static void SetUniqueInstance(const int _YYYYMMDD_) {
    if (p_uniqueinstance_ == NULL) {
      CurrencyConvertor::SetUniqueInstance(_YYYYMMDD_);
      p_uniqueinstance_ = new ExchangeSymbolManager(_YYYYMMDD_);
    } else if (p_uniqueinstance_->YYYYMMDD() != _YYYYMMDD_) {
      std::cerr << "ExchangeSymbolManager::SetUniqueInstance already called earlier with date: "
                << p_uniqueinstance_->YYYYMMDD() << ". While this date is: " << _YYYYMMDD_ << std::endl;
    }
  }

  static ExchangeSymbolManager* GetSingleInstance(const int _YYYYMMDD_) {
    return (new ExchangeSymbolManager(_YYYYMMDD_));
  }

  static ExchangeSymbolManager& GetUniqueInstance() {
    if (p_uniqueinstance_ == NULL) {
      ExitVerbose(kExchangeSymbolManagerUnset, "please call ExchangeSymbolManager::SetUniqueInstance first");
    }
    return *p_uniqueinstance_;  // unless initialized this will return an erroneous
  }

  static void RemoveUniqueInstance() {
    if (p_uniqueinstance_ != NULL) {
      delete p_uniqueinstance_;
      p_uniqueinstance_ = NULL;
    }
  }

  /** @brief to see what symbol the exchange is using for what we depict by this shortcode
   * see if symbol already computed in the map
   * else compute it and insert into map
   */
  static bool CheckIfContractSpecExists(const std::string& _shortcode_) {
    return GetUniqueInstance()._CheckIfContractSpecExists(_shortcode_);
  }
  static const char* GetExchSymbol(const std::string& _shortcode_) {
    return GetUniqueInstance()._GetExchSymbol(_shortcode_);
  }

  static void ReloadRolloverFile(std::string shortcode) {
    GetUniqueInstance().LoadRolloverOverrideFile(GetUniqueInstance().YYYYMMDD());
    GetUniqueInstance()._removefromlocalstore(shortcode);
  }

  static const char* GetExchSymbolCME(const std::string& _shortcode_) {
    return GetUniqueInstance()._GetExchSymbolCME(_shortcode_);
  }
  static const char* GetExchSymbolBMF(const std::string& _shortcode_) {
    return GetUniqueInstance()._GetExchSymbolBMF(_shortcode_);
  }
  static const char* GetExchSymbolTMX(const std::string& _shortcode_) {
    return GetUniqueInstance()._GetExchSymbolTMX(_shortcode_);
  }
  static const char* GetExchSymbolLIFFE(const std::string& _shortcode_) {
    return GetUniqueInstance()._GetExchSymbolLIFFE(_shortcode_);
  }
  static const char* GetExchSymbolICE(const std::string& _shortcode_) {
    return GetUniqueInstance()._GetExchSymbolICE(_shortcode_);
  }
  static const char* GetExchSymbolOSE(const std::string& _shortcode_) {
    return GetUniqueInstance()._GetExchSymbolOSE(_shortcode_);
  }
  static const char* GetExchSymbolEBS(const std::string& _shortcode_) {
    return GetUniqueInstance()._GetExchSymbolEBS(_shortcode_);
  }

  static int GetASXLastTradingDateYYYYMM_(const std::string& _pure_basename_, const int next_asx_month_,
                                          const int next_asx_year_) {
    return GetUniqueInstance().GetASXLastTradingDateYYYYMM(_pure_basename_, next_asx_month_, next_asx_year_);
  };

  const char* GetExchSymbolSingleInstance(const std::string& _shortcode_) { return _GetExchSymbol(_shortcode_); }

 protected:
  void LoadRolloverOverrideFile(int _YYYYMMDD_);
  void LoadVolumeBasedSymbolFile(int _YYYYMMDD_);
  //    void LoadMultipleVolumeBasedSymbolFile ( int _YYYYMMDD_ );
  void LoadStaticDIMappingFile(int _YYYYMMDD_);
  void LoadStaticYFEBMMappingFile(int _YYYYMMDD_);
  void LoadLastBMFTradingDatesOfMonth(const int _YYYYMMDD_);
  bool IsLastBMFTradingDateOfMonth(const int _YYYYMMDD_);
  bool IsTSEHoliday(const int _YYYYMMDD_);
  bool IsCombinedShortcode(const std::string& _this_shortcode_);

  const char* fromRolloverOverrideFile(const std::string& _shortcode_) const;
  const char* fromStaticDIMappingFile(const std::string& _shortcode_) const;
  const char* fromStaticYFEBMMappingFile(const std::string& _shortcode_) const;
  const char* fromVolumeSymbolFile(const std::string& _shortcode_) const;

  const bool _CheckIfContractSpecExists(const std::string& _shortcode_);

  const char* _GetExchSymbol(const std::string& _shortcode_);
  const char* ConvertToICESymbol(std::string _this_shortcode_);

  /// \brief fetches new exchange_symbol for CME corresponding to the given shortcode and constant YYYYMMDD_ of this
  /// ExchangeSymbolManager
  /// @param _shortcode_ our notation for this product
  const char* _GetExchSymbolCME(const std::string& _shortcode_);

  const char* _GetExchSymbolKRX(const std::string& _shortcode_);

  const char* _GetExchSymbolSGX(const std::string& _shortcode_);

  /// \brief fetches new exchange_symbol for BATSCHI corresponding to the given shortcode and constant YYYYMMDD_ of this
  /// ExchangeSymbolManager
  /// @param _shortcode_ our notation for this product
  const char* _GetExchSymbolBATSCHI(const std::string& _shortcode_);

  const char* _GetExchSymbolESPEED(const std::string& _shortcode_);
  /// \brief fetches new exchange_symbol for EUREX corresponding to the given shortcode and constant YYYYMMDD_ of this
  /// ExchangeSymbolManager
  /// @param _shortcode_ our notation for this product
  const char* _GetExchSymbolEUREX(const std::string& _shortcode_, bool is_spread);

  /// \brief fetches new exchange_symbol for BMF corresponding to the given shortcode and constant YYYYMMDD_ of this
  /// ExchangeSymbolManager
  /// @param _shortcode_ our notation for this product
  const char* _GetExchSymbolBMF(const std::string& _shortcode_);

  /// \brief fetches new exchange_symbol for TMX corresponding to the given shortcode and constant YYYYMMDD_ of this
  /// ExchangeSymbolManager
  /// @param _shortcode_ our notation for this product
  const char* _GetExchSymbolTMX(const std::string& _shortcode_);

  /// \brief fetches new exchange_symbol for LIFFE corresponding to the given shortcode and constant YYYYMMDD_ of this
  /// ExchangeSymbolManager
  /// @param _shortcode_ our notation for this product
  const char* _GetExchSymbolLIFFE(const std::string& _shortcode_);

  /// \brief fetches new exchange_symbol for LIFFE corresponding to the given shortcode and constant YYYYMMDD_ of this
  /// ExchangeSymbolManager
  /// @param _shortcode_ our notation for this product
  const char* _GetExchSymbolICE(const std::string& _shortcode_);                         // For outrights
  const char* _GetExchSymbolShortICE(const std::string& _shortcode_, bool _is_spread_);  // For spread components

  /// \brief fetches new exchange_symbol for RTS corresponding to the given shortcode and constant YYYYMMDD_ of this
  /// ExchangeSymbolManager
  /// @param _shortcode_ our notation for this product
  const char* _GetExchSymbolRTS(const std::string& _shortcode_);
  const char* _GetExchSymbolOSE(const std::string& _shortcode_);

  /// \brief fetches new exchange_symbol for HKEX corresponding to the given shortcode and constant YYYYMMDD_ of this
  /// ExchangeSymbolManager
  //    /// @param _shortcode_ our notation for this product
  const char* _GetExchSymbolHKEX(const std::string& _shortcode_);

  const char* _GetExchSymbolTSE(const std::string& _shortcode_);
  const char* _GetExchSymbolEBS(const std::string& _shortcode_);
  const char* _GetExchSymbolCFE(const std::string& _shortcode_, bool _is_spread_ = false, bool _is_expiry_ = false);
  const char* _GetExchSymbolASX(const std::string& _shortcode_, bool _is_spread_ = false);

  /// \brief stores computations already done ... does not check for duplicacy, and returns a pointer to the stored
  /// result
  inline const char* _localstore(const std::string& _shortcode_, const char* _src_str_) {
    if (large_es_db_front_index_ < LARGE_ES_DB_SIZE)  // we only support LARGE_ES_DB_SIZE number of shortcodes
    {
      // printf ( "ExchangeSymbolManager::_localstore Adding %s -> %s\n", _shortcode_.c_str(), _src_str_ );
      Char16_t _new_c_str_(_src_str_);
      large_es_db_[large_es_db_front_index_] = _new_c_str_;

      // mapping the shortcode to the permanent storage Char16_t, which are elements in large_es_db_
      // since the vector is defined already ( at startup in fact ), the location of elements in the vector will not
      // change
      // hence returning large_es_db_ [ current_index_ ].c_str_ is safe.
      // That is the address or char * large_es_db_ [ current_index_ ].c_str_ is likely to stay the same for the
      // remainder of the execution
      shortcode_exchange_symbol_map_[_shortcode_] = (large_es_db_[large_es_db_front_index_])();
      size_t current_index_ = large_es_db_front_index_;
      large_es_db_front_index_++;
      return large_es_db_[current_index_].c_str_;
    }
    return _src_str_;
  }

  /// \brief looks up previously computed exchange symbols for given _shortcode_, otherwise returns NULL
  inline const char* _findlocalstore(const std::string& _shortcode_) const {
    ShortcodeExchangeSymbolMap::const_iterator _citer_ = shortcode_exchange_symbol_map_.find(_shortcode_);
    if (_citer_ != shortcode_exchange_symbol_map_.end()) {
      return _citer_->second;
    }
    return NULL;
  }

  /// \brief removes the shortcode from the map
  inline void _removefromlocalstore(const std::string& _shortcode_) {
    auto _citer_ = shortcode_exchange_symbol_map_.find(_shortcode_);
    if (_citer_ != shortcode_exchange_symbol_map_.end()) {
      shortcode_exchange_symbol_map_.erase(_citer_);
    }
  }

  bool IsCMEMonth(const std::string& _pure_basename_, const int _this_month_);
  void SetToNextCMEMonth(const std::string& _pure_basename_, int& _next_cme_month_, int& _next_cme_year_);
  int GetCMELastTradingDateYYYYMM(const std::string& _pure_basename_, const int next_cme_month_,
                                  const int next_cme_year_);
  std::string GetCMESymbolFromLastTradingDate(const std::string& _pure_basename_,
                                              const int current_min_last_trading_date);

  bool IsSGXMonth(const std::string& _pure_basename_, const int _this_month_);
  void SetToNextSGXMonth(const std::string& _pure_basename_, int& _next_cme_month_, int& _next_cme_year_);
  int GetSGXLastTradingDateYYYYMM(const std::string& _pure_basename_, const int next_cme_month_,
                                  const int next_cme_year_);
  std::string GetSGXSymbolFromLastTradingDate(const std::string& _pure_basename_,
                                              const int current_min_last_trading_date);
  int GetDateNBusinessDaysBefore(std::string shortcode, int current_YYYYMMDD, int num_bussiness_days);

  bool IsEUREXMonth(const std::string& _pure_basename_, const int _this_month_);
  void SetToNextEUREXMonth(const std::string& _pure_basename_, int& _next_eurex_month_, int& _next_eurex_year_);
  int GetEUREXLastTradingDateYYYYMM(std::string _pure_basename_, const int next_eurex_month_,
                                    const int next_eurex_year_);
  // is_spread_ is to be used for ASX
  std::string GetEUREXSymbolFromLastTradingDate(std::string _pure_basename_, const int current_min_last_trading_date,
                                                bool _is_spread_ = false);

  std::string GetASXSymbolFromLastTradingDate(std::string _pure_basename_, const int current_min_last_trading_date,
                                              bool _is_spread_ = false);

  bool IsLIFFEMonth(const std::string& _pure_basename_, const int _this_month_, const int _this_year_ = -1);
  void SetToNextLIFFEMonth(const std::string& _pure_basename_, int& _next_eurex_month_, int& _next_eurex_year_);
  int GetLIFFELastTradingDateYYYYMM(std::string _pure_basename_, const int next_eurex_month_,
                                    const int next_eurex_year_);
  bool IsWheatCloseToExpiry(std::string _pure_basename_, const int current_month_, const int current_year_);
  void LoadLIFFESpreadMap(int YYYYMMDD_);

  char* _GetSpreadNameForProdLIFFE(const std::string& _shorcode1_, const std::string& _shorcode2_);
  char* _GetFlyNameForProdLIFFE(const std::string& _shorcode1_, const std::string& _shorcode2_,
                                const std::string& _shorcode3_);
  std::string GetLIFFESymbolFromLastTradingDate(std::string _pure_basename_, const int current_min_last_trading_date);
  std::string GetICESymbolFromLastTradingDate(std::string _pure_basename_, const int current_min_last_trading_date);
  std::string GetICEShortSymbolFromLastTradingDate(std::string _pure_basename_,
                                                   const int current_min_last_trading_date);

  bool IsBMFMonth(const std::string& _pure_basename_, const int _this_month_);
  void SetToNextBMFMonth(const std::string& _pure_basename_, int& _next_bmf_month_, int& _next_bmf_year_);
  int GetBMFLastTradingDateYYYYMM(const std::string& _pure_basename_, const int next_bmf_month_,
                                  const int next_bmf_year_);
  std::string GetBMFSymbolFromLastTradingDate(const std::string& _pure_basename_,
                                              const int current_min_last_trading_date);

  bool IsRTSMonth(const std::string& _pure_basename_, const int _this_month_);
  void SetToNextRTSMonth(const std::string& _pure_basename_, int& _next_rts_month_, int& _next_rts_year_);
  int GetRTSLastTradingDateYYYYMM(const std::string _pure_basename_, const int next_rts_month_,
                                  const int next_rts_year_);
  std::string GetRTSSymbolFromLastTradingDate(const std::string _pure_basename_,
                                              const int current_min_last_trading_date);

  /// Returns true if this is a contract month in TMX
  bool IsTMXMonth(const int _this_month_);
  void SetToNextTMXMonth(int& _next_tmx_month_, int& _next_tmx_year_);
  int GetTMXLastTradingDateYYYYMM(const std::string& _pure_basename_, const int next_tmx_month_,
                                  const int next_tmx_year_);
  std::string GetTMXSymbolFromLastTradingDate(const std::string& _pure_basename_,
                                              const int current_min_last_trading_date);

  // OSE
  bool IsOSEMonth(const std::string& _pure_basename_, const int _this_month_);
  bool IsOSEFrontMonth(const std::string& _pure_basename_, const int _this_month_);
  int GetOSELastTradingDateYYYYMM(const std::string& _pure_basename_, const int next_tmx_month_,
                                  const int next_tmx_year_);
  void SetToNextOSEMonth(const std::string& _pure_basename_, int& _next_ose_month_, int& _next_ose_year_);
  // HKEX
  int GetHKEXLastTradingDay(const std::string& _pure_basename_, const int next_tmx_month_, const int next_tmx_year_);

  bool IsTSEMonth(const std::string& _pure_basename_, const int _this_month_);
  void SetToNextTSEMonth(const std::string& _pure_basename_, int& _next_tse_month_, int& _next_tse_year_);
  int GetTSELastTradingDateYYYYMM(const std::string& _pure_basename_, const int next_tse_month_,
                                  const int next_tse_year_);
  std::string GetTSESymbolFromLastTradingDate(std::string _pure_basename_, const int current_min_last_trading_date);

  // CFE
  bool IsCFEMonth(const std::string& _pure_basename_, const int _this_month_);
  bool IsCFEWeek(const std::string& _pure_basename_, const int _this_week_, const int this_year);
  void SetToNextCFEMonth(const std::string& _pure_basename_, int& _next_cfe_month_, int& _next_cfe_year_);
  int GetCFELastTradingDateYYYYMM(const std::string& _pure_basename_, const int next_cfe_month_,
                                  const int next_cfe_year_);
  std::string GetCFESymbolFromLastTradingDate(std::string _pure_basename_, const int current_min_last_trading_date,
                                              bool _is_spread_ = false, bool _is_expiry_ = false);
  bool IsCFEHoliday(const int _YYYYMMDD_);

  // ASX
  bool IsASXMonth(const std::string& _pure_basename_, const int _this_month_);
  void SetToNextASXMonth(const std::string& _pure_basename_, int& _next_asx_month_, int& _next_asx_year_);
  int GetASXLastTradingDateYYYYMM(const std::string& _pure_basename_, const int next_asx_month_,
                                  const int next_asx_year_);
  // KRX
  bool IsKRXMonth(const std::string& _pure_basename_, const int _this_month_);
  void SetToNextKRXMonth(const std::string& _pure_basename_, int& _next_krx_month_, int& _next_krx_year_);
  int GetKRXLastTradingDateYYYYMM(const std::string& _pure_basename_, const int next_krx_month_,
                                  const int next_krx_year_);
  std::string GetKRXSymbolFromLastTradingDate(std::string _pure_basename_, const int current_min_last_trading_date);
};
}

#endif  // BASE_CDEF_EXCHANGE_SYMBOL_MANAGER_H
