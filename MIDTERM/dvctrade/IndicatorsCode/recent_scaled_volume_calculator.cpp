/**
    \file IndicatorsCode/recent_scaled_volume_calculator.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
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

#include "dvctrade/Indicators/recent_scaled_volume_calculator.hpp"

namespace HFSAT {

void RecentScaledVolumeCalculator::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                     std::vector<std::string>& _ors_source_needed_vec_,
                                                     const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

RecentScaledVolumeCalculator* RecentScaledVolumeCalculator::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                              const Watch& r_watch_,
                                                                              const std::vector<const char*>& r_tokens_,
                                                                              PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _num_levels_ _decay_factor_
  if (r_tokens_.size() < 5) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "RecentScaledVolumeCalculator incorrect syntax. Should be INDICATOR _this_weight_ _indicator_string_ "
                "_indep_market_view_ _fractional_seconds_ ");
  }
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]));
}

RecentScaledVolumeCalculator* RecentScaledVolumeCalculator::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const SecurityMarketView& _indep_market_view_,
    double _fractional_seconds_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _fractional_seconds_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, RecentScaledVolumeCalculator*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new RecentScaledVolumeCalculator(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _fractional_seconds_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

RecentScaledVolumeCalculator::RecentScaledVolumeCalculator(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                           const std::string& t_concise_indicator_description_,
                                                           const SecurityMarketView& _indep_market_view_,
                                                           double _fractional_seconds_)
    : CommonIndicator(t_dbglogger_, r_watch_, t_concise_indicator_description_),
      r_indep_market_view_(_indep_market_view_),
      p_recent_indep_volume_measure_(RecentSimpleVolumeMeasure::GetUniqueInstance(
          t_dbglogger_, r_watch_, _indep_market_view_, _fractional_seconds_)),
      linear_time_scaling_factor_(_fractional_seconds_ / 300),
      last_expected_volume_updated_msecs_(0),
      indep_rvm_(0),
      trading_volume_expected_(0),
      scaled_volume_(1.0),
      recent_scaled_volume_listener_vec_(),
      utc_time_to_vol_map_() {
  r_watch_.subscribe_BigTimePeriod(this);  // for UpdateLRVolumeInfo and updating volume adjustment
  p_recent_indep_volume_measure_->AddRecentSimpleVolumeListener(1u, this);
}

void RecentScaledVolumeCalculator::Initialize() {
  SampleDataUtil::GetAvgForPeriod(r_indep_market_view_.shortcode(), watch_.YYYYMMDD(), 60, trading_start_mfm_,
                                  trading_end_mfm_, "VOL", utc_time_to_vol_map_);
}
void RecentScaledVolumeCalculator::OnVolumeUpdate(unsigned int index_to_send_, double _new_volume_value_) {
  indep_rvm_ = _new_volume_value_;

  if (!is_ready_) {
    if (r_indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      Initialize();
    }
  } else {
    if (trading_volume_expected_ <= 0) {
      scaled_volume_ = 1.0;
    } else {
      scaled_volume_ = indep_rvm_ / trading_volume_expected_;
    }
  }

  indicator_value_ = scaled_volume_;
  NotifyIndicatorListeners(indicator_value_);
  NotifyListeners();
}

void RecentScaledVolumeCalculator::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if ((last_expected_volume_updated_msecs_ == 0) ||
      (watch_.msecs_from_midnight() - last_expected_volume_updated_msecs_ > TENMINUTESMSECS)) {
    int hhmmss = GetHHMMSSFromMsecsFromMidnight(watch_.msecs_from_midnight(), 300, 1);
    // Finding the previous time slot for accountingg for weightage
    int prev_hhmmss = GetHHMMSSFromMsecsFromMidnight(watch_.msecs_from_midnight(), 300, 0);
    int excess_minutes = (int)(((int)watch_.msecs_from_midnight() / 1000 - GetSecondsFromHHMMSS(prev_hhmmss)) / 60);

    if (utc_time_to_vol_map_.find(hhmmss) != utc_time_to_vol_map_.end()) {
      // weightage
      double vol_rate_per_half_hour_ = 0;
      if ((excess_minutes > 0) && (utc_time_to_vol_map_.find(prev_hhmmss) != utc_time_to_vol_map_.end())) {
        vol_rate_per_half_hour_ = (excess_minutes * (utc_time_to_vol_map_[hhmmss]) +
                                   (5 - excess_minutes) * (utc_time_to_vol_map_[prev_hhmmss])) /
                                  5.0;
      } else {
        vol_rate_per_half_hour_ = utc_time_to_vol_map_[hhmmss];
      }
      trading_volume_expected_ = vol_rate_per_half_hour_ * linear_time_scaling_factor_;
      // DBGLOG_TIME_CLASS << " currenttradingvolumeexpected of " << r_indep_market_view_.shortcode() << " = " <<
      // trading_volume_expected_ << " from " << vol_rate_per_half_hour_ << " hhmmss " << hhmmss << " excessminutes " <<
      // excess_minutes << DBGLOG_ENDL_FLUSH ;
    }

    last_expected_volume_updated_msecs_ =
        watch_.msecs_from_midnight() - (watch_.msecs_from_midnight() % TENMINUTESMSECS);
  }
}
}
