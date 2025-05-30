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

#include "dvctrade/Indicators/offline_breakout_adjusted_pairs.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"

namespace HFSAT {

void OfflineBreakoutAdjustedPairs::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                     std::vector<std::string>& _ors_source_needed_vec_,
                                                     const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

OfflineBreakoutAdjustedPairs* OfflineBreakoutAdjustedPairs::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                              const Watch& r_watch_,
                                                                              const std::vector<const char*>& r_tokens_,
                                                                              PriceType_t _basepx_pxtype_) {
  // INDICATOR  _this_weight_  _indicator_string_  _dep_market_view_  _indep_market_view_  _fractional_seconds_
  // _volume_measure_seconds_  _trend_weight_  _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);

  if (r_tokens_.size() >= 9) {
    return GetUniqueInstance(
        t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
        *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])), atof(r_tokens_[5]),
        atoi(r_tokens_[6]), atof(r_tokens_[7]), StringToPriceType_t(r_tokens_[8]));
  } else {
    return nullptr;
  }
}

OfflineBreakoutAdjustedPairs* OfflineBreakoutAdjustedPairs::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const SecurityMarketView& _dep_market_view_,
    const SecurityMarketView& _indep_market_view_, double _fractional_seconds_, int t_volume_measure_seconds_,
    double t_trend_weight_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep_market_view_.secname() << ' '
              << _fractional_seconds_ << ' ' << t_volume_measure_seconds_ << ' ' << t_trend_weight_ << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OfflineBreakoutAdjustedPairs*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new OfflineBreakoutAdjustedPairs(
        t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _indep_market_view_,
        _fractional_seconds_, t_volume_measure_seconds_, t_trend_weight_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OfflineBreakoutAdjustedPairs::OfflineBreakoutAdjustedPairs(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                           const std::string& concise_indicator_description_,
                                                           const SecurityMarketView& t_dep_market_view_,
                                                           const SecurityMarketView& t_indep_market_view_,
                                                           double _fractional_seconds_, int t_volume_measure_seconds_,
                                                           double t_trend_weight_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      indep_market_view_(t_indep_market_view_),
      dep_price_trend_(0),
      indep_price_trend_(0),
      trend_weight_(t_trend_weight_),
      dep_scaled_volume_calculator_(*(RecentScaledVolumeCalculator::GetUniqueInstance(
          t_dbglogger_, r_watch_, t_dep_market_view_, t_volume_measure_seconds_))),
      indep_scaled_volume_calculator_(*(RecentScaledVolumeCalculator::GetUniqueInstance(
          t_dbglogger_, r_watch_, t_indep_market_view_, t_volume_measure_seconds_))),
      lrdb_(OfflineReturnsLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_.shortcode())),
      last_lrinfo_updated_msecs_(0),
      current_lrinfo_(0.0, 0.0),
      current_projection_multiplier_(0.0),
      current_projected_trend_(0),
      scaled_dep_volume_(0.0),
      scaled_indep_volume_(0.0),
      pred_mode_(1u),
      p_dep_indicator_(nullptr),
      p_indep_indicator_(nullptr) {
  watch_.subscribe_BigTimePeriod(this);  // for UpdateLRInfo and updating volume adjustment

  bool lrdb_absent_ = false;

  if (t_dep_market_view_.security_id() == t_indep_market_view_.security_id()) {  // added this since for convenience one
                                                                                 // could add a combo or portfolio as
                                                                                 // source with a security
    // that is also the dependant
    lrdb_absent_ = true;
  }

  if (!(lrdb_.LRCoeffPresent(t_dep_market_view_.shortcode(), t_indep_market_view_.shortcode()))) {
    lrdb_absent_ = true;
  }

  if (lrdb_absent_) {
    indicator_value_ = 0;
    is_ready_ = true;
  } else {
    p_dep_indicator_ =
        SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_.shortcode(), _fractional_seconds_, _price_type_);
    if (p_dep_indicator_ == nullptr) {
      indicator_value_ = 0;
      is_ready_ = true;
      return;
    }

    p_indep_indicator_ = SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, t_indep_market_view_.shortcode(),
                                                        _fractional_seconds_, _price_type_);
    if (p_indep_indicator_ == nullptr) {
      indicator_value_ = 0;
      is_ready_ = true;
      return;
    }

    p_dep_indicator_->add_unweighted_indicator_listener(1u, this);
    p_indep_indicator_->add_unweighted_indicator_listener(2u, this);

    dep_scaled_volume_calculator_.AddRecentScaledVolumeListener(1, this);
    indep_scaled_volume_calculator_.AddRecentScaledVolumeListener(2, this);
  }
}

