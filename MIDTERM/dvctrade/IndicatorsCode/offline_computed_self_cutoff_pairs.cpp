/**
    \file IndicatorsCode/offline_computed_self_cutoff_pairs.cpp

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

#include "dvctrade/Indicators/offline_computed_self_cutoff_pairs.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"

namespace HFSAT {

void OfflineComputedSelfCutoffPairs::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                       std::vector<std::string>& _ors_source_needed_vec_,
                                                       const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

OfflineComputedSelfCutoffPairs* OfflineComputedSelfCutoffPairs::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ t_dep_market_view_ t_indep_market_view_ t_fractional_seconds_
  // t_price_type_

  if (r_tokens_.size() < 7) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight OfflineComputedSelfCutoffPairs _dep_market_view_ _indep_market_view_ "
                "_fractional_seconds_ _trend_fractional_seconds_ (optional) _price_type_ ");
  }

  if (r_tokens_.size() == 7) {
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                             atof(r_tokens_[5]), -1.0, StringToPriceType_t(r_tokens_[6]));
  }
  if (std::string(r_tokens_[7]).compare("#") == 0) {
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                             atof(r_tokens_[5]), -1.0, StringToPriceType_t(r_tokens_[6]));
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           atof(r_tokens_[5]), atof(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
}

OfflineComputedSelfCutoffPairs* OfflineComputedSelfCutoffPairs::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, SecurityMarketView& t_dep_market_view_,
    SecurityMarketView& t_indep_market_view_, double t_fractional_seconds_, double t_trend_fractional_seconds_,
    PriceType_t t_price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_dep_market_view_.secname() << ' ' << t_indep_market_view_.secname() << ' '
              << t_fractional_seconds_ << ' ' << t_trend_fractional_seconds_ << ' ' << t_price_type_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OfflineComputedSelfCutoffPairs*> concise_indicator_description_map_;

  double _trend_fractional_seconds_ =
      t_trend_fractional_seconds_ > 0 ? t_trend_fractional_seconds_ : t_fractional_seconds_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new OfflineComputedSelfCutoffPairs(
        t_dbglogger_, r_watch_, concise_indicator_description_, t_dep_market_view_, t_indep_market_view_,
        t_fractional_seconds_, _trend_fractional_seconds_, t_price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OfflineComputedSelfCutoffPairs::OfflineComputedSelfCutoffPairs(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_,
    SecurityMarketView& t_dep_market_view_, SecurityMarketView& t_indep_market_view_, double t_fractional_seconds_,
    double t_trend_fractional_seconds_, PriceType_t t_price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      indep_market_view_(t_indep_market_view_),
      dep_price_trend_(0),
      indep_price_trend_(0),
      dep_cutoff_price_trend_(0),
      lrdb_(OfflineReturnsLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_.shortcode())),
      last_lrinfo_updated_msecs_(0),
      current_lrinfo_(0.0, 0.0),
      current_projection_multiplier_(0.0),
      current_projected_trend_(0),
      trend_duration_ratio_(sqrt(t_fractional_seconds_ / t_trend_fractional_seconds_)),
      p_dep_indicator_(NULL),
      p_indep_indicator_(NULL),
      p_dep_trend_indicator_(NULL) {
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
    //  p_dep_indicator_ = NULL ;
    //  p_indep_indicator_ = NULL ; constructor intializes them to NULL

  } else {
    p_dep_indicator_ = SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_.shortcode(), t_fractional_seconds_,
                                                      t_price_type_);
    if (p_dep_indicator_ == NULL) {
      indicator_value_ = 0;
      is_ready_ = true;
      std::cerr << __FUNCTION__ << " Failed for Dep MktView " << t_dep_market_view_.shortcode() << " "
                << t_fractional_seconds_ << " " << PriceType_t_To_String(t_price_type_) << std::endl;
      return;
    }

    p_indep_indicator_ = SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, t_indep_market_view_.shortcode(),
                                                        t_fractional_seconds_, t_price_type_);
    if (p_indep_indicator_ == NULL) {
      indicator_value_ = 0;
      is_ready_ = true;
      std::cerr << __FUNCTION__ << " Failed for InDep MktView " << t_indep_market_view_.shortcode() << " "
                << t_fractional_seconds_ << " " << PriceType_t_To_String(t_price_type_) << std::endl;
      return;
    }

    p_dep_trend_indicator_ = SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_.shortcode(),
                                                            t_trend_fractional_seconds_, t_price_type_);
    if (p_dep_trend_indicator_ == NULL) {
      indicator_value_ = 0;
      is_ready_ = true;
      std::cerr << __FUNCTION__ << " Failed for InDep MktView " << t_dep_market_view_.shortcode() << " "
                << t_trend_fractional_seconds_ << " " << PriceType_t_To_String(t_price_type_) << std::endl;
      return;
    }

    p_dep_indicator_->add_unweighted_indicator_listener(1u, this);
    p_indep_indicator_->add_unweighted_indicator_listener(2u, this);
    p_dep_trend_indicator_->add_unweighted_indicator_listener(3u, this);
  }
}

void OfflineComputedSelfCutoffPairs::OnIndicatorUpdate(const unsigned int& t_indicator_index_,
                                                       const double& t_new_indicator_value_) {
  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    switch (t_indicator_index_) {
      case 1u: {
        dep_price_trend_ = t_new_indicator_value_;
      } break;
      case 2u: {
        indep_price_trend_ = t_new_indicator_value_;
        current_projected_trend_ = indep_price_trend_ * current_projection_multiplier_;
      } break;
      case 3u: {
        dep_cutoff_price_trend_ = t_new_indicator_value_ * trend_duration_ratio_;
      } break;
    }

    if (current_projected_trend_ < 0.01 * dep_market_view_.min_price_increment()) {
      // To avoid division by zero
      indicator_value_ = dep_cutoff_price_trend_;
    } else {
      // trend factor measures how much of the source has already been captured in dep price action. Constrained to
      // (0,1)
      double dep_indep_trend_factor_ = std::min(1.0, std::max(0.0, dep_price_trend_ / current_projected_trend_));
      // this is sort of counter-intuitive to the notion of relative indicators, but might help overall.
      indicator_value_ = (1 - dep_indep_trend_factor_) * (current_projected_trend_ - dep_price_trend_) +
                         dep_indep_trend_factor_ * dep_cutoff_price_trend_;
    }

    NotifyIndicatorListeners(indicator_value_);
  }
}

void OfflineComputedSelfCutoffPairs::WhyNotReady() {
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
void OfflineComputedSelfCutoffPairs::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                             const int msecs_since_last_receive_) {
  if (_security_id_ == indep_market_view_.security_id()) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void OfflineComputedSelfCutoffPairs::OnMarketDataResumed(const unsigned int _security_id_) {
  if (_security_id_ == indep_market_view_.security_id()) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

void OfflineComputedSelfCutoffPairs::InitializeValues() {
  indicator_value_ = 0;
  UpdateLRInfo();
}

void OfflineComputedSelfCutoffPairs::UpdateLRInfo() {
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
