/**
    \file IndicatorsCode/offline_low_correlation_pairs.cpp

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
#include "dvctrade/Indicators/offline_low_correlation_pairs.hpp"

namespace HFSAT {

void OfflineLowCorrelationPairs::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                   std::vector<std::string>& _ors_source_needed_vec_,
                                                   const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

OfflineLowCorrelationPairs* OfflineLowCorrelationPairs::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                          const Watch& r_watch_,
                                                                          const std::vector<const char*>& r_tokens_,
                                                                          PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ r_dep_market_view_ _indep_market_view_ _fractional_seconds_ _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           atof(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
}

OfflineLowCorrelationPairs* OfflineLowCorrelationPairs::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, SecurityMarketView& r_dep_market_view_,
    SecurityMarketView& _indep_market_view_, double _fractional_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << r_dep_market_view_.secname() << ' ' << _indep_market_view_.secname() << ' '
              << _fractional_seconds_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OfflineLowCorrelationPairs*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new OfflineLowCorrelationPairs(t_dbglogger_, r_watch_, concise_indicator_description_, r_dep_market_view_,
                                       _indep_market_view_, _fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OfflineLowCorrelationPairs::OfflineLowCorrelationPairs(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                       const std::string& concise_indicator_description_,
                                                       SecurityMarketView& r_dep_market_view_,
                                                       SecurityMarketView& _indep_market_view_,
                                                       double _fractional_seconds_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(r_dep_market_view_),
      indep_market_view_(_indep_market_view_),
      dep_price_trend_(0),
      indep_price_trend_(0),
      lrdb_(OfflineReturnsLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, r_dep_market_view_.shortcode())),
      last_lrinfo_updated_msecs_(0),
      current_lrinfo_(0.0, 0.0),
      current_projection_multiplier_(0.0),
      current_projected_trend_(0),
      slow_stdev_calculator_(
          *(SlowStdevCalculator::GetUniqueInstance(t_dbglogger_, r_watch_, _indep_market_view_.shortcode()))),
      p_dep_indicator_(NULL),
      p_indep_indicator_(NULL),
      stable_stdev_value_(1),
#define TIME_USED_STDEV_CALC 300.00
      sqrt_time_factor_(sqrt(_fractional_seconds_ / TIME_USED_STDEV_CALC)),
#undef TIME_USED_STDEV_CALC
      stdev_high_mult_factor_(1.00) {
  watch_.subscribe_BigTimePeriod(this);  // for UpdateLRInfo and updating volume adjustment

  bool lrdb_absent_ = false;

  if (dep_market_view_.security_id() == indep_market_view_.security_id()) {  // added this since for convenience one
                                                                             // could add a combo or portfolio as source
                                                                             // with a security
    // that is also the dependant
    lrdb_absent_ = true;
  }

  if (!(lrdb_.LRCoeffPresent(dep_market_view_.shortcode(), indep_market_view_.shortcode()))) {
    lrdb_absent_ = true;
  }

  if (lrdb_absent_) {
    indicator_value_ = 0;
    is_ready_ = true;
  } else {
    slow_stdev_calculator_.AddSlowStdevCalculatorListener(this);

    // note since we are not storing these pointers, memory leak unless we call a static method to free it somehow
    p_dep_indicator_ = SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, r_dep_market_view_.shortcode(),
                                                      _fractional_seconds_, _price_type_);
    if (p_dep_indicator_ == NULL) {
      indicator_value_ = 0;
      is_ready_ = true;
      return;
    }

    p_indep_indicator_ = SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, _indep_market_view_.shortcode(),
                                                        _fractional_seconds_, _price_type_);
    if (p_indep_indicator_ == NULL) {
      indicator_value_ = 0;
      is_ready_ = true;
      return;
    }

    p_dep_indicator_->add_unweighted_indicator_listener(1u, this);
    p_indep_indicator_->add_unweighted_indicator_listener(2u, this);

// this paramter HIGH_DEVIATION_CONSTANT should be made an input argument
// or basically tested on past data
#define HIGH_DEVIATION_CONSTANT 2.00
    // assuming that the stdev of price change for time _fractional_seconds_ = stdev of price change for 300 seconds *
    // sqrt ( _fractional_seconds_ / 300 )
    // this might not make sense for small durations
    stdev_high_mult_factor_ = HIGH_DEVIATION_CONSTANT * sqrt_time_factor_;
#undef HIGH_DEVIATION_CONSTANT
  }
}

void OfflineLowCorrelationPairs::OnStdevUpdate(const unsigned int _security_id_, const double& cr_new_stdev_value_) {
  stable_stdev_value_ = std::max(0.0000001, cr_new_stdev_value_ * stdev_high_mult_factor_);
}

void OfflineLowCorrelationPairs::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    if (_indicator_index_ ==
        1u) {  // cheat sheet way .. this only works if the id of 1 is passed when _dep_indicator_ is initialized above.
      dep_price_trend_ = _new_value_;
    } else {
      indep_price_trend_ = _new_value_;
      current_projected_trend_ = indep_price_trend_ * current_projection_multiplier_;
    }

    double activity_mult_factor_ = std::min(2.00, std::max(0.25, fabs(indep_price_trend_) / stable_stdev_value_));

    // if the magnitude of the change we are projecting is more than
    // a stable high threshold, which is HIGH_DEVIATION_CONSTANT * a long-period-stdev
    indicator_value_ = activity_mult_factor_ * (current_projected_trend_ - dep_price_trend_);

    NotifyIndicatorListeners(indicator_value_);
  }
}

void OfflineLowCorrelationPairs::WhyNotReady() {
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
void OfflineLowCorrelationPairs::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                         const int msecs_since_last_receive_) {
  if (_security_id_ == indep_market_view_.security_id()) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void OfflineLowCorrelationPairs::OnMarketDataResumed(const unsigned int _security_id_) {
  if (_security_id_ == indep_market_view_.security_id()) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

void OfflineLowCorrelationPairs::InitializeValues() {
  indicator_value_ = 0;
  UpdateLRInfo();
}

void OfflineLowCorrelationPairs::UpdateLRInfo() {
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
