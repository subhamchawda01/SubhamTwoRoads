#include "dvctrade/Indicators/dynamic_weight_port_trend.hpp"

namespace HFSAT {

void DynamicWeightPortTrend::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                               std::vector<std::string>& _ors_source_needed_vec_,
                                               const std::vector<const char*>& _tokens_) {
  IndicatorUtil::AddPortfolioShortCodeVec(std::string(_tokens_[3]), _shortcodes_affecting_this_indicator_);
}

DynamicWeightPortTrend* DynamicWeightPortTrend::GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                                  const std::vector<const char*>& _tokens_,
                                                                  PriceType_t _basepx_pxtype_) {
  if (_tokens_.size() < 8) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, _dbglogger_,
                "INDICATOR weight DynamicWeightPortTrend _portfolio_shortcode_ [STDEV/VOL/STDEV_VOL] "
                "_indicator_duration_ trend_duration_ _price_type_ ");
  }
  return GetUniqueInstance(_dbglogger_, _watch_, std::string(_tokens_[3]), std::string(_tokens_[4]), atof(_tokens_[5]),
                           atof(_tokens_[6]), StringToPriceType_t(_tokens_[7]));
}

DynamicWeightPortTrend* DynamicWeightPortTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                                                                  const std::string& t_portfolio_descriptor_shortcode_,
                                                                  const std::string& _stdev_vol_,
                                                                  const double _indicator_duration_,
                                                                  const double _trend_duration_,
                                                                  const PriceType_t _price_type_) {
  std::ostringstream t_temp_oss;
  t_temp_oss << VarName() << ' ' << t_portfolio_descriptor_shortcode_ << ' ' << _stdev_vol_ << ' '
             << _indicator_duration_ << ' ' << _trend_duration_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_dynamic_weight_port_trend_description(t_temp_oss.str());

  static std::map<std::string, DynamicWeightPortTrend*> concise_dynamic_weight_port_trend_description_map;
  if (concise_dynamic_weight_port_trend_description_map.find(concise_dynamic_weight_port_trend_description) ==
      concise_dynamic_weight_port_trend_description_map.end()) {
    concise_dynamic_weight_port_trend_description_map[concise_dynamic_weight_port_trend_description] =
        new DynamicWeightPortTrend(t_dbglogger_, t_watch_, concise_dynamic_weight_port_trend_description,
                                   t_portfolio_descriptor_shortcode_, _stdev_vol_, _indicator_duration_,
                                   _trend_duration_, _price_type_);
  }

  return concise_dynamic_weight_port_trend_description_map[concise_dynamic_weight_port_trend_description];
}

