/**
    \file IndicatorsCode/Stdev_based_regime.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/stdev_ratio_regime.hpp"

namespace HFSAT {

void StdevRatioRegime::CollectShortCodes(std::vector<std::string>& t_shortcodes_affecting_this_indicator,
                                         std::vector<std::string>& ors_source_needed_vec,
                                         const std::vector<const char*>& r_tokens) {
  VectorUtils::UniqueVectorAdd(t_shortcodes_affecting_this_indicator, (std::string)r_tokens[3]);
  if (IndicatorUtil::IsPortfolioShortcode(r_tokens[4])) {
    std::vector<std::string> shortcode_vec;
    IndicatorUtil::GetPortfolioShortCodeVec(r_tokens[4], shortcode_vec);
    VectorUtils::UniqueVectorAdd(t_shortcodes_affecting_this_indicator, shortcode_vec);
  } else {
    VectorUtils::UniqueVectorAdd(t_shortcodes_affecting_this_indicator, (std::string)r_tokens[4]);
  }
}

StdevRatioRegime* StdevRatioRegime::GetUniqueInstance(DebugLogger& t_dbglogger, const Watch& r_watch,
                                                      const std::vector<const char*>& r_tokens,
                                                      PriceType_t basepx_pxtype) {
  // INDICATOR this_weight_ indicator_string_ dep_market_view_ indep_market_view_ fractional_seconds_ price_type_
  // switch_threshold_
  if (r_tokens.size() < 8) {
    ExitVerbose(kExitErrorCodeGeneral,
                " StdevRatioRegime: Incomplete Argument\n INDICATOR this_weight_ indicator_string_ "
                "_dep_market_view_ indep_market_view_ fractional_seconds_ price_type_ switch_threshold_");
  }
  return GetUniqueInstance(t_dbglogger, r_watch,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens[3])),
                           std::string(r_tokens[4]), atof(r_tokens[5]), atof(r_tokens[6]), (atoi(r_tokens[7]) != 0));
}

StdevRatioRegime* StdevRatioRegime::GetUniqueInstance(DebugLogger& t_dbglogger, const Watch& r_watch,
                                                      SecurityMarketView& dep_market_view,
                                                      const std::string& indep_name, double fractional_seconds,
                                                      double switch_threshold, bool scaled_stdev) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << dep_market_view.secname() << ' ' << indep_name << ' ' << fractional_seconds << ' '
              << switch_threshold << ' ' << scaled_stdev;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, StdevRatioRegime*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new StdevRatioRegime(t_dbglogger, r_watch, concise_indicator_description_, dep_market_view, indep_name,
                             fractional_seconds, switch_threshold, scaled_stdev);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

StdevRatioRegime::StdevRatioRegime(DebugLogger& t_dbglogger, const Watch& r_watch,
                                   const std::string& concise_indicator_description,
                                   SecurityMarketView& dep_market_view, const std::string& indep_name,
                                   double fractional_seconds, double switch_threshold, bool scaled_stdev)
    : CommonIndicator(t_dbglogger, r_watch, concise_indicator_description),
      dep_market_view_(dep_market_view),
      indep_market_view_vec_(),
      trend_history_msecs_(std::max(MIN_MSEC_HISTORY_INDICATORS, (int)round(1000 * fractional_seconds))),
      switch_threshold_(switch_threshold),
      inverse_switch_threshold_(1.00),
      dep_Stdev_ratio_(1.0),
      indep_Stdev_ratio_(1.0),
      dep_source_Stdev_ratio_(1.0),
      dep_Stdev_ratio_calculator_(nullptr),
      indep_Stdev_ratio_calculator_(nullptr) {
  ///

  watch_.subscribe_FifteenSecondPeriod(this);

  if (switch_threshold != 0) {
    inverse_switch_threshold_ = 1.00 / switch_threshold;
  } else {
    inverse_switch_threshold_ = 100;
  }

  int idx = 0;
  dep_Stdev_ratio_calculator_ =
      StdevRatioCalculator::GetUniqueInstance(t_dbglogger, watch_, dep_market_view, fractional_seconds, scaled_stdev);
  if (dep_Stdev_ratio_calculator_ != nullptr) {
    dep_Stdev_ratio_calculator_->AddStdevRatioListener(idx, this);
  }

  idx++;
  if (IndicatorUtil::IsPortfolioShortcode(indep_name)) {
    /// If its portfolio then calculate portfolio vol-ratio
    std::vector<std::string> shortcode_vec;
    IndicatorUtil::GetPortfolioShortCodeVec(indep_name, shortcode_vec);
    (ShortcodeSecurityMarketViewMap::GetUniqueInstance())
        .GetSecurityMarketViewVec(shortcode_vec, indep_market_view_vec_);
    //    indep_Stdev_ratio_calculator_port_ =
    //        StdevRatioCalculatorPort2::GetUniqueInstance(dbglogger_, watch_, indep_name, fractional_seconds);
    //    if (indep_Stdev_ratio_calculator_port_ != nullptr) {
    //      indep_Stdev_ratio_calculator_port_->AddStdevRatioListener(idx, this);
    //    }
  } else {
    indep_market_view_vec_.push_back(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(indep_name));
    indep_Stdev_ratio_calculator_ = StdevRatioCalculator::GetUniqueInstance(
        t_dbglogger, watch_, *indep_market_view_vec_[0], fractional_seconds, scaled_stdev);
    if (indep_Stdev_ratio_calculator_ != nullptr) {
      indep_Stdev_ratio_calculator_->AddStdevRatioListener(idx, this);
    }
  }

  DBGLOG_TIME_CLASS_FUNC_LINE << " switch_thresh: " << switch_threshold_ << " inv: " << inverse_switch_threshold_
                              << DBGLOG_ENDL_FLUSH;
  indicator_value_ = 3;
}

void StdevRatioRegime::OnTimePeriodUpdate(const int num_pages_to_add) { ComputeRegime(); }

void StdevRatioRegime::OnStdevRatioUpdate(const unsigned int index_to_send, const double& r_new_scaled_Stdev_value) {
  if (!is_ready_) {
    if (dep_Stdev_ratio_calculator_->IsIndicatorReady()) {
      if (indep_Stdev_ratio_calculator_ && indep_Stdev_ratio_calculator_->IsIndicatorReady()) {
        is_ready_ = true;
      }
      //      if (indep_Stdev_ratio_calculator_port_ && indep_Stdev_ratio_calculator_port_->IsIndicatorReady()) {
      //        is_ready_ = true;
      //      }
    }
  }

  if (index_to_send == 0u) {
    dep_Stdev_ratio_ = r_new_scaled_Stdev_value;
  } else if (index_to_send == 1u) {
    indep_Stdev_ratio_ = r_new_scaled_Stdev_value;
  }

  if (is_ready_) {
    dep_source_Stdev_ratio_ = dep_Stdev_ratio_ / indep_Stdev_ratio_;
  }
  //  DBGLOG_TIME_CLASS_FUNC << " Dep: " << dep_Stdev_ratio_ << " Indep: " << indep_Stdev_ratio_
  //                         << " ratio: " << dep_source_Stdev_ratio_ << DBGLOG_ENDL_FLUSH;
}

void StdevRatioRegime::ComputeRegime() {
  //  DBGLOG_TIME_CLASS_FUNC << " Dep: " << dep_source_Stdev_ratio_ << " thresh: " << switch_threshold_
  //                         << DBGLOG_ENDL_FLUSH;
  if (dep_source_Stdev_ratio_ >= switch_threshold_) {
    // dependent Stdev is very high compared to indep
    indicator_value_ = 1u;
  } else if (dep_source_Stdev_ratio_ <= inverse_switch_threshold_) {
    // indep Stdev is very high compared to dep
    indicator_value_ = 2u;
  } else {
    // Stdevs in both dep and indep are normal
    indicator_value_ = 3u;
  }
  NotifyIndicatorListeners(indicator_value_);
}

// market_interrupt_listener interface
void StdevRatioRegime::OnMarketDataInterrupted(const unsigned int security_id, const int msecs_since_last_receive) {
  indicator_value_ = 3u;
}

void StdevRatioRegime::OnMarketDataResumed(const unsigned int security_id) { indicator_value_ = 3u; }
}
