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

#include "dvctrade/Indicators/trend_vol_based_regime.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"

namespace HFSAT {

void TrendVolBasedRegime::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                            std::vector<std::string>& _ors_source_needed_vec_,
                                            const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

TrendVolBasedRegime* TrendVolBasedRegime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                            const std::vector<const char*>& r_tokens_,
                                                            PriceType_t _basepx_pxtype_) {
  // INDICATOR  _this_weight_  _indicator_string_  _dep_market_view_  _indep_market_view_  _fractional_seconds_
  // _volume_measure_seconds_  _trend_weight_  _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);

  if (r_tokens_.size() >= 8) {
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                             atof(r_tokens_[5]), atoi(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
  } else {
    return NULL;
  }
}

TrendVolBasedRegime* TrendVolBasedRegime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                            const SecurityMarketView& _dep_market_view_,
                                                            const SecurityMarketView& _indep_market_view_,
                                                            double _fractional_seconds_, int t_volume_measure_seconds_,
                                                            PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep_market_view_.secname() << ' '
              << _fractional_seconds_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, TrendVolBasedRegime*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new TrendVolBasedRegime(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                                _indep_market_view_, _fractional_seconds_, t_volume_measure_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

TrendVolBasedRegime::TrendVolBasedRegime(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                         const std::string& concise_indicator_description_,
                                         const SecurityMarketView& t_dep_market_view_,
                                         const SecurityMarketView& t_indep_market_view_, double _fractional_seconds_,
                                         int t_volume_measure_seconds_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      indep_market_view_(t_indep_market_view_),
      dep_price_trend_(0),
      indep_price_trend_(0),
      dep_price_trend_brk_(0),
      indep_price_trend_brk_(0),
      dep_volume_ratio_calculator_(*(VolumeRatioCalculator::GetUniqueInstance(
          t_dbglogger_, r_watch_, t_dep_market_view_, t_volume_measure_seconds_))),
      indep_volume_ratio_calculator_(*(VolumeRatioCalculator::GetUniqueInstance(
          t_dbglogger_, r_watch_, t_indep_market_view_, t_volume_measure_seconds_))),
      current_projection_multiplier_(0.0),
      current_projected_trend_(0),
      dep_l1_norm_ratio_(1.0),
      indep_l1_norm_ratio_(1.0),
      dep_mean_l1_norm_(LoadMeanL1Norm(r_watch_.YYYYMMDD(), dep_market_view_.shortcode(), NUM_DAYS_HISTORY)),
      indep_mean_l1_norm_(LoadMeanL1Norm(r_watch_.YYYYMMDD(), indep_market_view_.shortcode(), NUM_DAYS_HISTORY)),
      dep_volume_ratio_(0.0),
      indep_volume_ratio_(0.0),
      max_ratio_dep_(1.0),
      max_ratio_indep_(1.0),
      pred_mode_(1u),
      p_dep_indicator_(NULL),
      p_indep_indicator_(NULL),
      p_dep_indicator_brk_(NULL),
      p_indep_indicator_brk_(NULL) {
  watch_.subscribe_FifteenSecondPeriod(this);  // for UpdateLRInfo and updating trend adjustment
  p_dep_indicator_ =
      SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_.shortcode(), _fractional_seconds_, _price_type_);
  if (p_dep_indicator_ == NULL) {
    indicator_value_ = 0;
    is_ready_ = true;
    return;
  }

  p_indep_indicator_ =
      SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, t_indep_market_view_.shortcode(), _fractional_seconds_, _price_type_);
  if (p_indep_indicator_ == NULL) {
    indicator_value_ = 0;
    is_ready_ = true;
    return;
  }

  p_dep_indicator_brk_ = SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_.shortcode(),
                                                        BREAKOUT_TREND_SECONDS, kPriceTypeMktSizeWPrice);
  if (p_dep_indicator_brk_ == NULL) {
    indicator_value_ = 0;
    is_ready_ = true;
    return;
  }

  p_indep_indicator_brk_ = SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, t_indep_market_view_.shortcode(),
                                                          BREAKOUT_TREND_SECONDS, kPriceTypeMktSizeWPrice);
  if (p_indep_indicator_brk_ == NULL) {
    indicator_value_ = 0;
    is_ready_ = true;
    return;
  }

  p_dep_indicator_->add_unweighted_indicator_listener(1u, this);
  p_indep_indicator_->add_unweighted_indicator_listener(2u, this);
  p_dep_indicator_brk_->add_unweighted_indicator_listener(3u, this);
  p_indep_indicator_brk_->add_unweighted_indicator_listener(4u, this);

  dep_volume_ratio_calculator_.AddVolumeRatioListener(1, this);
  indep_volume_ratio_calculator_.AddVolumeRatioListener(2, this);
}

void TrendVolBasedRegime::OnTimePeriodUpdate(const int num_pages_to_add_) {
  dep_l1_norm_ratio_ = fabs(dep_price_trend_brk_) / dep_mean_l1_norm_;
  indep_l1_norm_ratio_ = fabs(indep_price_trend_brk_) / indep_mean_l1_norm_;
  max_ratio_dep_ = std::max(dep_volume_ratio_, dep_l1_norm_ratio_);
  max_ratio_indep_ = std::max(indep_volume_ratio_, indep_l1_norm_ratio_);
  if (max_ratio_indep_ > 1.5 && max_ratio_dep_ > 1.5)
    pred_mode_ = 0u;
  else if (max_ratio_indep_ <= 1.5 && max_ratio_dep_ > 1.5)
    pred_mode_ = 1u;
  else if (max_ratio_indep_ > 1.5 && max_ratio_dep_ <= 1.5)
    pred_mode_ = 2u;
  else if (max_ratio_indep_ > max_ratio_dep_)
    pred_mode_ = 3u;
  else
    pred_mode_ = 4u;
}

void TrendVolBasedRegime::OnVolumeRatioUpdate(const unsigned int r_security_id_, const double& r_new_volume_ratio_) {
  if (r_security_id_ == 1) {
    dep_volume_ratio_ = r_new_volume_ratio_;
  } else {
    indep_volume_ratio_ = r_new_volume_ratio_;
  }
  // DBGLOG_TIME_CLASS << " scaled volume update " << r_security_id_ << " " << r_new_volume_ratio_ << DBGLOG_ENDL_FLUSH
  // ;
}

void TrendVolBasedRegime::OnIndicatorUpdate(const unsigned int& t_indicator_index_,
                                            const double& t_new_indicator_value_) {
  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else {
    if (!data_interrupted_) {
      switch (t_indicator_index_) {
        case 3u: {
          dep_price_trend_brk_ = t_new_indicator_value_;
        } break;
        case 4u: {
          indep_price_trend_brk_ = t_new_indicator_value_;
        } break;
        default: { } break; }
      indicator_value_ = pred_mode_ + 1;
    }

    NotifyIndicatorListeners(indicator_value_);
  }
}

void TrendVolBasedRegime::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

// market_interrupt_listener interface
void TrendVolBasedRegime::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                  const int msecs_since_last_receive_) {
  if (_security_id_ == indep_market_view_.security_id()) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void TrendVolBasedRegime::OnMarketDataResumed(const unsigned int _security_id_) {
  if (_security_id_ == indep_market_view_.security_id()) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

void TrendVolBasedRegime::InitializeValues() { indicator_value_ = 0; }
}
