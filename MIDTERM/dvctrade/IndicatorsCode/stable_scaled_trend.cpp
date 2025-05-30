/**
    \file IndicatorsCode/stable_scaled_trend.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/stable_scaled_trend.hpp"

namespace HFSAT {

void StableScaledTrend::CollectShortCodes(std::vector<std::string>& t_shortcodes_affecting_this_indicator_,
                                          std::vector<std::string>& _ors_source_needed_vec_,
                                          const std::vector<const char*>& r_tokens_) {
  IndicatorUtil::CollectShortcodeOrPortfolio(t_shortcodes_affecting_this_indicator_, _ors_source_needed_vec_,
                                             r_tokens_);
}

StableScaledTrend* StableScaledTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                        const std::vector<const char*>& r_tokens_,
                                                        PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ t_indep_market_view_ _fractional_seconds_ _price_type_
  return GetUniqueInstance(t_dbglogger_, r_watch_, r_tokens_[3], atof(r_tokens_[4]), StringToPriceType_t(r_tokens_[5]));
}

StableScaledTrend* StableScaledTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                        std::string _shortcode_, double _fractional_seconds_,
                                                        PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _shortcode_ << ' ' << _fractional_seconds_ << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  // static std::map<std::string, StableScaledTrend*> concise_indicator_description_map_;
  if (global_concise_indicator_description_map_.find(concise_indicator_description_) ==
      global_concise_indicator_description_map_.end()) {
    global_concise_indicator_description_map_[concise_indicator_description_] = new StableScaledTrend(
        t_dbglogger_, r_watch_, concise_indicator_description_, _shortcode_, _fractional_seconds_, _price_type_);
  }
  return dynamic_cast<StableScaledTrend*>(global_concise_indicator_description_map_[concise_indicator_description_]);
}

StableScaledTrend::StableScaledTrend(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                     const std::string& concise_indicator_description_, std::string _shortcode_,
                                     double _fractional_seconds_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      price_type_(_price_type_),
      slow_stdev_calculator_(*(SlowStdevCalculator::GetUniqueInstance(t_dbglogger_, r_watch_, _shortcode_))),
      stable_stdev_value_(1),
      fast_math_multiplier_(1),
      moving_avg_price_(0),
      last_price_recorded_(0),
      current_indep_price_(0),
      is_indep_portfolio_(false) {
  trend_history_msecs_ = std::max(MIN_MSEC_HISTORY_INDICATORS, (int)round(1000 * _fractional_seconds_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();
  slow_stdev_calculator_.AddSlowStdevCalculatorListener(this);
  slow_stdev_calculator_.add_unweighted_indicator_listener(1, this);
  if (ShortcodeSecurityMarketViewMap::StaticCheckValidPortWithoutExit(_shortcode_)) {
    indep_portfolio_ = PricePortfolio::GetUniqueInstance(t_dbglogger_, r_watch_, _shortcode_, 5, _price_type_);

    indep_portfolio_->add_unweighted_indicator_listener(0, this);
    is_indep_portfolio_ = true;

  } else {
    ShortcodeSecurityMarketViewMap::StaticCheckValid(_shortcode_);

    indep_market_view_ = (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(_shortcode_));
    if (!indep_market_view_->subscribe_price_type(this, _price_type_)) {
      PriceType_t t_error_price_type_ = _price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << t_error_price_type_ << std::endl;
    }
  }
  /* if (!indep_market_view_.subscribe_price_type(this, _price_type_)) {
     PriceType_t t_error_price_type_ = _price_type_;
     std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
               << " passed " << t_error_price_type_ << std::endl;
   }*/
}

void StableScaledTrend::OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_) {
  if (is_indep_portfolio_) {
    if (!is_ready_ && indep_portfolio_->is_ready()) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!is_ready_ && indep_market_view_->is_ready_complex(2)) {
    is_ready_ = true;
    InitializeValues();
  }
  stable_stdev_value_ = _new_stdev_value_;
  fast_math_multiplier_ = ((1 - inv_decay_sum_) / stable_stdev_value_);
}

void StableScaledTrend::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (_indicator_index_ == 0 && is_indep_portfolio_ == true) {
    current_indep_price_ = _new_value_;
    if (is_ready_ && (!data_interrupted_)) {
      UpdateComputedVariables();
    }
  } else if (_indicator_index_ == 1 && is_indep_portfolio_ == true) {
    if (is_indep_portfolio_) {
      if (!is_ready_ && indep_portfolio_->is_ready()) {
        is_ready_ = true;
        InitializeValues();
      }
    }
    stable_stdev_value_ = _new_value_;
    fast_math_multiplier_ = ((1 - inv_decay_sum_) / stable_stdev_value_);
  }
}

void StableScaledTrend::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (is_indep_portfolio_) return;
  current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);

  if (is_ready_ && (!data_interrupted_)) {
    UpdateComputedVariables();
  }
}

void StableScaledTrend::UpdateComputedVariables() {
  if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
    moving_avg_price_ += (current_indep_price_ - last_price_recorded_) * inv_decay_sum_;
    indicator_value_ = (current_indep_price_ - moving_avg_price_) / stable_stdev_value_;

    // TODO : we can change the normal case of the math from :
    //
    //    moving_avg_price_ += inv_decay_sum_ * ( current_indep_price_ - last_price_recorded_ ) ;
    //    indicator_value_ = ( current_indep_price_ - moving_avg_price_ ) / stable_stdev_value_ ;
    //    last_price_recorded_ = current_indep_price_ ;
    //
    // to
    //
    //    moving_avg_price_ += ( current_indep_price_ - last_price_recorded_ ) * inv_decay_sum_ ;
    //    indicator_value_ += ( current_indep_price_ - last_price_recorded_ ) * fast_math_multiplier_ ;
    //    last_price_recorded_ = current_indep_price_ ;
    //
    // and
    //    fast_math_multiplier_ = ( ( 1 - inv_decay_sum_ ) / stable_stdev_value_ ) ;
    // which can be computed in OnStdevUpdate

  } else {
    int num_pages_to_add_ =
        (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);  ///< will always be > 0
    if (num_pages_to_add_ >= (int)decay_vector_.size()) {
      InitializeValues();
    } else {
      if (num_pages_to_add_ == 1) {
        moving_avg_price_ = (current_indep_price_ * inv_decay_sum_) + (moving_avg_price_ * decay_vector_[1]);
      } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
        moving_avg_price_ = (current_indep_price_ * inv_decay_sum_) +
                            (last_price_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                            (moving_avg_price_ * decay_vector_[num_pages_to_add_]);
      }
      last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
    }
    indicator_value_ = (current_indep_price_ - moving_avg_price_) / stable_stdev_value_;
  }

  last_price_recorded_ = current_indep_price_;

  /*if(data_interrupted_)
    indicator_value_ = 0;*/

  NotifyIndicatorListeners(indicator_value_);
}

void StableScaledTrend::InitializeValues() {
  moving_avg_price_ = current_indep_price_;
  last_price_recorded_ = current_indep_price_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}

// market_interrupt_listener interface
void StableScaledTrend::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (!is_indep_portfolio_ && indep_market_view_->security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void StableScaledTrend::OnMarketDataResumed(const unsigned int _security_id_) {
  if (!is_indep_portfolio_ && indep_market_view_->security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  }
}
}
