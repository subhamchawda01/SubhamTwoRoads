/**
    \file IndicatorsCode/slow_stdev_trend_calculator.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/slow_stdev_returns_calculator.hpp"

namespace HFSAT {
void SlowStdevReturnsCalculator::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                   std::vector<std::string>& _ors_source_needed_vec_,
                                                   const std::vector<const char*>& r_tokens_) {
  std::string t_source_shortcode_ = (std::string)r_tokens_[3];
  if (IndicatorUtil::IsPortfolioShortcode(t_source_shortcode_)) {
    IndicatorUtil::AddPortfolioShortCodeVec(t_source_shortcode_, _shortcodes_affecting_this_indicator_);
  } else {
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, t_source_shortcode_);
  }
}

SlowStdevReturnsCalculator* SlowStdevReturnsCalculator::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                          const Watch& r_watch_,
                                                                          const std::vector<const char*>& r_tokens_,
                                                                          PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_  _stdev_duration_ trend_duration_ _price_type_
  // _min_stdev_value_factor_
  double t_min_stdev_value_factor_ = 0.1;
  if (r_tokens_.size() > 7) {
    t_min_stdev_value_factor_ = atof(r_tokens_[7]);
  }
  return GetUniqueInstance(t_dbglogger_, r_watch_, (std::string)r_tokens_[3], atoi(r_tokens_[4]), atoi(r_tokens_[5]),
                           StringToPriceType_t(r_tokens_[6]), t_min_stdev_value_factor_);
}

SlowStdevReturnsCalculator* SlowStdevReturnsCalculator::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, std::string _source_shortcode_,
    const unsigned int _indicator_duration_, const unsigned int t_trend_history_secs_, PriceType_t _t_price_type_,
    double t_min_stdev_value_factor_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _source_shortcode_ << ' ' << _indicator_duration_ << ' ' << t_trend_history_secs_
              << ' ' << t_min_stdev_value_factor_ << ' ' << _t_price_type_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, SlowStdevReturnsCalculator*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new SlowStdevReturnsCalculator(
        t_dbglogger_, r_watch_, concise_indicator_description_, _source_shortcode_, _indicator_duration_,
        t_trend_history_secs_, _t_price_type_, t_min_stdev_value_factor_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

SlowStdevReturnsCalculator::SlowStdevReturnsCalculator(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_,
    std::string _source_shortcode_, const unsigned int _indicator_duration_, const unsigned int t_trend_history_secs_,
    PriceType_t _t_price_type_, const double t_min_stdev_value_factor_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      source_shortcode_(_source_shortcode_),
      price_type_(_t_price_type_),
      indep_portfolio_price__(nullptr),
      indep_market_view_(nullptr),
      moving_avg_trend_(0),
      moving_avg_squared_trend_(0),
      last_trend_recorded_(0),
      stdev_value_(0),
      current_indep_trend_(0),
      min_stdev_set_(false),
      is_price_ready_(false) {
  trend_history_msecs_ = std::max(100, (int)round(1000 * _indicator_duration_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();
  if (IndicatorUtil::IsPortfolioShortcode(_source_shortcode_)) {
    indep_trend_indicator_ = SimpleReturnsPort::GetUniqueInstance(t_dbglogger_, r_watch_, _source_shortcode_,
                                                                  t_trend_history_secs_, _t_price_type_);
    double min_price_increment_ = PCAPortPrice::GetUniqueInstance(t_dbglogger_, r_watch_, _source_shortcode_,
                                                                  kPriceTypeMktSizeWPrice)->min_price_increment();
    min_unbiased_l2_norm_ =
        min_price_increment_ * min_price_increment_ * t_min_stdev_value_factor_ * t_min_stdev_value_factor_;

    indep_portfolio_price__ =
        PCAPortPrice::GetUniqueInstance(t_dbglogger_, r_watch_, _source_shortcode_, _t_price_type_);
    indep_portfolio_price__->AddPriceChangeListener(this);
  } else {
    ShortcodeSecurityMarketViewMap::StaticCheckValid(_source_shortcode_);
    indep_market_view_ = ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(_source_shortcode_);
    indep_trend_indicator_ = SimpleReturns::GetUniqueInstance(t_dbglogger_, r_watch_, *indep_market_view_,
                                                              t_trend_history_secs_, _t_price_type_);
    min_unbiased_l2_norm_ = indep_market_view_->min_price_increment() * indep_market_view_->min_price_increment() *
                            t_min_stdev_value_factor_ * t_min_stdev_value_factor_;

    if (!indep_market_view_->subscribe_price_type(this, _t_price_type_)) {
      PriceType_t t_error_price_type_ = _t_price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << t_error_price_type_ << std::endl;
    }
  }
  indep_trend_indicator_->add_unweighted_indicator_listener(0, this);
}

void SlowStdevReturnsCalculator::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  current_indep_trend_ = _new_value_;

  if (!is_ready_) {
    if (min_stdev_set_) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_ && min_stdev_set_) {
    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      moving_avg_trend_ += inv_decay_sum_ * (current_indep_trend_ - last_trend_recorded_);
      moving_avg_squared_trend_ += inv_decay_sum_ * ((current_indep_trend_ * current_indep_trend_) -
                                                     (last_trend_recorded_ * last_trend_recorded_));
      last_trend_recorded_ = current_indep_trend_;
    } else {  // new page(s)
      int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {
          moving_avg_trend_ = (current_indep_trend_ * inv_decay_sum_) + (moving_avg_trend_ * decay_page_factor_);
          moving_avg_squared_trend_ = (current_indep_trend_ * current_indep_trend_ * inv_decay_sum_) +
                                      (moving_avg_squared_trend_ * decay_page_factor_);
        } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
          moving_avg_trend_ = (current_indep_trend_ * inv_decay_sum_) +
                              (last_trend_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                              (moving_avg_trend_ * decay_vector_[num_pages_to_add_]);
          moving_avg_squared_trend_ = (current_indep_trend_ * current_indep_trend_ * inv_decay_sum_) +
                                      (last_trend_recorded_ * last_trend_recorded_ * inv_decay_sum_ *
                                       decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                                      (moving_avg_squared_trend_ * decay_vector_[num_pages_to_add_]);
        }
        last_trend_recorded_ = current_indep_trend_;
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }

      // only changing stdev value on a new page ... can be done all the time, but saving computation
      double unbiased_l2_norm_ =
          std::max(min_unbiased_l2_norm_, (moving_avg_squared_trend_ - (moving_avg_trend_ * moving_avg_trend_)));
      stdev_value_ = sqrt(unbiased_l2_norm_);
      indicator_value_ = stdev_value_;

      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void SlowStdevReturnsCalculator::OnMarketUpdate(const unsigned int _security_id_,
                                                const MarketUpdateInfo& _market_update_info_) {
  if (!min_stdev_set_) {
    if (!is_price_ready_) {
      if (indep_market_view_->is_ready_complex(2)) {
        is_price_ready_ = true;
      }
    } else if (!data_interrupted_) {
      double t_current_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
      if (t_current_price_ != 0) {
        min_unbiased_l2_norm_ = min_unbiased_l2_norm_ / (t_current_price_ * t_current_price_);
        min_stdev_set_ = true;
      }
    }
  }
}

void SlowStdevReturnsCalculator::OnPortfolioPriceReset(double t_new_portfolio_price_, double t_old_portfolio_price_,
                                                       unsigned int is_data_interrupted_) {
  if (is_data_interrupted_ == 1u) {
    data_interrupted_ = true;
    NotifyIndicatorListeners(indicator_value_);
  } else if (is_data_interrupted_ == 2u) {
    InitializeValues();
    data_interrupted_ = false;
  } else if (is_data_interrupted_ == 0u) {
    if (!min_stdev_set_) {
      if (!is_price_ready_) {
        is_price_ready_ = true;
      } else {
        double t_current_price_ = t_new_portfolio_price_;
        if (t_current_price_ != 0) {
          min_unbiased_l2_norm_ = min_unbiased_l2_norm_ / (t_current_price_ * t_current_price_);
          min_stdev_set_ = true;
        }
      }
    }
  }
}

void SlowStdevReturnsCalculator::OnPortfolioPriceChange(double _new_price_) {
  if (!min_stdev_set_) {
    if (!is_price_ready_) {
      is_price_ready_ = true;
    } else {
      double t_current_price_ = _new_price_;
      if (t_current_price_ != 0) {
        min_unbiased_l2_norm_ = min_unbiased_l2_norm_ / (t_current_price_ * t_current_price_);
        min_stdev_set_ = true;
      }
    }
  }
}

void SlowStdevReturnsCalculator::InitializeValues() {
  moving_avg_trend_ = current_indep_trend_;
  moving_avg_squared_trend_ = current_indep_trend_ * current_indep_trend_;
  last_trend_recorded_ = current_indep_trend_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = sqrt(min_unbiased_l2_norm_);
}
void SlowStdevReturnsCalculator::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                         const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  is_price_ready_ = false;
  min_stdev_set_ = false;
}

void SlowStdevReturnsCalculator::OnMarketDataResumed(const unsigned int _security_id_) {
  InitializeValues();
  data_interrupted_ = false;
}
}