void OfflineBreakoutAdjustedPairs::OnScaledVolumeUpdate(const unsigned int r_security_id_,
                                                        const double& r_new_scaled_volume_) {
  if (r_security_id_ == 1) {
    scaled_dep_volume_ = r_new_scaled_volume_;
  } else {
    scaled_indep_volume_ = r_new_scaled_volume_;
  }
  // DBGLOG_TIME_CLASS << " scaled volume update " << r_security_id_ << " " << r_new_scaled_volume_ << DBGLOG_ENDL_FLUSH
  // ;
  if (scaled_indep_volume_ < 0.8) {
    if (scaled_dep_volume_ > 1) {
      pred_mode_ = 0u;  // full dt
    } else {
      pred_mode_ = 3u;  // just 0
    }
  } else {
    if (scaled_indep_volume_ > scaled_dep_volume_) {
      pred_mode_ = 1u;  // full offline
    } else {
      if (scaled_indep_volume_ > 0.8 * scaled_dep_volume_) {
        pred_mode_ = 2u;  // half offline
      } else {
        pred_mode_ = 0u;  // full dt
      }
    }
  }
}

void OfflineBreakoutAdjustedPairs::OnIndicatorUpdate(const unsigned int& t_indicator_index_,
                                                     const double& t_new_indicator_value_) {
  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else {
    if (!data_interrupted_) {
      switch (t_indicator_index_) {
        case 1u: {
          dep_price_trend_ = t_new_indicator_value_;
        } break;
        case 2u: {
          indep_price_trend_ = t_new_indicator_value_;
          current_projected_trend_ = indep_price_trend_ * current_projection_multiplier_;
        } break;
      }

      switch (pred_mode_) {
        case 1u: {
          indicator_value_ = current_projected_trend_ - dep_price_trend_;
        } break;
        case 2u: {
          indicator_value_ =
              0.5 * (current_projected_trend_ - dep_price_trend_) + 0.5 * trend_weight_ * dep_price_trend_;
        } break;
        case 0u: {
          indicator_value_ = trend_weight_ * dep_price_trend_;
        } break;
        case 3u: {
          indicator_value_ = 0;
        } break;
      }

      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void OfflineBreakoutAdjustedPairs::WhyNotReady() {
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
void OfflineBreakoutAdjustedPairs::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                           const int msecs_since_last_receive_) {
  if (_security_id_ == indep_market_view_.security_id()) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void OfflineBreakoutAdjustedPairs::OnMarketDataResumed(const unsigned int _security_id_) {
  if (_security_id_ == indep_market_view_.security_id()) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

void OfflineBreakoutAdjustedPairs::InitializeValues() {
  indicator_value_ = 0;
  UpdateLRInfo();
}

void OfflineBreakoutAdjustedPairs::UpdateLRInfo() {
  if ((last_lrinfo_updated_msecs_ == 0) ||
      (watch_.msecs_from_midnight() - last_lrinfo_updated_msecs_ > TENMINUTESMSECS)) {
    current_lrinfo_ = lrdb_.GetLRCoeff(dep_market_view_.shortcode(), indep_market_view_.shortcode());
    ComputeMultiplier();

    if (dbglogger_.CheckLoggingLevel(LRDB_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "lrinfo ( " << dep_market_view_.shortcode() << ", " << indep_market_view_.shortcode()
                             << " ) " << current_lrinfo_.lr_coeff_ << ' ' << current_lrinfo_.lr_correlation_ << " -> "
                             << current_projection_multiplier_ << DBGLOG_ENDL_FLUSH;
    }

    last_lrinfo_updated_msecs_ = watch_.msecs_from_midnight() - (watch_.msecs_from_midnight() % TENMINUTESMSECS);
  }
}
}
