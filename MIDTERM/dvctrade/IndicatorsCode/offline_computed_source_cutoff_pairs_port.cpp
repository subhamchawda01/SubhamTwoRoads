/**
    \file IndicatorsCode/offline_computed_source_cutoff_pairs_port.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/offline_computed_source_cutoff_pairs_port.hpp"

namespace HFSAT {

void OfflineComputedSourceCutoffPairsPort::CollectShortCodes(
    std::vector<std::string>& _shortcodes_affecting_this_indicator_, std::vector<std::string>& _ors_source_needed_vec_,
    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[4], _shortcodes_affecting_this_indicator_);
}

OfflineComputedSourceCutoffPairsPort* OfflineComputedSourceCutoffPairsPort::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _basepx_pxtype_) {
  // INDICATOR  _this_weight_  _indicator_string_  _dep_market_view_  _portfolio_descriptor_shortcode_
  // _fractional_seconds_ _trend_fractional_seconds_(optional) _price_type_

  if (r_tokens_.size() < 7) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight OfflineComputedSourceCutoffPairsPort _dep_market_view_ _indep_market_view_ "
                "_fractional_seconds_ _trend_fractional_seconds_ (optional) _price_type_ ");
  }

  if (r_tokens_.size() == 7) {
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             (std::string)(r_tokens_[4]), atof(r_tokens_[5]), -1.0, StringToPriceType_t(r_tokens_[6]));
  }
  if (std::string(r_tokens_[7]).compare("#") == 0) {
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             (std::string)(r_tokens_[4]), atof(r_tokens_[5]), -1.0, StringToPriceType_t(r_tokens_[6]));
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(
      t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
      (std::string)(r_tokens_[4]), atof(r_tokens_[5]), atof(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
}

OfflineComputedSourceCutoffPairsPort* OfflineComputedSourceCutoffPairsPort::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, SecurityMarketView& _dep_market_view_,
    std::string _portfolio_descriptor_shortcode_, double _fractional_seconds_, double _trend_fractional_seconds_,
    PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _portfolio_descriptor_shortcode_ << ' '
              << _fractional_seconds_ << ' ' << _trend_fractional_seconds_ << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  double t_trend_fractional_seconds_ =
      _trend_fractional_seconds_ > 0 ? _trend_fractional_seconds_ : _fractional_seconds_;

  static std::map<std::string, OfflineComputedSourceCutoffPairsPort*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new OfflineComputedSourceCutoffPairsPort(
        t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _portfolio_descriptor_shortcode_,
        _fractional_seconds_, t_trend_fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OfflineComputedSourceCutoffPairsPort::OfflineComputedSourceCutoffPairsPort(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_,
    SecurityMarketView& t_dep_market_view_, std::string _portfolio_descriptor_shortcode_, double _fractional_seconds_,
    double _trend_fractional_seconds_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      indep_portfolio_price_(
          *(PCAPortPrice::GetUniqueInstance(t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, _price_type_))),
      dep_price_trend_(0.0),
      indep_price_trend_(0.0),
      indep_cutoff_price_trend_(0.0),
      p_dep_indicator_(NULL),
      p_indep_indicator_(NULL),
      p_indep_trend_indicator_(NULL),
      lrdb_(OfflineReturnsLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_.shortcode())),
      last_lrinfo_updated_msecs_(0),
      current_lrinfo_(0.0, 0.0),
      current_projection_multiplier_(0.0),
      current_projected_trend_(0),
      current_cutoff_projected_trend_(0),
      trend_duration_ratio_(sqrt(_fractional_seconds_ / _trend_fractional_seconds_)) {
  watch_.subscribe_BigTimePeriod(this);  // for UpdateLRInfo and updating volume adjustment

  bool lrdb_absent_ = false;

  if (!(lrdb_.LRCoeffPresent(t_dep_market_view_.shortcode(), indep_portfolio_price_.shortcode()))) {
    lrdb_absent_ = true;
  }

  if (lrdb_absent_) {
    indicator_value_ = 0;
    is_ready_ = true;
  } else {
    p_dep_indicator_ =
        SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_.shortcode(), _fractional_seconds_, _price_type_);
    p_dep_indicator_->add_unweighted_indicator_listener(1u, this);

    p_indep_indicator_ = SimpleTrendPort::GetUniqueInstance(t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_,
                                                            _fractional_seconds_, _price_type_);
    p_indep_indicator_->add_unweighted_indicator_listener(2u, this);
    p_indep_trend_indicator_ = SimpleTrendPort::GetUniqueInstance(
        t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, _trend_fractional_seconds_, _price_type_);
    p_indep_trend_indicator_->add_unweighted_indicator_listener(3u, this);
  }

  // #if EQUITY_INDICATORS_ALWAYS_READY
  //     if ( _dep_indicator_ -> IsIndicatorReady ( ) && _indep_indicator_ -> IsIndicatorReady ( ) ) { is_ready_ = true;
  //     }
  // #endif
}

void OfflineComputedSourceCutoffPairsPort::OnIndicatorUpdate(const unsigned int& _indicator_index_,
                                                             const double& _new_value_) {
  if (!is_ready_) {
    // #if EQUITY_INDICATORS_ALWAYS_READY
    //  if ( ( dep_market_view_.is_ready_complex ( 2 ) || IndicatorUtil::IsEquityShortcode ( dep_market_view_.shortcode
    // ( ) ) ) &&
    //       ( indep_portfolio_price_.is_ready ( ) ) )
    // #else
    if (dep_market_view_.is_ready_complex(2) && indep_portfolio_price_.is_ready())
    // #endif
    {
      is_ready_ = true;
      InitializeValues();
    }
  }

  else {
    if (_indicator_index_ == 1u) {
      dep_price_trend_ = _new_value_;
    } else if (_indicator_index_ == 2u) {
      indep_price_trend_ = _new_value_;
      current_projected_trend_ = indep_price_trend_ * current_projection_multiplier_;
    } else if (_indicator_index_ == 3u) {
      indep_cutoff_price_trend_ = _new_value_ * trend_duration_ratio_;
      current_cutoff_projected_trend_ = indep_cutoff_price_trend_ * current_projection_multiplier_;
    }

    if (current_projected_trend_ < 0.01 * dep_market_view_.min_price_increment()) {
      // To avoid division by zero
      indicator_value_ = current_projected_trend_;
    } else {
      // trend factor measures how much of the source has already been captured in dep price action. Constrained to
      // (0,1)
      double dep_indep_trend_factor_ = std::min(1.0, std::max(0.0, dep_price_trend_ / current_projected_trend_));
      // this is sort of counter-intuitive to the notion of relative indicators, but might help overall.
      indicator_value_ = (1 - dep_indep_trend_factor_) * (current_projected_trend_ - dep_price_trend_) +
                         dep_indep_trend_factor_ * current_cutoff_projected_trend_;
    }

    if (data_interrupted_) {
      indicator_value_ = 0;
    }

    NotifyIndicatorListeners(indicator_value_);
  }
}

void OfflineComputedSourceCutoffPairsPort::InitializeValues() {
  indicator_value_ = 0;
  UpdateLRInfo();
}

void OfflineComputedSourceCutoffPairsPort::UpdateLRInfo() {
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

void OfflineComputedSourceCutoffPairsPort::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                                   const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void OfflineComputedSourceCutoffPairsPort::OnMarketDataResumed(const unsigned int _security_id_) {
  InitializeValues();
  data_interrupted_ = false;
}
}
