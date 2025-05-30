/**
    \file IndicatorsCode/curve_adjusted_simple_trend.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
//#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/curve_adjusted_volume_weighted_simple_trend.hpp"

namespace HFSAT {

void CurveAdjustedVolumeWeightedSimpleTrend::CollectShortCodes(
    std::vector<std::string>& _shortcodes_affecting_this_indicator_, std::vector<std::string>& _ors_source_needed_vec_,
    const std::vector<const char*>& r_tokens_) {
  if (r_tokens_.size() > 7u)  // doesnt mean indicator string syntax is valid .
  {
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);  // dep
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);  // indep1
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[5]);  // indep2
  }
}

/**
 *
 * @param t_dbglogger_
 * @param r_watch_
 * @param r_tokens_
 * @param _base_price_type_
 * @return
 */
CurveAdjustedVolumeWeightedSimpleTrend* CurveAdjustedVolumeWeightedSimpleTrend::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _base_price_type_) {
  if (r_tokens_.size() < 10u) {
    ExitVerbose(kExitErrorCodeGeneral, "CurveAdjustedVolumeWeightedSimpleTrend needs 8 tokens");
    return NULL;
  }

  // INDICATOR 1.00 CurveAdjustedVolumeWeightedSimpleTrend DEP INDEP1 INDEP2 HALFLIFE PRICE_TYPE
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);  // dep
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);  // indep1
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[5]);  // indep2

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           // dep is not necessary to hold for now but just in case...
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[5])),
                           atof(r_tokens_[6]), atof(r_tokens_[7]), atof(r_tokens_[8]),
                           StringToPriceType_t(r_tokens_[9]));
}

/**
 *
 * @param t_dbglogger
 * @param r_watch
 * @param t_dep_market_view
 * @param t_indep1_market_view
 * @param t_indep2_market_view
 * @param t_fractional_secs
 * @param volume_ratio_duration
 * @param volume_ratio_exponent
 * @param t_price_type_
 * @return
 */
CurveAdjustedVolumeWeightedSimpleTrend* CurveAdjustedVolumeWeightedSimpleTrend::GetUniqueInstance(
    DebugLogger& t_dbglogger, const Watch& r_watch, const SecurityMarketView& t_dep_market_view,
    const SecurityMarketView& t_indep1_market_view, const SecurityMarketView& t_indep2_market_view,
    double t_fractional_secs, double volume_ratio_duration, double volume_ratio_exponent, PriceType_t t_price_type_) {
  std::ostringstream t_temp_oss_;

  t_temp_oss_ << VarName() << ' ' << t_dep_market_view.secname() << ' ' << t_indep1_market_view.secname() << ' '
              << t_indep2_market_view.secname() << ' ' << t_fractional_secs << ' ' << volume_ratio_duration << ' '
              << volume_ratio_exponent << ' ' << PriceType_t_To_String(t_price_type_);

  std::string concise_indicator_description_(t_temp_oss_.str());
  static std::map<std::string, CurveAdjustedVolumeWeightedSimpleTrend*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new CurveAdjustedVolumeWeightedSimpleTrend(
        t_dbglogger, r_watch, concise_indicator_description_, t_dep_market_view, t_indep1_market_view,
        t_indep2_market_view, t_fractional_secs, volume_ratio_duration, volume_ratio_exponent, t_price_type_);
  }

  return concise_indicator_description_map_[concise_indicator_description_];
}

/**
 *
 * @param t_dbglogger
 * @param r_watch
 * @param t_concise_indicator_description
 * @param t_dep_market_view
 * @param t_indep1_market_view
 * @param t_indep2_market_view
 * @param t_fractional_secs
 * @param volume_ratio_duration
 * @param volume_ratio_exponent
 * @param t_price_type
 */
