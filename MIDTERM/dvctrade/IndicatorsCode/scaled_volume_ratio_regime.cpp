/**
    \file IndicatorsCode/volume_based_regime.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/recent_scaled_volume_calculator.hpp"
#include "dvctrade/Indicators/scaled_volume_ratio_regime.hpp"

namespace HFSAT {

void ScaledVolumeRatioRegime::CollectShortCodes(std::vector<std::string>& t_shortcodes_affecting_this_indicator_,
                                                std::vector<std::string>& _ors_source_needed_vec_,
                                                const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(t_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  if (IndicatorUtil::IsPortfolioShortcode(r_tokens_[4])) {
    std::vector<std::string> shortcode_vec;
    IndicatorUtil::GetPortfolioShortCodeVec(r_tokens_[4], shortcode_vec);
    VectorUtils::UniqueVectorAdd(t_shortcodes_affecting_this_indicator_, shortcode_vec);
  } else {
    VectorUtils::UniqueVectorAdd(t_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
  }
}

ScaledVolumeRatioRegime* ScaledVolumeRatioRegime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                    const std::vector<const char*>& r_tokens_,
                                                                    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _indep_market_view_ _fractional_seconds_ _price_type_
  // _switch_threshold_
  if (r_tokens_.size() < 8) {
    ExitVerbose(kExitErrorCodeGeneral,
                " ScaledVolumeRatioRegime: Incomplete Argument\n INDICATOR _this_weight_ _indicator_string_ "
                "_dep_market_view_ _indep_market_view_ _fractional_seconds_ _price_type_ _switch_threshold_");
  }
  return GetUniqueInstance(
      t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
      std::string(r_tokens_[4]), atof(r_tokens_[5]), atof(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
}

ScaledVolumeRatioRegime* ScaledVolumeRatioRegime::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, SecurityMarketView& _dep_market_view_,
    const std::string& indep_name, double _fractional_seconds_, double _switch_threshold_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << indep_name << ' ' << _fractional_seconds_
              << ' ' << _switch_threshold_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, ScaledVolumeRatioRegime*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new ScaledVolumeRatioRegime(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                                    indep_name, _fractional_seconds_, _switch_threshold_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

ScaledVolumeRatioRegime::ScaledVolumeRatioRegime(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                 const std::string& concise_indicator_description_,
                                                 SecurityMarketView& _dep_market_view_, const std::string& indep_name,
                                                 double _fractional_seconds_, double switch_threshold,
                                                 PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_market_view_vec_(),
      trend_history_msecs_(std::max(MIN_MSEC_HISTORY_INDICATORS, (int)round(1000 * _fractional_seconds_))),
      switch_threshold_(switch_threshold),
      inverse_switch_threshold_(1.00),
      dep_volume_ratio_(1.0),
      indep_volume_ratio_(1.0),
      dep_source_volume_ratio_(1.0),
      dep_volume_ratio_calculator_(nullptr),
      indep_volume_ratio_calculator_(nullptr),
      indep_volume_ratio_calculator_port_(nullptr) {
  ///

  watch_.subscribe_FifteenSecondPeriod(this);

  if (switch_threshold != 0) {
    inverse_switch_threshold_ = 1.00 / switch_threshold;
  } else {
    inverse_switch_threshold_ = 100;
  }

  int idx = 0;
  dep_volume_ratio_calculator_ =
      RecentScaledVolumeCalculator::GetUniqueInstance(t_dbglogger_, watch_, _dep_market_view_, _fractional_seconds_);
  if (dep_volume_ratio_calculator_ != nullptr) {
    dep_volume_ratio_calculator_->AddRecentScaledVolumeListener(idx, this);
  }

  idx++;
  if (IndicatorUtil::IsPortfolioShortcode(indep_name)) {
    /// If its portfolio then calculate portfolio-vol-ratio

    std::vector<std::string> shortcode_vec;
    IndicatorUtil::GetPortfolioShortCodeVec(indep_name, shortcode_vec);
    (ShortcodeSecurityMarketViewMap::GetUniqueInstance())
        .GetSecurityMarketViewVec(shortcode_vec, indep_market_view_vec_);
    indep_volume_ratio_calculator_port_ =
        RecentScaledVolumeCalculatorPort2::GetUniqueInstance(t_dbglogger_, watch_, indep_name, _fractional_seconds_);
    if (indep_volume_ratio_calculator_port_ != nullptr) {
      indep_volume_ratio_calculator_port_->AddRecentScaledVolumeListener(idx, this);
    }
  } else {
    indep_volume_ratio_calculator_ = RecentScaledVolumeCalculator::GetUniqueInstance(
        t_dbglogger_, watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(indep_name)),
        _fractional_seconds_);
    if (indep_volume_ratio_calculator_ != nullptr) {
      indep_volume_ratio_calculator_->AddRecentScaledVolumeListener(idx, this);
    }
  }
  indicator_value_ = 3;
}

void ScaledVolumeRatioRegime::OnTimePeriodUpdate(const int num_pages_to_add_) { ComputeRegime(); }

void ScaledVolumeRatioRegime::OnScaledVolumeUpdate(const unsigned int index_to_send_,
                                                   const double& r_new_scaled_volume_value_) {
  if (!is_ready_) {
    if (dep_volume_ratio_calculator_->IsIndicatorReady()) {
      if (indep_volume_ratio_calculator_ && indep_volume_ratio_calculator_->IsIndicatorReady()) {
        is_ready_ = true;
      }
      if (indep_volume_ratio_calculator_port_ && indep_volume_ratio_calculator_port_->IsIndicatorReady()) {
        is_ready_ = true;
      }
    }
  }

  if (index_to_send_ == 0u) {
    dep_volume_ratio_ = r_new_scaled_volume_value_;
  } else if (index_to_send_ == 1u) {
    indep_volume_ratio_ = r_new_scaled_volume_value_;
  }

  if (is_ready_) {
    dep_source_volume_ratio_ = dep_volume_ratio_ / indep_volume_ratio_;
  }
}

void ScaledVolumeRatioRegime::ComputeRegime() {
  if (dep_source_volume_ratio_ >= switch_threshold_) {
    // dependent volume is very high compared to indep
    indicator_value_ = 1u;
  } else if (dep_source_volume_ratio_ <= inverse_switch_threshold_) {
    // indep volume is very high compared to dep
    indicator_value_ = 2u;
  } else {
    // volumes in both dep and indep are normal
    indicator_value_ = 3u;
  }
  NotifyIndicatorListeners(indicator_value_);
}

// market_interrupt_listener interface
void ScaledVolumeRatioRegime::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                      const int msecs_since_last_receive_) {
  indicator_value_ = 3;
}

void ScaledVolumeRatioRegime::OnMarketDataResumed(const unsigned int _security_id_) { indicator_value_ = 3; }
}
