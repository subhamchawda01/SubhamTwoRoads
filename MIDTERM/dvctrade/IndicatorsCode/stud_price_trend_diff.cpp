/**
    \file IndicatorsCode/stud_price_diff.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/stud_price_trend_diff.hpp"

namespace HFSAT {

void StudPriceTrendDiff::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                           std::vector<std::string>& _ors_source_needed_vec_,
                                           const std::vector<const char*>& r_tokens_) {
  IndicatorUtil::CollectShortcodeOrPortfolio(_shortcodes_affecting_this_indicator_, _ors_source_needed_vec_, r_tokens_);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

StudPriceTrendDiff* StudPriceTrendDiff::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                          const std::vector<const char*>& r_tokens_,
                                                          PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _source_shortcode_ _fractional_seconds_
  // _stdev_duration_ include_lrdb_sign_ _price_type_
  if (r_tokens_.size() < 8) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight StudPriceTrendDiff _dep_market_view_ _indep_market_view_ _fractional_seconds_ "
                "_stdev_duration_ _price_type_ ");
  }
  if (r_tokens_.size() == 8) {
    //  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
    return GetUniqueInstance(t_dbglogger_, r_watch_, r_tokens_[3],
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                             atof(r_tokens_[5]), atof(r_tokens_[6]), -1, StringToPriceType_t(r_tokens_[7]));
  }
  if (std::string(r_tokens_[8]).compare("#") == 0) {
    // ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
    return GetUniqueInstance(t_dbglogger_, r_watch_, r_tokens_[3],
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                             atof(r_tokens_[5]), atof(r_tokens_[6]), -1, StringToPriceType_t(r_tokens_[7]));
  }
  // ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
  return GetUniqueInstance(t_dbglogger_, r_watch_, r_tokens_[3],
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           atof(r_tokens_[5]), atof(r_tokens_[6]), atof(r_tokens_[7]),
                           StringToPriceType_t(r_tokens_[8]));
}

StudPriceTrendDiff* StudPriceTrendDiff::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                          std::string _shortcode_,
                                                          const SecurityMarketView& _indep_market_view_,
                                                          double _indicator_duration_, double _stdev_duration_,
                                                          double _lrdb_sign_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _shortcode_ << ' ' << _indep_market_view_.secname() << ' ' << _indicator_duration_
              << ' ' << _stdev_duration_ << ' ' << _lrdb_sign_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, StudPriceTrendDiff*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new StudPriceTrendDiff(t_dbglogger_, r_watch_, concise_indicator_description_, _shortcode_, _indep_market_view_,
                               _indicator_duration_, _stdev_duration_, _lrdb_sign_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

StudPriceTrendDiff::StudPriceTrendDiff(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                       const std::string& concise_indicator_description_, std::string _shortcode_,
                                       const SecurityMarketView& _indep_market_view_, double _indicator_duration_,
                                       double _stdev_duration_, double _lrdb_sign_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      shortcode_(_shortcode_),
      indep_market_view_(_indep_market_view_),
      trend_history_msecs_(std::max(20, (int)round(1000 * _indicator_duration_))),
      price_type_(_price_type_),
      last_new_page_msecs_(0),
      page_width_msecs_(500),
      decay_page_factor_(0.95),
      inv_decay_sum_(0.05),
      moving_avg_dep_(0),
      last_dep_price_(0),
      current_dep_price_(0),
      moving_avg_indep_(0),
      last_indep_price_(0),
      current_indep_price_(0),
      stdev_dep_(-1),
      stdev_indep_(-1),
      dep_interrupted_(false),
      indep_interrupted_(false),
      lrdb_sign_(1),
      is_dep_portfolio_(false),
      dep_portfolio_(nullptr) {
  SetTimeDecayWeights();

  if (ShortcodeSecurityMarketViewMap::StaticCheckValidPortWithoutExit(_shortcode_)) {
    dep_portfolio_ = PricePortfolio::GetUniqueInstance(t_dbglogger_, r_watch_, _shortcode_, 5, _price_type_);

    dep_portfolio_->add_unweighted_indicator_listener(3, this);
    is_dep_portfolio_ = true;
  } else {
    ShortcodeSecurityMarketViewMap::StaticCheckValid(_shortcode_);

    dep_market_view_ = (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(_shortcode_));
    if (!dep_market_view_->subscribe_price_type(this, _price_type_)) {
      PriceType_t t_error_price_type_ = _price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << t_error_price_type_ << std::endl;
    }
  }

  if (!indep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }

  dep_stdev_trend_calculator_ = SlowStdevTrendCalculator::GetUniqueInstance(
      t_dbglogger_, r_watch_, shortcode_, _stdev_duration_, _indicator_duration_, _price_type_);
  indep_stdev_trend_calculator_ = SlowStdevTrendCalculator::GetUniqueInstance(
      t_dbglogger_, r_watch_, indep_market_view_.shortcode(), _stdev_duration_, _indicator_duration_, _price_type_);
  dep_stdev_trend_calculator_->add_unweighted_indicator_listener(1u, this);
  indep_stdev_trend_calculator_->add_unweighted_indicator_listener(2u, this);

  if (_lrdb_sign_ > 0 && !is_dep_portfolio_) {
    if (OfflineReturnsLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, shortcode_)
            .GetLRCoeff(dep_market_view_->shortcode(), indep_market_view_.shortcode())
            .lr_correlation_ < 0) {
      lrdb_sign_ = -1;
    }
  } else {
    lrdb_sign_ = 1;
  }
}

void StudPriceTrendDiff::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (_indicator_index_ == 1u) {
    stdev_dep_ = _new_value_;
  } else if (_indicator_index_ == 2u) {
    stdev_indep_ = _new_value_;
  } else if (_indicator_index_ == 3u) {
    current_dep_price_ = _new_value_;
  }
  if (!is_ready_) {
    if (is_dep_portfolio_ == true) {
      if (indep_market_view_.is_ready_complex(2) && dep_portfolio_->is_ready()) {
        is_ready_ = true;
        InitializeValues();
      }
    } else if (indep_market_view_.is_ready_complex(2) && dep_market_view_->is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_ && stdev_dep_ > 0 && stdev_indep_ > 0) {
    UpdateComputedVariables();
  }
}

void StudPriceTrendDiff::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void StudPriceTrendDiff::OnMarketUpdate(const unsigned int _security_id_,
                                        const MarketUpdateInfo& _market_update_info_) {
  if (_security_id_ == indep_market_view_.security_id()) {
    current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  } else {
    current_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  }

  if (!is_ready_) {
    if (is_dep_portfolio_ == true) {
      if (indep_market_view_.is_ready_complex(2) && dep_portfolio_->is_ready()) {
        is_ready_ = true;
        InitializeValues();
      }
    } else if (indep_market_view_.is_ready_complex(2) && dep_market_view_->is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_ && stdev_dep_ > 0 && stdev_indep_ > 0) {
    UpdateComputedVariables();
  }
}
void StudPriceTrendDiff::UpdateComputedVariables() {
  if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
    moving_avg_dep_ += inv_decay_sum_ * (current_dep_price_ - last_dep_price_);
    moving_avg_indep_ += inv_decay_sum_ * (current_indep_price_ - last_indep_price_);
  } else {
    int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
    if (num_pages_to_add_ >= (int)decay_vector_.size()) {
      InitializeValues();
    } else {
      if (num_pages_to_add_ == 1) {
        moving_avg_dep_ = (current_dep_price_ * inv_decay_sum_) + (moving_avg_dep_ * decay_vector_[1]);
        moving_avg_indep_ = (current_indep_price_ * inv_decay_sum_) + (moving_avg_indep_ * decay_vector_[1]);
      } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
        moving_avg_dep_ = (current_dep_price_ * inv_decay_sum_) +
                          (last_dep_price_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                          (moving_avg_dep_ * decay_vector_[num_pages_to_add_]);
        moving_avg_indep_ = (current_indep_price_ * inv_decay_sum_) +
                            (last_indep_price_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                            (moving_avg_indep_ * decay_vector_[num_pages_to_add_]);
      }
      last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
    }
  }

  last_dep_price_ = current_dep_price_;
  last_indep_price_ = current_indep_price_;

  indicator_value_ = lrdb_sign_ * (current_indep_price_ - moving_avg_indep_) / stdev_indep_ -
                     (current_dep_price_ - moving_avg_dep_) / stdev_dep_;

  NotifyIndicatorListeners(indicator_value_);
}
void StudPriceTrendDiff::SetTimeDecayWeights() {
  const unsigned int kDecayLength =
      20;  ///< here number of samples are not required to be very high and hence the decaylength target is just 20
  const unsigned int kMinPageWidth = 10;
  const unsigned int kMaxPageWidth =
      200;  ///< keeping kMaxPageWidth low makes the number_fadeoffs_ pretty high and hence keeps lots of sample points
  page_width_msecs_ = std::min(kMaxPageWidth, std::max(kMinPageWidth, (trend_history_msecs_ / kDecayLength)));

  int number_fadeoffs_ = std::max(1, (int)ceil(trend_history_msecs_ / page_width_msecs_));

  decay_page_factor_ = MathUtils::CalcDecayFactor(number_fadeoffs_);

  decay_vector_.resize(2 * number_fadeoffs_);
  decay_vector_sums_.resize(2 * number_fadeoffs_);

  for (auto i = 0u; i < decay_vector_.size(); i++) {
    decay_vector_[i] = pow(decay_page_factor_, (int)i);
  }

  decay_vector_sums_[0] = 0;
  for (unsigned int i = 1; i < decay_vector_sums_.size(); i++) {
    decay_vector_sums_[i] = decay_vector_sums_[i - 1] + decay_vector_[i];
  }

  inv_decay_sum_ = (1 - decay_page_factor_);
}

void StudPriceTrendDiff::InitializeValues() {
  moving_avg_dep_ = current_dep_price_;
  moving_avg_indep_ = current_indep_price_;

  last_indep_price_ = current_indep_price_;
  last_dep_price_ = current_dep_price_;

  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}

// market_interrupt_listener interface
void StudPriceTrendDiff::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                 const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    indep_interrupted_ = true;
  } else if (!is_dep_portfolio_ && dep_market_view_->security_id() == _security_id_) {
    dep_interrupted_ = true;
  }
  if (indep_interrupted_ || dep_interrupted_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void StudPriceTrendDiff::OnMarketDataResumed(const unsigned int _security_id_) {
  if (data_interrupted_) {
    if (indep_market_view_.security_id() == _security_id_) {
      indep_interrupted_ = false;
    } else if (!is_dep_portfolio_ && dep_market_view_->security_id() == _security_id_) {
      dep_interrupted_ = false;
    }
    if (!(dep_interrupted_ || indep_interrupted_)) {
      InitializeValues();
      data_interrupted_ = false;
    }
  }
}
}
