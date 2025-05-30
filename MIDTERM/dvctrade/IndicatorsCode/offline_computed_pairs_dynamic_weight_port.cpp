#include "dvctrade/Indicators/offline_computed_pairs_dynamic_weight_port.hpp"

namespace HFSAT {

void OfflineComputedPairsDynamicWeightPort::CollectShortCodes(
    std::vector<std::string>& _shortcodes_affecting_this_indicator_, std::vector<std::string>& _ors_source_needed_vec_,
    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[4], _shortcodes_affecting_this_indicator_);
}

OfflineComputedPairsDynamicWeightPort* OfflineComputedPairsDynamicWeightPort::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _basepx_pxtype_) {
  // INDICATOR  _this_weight_  _indicator_string_  _dep_market_view_  _portfolio_descriptor_shortcode_ STDEV/VOL
  // ratio_duration trend_duration
  // _price_type_
  if (r_tokens_.size() < 9) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight OfflineComputedPairsDynamicWeightPort _dep_market_view_ _portfolio_ STDEV/VOL "
                "_ratio_duration_ "
                "_trend_duration_ "
                "_price_type_ ");
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           (std::string)(r_tokens_[4]), (std::string)(r_tokens_[5]), atof(r_tokens_[6]),
                           atof(r_tokens_[7]), StringToPriceType_t(r_tokens_[8]));
}

OfflineComputedPairsDynamicWeightPort* OfflineComputedPairsDynamicWeightPort::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, SecurityMarketView& _dep_market_view_,
    std::string _portfolio_descriptor_shortcode_, std::string _stdev_vol_, double _ratio_duration_,
    double _trend_duration_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _portfolio_descriptor_shortcode_ << ' '
              << _stdev_vol_ << ' ' << _ratio_duration_ << ' ' << _trend_duration_ << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OfflineComputedPairsDynamicWeightPort*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new OfflineComputedPairsDynamicWeightPort(
        t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _portfolio_descriptor_shortcode_,
        _stdev_vol_, _ratio_duration_, _trend_duration_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OfflineComputedPairsDynamicWeightPort::OfflineComputedPairsDynamicWeightPort(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_,
    SecurityMarketView& t_dep_market_view_, std::string _portfolio_descriptor_shortcode_, std::string _stdev_vol_,
    double _ratio_duration_, double _trend_duration_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      indep_portfolio_price_(
          *(PCAPortPrice::GetUniqueInstance(t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, _price_type_))),
      dep_price_trend_(0.0),
      indep_price_trend_(0.0),
      p_dep_indicator_(NULL),
      p_indep_indicator_(NULL),
      lrdb_(OfflineReturnsLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_.shortcode())),
      last_lrinfo_updated_msecs_(0),
      current_lrinfo_(0.0, 0.0),
      current_projection_multiplier_(0.0),
      current_projected_trend_(0),
      indep_interrupted_(false),
      dep_interrupted_(false) {
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
        SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_.shortcode(), _trend_duration_, _price_type_);
    p_dep_indicator_->add_unweighted_indicator_listener(1u, this);

    p_indep_indicator_ =
        DynamicWeightPortTrend::GetUniqueInstance(t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, _stdev_vol_,
                                                  _ratio_duration_, _trend_duration_, _price_type_);
    p_indep_indicator_->add_unweighted_indicator_listener(2u, this);
  }
}

void OfflineComputedPairsDynamicWeightPort::OnIndicatorUpdate(const unsigned int& _indicator_index_,
                                                              const double& _new_value_) {
  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indep_portfolio_price_.is_ready()) {
      is_ready_ = true;
      InitializeValues();
    }
  } else {
    if (_indicator_index_ == 1) {
      dep_price_trend_ = _new_value_;
    } else if (_indicator_index_ == 2) {
      indep_price_trend_ = _new_value_;
      current_projected_trend_ = indep_price_trend_ * current_projection_multiplier_;
    }

    indicator_value_ = current_projected_trend_ - dep_price_trend_;
    if (data_interrupted_) {
      indicator_value_ = 0;
    }

    NotifyIndicatorListeners(indicator_value_);
  }
}

void OfflineComputedPairsDynamicWeightPort::InitializeValues() {
  indicator_value_ = 0;
  dep_price_trend_ = 0.0;
  indep_price_trend_ = 0.0;
  UpdateLRInfo();
}

void OfflineComputedPairsDynamicWeightPort::UpdateLRInfo() {
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

void OfflineComputedPairsDynamicWeightPort::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                                    const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void OfflineComputedPairsDynamicWeightPort::OnMarketDataResumed(const unsigned int _security_id_) {
  if (data_interrupted_) {
    if (dep_market_view_.security_id() == _security_id_) dep_interrupted_ = false;
    if (!p_indep_indicator_->IsDataInterrupted()) indep_interrupted_ = false;
    if (!dep_interrupted_ && !indep_interrupted_) {
      data_interrupted_ = false;
      InitializeValues();
    }
  }
}
}