DynamicWeightPortTrend::DynamicWeightPortTrend(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                                               const std::string& concise_indicator_description_,
                                               const std::string& t_portfolio_descriptor_shortcode_,
                                               const std::string& t_stdev_vol_, const double _indicator_duration_,
                                               const double _trend_duration_, const PriceType_t& _price_type_)
    : CommonIndicator(t_dbglogger_, t_watch_, concise_indicator_description_),
      dbglogger_(t_dbglogger_),
      watch_(t_watch_),
      portfolio_descriptor_shortcode_(t_portfolio_descriptor_shortcode_),
      is_ready_(false),
      security_id_weight_vec_(),
      current_security_id_weight_vec_(),
      price_type_(_price_type_),
      trend_history_secs_(_trend_duration_),
      indicator_duration_(_indicator_duration_),
      is_stdev_(false),
      is_vol_(false),
      indicator_n_(0.0),
      recompute_indicator_value_(false),
      indicator_d_(0.0),
      first_update_(false),
      is_ready_vec_(),
      data_interrupted_vec_() {
  std::vector<std::string> shortcode_vec_;
  IndicatorUtil::GetPortfolioShortCodeVec(t_portfolio_descriptor_shortcode_, shortcode_vec_);

  (ShortcodeSecurityMarketViewMap::GetUniqueInstance())
      .GetSecurityMarketViewVec(shortcode_vec_, indep_market_view_vec_);

  // get eigen components for the portfolio
  const EigenConstituentsVec& eigen_constituent_vec =
      (PcaWeightsManager::GetUniqueInstance()).GetEigenConstituentVec(t_portfolio_descriptor_shortcode_);
  // get stdev for every product in portfolio
  const std::vector<double>& stdev_constituent_vec =
      (PcaWeightsManager::GetUniqueInstance()).GetPortfolioStdevs(t_portfolio_descriptor_shortcode_);
  is_pca_ = (PcaWeightsManager::GetUniqueInstance().IsPCA(t_portfolio_descriptor_shortcode_));

  num_products_ = indep_market_view_vec_.size();

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " Shortcode stdev Values for port : " << t_portfolio_descriptor_shortcode_
                                << DBGLOG_ENDL_FLUSH;
    for (unsigned i = 0; i < stdev_constituent_vec.size(); i++) {
      dbglogger_ << stdev_constituent_vec[i] << " ";
    }
    dbglogger_ << DBGLOG_ENDL_FLUSH;
  }

  if (eigen_constituent_vec.empty()) {
    std::cerr << "DynamicWeightPortTrend::EigenConstituentsVec " << t_portfolio_descriptor_shortcode_
              << " does not have even single eigenvectors/values computed " << std::endl;
    ExitVerbose(kPCAWeightManagerMissingPortFromMap, t_portfolio_descriptor_shortcode_.c_str());
  }

  if (stdev_constituent_vec.empty()) {
    std::cerr << "DynamicWeightPortTrend::Stdevs computed for " << stdev_constituent_vec.size()
              << " but portfolio size is " << shortcode_vec_.size() << std::endl;
    ExitVerbose(kPCAWeightManagerMissingPortFromMap, t_portfolio_descriptor_shortcode_.c_str());
  }

  if (!eigen_constituent_vec.empty()) {
    if (eigen_constituent_vec[0].eigenvec_components_.size() != shortcode_vec_.size()) {
      std::cerr << "DynamicWeightPortTrend::PCA Eigenvector components size "
                << eigen_constituent_vec[0].eigenvec_components_.size() << " but portfolio size is "
                << shortcode_vec_.size() << std::endl;
      ExitVerbose(kPCAWeightManagerMissingPortFromMap, t_portfolio_descriptor_shortcode_.c_str());
    }
  }

  // if using stdev ratio, take historical volume ratio to be 1, setting stdev ratio values to be unupdated
  if (t_stdev_vol_ == "STDEV") {
    is_stdev_ = true;
    vol_updated_vec_.resize(num_products_, true);
    stdev_updated_vec_.resize(num_products_, false);
  }
  // if using vol ratio, take stdev ratio to be 1, setting  vol ratio values to be unupdated
  else if (t_stdev_vol_ == "VOL") {
    is_vol_ = true;
    stdev_updated_vec_.resize(num_products_, true);
    vol_updated_vec_.resize(num_products_, false);
  } else if (t_stdev_vol_ == "STDEV_VOL") {
    is_stdev_ = true;
    is_vol_ = true;
    stdev_updated_vec_.resize(num_products_, false);
    vol_updated_vec_.resize(num_products_, false);
  }
  trend_updated_vec_.resize(num_products_, false);
  stdev_vec_.resize(num_products_, 1);
  vol_vec_.resize(num_products_, 1);
  trend_vec_.resize(num_products_, 0);
  is_ready_vec_.resize(num_products_, false);
  data_interrupted_vec_.resize(num_products_, false);
  security_id_weight_vec_.resize(num_products_, 0.0);
  current_security_id_weight_vec_.resize(num_products_, 0.00);
  p_stdev_trend_indicator_vec_.resize(num_products_);
  p_simple_trend_indicator_vec_.resize(num_products_);
  p_volume_indicator_vec_.resize(num_products_);

  double sum_abs_ = 0.0;
  double abs_sum_ = 0.0;
  for (unsigned i = 0; i < num_products_; i++) {
    if (indep_market_view_vec_[i]->is_ready()) {
      is_ready_vec_[i] = true;
    }

    // retrieving historical volume and trendstdev from sample data
    double hist_volume_ = 1, hist_trend_stdev_ = 1;

    if (is_vol_) {
      hist_volume_ = SampleDataUtil::GetAvgForPeriod(indep_market_view_vec_[i]->shortcode(), watch_.YYYYMMDD(), 250,
                                                     trading_start_mfm_, trading_end_mfm_, "VOL");
    }
    if (is_stdev_) {
      hist_trend_stdev_ =
          SampleDataUtil::GetAvgForPeriod(indep_market_view_vec_[i]->shortcode(), watch_.YYYYMMDD(), 250,
                                          trading_start_mfm_, trading_end_mfm_, "RollingTrendStdev300");
    }

    double norm_factor_ = hist_volume_ * hist_trend_stdev_;

    // storing weights of product in portfolio
    double t_this_weight_ = (eigen_constituent_vec[0].eigenvec_components_[i] / stdev_constituent_vec[i]);
    sum_abs_ += fabs(t_this_weight_);
    abs_sum_ += t_this_weight_;
    if (is_pca_) {
      security_id_weight_vec_[i] = t_this_weight_ / norm_factor_;
      current_security_id_weight_vec_[i] = t_this_weight_ / norm_factor_;
    } else {
      security_id_weight_vec_[i] = eigen_constituent_vec[0].eigenvec_components_[i] / norm_factor_;
      current_security_id_weight_vec_[i] = eigen_constituent_vec[0].eigenvec_components_[i] / norm_factor_;
    }

    if (is_stdev_) {
      // initializing indicators for all products in portfolio
      SlowStdevTrendCalculator* _p_stdev_trend_indicator_ =
          SlowStdevTrendCalculator::GetUniqueInstance(dbglogger_, watch_, (*(indep_market_view_vec_[i])).shortcode(),
                                                      indicator_duration_, trend_history_secs_, price_type_);
      _p_stdev_trend_indicator_->add_unweighted_indicator_listener(i, this);
      p_stdev_trend_indicator_vec_[i] = _p_stdev_trend_indicator_;
    }
    SimpleTrend* _p_simple_trend_indicator_ = SimpleTrend::GetUniqueInstance(
        dbglogger_, watch_, indep_market_view_vec_[i]->shortcode(), trend_history_secs_, price_type_);
    _p_simple_trend_indicator_->add_unweighted_indicator_listener(num_products_ + i, this);
    p_simple_trend_indicator_vec_[i] = _p_simple_trend_indicator_;

    if (is_vol_) {
      RecentSimpleVolumeMeasure* _p_volume_indicator_ = RecentSimpleVolumeMeasure::GetUniqueInstance(
          dbglogger_, watch_, *(indep_market_view_vec_[i]), indicator_duration_);
      _p_volume_indicator_->AddRecentSimpleVolumeListener(i, this);
      p_volume_indicator_vec_[i] = _p_volume_indicator_;
    }
  }
  abs_sum_ = fabs(abs_sum_);
  for (unsigned i = 0; i < num_products_; i++) {
    security_id_weight_vec_[i] *= (sum_abs_ / abs_sum_);
    current_security_id_weight_vec_[i] *= (sum_abs_ / abs_sum_);
  }
  is_ready_ = AreAllReady();
  is_updated_ = AreAllUpdated();
}

