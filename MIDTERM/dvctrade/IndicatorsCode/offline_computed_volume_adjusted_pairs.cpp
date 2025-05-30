/**
    \file IndicatorsCode/offline_computed_volume_adjusted_pairs.cpp

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

#include "dvctrade/Indicators/offline_computed_volume_adjusted_pairs.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"

namespace HFSAT {

void OfflineComputedVolumeAdjustedPairs::CollectShortCodes(
    std::vector<std::string>& _shortcodes_affecting_this_indicator_, std::vector<std::string>& _ors_source_needed_vec_,
    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

OfflineComputedVolumeAdjustedPairs* OfflineComputedVolumeAdjustedPairs::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ t_dep_market_view_ t_indep_market_view_ t_fractional_seconds_
  // t_price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           atof(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
}

OfflineComputedVolumeAdjustedPairs* OfflineComputedVolumeAdjustedPairs::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, SecurityMarketView& t_dep_market_view_,
    SecurityMarketView& t_indep_market_view_, double t_fractional_seconds_, PriceType_t t_price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_dep_market_view_.secname() << ' ' << t_indep_market_view_.secname() << ' '
              << t_fractional_seconds_ << ' ' << t_price_type_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OfflineComputedVolumeAdjustedPairs*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new OfflineComputedVolumeAdjustedPairs(
        t_dbglogger_, r_watch_, concise_indicator_description_, t_dep_market_view_, t_indep_market_view_,
        t_fractional_seconds_, t_price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OfflineComputedVolumeAdjustedPairs::OfflineComputedVolumeAdjustedPairs(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_,
    SecurityMarketView& t_dep_market_view_, SecurityMarketView& t_indep_market_view_, double t_fractional_seconds_,
    PriceType_t t_price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      indep_market_view_(t_indep_market_view_),
      dep_price_trend_(0),
      indep_price_trend_(0),
      lrdb_(OfflineReturnsLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_.shortcode())),
      last_lrinfo_updated_msecs_(0),
      current_lrinfo_(0.0, 0.0),
      current_projection_multiplier_(0.0),
      current_projected_trend_(0),
      recent_volume_ratio_(1.0),
      p_dep_indicator_(NULL),
      p_indep_indicator_(NULL) {
  watch_.subscribe_BigTimePeriod(this);  // for UpdateLRInfo and updating volume adjustment

  bool lrdb_absent_ = false;

  if (!(lrdb_.LRCoeffPresent(t_dep_market_view_.shortcode(), t_indep_market_view_.shortcode()))) {
    lrdb_absent_ = true;
  }

  if (dep_market_view_.security_id() == indep_market_view_.security_id()) {  // added this since for convenience one
                                                                             // could add a combo or portfolio as source
                                                                             // with a security
    // that is also the dependant
    lrdb_absent_ = true;
  }

  if (lrdb_absent_) {
    indicator_value_ = 0;
    is_ready_ = true;
  } else {
    RecentVolumeRatioCalculator* p_rvrc_ = RecentVolumeRatioCalculator::GetUniqueInstance(
        t_dbglogger_, r_watch_, t_dep_market_view_, t_indep_market_view_, 3600);

    if (p_rvrc_ != NULL) {
      p_rvrc_->AddRecentVolumeRatioListener(this);
    }

    // note the order of calling add_unweighted_indicator_listener is assumed in OnIndicatorUpdate
    p_dep_indicator_ = SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_.shortcode(), t_fractional_seconds_,
                                                      t_price_type_);
    p_dep_indicator_->add_unweighted_indicator_listener(1, this);

    p_indep_indicator_ = SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, t_indep_market_view_.shortcode(),
                                                        t_fractional_seconds_, t_price_type_);
    p_indep_indicator_->add_unweighted_indicator_listener(2, this);
  }
}

void OfflineComputedVolumeAdjustedPairs::OnIndicatorUpdate(const unsigned int& t_indicator_index_,
                                                           const double& t_new_indicator_value_) {
  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else {
    if (t_indicator_index_ == 1) {
      dep_price_trend_ = t_new_indicator_value_;
    } else if (t_indicator_index_ == 2) {
      indep_price_trend_ = t_new_indicator_value_;
      current_projected_trend_ = indep_price_trend_ * current_projection_multiplier_;
    }

    indicator_value_ = recent_volume_ratio_ * (current_projected_trend_ - dep_price_trend_);

    if (p_indep_indicator_->IsDataInterrupted()) indicator_value_ = 0;

    NotifyIndicatorListeners(indicator_value_);
  }
}

void OfflineComputedVolumeAdjustedPairs::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                                 const int msecs_since_last_receive_) {
  if (_security_id_ == indep_market_view_.security_id()) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void OfflineComputedVolumeAdjustedPairs::OnMarketDataResumed(const unsigned int _security_id_) {
  if (_security_id_ == indep_market_view_.security_id()) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

void OfflineComputedVolumeAdjustedPairs::InitializeValues() {
  indicator_value_ = 0;
  UpdateLRInfo();
}

void OfflineComputedVolumeAdjustedPairs::UpdateLRInfo() {
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
