/**
   \file IndicatorsCode/time_based_regime_binary.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 351, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include <fstream>
#include <strings.h>
#include <stdlib.h>
#include <sstream>

#include "dvccode/CDef/math_utils.hpp"

#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvctrade/Indicators/time_based_regime_binary.hpp"

namespace HFSAT {

void TimeBasedRegimeBinary::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                              std::vector<std::string>& _ors_source_needed_vec_,
                                              const std::vector<const char*>& r_tokens_) {}

TimeBasedRegimeBinary* TimeBasedRegimeBinary::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                const std::vector<const char*>& r_tokens_,
                                                                PriceType_t _basepx_pxtype_) {
  // INDICATOR  _this_weight_  _indicator_string_  _first_break_start_ _first_break_end_ _second_break_start_
  // _second_break_end_  ...
  unsigned int* regime_times_ = new unsigned int[r_tokens_.size() - 3];
  const int num_time_tokens_ = r_tokens_.size() - 3;
  for (auto i = 0u; i < r_tokens_.size() - 3; i++) {
    const char* tz_hhmm_str_ = r_tokens_[i + 3];
    if ((strncmp(tz_hhmm_str_, "EST_", 4) == 0) || (strncmp(tz_hhmm_str_, "CST_", 4) == 0) ||
        (strncmp(tz_hhmm_str_, "CET_", 4) == 0) || (strncmp(tz_hhmm_str_, "BRT_", 4) == 0) ||
        (strncmp(tz_hhmm_str_, "UTC_", 4) == 0) || (strncmp(tz_hhmm_str_, "KST_", 4) == 0) ||
        (strncmp(tz_hhmm_str_, "HKT_", 4) == 0) || (strncmp(tz_hhmm_str_, "MSK_", 4) == 0) ||
        (strncmp(tz_hhmm_str_, "IST_", 4) == 0) || (strncmp(tz_hhmm_str_, "JST_", 4) == 0) ||
        (strncmp(tz_hhmm_str_, "BST_", 4) == 0) || (strncmp(tz_hhmm_str_, "AST_", 4) == 0)) {
      // get time in msecs_from_midnight for each time token
      regime_times_[i] = GetMsecsFromMidnightFromHHMMSS(DateTime::GetUTCHHMMSSFromTZHHMMSS(
          r_watch_.YYYYMMDD(), DateTime::GetHHMMSSTime(tz_hhmm_str_ + 4), tz_hhmm_str_));
    } else {
      regime_times_[i] = GetMsecsFromMidnightFromHHMMSS(DateTime::GetHHMMSSTime(tz_hhmm_str_));
    }
  }
  return GetUniqueInstance(t_dbglogger_, r_watch_, regime_times_, num_time_tokens_);
}

TimeBasedRegimeBinary* TimeBasedRegimeBinary::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                unsigned int* regime_times_,
                                                                const int _num_time_tokens_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName();

  for (int i = 0; i < _num_time_tokens_; i++) {
    t_temp_oss_ << ' ' << regime_times_[i];
  }
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, TimeBasedRegimeBinary*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new TimeBasedRegimeBinary(t_dbglogger_, r_watch_, concise_indicator_description_, regime_times_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

TimeBasedRegimeBinary::TimeBasedRegimeBinary(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                             const std::string& concise_indicator_description_,
                                             unsigned int* regime_times_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      regime_times_(regime_times_),
      current_time_slot_(0),
      next_time_(regime_times_[0]) {
  watch_.subscribe_BigTimePeriod(this);
}

void TimeBasedRegimeBinary::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if (watch_.msecs_from_midnight() > next_time_) {
    current_time_slot_ = current_time_slot_ + 1;
    next_time_ = regime_times_[current_time_slot_];
  }

  // Time slot given in pairs... 0 for no break and 1 for break
  if ((current_time_slot_ % 2) == 0) {
    indicator_value_ = 1;
  } else {
    indicator_value_ = 2;
  }

  NotifyIndicatorListeners(indicator_value_);
}

void TimeBasedRegimeBinary::OnMarketUpdate(const unsigned int _security_id_,
                                           const MarketUpdateInfo& _market_update_info_) {}

void TimeBasedRegimeBinary::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                         const MarketUpdateInfo& _market_update_info_) {}

void TimeBasedRegimeBinary::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                    const int msecs_since_last_receive_) {}

void TimeBasedRegimeBinary::OnMarketDataResumed(const unsigned int _security_id_) {}
}
