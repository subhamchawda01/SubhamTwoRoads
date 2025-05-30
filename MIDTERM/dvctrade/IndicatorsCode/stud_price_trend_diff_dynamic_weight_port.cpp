#include "dvctrade/Indicators/stud_price_trend_diff_dynamic_weight_port.hpp"

namespace HFSAT {

void StudPriceTrendDiffDynamicWeightPort::CollectShortCodes(
    std::vector<std::string>& _shortcodes_affecting_this_indicator_, std::vector<std::string>& _ors_source_needed_vec_,
    const std::vector<const char*>& r_tokens_) {
  IndicatorUtil::CollectShortcodeOrPortfolio(_shortcodes_affecting_this_indicator_, _ors_source_needed_vec_, r_tokens_);
  IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[4], _shortcodes_affecting_this_indicator_);
}

StudPriceTrendDiffDynamicWeightPort* StudPriceTrendDiffDynamicWeightPort::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _indep_market_view_port_ _ratio_duration_
  // _trend_duration_
  // _stdev_duration_ _price_type_
  if (r_tokens_.size() < 9) {
    ExitVerbose(
        kModelCreationIndicatorLineLessArgs, t_dbglogger_,
        "INDICATOR weight StudPriceTrendDiffDynamicWeightPort _shortcode_ _portfolio_ STDEV/VOL _ratio_duration_ "
        "_trend_duration_ "
        "_price_type_ ");
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValidShortCodeOrPortWithExit(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_, r_tokens_[3], (const std::string)(r_tokens_[4]),
                           (const std::string)(r_tokens_[5]), atof(r_tokens_[6]), atof(r_tokens_[7]),
                           StringToPriceType_t(r_tokens_[8]));
}

StudPriceTrendDiffDynamicWeightPort* StudPriceTrendDiffDynamicWeightPort::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, std::string _shortcode_,
    const std::string& _portfolio_descriptor_shortcode_, const std::string& _stdev_vol_, double _ratio_duration_,
    double _trend_duration_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _shortcode_ << ' ' << _portfolio_descriptor_shortcode_ << ' ' << _stdev_vol_ << ' '
              << _ratio_duration_ << ' ' << _trend_duration_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, StudPriceTrendDiffDynamicWeightPort*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new StudPriceTrendDiffDynamicWeightPort(
        t_dbglogger_, r_watch_, concise_indicator_description_, _shortcode_, _portfolio_descriptor_shortcode_,
        _stdev_vol_, _ratio_duration_, _trend_duration_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

StudPriceTrendDiffDynamicWeightPort::StudPriceTrendDiffDynamicWeightPort(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description,
    std::string _shortcode_, const std::string& _portfolio_descriptor_shortcode_, const std::string& _stdev_vol_,
    double _ratio_duration_, double _trend_duration_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description),
      ratio_duration_(_ratio_duration_),
      trend_duration_(_trend_duration_),
      lrdb_sign_(1),
      trend_dep_(0),
      trend_indep_(0),
      stdev_dep_updated_(false),
      stdev_indep_updated_(false),
      trend_indep_updated_(false),
      trend_dep_updated_(false),
      dep_interrupted_(false),
      indep_interrupted_(false),
      stdev_dep_(-1),
      stdev_indep_(-1),
      is_dep_portfolio_(false) {
  indep_portfolio_trend_ =
      DynamicWeightPortTrend::GetUniqueInstance(t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, _stdev_vol_,
                                                _ratio_duration_, _trend_duration_, _price_type_);
  dep_trend_ = SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, _shortcode_, _trend_duration_, _price_type_);
  if (ShortcodeSecurityMarketViewMap::StaticCheckValidPortWithoutExit(_shortcode_)) {
    lrdb_sign_ = 1;
    is_dep_portfolio_ = true;
  } else {
    ShortcodeSecurityMarketViewMap::StaticCheckValid(_shortcode_);

    dep_market_view_ = (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(_shortcode_));
    if (OfflineReturnsLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, _shortcode_)
            .GetLRCoeff(_shortcode_, _portfolio_descriptor_shortcode_)
            .lr_correlation_ < 0) {
      lrdb_sign_ = -1;
    }
  }

  SlowStdevTrendCalculator* dep_stdev_trend_ = SlowStdevTrendCalculator::GetUniqueInstance(
      t_dbglogger_, r_watch_, _shortcode_, ratio_duration_, _trend_duration_, _price_type_);
  if (dep_stdev_trend_ != NULL) dep_stdev_trend_->add_unweighted_indicator_listener(1, this);

  SlowStdevTrendCalculator* indep_stdev_trend_ = SlowStdevTrendCalculator::GetUniqueInstance(
      t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, ratio_duration_, _trend_duration_, _price_type_);
  if (indep_stdev_trend_ != NULL) indep_stdev_trend_->add_unweighted_indicator_listener(2, this);

  indep_portfolio_trend_->add_unweighted_indicator_listener(3, this);
  dep_trend_->add_unweighted_indicator_listener(4, this);
}

void StudPriceTrendDiffDynamicWeightPort::OnIndicatorUpdate(const unsigned int& _indicator_index_,
                                                            const double& _new_value_) {
  if (!is_ready_) {
    if (indep_portfolio_trend_->is_ready() && dep_trend_->IsReady()) {
      is_ready_ = true;
    }
  } else {
    if (_indicator_index_ == 1) {
      stdev_dep_ = _new_value_;
      stdev_dep_updated_ = true;
    } else if (_indicator_index_ == 2) {
      stdev_indep_ = _new_value_;
      stdev_indep_updated_ = true;
    } else if (_indicator_index_ == 3) {
      trend_indep_ = _new_value_;
      trend_indep_updated_ = true;
    } else if (_indicator_index_ == 4) {
      trend_dep_ = _new_value_;
      trend_dep_updated_ = true;
    }
    if (!data_interrupted_ && stdev_dep_updated_ && stdev_indep_updated_ && trend_dep_updated_ &&
        trend_indep_updated_) {
      indicator_value_ = lrdb_sign_ * trend_indep_ / stdev_indep_ - trend_dep_ / stdev_dep_;
    } else {
      indicator_value_ = 0;
    }
    NotifyIndicatorListeners(indicator_value_);
  }
}
void StudPriceTrendDiffDynamicWeightPort::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                                  const int msecs_since_last_receive_) {
  if (!is_dep_portfolio_ && dep_market_view_->security_id() == _security_id_) {
    dep_interrupted_ = true;
  }
  if (indep_portfolio_trend_->IsDataInterrupted()) {
    indep_interrupted_ = true;
  }
  if (indep_interrupted_ || dep_interrupted_) {
    data_interrupted_ = true;
    stdev_indep_updated_ = false;
    trend_indep_updated_ = false;
    stdev_dep_updated_ = false;
    trend_dep_updated_ = false;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void StudPriceTrendDiffDynamicWeightPort::OnMarketDataResumed(const unsigned int _security_id_) {
  if (data_interrupted_) {
    if (!is_dep_portfolio_ && dep_market_view_->security_id() == _security_id_) {
      dep_interrupted_ = false;
    }
    if (!indep_portfolio_trend_->IsDataInterrupted()) {
      indep_interrupted_ = false;
    }
    if (!(dep_interrupted_ || indep_interrupted_)) {
      data_interrupted_ = false;
    }
  }
}
}
