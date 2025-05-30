/**
    \file IndicatorsCode/volatility_regime.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/volatility_regime.hpp"

namespace HFSAT {

void VolatilityRegime::CollectShortCodes(std::vector<std::string>& t_shortcodes_affecting_this_indicator_,
                                         std::vector<std::string>& _ors_source_needed_vec_,
                                         const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(t_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(t_shortcodes_affecting_this_indicator_, (std::string) "ES_0");
  VectorUtils::UniqueVectorAdd(t_shortcodes_affecting_this_indicator_, (std::string) "6M_0");
  // HFSAT::GetCoreShortcodes((std::string) r_tokens_[3], t_shortcodes_affecting_this_indicator_);
}

VolatilityRegime* VolatilityRegime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      const std::vector<const char*>& r_tokens_,
                                                      PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ t_indep_market_view_ _fractional_seconds_ _price_type_
  // _switch_threshold_
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), StringToPriceType_t(r_tokens_[5]), atof(r_tokens_[6]));
}

VolatilityRegime* VolatilityRegime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      SecurityMarketView& t_indep_market_view_,
                                                      double _fractional_seconds_, PriceType_t _price_type_,
                                                      double _switch_threshold_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_indep_market_view_.secname() << ' ' << _fractional_seconds_ << ' '
              << PriceType_t_To_String(_price_type_) << ' ' << _switch_threshold_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, VolatilityRegime*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new VolatilityRegime(t_dbglogger_, r_watch_, concise_indicator_description_, t_indep_market_view_,
                             _fractional_seconds_, _price_type_, _switch_threshold_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

VolatilityRegime::VolatilityRegime(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                   const std::string& concise_indicator_description_,
                                   SecurityMarketView& t_indep_market_view_, double _fractional_seconds_,
                                   PriceType_t _price_type_, double switch_threshold)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(t_indep_market_view_),
      trend_history_msecs_(std::max(MIN_MSEC_HISTORY_INDICATORS, (int)round(1000 * _fractional_seconds_))),
      price_type_(_price_type_),
      _switch_threshold_(switch_threshold) {
  // HFSAT::GetCoreShortcodes(indep_market_view_.shortcode(), core_shortcode_vec_);
  // core_shortcode_vec_.pop_back();
  core_shortcode_vec_.push_back("ES_0");
  core_shortcode_vec_.push_back("6M_0");
  length = 0;
  for (std::vector<std::string>::iterator it = core_shortcode_vec_.begin(); it != core_shortcode_vec_.end(); ++it) {
    smv.push_back(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(*it));
    trend.push_back(0.0);
    l1_norm.push_back(LoadMeanL1Norm(r_watch_.YYYYMMDD(), *it, NUM_DAYS_HISTORY));
    trend_instance.push_back(
        SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, smv[length]->shortcode(), _fractional_seconds_, _price_type_));
    trend_instance[length]->add_unweighted_indicator_listener(length, this);
    length++;
  }
}

void VolatilityRegime::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (_indicator_index_ < length) trend[_indicator_index_] = _new_value_;
  indicator_value_ = 2.0;
  if (fabs(_new_value_) < _switch_threshold_) {
    double max = 0.0;
    for (auto i = 0u; i < length; i++) {
      if (fabs(trend[i]) > max) {
        max = fabs(trend[i]);
      }
    }
    if (max < _switch_threshold_) indicator_value_ = 1;
  }
  NotifyIndicatorListeners(indicator_value_);
}

// market_interrupt_listener interface
void VolatilityRegime::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 1;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void VolatilityRegime::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  }
}
}
