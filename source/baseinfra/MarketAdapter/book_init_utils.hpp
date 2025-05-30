/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#pragma once

namespace HFSAT {

static inline bool CommonSimIndexedBookBool(const ExchSource_t& dep_exch_source_, const std::string& dep_shortcode_,
                                            TradingLocation_t trading_location = kTLocMAX, int _tradingdate_ = 0) {
  bool set_temporary_bool_checking_if_this_is_an_indexed_book_ = false;

  if ((dep_exch_source_ == HFSAT::kExchSourceLIFFE) || (dep_exch_source_ == HFSAT::kExchSourceICE) ||
      (dep_exch_source_ == HFSAT::kExchSourceTSE) || (dep_exch_source_ == HFSAT::kExchSourceEUREX) ||
      (dep_exch_source_ == HFSAT::kExchSourceCME) || (dep_exch_source_ == HFSAT::kExchSourceMICEX) ||
      (dep_exch_source_ == HFSAT::kExchSourceMICEX_EQ) || (dep_exch_source_ == HFSAT::kExchSourceMICEX_CR) ||
      (dep_exch_source_ == HFSAT::kExchSourceHONGKONG) || (dep_exch_source_ == HFSAT::kExchSourceHKOMD) ||
      (dep_exch_source_ == HFSAT::kExchSourceHKOMDCPF) || (dep_exch_source_ == HFSAT::kExchSourceHKOMDPF) ||
      (dep_exch_source_ == HFSAT::kExchSourceESPEED) || (dep_exch_source_ == HFSAT::kExchSourceEOBI) ||
      (dep_exch_source_ == HFSAT::kExchSourceBMF) || (dep_exch_source_ == HFSAT::kExchSourceNTP) ||
      (dep_exch_source_ == HFSAT::kExchSourceBMFEQ) || (dep_exch_source_ == HFSAT::kExchSourceRTS) ||
      ((dep_exch_source_ == HFSAT::kExchSourceJPY) &&
       ((trading_location != HFSAT::kTLocHK) || (_tradingdate_ >= 20151125 && _tradingdate_ <= 20151218) ||
        (_tradingdate_ >= 20160719))) ||
      (dep_exch_source_ == HFSAT::kExchSourceCFE) || (dep_exch_source_ == HFSAT::kExchSourceTMX) ||
      (dep_exch_source_ == HFSAT::kExchSourceQUINCY) || (dep_exch_source_ == HFSAT::kExchSourceASX) ||
      (dep_exch_source_ == HFSAT::kExchSourceNSE) || (dep_exch_source_ == HFSAT::kExchSourceSGX) ||
      (dep_exch_source_ == HFSAT::kExchSourceBSE) || (dep_exch_source_ == HFSAT::kExchSourceKRX)) {
    set_temporary_bool_checking_if_this_is_an_indexed_book_ = true;
  }

  return set_temporary_bool_checking_if_this_is_an_indexed_book_;
}

static inline bool UseEOBIData(const HFSAT::TradingLocation_t _dep_trading_location_, const int _tradingdate_,
                               const std::string& _shc_) {
  HFSAT::ExchSource_t t_exch_source_ = HFSAT::SecurityDefinitions::GetContractExchSource(_shc_, _tradingdate_);
  if (t_exch_source_ == HFSAT::kExchSourceEUREX) {
    if (((_dep_trading_location_ == HFSAT::kTLocFR2) && (_tradingdate_ >= 20131228)) ||
        ((_dep_trading_location_ == HFSAT::kTLocBSL) && (_tradingdate_ >= 20140108)) ||
        ((_dep_trading_location_ == HFSAT::kTLocCHI) && (_tradingdate_ >= 20140115)) ||
        ((_dep_trading_location_ == HFSAT::kTLocHK) && (_tradingdate_ >= 20140115)) ||
        ((_dep_trading_location_ == HFSAT::kTLocJPY) && (_tradingdate_ >= 20140115)) ||
        ((_dep_trading_location_ == HFSAT::kTLocBMF) && (_tradingdate_ >= 20140203)) ||
        ((_dep_trading_location_ == HFSAT::kTLocM1) && (_tradingdate_ >= 20140211)) ||
        ((_dep_trading_location_ == HFSAT::kTLocCFE) && (_tradingdate_ >= 20140425)) ||
        (_dep_trading_location_ == HFSAT::kTLocSYD) || (_dep_trading_location_ == HFSAT::kTLocNSE) ||
        (_dep_trading_location_ == HFSAT::kTLocSPR) || (_tradingdate_ >= 20151120) ||
        ((_shc_ == "FGBL_0") && (_dep_trading_location_ == HFSAT::kTLocTMX) && (_tradingdate_ >= 20150128))) {
      return true;
    }
  }
  return false;
}

static inline bool UseHKOMDData(const HFSAT::TradingLocation_t _dep_trading_location_, const int _tradingdate_,
                                HFSAT::ExchSource_t t_exch_source_) {
  if ((t_exch_source_ == HFSAT::kExchSourceHONGKONG) &&
      ((_tradingdate_ >= 20150304) || (_dep_trading_location_ == HFSAT::kTLocHK && (_tradingdate_ >= 20141204)) ||
       (_dep_trading_location_ == HFSAT::kTLocJPY && (_tradingdate_ >= 20150121)))) {
    return true;
  }
  return false;
}
}
