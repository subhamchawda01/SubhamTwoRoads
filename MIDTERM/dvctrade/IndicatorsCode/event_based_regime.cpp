/**
   \file IndicatorsCode/event_based_regime.cpp

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
#include "dvctrade/Indicators/event_based_regime.hpp"

namespace HFSAT {

void EventBasedRegime::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                         std::vector<std::string>& _ors_source_needed_vec_,
                                         const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

EventBasedRegime* EventBasedRegime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      const std::vector<const char*>& r_tokens_,
                                                      PriceType_t _basepx_pxtype_) {
  // INDICATOR  _this_weight_  _indicator_string_  _dep_market_view_ _event_zone_ _applicable_severity_ _start_lag_mins_
  // _end_lag_mins_
  // INDICATOR 1.00 EventBasedRegime 6M_0 USD 3 1 10
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);

  if (r_tokens_.size() == 8 && (atoi(r_tokens_[6]) < atoi(r_tokens_[7]))) {
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])), r_tokens_[4],
                             atoi(r_tokens_[5]), atoi(r_tokens_[6]), atoi(r_tokens_[7]));
  } else {
    std::cerr << "EventBasedRegime syntax incorrect! given text:";
    for (auto i = 0u; i < r_tokens_.size(); i++) {
      std::cerr << " " << r_tokens_[i];
    }
    std::cerr << std::endl;
    std::cerr << "Expected syntax: INDICATOR  _this_weight_ EventBasedRegime _dep_market_view_ _event_zone_ "
                 "_applicable_severity_ _start_lag_mins_ _end_lag_mins_  "
              << std::endl;

    exit(1);
  }
}

EventBasedRegime* EventBasedRegime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      SecurityMarketView& _dep_market_view_, std::string _event_zone_,
                                                      int _applicable_severity_, int _start_lag_mins_,
                                                      int _end_lag_mins_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _event_zone_ << ' ' << _applicable_severity_
              << ' ' << _start_lag_mins_ << ' ' << _end_lag_mins_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, EventBasedRegime*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new EventBasedRegime(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _event_zone_,
                             _applicable_severity_, _start_lag_mins_, _end_lag_mins_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

EventBasedRegime::EventBasedRegime(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                   const std::string& concise_indicator_description_,
                                   SecurityMarketView& t_dep_market_view_, std::string _event_zone_,
                                   int _applicable_severity_, int _start_lag_mins_, int _end_lag_mins_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      economic_events_manager_(t_dbglogger_, r_watch_, t_dep_market_view_.shortcode()),
      ezone_traded_(GetEZFromStr(_event_zone_)),
      applicable_severity_(_applicable_severity_),
      event_start_lag_msecs_(_start_lag_mins_ * 60 * 1000),
      event_end_lag_msecs_(_end_lag_mins_ * 60 * 1000) {
  watch_.subscribe_FifteenSecondPeriod(this);

  dep_market_view_.subscribe_L2(this);
  InitializeValues();
}

void EventBasedRegime::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if (event_vec_size_ > 0 && current_idx_ < event_vec_size_) {
    if (indicator_value_ == 1 && watch_.msecs_from_midnight() >= event_time_for_day_[current_idx_].event_start_mfm_) {
      indicator_value_ = 2;
      NotifyIndicatorListeners(indicator_value_);
    } else if (indicator_value_ == 2 &&
               watch_.msecs_from_midnight() >= event_time_for_day_[current_idx_].event_end_mfm_) {
      indicator_value_ = 1;
      current_idx_ += 1;
      NotifyIndicatorListeners(indicator_value_);
    } else if (indicator_value_ == 2 && current_idx_ + 1 < event_vec_size_ &&
               watch_.msecs_from_midnight() >= event_time_for_day_[current_idx_ + 1].event_start_mfm_) {
      current_idx_ += 1;
    }
  }
}

void EventBasedRegime::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    } else {
      is_ready_ = true;
      indicator_value_ = 1;
      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

// market_interrupt_listener interface
void EventBasedRegime::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    data_interrupted_ = true;
    indicator_value_ = 1;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void EventBasedRegime::OnMarketDataResumed(const unsigned int _security_id_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    data_interrupted_ = false;
  }
}

void EventBasedRegime::InitializeValues() {
  const std::vector<HFSAT::EventLine>& events_of_the_day_ = economic_events_manager_.events_of_the_day();
  for (auto i = 0u; i < events_of_the_day_.size(); i++) {
    if (events_of_the_day_[i].ez_ == ezone_traded_ && events_of_the_day_[i].severity_ >= applicable_severity_) {
      int t_event_start_mfm_ = events_of_the_day_[i].event_mfm_ - event_start_lag_msecs_;
      int t_event_end_mfm_ = events_of_the_day_[i].event_mfm_ + event_end_lag_msecs_;
      HFSAT::EventTime t_event_time_(t_event_start_mfm_, t_event_end_mfm_);
      event_time_for_day_.push_back(t_event_time_);
    }
  }
  event_vec_size_ = event_time_for_day_.size();
  current_idx_ = event_vec_size_ > 0 ? 0 : -1;
  indicator_value_ = 1;
}
}
