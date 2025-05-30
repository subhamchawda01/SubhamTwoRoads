/**
   \file MinuteBar/minute_bar_strategy_desc.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#pragma once

#include <vector>
#include <iostream>

#include "baseinfra/MinuteBar/minute_bar_security_market_view.hpp"

namespace HFSAT {

struct MinuteBarStrategyLine {
  // inputs
  int runtime_id_;  ///< used for control screen, <dep_exchange_symbol_, runtime_id_> is used to send messages to the
  /// strategy from the control_screen

  // these variables are to be set during strategy initialization after trading date is known
  ttime_t trading_start_ttime_t_;
  ttime_t trading_end_ttime_t_;
  int trading_start_utc_mfm_;
  int trading_end_utc_mfm_;

  std::vector<MinuteBarSecurityMarketView*> dep_smv_list_;
  std::vector<MinuteBarSecurityMarketView*> indep_smv_list_;
  std::vector<std::vector<std::string>> signal_tokens_list_;
  ExchSource_t exch_traded_on_;

  // inputs
  std::vector<std::string> dep_shortcode_list_;    ///< the shortcode of the dependant
  std::vector<std::string> indep_shortcode_list_;  ///< the shortcode of the dependant
  std::string exec_name_;                          ///< name like "PriceBasedTrading"
  std::string config_file_;                        ///< the parameters for the trading strategy "PriceBasedTrading"

  MinuteBarStrategyLine()
      : runtime_id_(0),
        trading_start_ttime_t_(ttime_t(time_t(0), 0)),
        trading_end_ttime_t_(ttime_t(time_t(0), 0)),
        trading_start_utc_mfm_(8 * 60 * 60 * 1000),
        trading_end_utc_mfm_(8 * 60 * 60 * 1000),
        dep_smv_list_(),
        indep_smv_list_(),
        signal_tokens_list_(),
        exch_traded_on_(kExchSourceInvalid),
        dep_shortcode_list_(),
        indep_shortcode_list_(),
        exec_name_() {}
};

class MinuteBarStrategyDesc {
 public:
  std::vector<MinuteBarStrategyLine> strategy_vec_;
  MinuteBarStrategyDesc(DebugLogger& dbglogger, const std::string& strategy_desc_filename, const int tradingdate);
  std::vector<std::string> GetAllDepShortcodes();
  std::vector<std::string> GetAllSourceShortcodes();
  ttime_t GetMinStartTime();
  ttime_t GetMaxEndTime();

 protected:
  void GetTimeAndMFMFromString(int tradingdate, ttime_t& time, int& mfm, const char* tz_hhmm_str, bool is_end);
};
}
