/**
   \file IndicatorsCode/time_based_regime.cpp

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
#include "dvctrade/Indicators/time_based_regime.hpp"

namespace HFSAT {

void TimeBasedRegime::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                        std::vector<std::string>& _ors_source_needed_vec_,
                                        const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

TimeBasedRegime* TimeBasedRegime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                    const std::vector<const char*>& r_tokens_,
                                                    PriceType_t _basepx_pxtype_) {
  // INDICATOR  _this_weight_  _indicator_string_  _dep_market_view_  _indep_market_view_  _fractional_seconds_
  // _volume_measure_seconds_  _trend_weight_  _price_type_
  int* regime_times_ = new int[r_tokens_.size() - 4];
  for (auto i = 0u; i < r_tokens_.size() - 4; i++) {
    const char* tz_hhmm_str_ = r_tokens_[i + 4];
    if ((strncmp(tz_hhmm_str_, "EST_", 4) == 0) || (strncmp(tz_hhmm_str_, "CST_", 4) == 0) ||
        (strncmp(tz_hhmm_str_, "CET_", 4) == 0) || (strncmp(tz_hhmm_str_, "BRT_", 4) == 0) ||
        (strncmp(tz_hhmm_str_, "UTC_", 4) == 0) || (strncmp(tz_hhmm_str_, "KST_", 4) == 0) ||
        (strncmp(tz_hhmm_str_, "HKT_", 4) == 0) || (strncmp(tz_hhmm_str_, "MSK_", 4) == 0) ||
        (strncmp(tz_hhmm_str_, "IST_", 4) == 0) || (strncmp(tz_hhmm_str_, "JST_", 4) == 0) ||
        (strncmp(tz_hhmm_str_, "BST_", 4) == 0) || (strncmp(tz_hhmm_str_, "AST_", 4) == 0)) {
      regime_times_[i] = GetMsecsFromMidnightFromHHMMSS(DateTime::GetUTCHHMMSSFromTZHHMMSS(
          r_watch_.YYYYMMDD(), DateTime::GetHHMMSSTime(tz_hhmm_str_ + 4), tz_hhmm_str_));
    } else {
      regime_times_[i] = GetMsecsFromMidnightFromHHMMSS(DateTime::GetHHMMSSTime(tz_hhmm_str_));
    }
  }
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])), regime_times_,
                           r_tokens_.size() - 4);
}

TimeBasedRegime* TimeBasedRegime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                    const SecurityMarketView& _dep_market_view_, int* regime_times_,
                                                    int regime_time_size_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname();

  for (int i = 0; i < regime_time_size_; i++) {
    t_temp_oss_ << regime_times_[i];
  }
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, TimeBasedRegime*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new TimeBasedRegime(
        t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, regime_times_, regime_time_size_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

TimeBasedRegime::TimeBasedRegime(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                 const std::string& concise_indicator_description_,
                                 const SecurityMarketView& t_dep_market_view_, int* regime_times_,
                                 const int regime_time_size_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      regime_times_(regime_times_),
      _regime_time_size_(regime_time_size_) {
  watch_.subscribe_BigTimePeriod(this);
  // InitializeValues();
  // is_ready_ = true;
}
void TimeBasedRegime::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if (current_time_slot_ == _regime_time_size_) {
    NotifyIndicatorListeners(current_time_slot_ + 1);
  } else {
    if (!is_ready_) {
      InitializeValues();

    } else {
      if (watch_.msecs_from_midnight() > next_time_) {
        current_time_slot_ = current_time_slot_ + 1;
        if (current_time_slot_ < _regime_time_size_) {
          next_time_ = regime_times_[current_time_slot_];
        } else {
          next_time_ = 0;
        }
        indicator_value_ = current_time_slot_ + 1;
      }
    }
    NotifyIndicatorListeners(current_time_slot_ + 1);
  }
}

void TimeBasedRegime::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}

void TimeBasedRegime::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                   const MarketUpdateInfo& _market_update_info_) {}

void TimeBasedRegime::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}

void TimeBasedRegime::OnMarketDataResumed(const unsigned int _security_id_) {}

void TimeBasedRegime::InitializeValues() {
  int i = 0;
  while ((i < _regime_time_size_) & (regime_times_[i] < watch_.msecs_from_midnight())) {
    i = i + 1;
  }
  current_time_slot_ = i;
  indicator_value_ = current_time_slot_ + 1;
  next_time_ = current_time_slot_ == _regime_time_size_ ? 0 : regime_times_[current_time_slot_];
  is_ready_ = true;
}
}
