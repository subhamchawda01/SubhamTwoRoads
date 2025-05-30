/**
   \file IndicatorsCode/offline_low_correlation_pairs_port.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/offline_low_correlation_pairs_port.hpp"

namespace HFSAT {

void OfflineLowCorrelationPairsPort::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                       std::vector<std::string>& _ors_source_needed_vec_,
                                                       const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[4], _shortcodes_affecting_this_indicator_);
}

OfflineLowCorrelationPairsPort* OfflineLowCorrelationPairsPort::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _basepx_pxtype_) {
  // INDICATOR  _this_weight_  _indicator_string_  _dep_market_view_  _portfolio_descriptor_shortcode_
  // _fractional_seconds_  _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           (std::string)(r_tokens_[4]), atof(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
}

OfflineLowCorrelationPairsPort* OfflineLowCorrelationPairsPort::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, SecurityMarketView& _dep_market_view_,
    std::string _portfolio_descriptor_shortcode_, double _fractional_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _portfolio_descriptor_shortcode_ << ' '
              << _fractional_seconds_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OfflineLowCorrelationPairsPort*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new OfflineLowCorrelationPairsPort(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                                           _portfolio_descriptor_shortcode_, _fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OfflineLowCorrelationPairsPort::OfflineLowCorrelationPairsPort(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                               const std::string& concise_indicator_description_,
                                                               SecurityMarketView& _dep_market_view_,
                                                               std::string _portfolio_descriptor_shortcode_,
                                                               double _fractional_seconds_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_portfolio_price_(
          *(PCAPortPrice::GetUniqueInstance(t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, _price_type_))),
      dep_price_trend_(0.0),
      indep_price_trend_(0.0),
      p_dep_indicator_(NULL),
      p_indep_indicator_(NULL),
      lrdb_(OfflineReturnsLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, _dep_market_view_.shortcode())),
      last_lrinfo_updated_msecs_(0),
      current_lrinfo_(0.0, 0.0),
      current_projection_multiplier_(0.0),
      current_projected_trend_(0),
      sd_adj_current_projected_trend_(0),
      slow_dep_stdev_calculator_(
          *(SlowStdevCalculator::GetUniqueInstance(t_dbglogger_, r_watch_, _dep_market_view_.shortcode()))),
      stable_dep_stdev_value_(1),
#define TIME_USED_STDEV_CALC 300.00
      sqrt_time_factor_(sqrt(_fractional_seconds_ / TIME_USED_STDEV_CALC)),
#undef TIME_USED_STDEV_CALC
      stdev_high_mult_factor_(1.00) {
  watch_.subscribe_BigTimePeriod(this);  // for UpdateLRInfo and updating volume adjustment

  slow_dep_stdev_calculator_.AddSlowStdevCalculatorListener(this);

  bool lrdb_absent_ = false;

  if (!(lrdb_.LRCoeffPresent(dep_market_view_.shortcode(), indep_portfolio_price_.shortcode()))) {
    lrdb_absent_ = true;
  }

  if (lrdb_absent_) {
    indicator_value_ = 0;
    is_ready_ = true;
  } else {
    // note since we are not storing these pointers, memory leak unless we call a static method to free it somehow
    p_dep_indicator_ = SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, _dep_market_view_.shortcode(),
                                                      _fractional_seconds_, _price_type_);
    p_dep_indicator_->add_unweighted_indicator_listener((unsigned int)1, this);

    p_indep_indicator_ = SimpleTrendPort::GetUniqueInstance(t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_,
                                                            _fractional_seconds_, _price_type_);
    p_indep_indicator_->add_unweighted_indicator_listener((unsigned int)2, this);

// thie paramter HIGH_DEVIATION_CONSTANT should be made an input argument
// or basically tested on past data
#define HIGH_DEVIATION_CONSTANT 1.20

    // assuming that the stdev of price change for time _fractional_seconds_ = stdev of price change for 300 seconds *
    // sqrt ( _fractional_seconds_ / 300 )

    stdev_high_mult_factor_ = HIGH_DEVIATION_CONSTANT * sqrt_time_factor_;
#undef HIGH_DEVIATION_CONSTANT

#if EQUITY_INDICATORS_ALWAYS_READY
    if (p_dep_indicator_->IsIndicatorReady() && p_indep_indicator_->IsIndicatorReady()) {
      is_ready_ = true;
      InitializeValues();
    }
#endif
  }
}

void OfflineLowCorrelationPairsPort::OnStdevUpdate(const unsigned int _security_id_,
                                                   const double& cr_new_stdev_value_) {
  stable_dep_stdev_value_ = std::max(0.0000001, cr_new_stdev_value_ * stdev_high_mult_factor_);
}

void OfflineLowCorrelationPairsPort::OnIndicatorUpdate(const unsigned int& _indicator_index_,
                                                       const double& _new_value_) {
  if (!is_ready_) {
#if EQUITY_INDICATORS_ALWAYS_READY
    if ((dep_market_view_.is_ready_complex(2) || IndicatorUtil::IsEquityShortcode(dep_market_view_.shortcode())) &&
        (indep_portfolio_price_.is_ready()))
#else
    if (dep_market_view_.is_ready_complex(2) && indep_portfolio_price_.is_ready())
#endif
    {
      is_ready_ = true;
      InitializeValues();
    }
  } else {
    if (_indicator_index_ == 1) {
      dep_price_trend_ = _new_value_;
    } else if (_indicator_index_ == 2) {
      indep_price_trend_ = _new_value_;
      current_projected_trend_ = indep_price_trend_ * current_projection_multiplier_;

      sd_adj_current_projected_trend_ =
          current_projected_trend_ /
          std::max(0.05, fabs(current_lrinfo_.lr_correlation_));  ///< form of current_projected_trend_ that is likely
      /// to have same stdev as the dependant
    }

    double activity_mult_factor_ =
        std::min(2.00, std::max(0.25, fabs(sd_adj_current_projected_trend_) / stable_dep_stdev_value_));

    // if the magnitude of the projected_sd_adj_change is more than
    // a stable high threshold, which is HIGH_DEVIATION_CONSTANT * a long-period-stdev
    // of the dependant then and only then consider this
    indicator_value_ = activity_mult_factor_ * (current_projected_trend_ - dep_price_trend_);
    if (data_interrupted_) {
      indicator_value_ = 0;
    }

    NotifyIndicatorListeners(indicator_value_);
  }
}

void OfflineLowCorrelationPairsPort::InitializeValues() {
  indicator_value_ = 0;
  UpdateLRInfo();
}

void OfflineLowCorrelationPairsPort::UpdateLRInfo() {
  if ((last_lrinfo_updated_msecs_ == 0) ||
      (watch_.msecs_from_midnight() - last_lrinfo_updated_msecs_ > TENMINUTESMSECS)) {
    current_lrinfo_ = lrdb_.GetLRCoeff(dep_market_view_.shortcode(), indep_portfolio_price_.shortcode());
    ComputeMultiplier();

    if (dbglogger_.CheckLoggingLevel(LRDB_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "lrinfo ( " << dep_market_view_.shortcode() << ", "
                             << indep_portfolio_price_.shortcode() << " ) " << current_lrinfo_.lr_coeff_ << ' '
                             << current_lrinfo_.lr_correlation_ << " -> " << current_projection_multiplier_
                             << DBGLOG_ENDL_FLUSH;
    }

    last_lrinfo_updated_msecs_ = watch_.msecs_from_midnight() - (watch_.msecs_from_midnight() % TENMINUTESMSECS);
  }
}

void OfflineLowCorrelationPairsPort::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                             const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void OfflineLowCorrelationPairsPort::OnMarketDataResumed(const unsigned int _security_id_) {
  InitializeValues();
  data_interrupted_ = false;
}
}
