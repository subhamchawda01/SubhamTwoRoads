/**
    \file IndicatorsCode/stdev_ratio_calculator.cpp

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

#include "dvctrade/Indicators/stdev_ratio_calculator.hpp"

namespace HFSAT {

void StdevRatioCalculator::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                             std::vector<std::string>& _ors_source_needed_vec_,
                                             const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

StdevRatioCalculator* StdevRatioCalculator::GetUniqueInstance(DebugLogger& dbglogger, const Watch& watch,
                                                              const std::vector<const char*>& tokens,
                                                              PriceType_t basepx_pxtype) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _num_levels_ _decay_factor_
  if (tokens.size() < 6) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, dbglogger,
                "StdevRatioCalculator incorrect syntax. Should be INDICATOR _this_weight_ _indicator_string_ "
                "_indep_market_view_ _fractional_seconds_ ");
  }
  return GetUniqueInstance(dbglogger, watch, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(tokens[3])),
                           atof(tokens[4]), (atoi(tokens[5]) != 0));
}

StdevRatioCalculator* StdevRatioCalculator::GetUniqueInstance(DebugLogger& dbglogger, const Watch& watch,
                                                              const SecurityMarketView& indep_market_view,
                                                              double fractional_seconds, bool scaled_vol) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << indep_market_view.secname() << ' ' << fractional_seconds << ' ' << scaled_vol;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, StdevRatioCalculator*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new StdevRatioCalculator(
        dbglogger, watch, concise_indicator_description_, indep_market_view, fractional_seconds, scaled_vol);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

StdevRatioCalculator::StdevRatioCalculator(DebugLogger& dbglogger, const Watch& watch,
                                           const std::string& concise_indicator_description,
                                           const SecurityMarketView& indep_market_view, double fractional_seconds,
                                           bool scaled_vol)
    : CommonIndicator(dbglogger, watch, concise_indicator_description),
      indep_market_view_(indep_market_view),
      slow_stdev_calculator_(nullptr),
      scaling_factor_(sqrt(2) * (fractional_seconds / 600.0)),  // It's computed ever 5 minutes in sample data (against
                                                                // 10 minutes in PeriodicStdev).. so the scaling factor
                                                                // is sqrt(2) times the prev version's scaling factor
      stdev_expected_(0.0),
      stdev_ratio_(1.0),
      last_expected_stdev_updated_msecs_(0),
      scaled_vol_(scaled_vol),
      stdev_ratio_listener_vec_(),
      utc_time_to_stdev_map_() {
  //

  watch.subscribe_FifteenSecondPeriod(this);
  slow_stdev_calculator_ = SlowStdevCalculator::GetUniqueInstance(dbglogger, watch, indep_market_view.shortcode(),
                                                                  fractional_seconds * 1000, 0.05);
  slow_stdev_calculator_->AddSlowStdevCalculatorListener(this);
}

void StdevRatioCalculator::Initialize() {
  SampleDataUtil::GetAvgForPeriod(indep_market_view_.shortcode(), watch_.YYYYMMDD(), 60, trading_start_mfm_,
                                  trading_end_mfm_, "STDEV", utc_time_to_stdev_map_);

  if (utc_time_to_stdev_map_.empty()) {
    std::stringstream exit_stream_;
    exit_stream_ << "Sample Stdev Data  not found for: " << indep_market_view_.secname() << " "
                 << indep_market_view_.shortcode() << "\n";
    ExitVerbose(kExitErrorCodeZeroValue, dbglogger_, exit_stream_.str().c_str());
  }
  stdev_expected_ = 0;
  for (auto iter = utc_time_to_stdev_map_.begin(); iter != utc_time_to_stdev_map_.end(); iter++) {
    stdev_expected_ += iter->second;
  }
  stdev_expected_ /= utc_time_to_stdev_map_.size();
  stdev_expected_ *= scaling_factor_;
  DBGLOG_TIME_CLASS_FUNC << " StdevExpected: " << stdev_expected_ << " shortcode: " << indep_market_view_.shortcode()
                         << DBGLOG_ENDL_FLUSH;
}

void StdevRatioCalculator::OnStdevUpdate(const unsigned int security_id, const double& new_stdev_value) {
  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      Initialize();
    }
  }

  if (is_ready_) {
    if (stdev_expected_ <= 0) {
      stdev_ratio_ = 1.0;
    } else {
      stdev_ratio_ = new_stdev_value / stdev_expected_;
    }
    //    DBGLOG_TIME_CLASS_FUNC << " new_std: " << new_stdev_value << " expected:  " << stdev_expected_
    //                           << " ratio: " << stdev_ratio_ << DBGLOG_ENDL_FLUSH;
    indicator_value_ = stdev_ratio_;
    NotifyIndicatorListeners(indicator_value_);
    NotifyListeners();
  }
}

void StdevRatioCalculator::OnTimePeriodUpdate(const int num_pages_to_add) {
  if (scaled_vol_) {
    if ((last_expected_stdev_updated_msecs_ == 0) ||
        (watch_.msecs_from_midnight() - last_expected_stdev_updated_msecs_ > TWOMINUTESMSECS)) {
      int hhmmss = GetHHMMSSFromMsecsFromMidnight(watch_.msecs_from_midnight(), 600, 1);

      if (utc_time_to_stdev_map_.find(hhmmss) != utc_time_to_stdev_map_.end()) {
        // weight
        stdev_expected_ = utc_time_to_stdev_map_[hhmmss] * scaling_factor_;
      }

      last_expected_stdev_updated_msecs_ =
          watch_.msecs_from_midnight() - (watch_.msecs_from_midnight() % TWOMINUTESMSECS);
    }
  }
}
void StdevRatioCalculator::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                   const int msecs_since_last_receive_) {
  data_interrupted_ = true;
}

void StdevRatioCalculator::OnMarketDataResumed(const unsigned int _security_id_) { data_interrupted_ = false; }
}