CurveAdjustedVolumeWeightedSimpleTrend::CurveAdjustedVolumeWeightedSimpleTrend(
    DebugLogger& t_dbglogger, const Watch& r_watch, const std::string& t_concise_indicator_description,
    const SecurityMarketView& t_dep_market_view, const SecurityMarketView& t_indep1_market_view,
    const SecurityMarketView& t_indep2_market_view, double t_fractional_secs, double volume_ratio_duration,
    double volume_ratio_exponent, PriceType_t t_price_type)
    : IndicatorListener(),
      CommonIndicator(t_dbglogger, r_watch, t_concise_indicator_description),
      dep_market_view_(t_dep_market_view),
      indep1_market_view_(t_indep1_market_view),
      indep2_market_view_(t_indep2_market_view),
      dep_term_(CurveUtils::_get_term_(r_watch.YYYYMMDD(), t_dep_market_view.secname())),
      indep1_term_(CurveUtils::_get_term_(r_watch.YYYYMMDD(), t_indep1_market_view.secname())),
      indep2_term_(CurveUtils::_get_term_(r_watch.YYYYMMDD(), t_indep2_market_view.secname())) {
  std::string t_dep_secname_ = std::string(t_dep_market_view.secname());

  // contributions from each outright
  double alpha1_ = 0.0;
  double alpha2_ = 0.0;
  CurveUtils::get_alphas(t_dep_secname_, indep1_term_, indep2_term_, dep_term_, alpha1_, alpha2_);

  dep_trendI_ =
      VolumeWeightedSimpleTrend::GetUniqueInstance(t_dbglogger, r_watch, t_dep_market_view, t_fractional_secs,
                                                   volume_ratio_duration, volume_ratio_exponent, t_price_type);
  indep1_trendI_ =
      VolumeWeightedSimpleTrend::GetUniqueInstance(t_dbglogger, r_watch, t_indep1_market_view, t_fractional_secs,
                                                   volume_ratio_duration, volume_ratio_exponent, t_price_type);
  indep2_trendI_ =
      VolumeWeightedSimpleTrend::GetUniqueInstance(t_dbglogger, r_watch, t_indep2_market_view, t_fractional_secs,
                                                   volume_ratio_duration, volume_ratio_exponent, t_price_type);

  dep_trendI_->add_indicator_listener(0, this, -1.0);
  indep1_trendI_->add_indicator_listener(1, this, alpha1_);
  indep2_trendI_->add_indicator_listener(2, this, alpha2_);

  is_ready_vec_.push_back(false);
  is_ready_vec_.push_back(false);
  is_ready_vec_.push_back(false);

  prev_value_vec_.push_back(0);
  prev_value_vec_.push_back(0);
  prev_value_vec_.push_back(0);

  indep1_interrupted_ = false;
  indep2_interrupted_ = false;
  dep_interrupted_ = false;
}

void CurveAdjustedVolumeWeightedSimpleTrend::OnIndicatorUpdate(const unsigned int& indicator_index_,
                                                               const double& new_value_) {
  if (!is_ready_) {
    is_ready_vec_[indicator_index_] = true;
    is_ready_ = AreAllReady();
  } else if (!data_interrupted_) {
    indicator_value_ += (new_value_ - prev_value_vec_[indicator_index_]);
    prev_value_vec_[indicator_index_] = new_value_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

// market_interrupt_listener interface
void CurveAdjustedVolumeWeightedSimpleTrend::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                                     const int msecs_since_last_receive_) {
  if (indep1_market_view_.security_id() == _security_id_) {
    indep1_interrupted_ = true;
  } else if (indep2_market_view_.security_id() == _security_id_) {
    indep2_interrupted_ = true;
  } else if (dep_market_view_.security_id() == _security_id_) {
    dep_interrupted_ = true;
  } else {
    return;
  }

  data_interrupted_ = true;
  InitializeValues();
  NotifyIndicatorListeners(indicator_value_);
}

void CurveAdjustedVolumeWeightedSimpleTrend::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep1_market_view_.security_id() == _security_id_) {
    indep1_interrupted_ = false;
  } else if (indep2_market_view_.security_id() == _security_id_) {
    indep2_interrupted_ = false;
  } else if (dep_market_view_.security_id() == _security_id_) {
    dep_interrupted_ = false;
  } else {
    return;
  }

  if ((!indep1_interrupted_) && (!indep2_interrupted_) && (!dep_interrupted_)) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

bool CurveAdjustedVolumeWeightedSimpleTrend::AreAllReady() {
  return VectorUtils::CheckAllForValue(is_ready_vec_, true);
}

void CurveAdjustedVolumeWeightedSimpleTrend::InitializeValues() { indicator_value_ = 0; }
}
