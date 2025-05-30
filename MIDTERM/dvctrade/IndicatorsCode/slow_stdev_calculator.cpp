/**
    \file IndicatorsCode/slow_stdev_calculator.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator.hpp"

namespace HFSAT {
void SlowStdevCalculator::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                            std::vector<std::string>& _ors_source_needed_vec_,
                                            const std::vector<const char*>& r_tokens_) {
  IndicatorUtil::CollectShortcodeOrPortfolio(_shortcodes_affecting_this_indicator_, _ors_source_needed_vec_, r_tokens_);
}

SlowStdevCalculator* SlowStdevCalculator::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                            const std::vector<const char*>& r_tokens_,
                                                            PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_  _fractional_seconds_ _min_stdev_value_factor_
  double t_min_stdev_value_factor_ = 1.0;
  if (r_tokens_.size() > 5) {
    t_min_stdev_value_factor_ = atof(r_tokens_[5]);
  }
  return GetUniqueInstance(t_dbglogger_, r_watch_, r_tokens_[3], atoi(r_tokens_[4]) * 1000u, t_min_stdev_value_factor_);
}

SlowStdevCalculator* SlowStdevCalculator::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                            const std::string& shortcode,
                                                            const unsigned int t_trend_history_msecs_,
                                                            double t_min_stdev_value_factor_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << shortcode << ' ' << t_trend_history_msecs_ << ' ' << t_min_stdev_value_factor_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  // static std::map<std::string, SlowStdevCalculator*> concise_indicator_description_map_;
  if (global_concise_indicator_description_map_.find(concise_indicator_description_) ==
      global_concise_indicator_description_map_.end()) {
    global_concise_indicator_description_map_[concise_indicator_description_] =
        new SlowStdevCalculator(t_dbglogger_, r_watch_, concise_indicator_description_, shortcode,
                                t_trend_history_msecs_, t_min_stdev_value_factor_);
  }
  return dynamic_cast<SlowStdevCalculator*>(global_concise_indicator_description_map_[concise_indicator_description_]);
}

SlowStdevCalculator::SlowStdevCalculator(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                         const std::string& concise_indicator_description_,
                                         const std::string& shortcode, const unsigned int t_trend_history_msecs_,
                                         const double t_min_stdev_value_factor)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(nullptr),
      shortcode_(shortcode),
      moving_avg_price_(0),
      moving_avg_squared_price_(0),
      last_price_recorded_(0),
      stdev_value_(0),
      current_indep_price_(0),
      min_unbiased_l2_norm_(0.0001),  // TODO : to be updated dependng on volatility of product
      is_price_portfolio_(false),
      slow_stdev_calculator_listener_ptr_vec_() {
  trend_history_msecs_ = std::max(100u, t_trend_history_msecs_);
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();
  price_type_ = kPriceTypeMktSizeWPrice;
  if (ShortcodeSecurityMarketViewMap::StaticCheckValidPortWithoutExit(shortcode)) {
    // this is single security
    double min_price_increment = IndicatorUtil::GetMinPriceIncrementForPricePortfolio(shortcode, r_watch_.YYYYMMDD());
    min_unbiased_l2_norm_ =
        min_price_increment * min_price_increment * t_min_stdev_value_factor * t_min_stdev_value_factor;

    price_portfolio_ = PricePortfolio::GetUniqueInstance(t_dbglogger_, r_watch_, shortcode, 5, price_type_);

    price_portfolio_->add_unweighted_indicator_listener(0, this);
    is_price_portfolio_ = true;

  } else {
    indep_market_view_ = ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(shortcode);
    min_unbiased_l2_norm_ = indep_market_view_->min_price_increment() * indep_market_view_->min_price_increment() *
                            t_min_stdev_value_factor * t_min_stdev_value_factor;
    indep_market_view_->subscribe_price_type(this, price_type_);
  }
}

void SlowStdevCalculator::OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value) {
  if (is_price_portfolio_ && indicator_index == 0) {
    // update the price irrespective of is_ready
    current_indep_price_ = new_value;

    if (!is_ready_) {
      if (price_portfolio_->is_ready()) {
        is_ready_ = true;
        InitializeValues();
      }
    } else {
      UpdateComputedVariables();
    }
  }
}

void SlowStdevCalculator::OnMarketUpdate(const unsigned int _security_id_,
                                         const MarketUpdateInfo& _market_update_info_) {
  if (is_price_portfolio_) {
    return;
  } else {
    current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  }

  if (!is_ready_) {
    if (indep_market_view_->is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    UpdateComputedVariables();
  }
}

void SlowStdevCalculator::UpdateComputedVariables() {
  // maintain expwmov_avg of this price_type
  // maintain expwmov_stdev of this price_type
  // typically value is ( current price - movavg ) / movstdev unless movstdev is less than a min price increment

  if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
    moving_avg_price_ += inv_decay_sum_ * (current_indep_price_ - last_price_recorded_);
    moving_avg_squared_price_ += inv_decay_sum_ * ((current_indep_price_ * current_indep_price_) -
                                                   (last_price_recorded_ * last_price_recorded_));
    last_price_recorded_ = current_indep_price_;
  } else {  // new page(s)
    int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
    if (num_pages_to_add_ >= (int)decay_vector_.size()) {
      InitializeValues();
    } else {
      if (num_pages_to_add_ == 1) {
        moving_avg_price_ = (current_indep_price_ * inv_decay_sum_) + (moving_avg_price_ * decay_page_factor_);
        moving_avg_squared_price_ = (current_indep_price_ * current_indep_price_ * inv_decay_sum_) +
                                    (moving_avg_squared_price_ * decay_page_factor_);
      } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
        moving_avg_price_ = (current_indep_price_ * inv_decay_sum_) +
                            (last_price_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                            (moving_avg_price_ * decay_vector_[num_pages_to_add_]);
        moving_avg_squared_price_ = (current_indep_price_ * current_indep_price_ * inv_decay_sum_) +
                                    (last_price_recorded_ * last_price_recorded_ * inv_decay_sum_ *
                                     decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                                    (moving_avg_squared_price_ * decay_vector_[num_pages_to_add_]);
      }
      last_price_recorded_ = current_indep_price_;
      last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
    }

    // only changing stdev value on a new page ... can be done all the time, but saving computation
    double unbiased_l2_norm_ =
        std::max(min_unbiased_l2_norm_, (moving_avg_squared_price_ - (moving_avg_price_ * moving_avg_price_)));
    stdev_value_ = sqrt(unbiased_l2_norm_);
    // stdev_value_ = moving_avg_squared_price_ - ( moving_avg_price_ * moving_avg_price_ ); //DEBUG

    if (!is_price_portfolio_) {
      // broadcast to listeners
      for (auto i = 0u; i < slow_stdev_calculator_listener_ptr_vec_.size(); i++) {
        slow_stdev_calculator_listener_ptr_vec_[i]->OnStdevUpdate(indep_market_view_->security_id(), stdev_value_);
      }
    }

    indicator_value_ = stdev_value_;
    NotifyIndicatorListeners(indicator_value_);
  }
}
void SlowStdevCalculator::InitializeValues() {
  moving_avg_price_ = current_indep_price_;
  moving_avg_squared_price_ = current_indep_price_ * current_indep_price_;
  last_price_recorded_ = current_indep_price_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}
void SlowStdevCalculator::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                  const int msecs_since_last_receive_) {
  if (!is_price_portfolio_) data_interrupted_ = true;
}

void SlowStdevCalculator::OnMarketDataResumed(const unsigned int _security_id_) {
  InitializeValues();
  data_interrupted_ = false;
}
}
