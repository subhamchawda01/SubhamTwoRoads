/**
    \file IndicatorsCode/regime_slow_stdev_trend.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/regime_slow_stdev_trend.hpp"

namespace HFSAT {

void RegimeSlowStdevTrend::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                             std::vector<std::string>& _ors_source_needed_vec_,
                                             const std::vector<const char*>& r_tokens_) {
  std::string t_source_shortcode_ = (std::string)r_tokens_[3];
  if (IndicatorUtil::IsPortfolioShortcode(t_source_shortcode_)) {
    IndicatorUtil::AddPortfolioShortCodeVec(t_source_shortcode_, _shortcodes_affecting_this_indicator_);
  } else {
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, t_source_shortcode_);
  }
}

RegimeSlowStdevTrend* RegimeSlowStdevTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                              const std::vector<const char*>& r_tokens_,
                                                              PriceType_t _basepx_pxtype_) {
  if (r_tokens_.size() < 8) {
    std::cerr << "Insufficient arguments to INDICATOR RegimeSlowStdevTrend, correct syntax : _this_weight_ "
                 "_indicator_string_ _source_shortcode_ t_stdev_history_secs_ _trend_secs_ _threshold_ _price_type_\n";
    exit(1);
  }
  return GetUniqueInstance(t_dbglogger_, r_watch_, (std::string)r_tokens_[3], atof(r_tokens_[4]), atof(r_tokens_[5]),
                           atof(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
}

RegimeSlowStdevTrend* RegimeSlowStdevTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                              std::string r_source_shortcode_, double _stdev_duration_,
                                                              double _trend_duration_, double switch_threshold,
                                                              PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << r_source_shortcode_ << ' ' << _stdev_duration_ << ' ' << _trend_duration_ << ' '
              << switch_threshold << ' ' << _price_type_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, RegimeSlowStdevTrend*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new RegimeSlowStdevTrend(t_dbglogger_, r_watch_, concise_indicator_description_, r_source_shortcode_,
                                 _stdev_duration_, _trend_duration_, switch_threshold, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

RegimeSlowStdevTrend::RegimeSlowStdevTrend(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                           const std::string& concise_indicator_description_,
                                           std::string _source_shortcode_, double _stdev_duration,
                                           double _trend_duration_, double switch_threshold, PriceType_t _price_type_)
    : IndicatorListener(),
      CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      switch_threshold_(switch_threshold) {
  p_slow_stdev_trend_calculator = SlowStdevTrendCalculator::GetUniqueInstance(
      t_dbglogger_, r_watch_, _source_shortcode_, _stdev_duration, _trend_duration_, _price_type_);
  p_slow_stdev_trend_calculator->add_unweighted_indicator_listener(0, this);
  double min_price_increment_ = 1;
  if (IndicatorUtil::IsPortfolioShortcode(_source_shortcode_)) {
    min_price_increment_ =
        PCAPortPrice::GetUniqueInstance(t_dbglogger_, r_watch_, _source_shortcode_, kPriceTypeMktSizeWPrice)
            ->min_price_increment();
  } else {
    ShortcodeSecurityMarketViewMap::StaticCheckValid(_source_shortcode_);
    SecurityMarketView& _indep_market_view_ =
        *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(_source_shortcode_));
    min_price_increment_ = _indep_market_view_.min_price_increment();
  }
  switch_threshold_ *= min_price_increment_;
}

void RegimeSlowStdevTrend::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (!is_ready_) {
    is_ready_ = true;
    indicator_value_ = 1;
    // NotifyIndicatorListeners ( indicator_value_ ) ;
  } else {
    if (std::abs(_new_value_) > switch_threshold_) {
      indicator_value_ = 2;
    } else {
      indicator_value_ = 1;
    }
    NotifyIndicatorListeners(indicator_value_);
  }
}
}
