/**
   \file IndicatorsCode/offline_breakout_adjusted_pairs.cpp

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

#include "dvctrade/Indicators/rolling_l1sz_regime.hpp"

namespace HFSAT {

void RollingL1SzRegime::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                          std::vector<std::string>& _ors_source_needed_vec_,
                                          const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

RollingL1SzRegime* RollingL1SzRegime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                        const std::vector<const char*>& r_tokens_,
                                                        PriceType_t _basepx_pxtype_) {
  // INDICATOR  _this_weight_  _indicator_string_  _dep_market_view_   _fractional_seconds_  _threshold_  _tolerance_
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), atof(r_tokens_[5]), atof(r_tokens_[6]), _basepx_pxtype_);
}

RollingL1SzRegime* RollingL1SzRegime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                        const SecurityMarketView& _dep_market_view_,
                                                        double _time_length_, double _thresh_, double _tolerance_,
                                                        PriceType_t _basepx_pxtype_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _time_length_ << ' ' << _thresh_ << ' '
              << _tolerance_ << '\n';

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, RollingL1SzRegime*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new RollingL1SzRegime(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _time_length_,
                              _thresh_, _tolerance_, _basepx_pxtype_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

RollingL1SzRegime::RollingL1SzRegime(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                     const std::string& concise_indicator_description_,
                                     const SecurityMarketView& t_dep_market_view_, double t_time_length_,
                                     double t_thresh_, double t_tolerance_, PriceType_t t_basepx_pxtype_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_), dep_market_view_(t_dep_market_view_) {
  l1_trend_var_ =
      L1SizeTrend::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_, t_time_length_, t_basepx_pxtype_);
  l1_trend_var_->add_unweighted_indicator_listener(1u, this);
  /// px type subscription done in L1SizeTrend
  tolerance_ = t_tolerance_;
  threshold_ = t_thresh_;
  InitializeValues();
}

void RollingL1SzRegime::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (indicator_value_ == 1 && _new_value_ <= (1 - tolerance_) * threshold_) {
    indicator_value_ = 2;
    NotifyIndicatorListeners(indicator_value_);
  } else if (indicator_value_ == 2 && _new_value_ >= (1 + tolerance_) * threshold_) {
    indicator_value_ = 1;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void RollingL1SzRegime::InitializeValues() {
  /// 1 is high l1sz and 2 is low l1sz
  indicator_value_ = 1;
  NotifyIndicatorListeners(indicator_value_);
  is_ready_ = true;
}
void RollingL1SzRegime::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  data_interrupted_ = true;
}

void RollingL1SzRegime::OnMarketDataResumed(const unsigned int _security_id_) { data_interrupted_ = false; }
}