void DynamicWeightPortTrend::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (!is_ready_) {
    if (!is_ready_vec_[_indicator_index_ % num_products_] &&
        indep_market_view_vec_[_indicator_index_ % num_products_]->is_ready()) {
      is_ready_vec_[_indicator_index_ % num_products_] = true;
    }

    is_ready_ = AreAllReady();
  }
  // setting the first values of the stdev, volume and trend to be initialized correctly
  else if (!is_updated_) {
    if (_indicator_index_ < num_products_ && is_stdev_) {
      stdev_updated_vec_[_indicator_index_ % num_products_] = true;
      stdev_vec_[_indicator_index_] = _new_value_;
    } else if (_indicator_index_ >= num_products_) {
      trend_updated_vec_[_indicator_index_ - num_products_] = true;
      trend_vec_[_indicator_index_ - num_products_] = _new_value_;
    }
    is_updated_ = AreAllUpdated();
    if (is_updated_) {
      recompute_indicator_value_ = true;
    }
  } else if (recompute_indicator_value_) {
    indicator_value_ = 0.0;
    indicator_n_ = 0.0;
    indicator_d_ = 0.0;
    for (unsigned i = 0; i < num_products_; i++) {
      indicator_n_ += current_security_id_weight_vec_[i] * stdev_vec_[i] * vol_vec_[i] * trend_vec_[i];
      indicator_d_ += fabs(current_security_id_weight_vec_[i]) * stdev_vec_[i] * vol_vec_[i];
    }
    if (indicator_d_ == 0) {
      indicator_value_ = 0;
    } else {
      indicator_value_ = indicator_n_ / indicator_d_;
    }
    recompute_indicator_value_ = false;
  } else if (!data_interrupted_) {
    if (_indicator_index_ < num_products_ && is_stdev_) {
      // if change in stdev values, calculating numerator and denominator using the difference in values
      indicator_n_ += current_security_id_weight_vec_[_indicator_index_] * vol_vec_[_indicator_index_] *
                      (_new_value_ - stdev_vec_[_indicator_index_]) * trend_vec_[_indicator_index_];
      indicator_d_ += fabs(current_security_id_weight_vec_[_indicator_index_]) * vol_vec_[_indicator_index_] *
                      (_new_value_ - stdev_vec_[_indicator_index_]);
      if (indicator_d_ == 0) {
        indicator_value_ = 0;
      } else {
        indicator_value_ = indicator_n_ / indicator_d_;
      }
      stdev_vec_[_indicator_index_] = _new_value_;
    } else if (_indicator_index_ >= num_products_) {
      // if change in trend values, calculating numerator using difference in values
      unsigned int _index_ = _indicator_index_ - num_products_;
      indicator_n_ += current_security_id_weight_vec_[_index_] * vol_vec_[_index_] * stdev_vec_[_index_] *
                      (_new_value_ - trend_vec_[_index_]);
      if (indicator_d_ == 0) {
        indicator_value_ = 0;
      } else {
        indicator_value_ = indicator_n_ / indicator_d_;
      }
      trend_vec_[_index_] = _new_value_;
    }
    NotifyIndicatorListeners(indicator_value_);
  }
}

void DynamicWeightPortTrend::OnVolumeUpdate(const unsigned int _indicator_index_, const double _new_value_) {
  if (!is_ready_) {
    if (!is_ready_vec_[_indicator_index_] && indep_market_view_vec_[_indicator_index_]->is_ready()) {
      is_ready_vec_[_indicator_index_] = true;
    }
    is_ready_ = AreAllReady();
  } else if (!is_updated_) {
    if (is_vol_ && _indicator_index_ < num_products_) {
      vol_updated_vec_[_indicator_index_] = true;
      vol_vec_[_indicator_index_] = _new_value_;
    }
    is_updated_ = AreAllUpdated();
    if (is_updated_) {
      recompute_indicator_value_ = true;
    }
  } else if (recompute_indicator_value_) {
    indicator_value_ = 0.0;
    indicator_n_ = 0.0;
    indicator_d_ = 0.0;
    for (unsigned i = 0; i < num_products_; i++) {
      indicator_n_ += current_security_id_weight_vec_[i] * stdev_vec_[i] * vol_vec_[i] * trend_vec_[i];
      indicator_d_ += fabs(current_security_id_weight_vec_[i]) * stdev_vec_[i] * vol_vec_[i];
    }
    if (indicator_d_ == 0) {
      indicator_value_ = 0;
    } else {
      indicator_value_ = indicator_n_ / indicator_d_;
    }
    recompute_indicator_value_ = false;
  } else if (!data_interrupted_) {
    if (is_vol_) {
      // if change in volume values, recalculating indicator value
      indicator_n_ += current_security_id_weight_vec_[_indicator_index_] * stdev_vec_[_indicator_index_] *
                      (_new_value_ - vol_vec_[_indicator_index_]) * trend_vec_[_indicator_index_];
      indicator_d_ += fabs(current_security_id_weight_vec_[_indicator_index_]) * stdev_vec_[_indicator_index_] *
                      (_new_value_ - vol_vec_[_indicator_index_]);
      if (indicator_d_ == 0) {
        indicator_value_ = 0;
      } else {
        indicator_value_ = indicator_n_ / indicator_d_;
      }
      vol_vec_[_indicator_index_] = _new_value_;
    }
    NotifyIndicatorListeners(indicator_value_);
  }
}

inline void DynamicWeightPortTrend::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                            const int msecs_since_last_receive_) {
  for (unsigned i = 0; i < num_products_; i++) {
    if (indep_market_view_vec_[i] != NULL) {
      if (indep_market_view_vec_[i]->security_id() == _security_id_) {
        indicator_d_ = 0.0;
        indicator_n_ = 0.0;
        indicator_value_ = 0.0;
        recompute_indicator_value_ = true;
        data_interrupted_ = true;
        data_interrupted_vec_[i] = true;
        trend_updated_vec_[i] = false;
        if (is_stdev_) {
          stdev_updated_vec_[i] = false;
        }
        if (is_vol_) {
          vol_updated_vec_[i] = false;
        }
        is_updated_ = false;

        NotifyIndicatorListeners(indicator_value_);
        break;
      }
    }
  }
}

inline void DynamicWeightPortTrend::OnMarketDataResumed(const unsigned int _security_id_) {
  for (unsigned i = 0; i < num_products_; i++) {
    if (indep_market_view_vec_[i] != NULL) {
      if (indep_market_view_vec_[i]->security_id() == _security_id_) {
        data_interrupted_vec_[i] = false;
        data_interrupted_ = VectorUtils::LinearSearchValue(data_interrupted_vec_, true);
        break;
      }
    }
  }
}
}
