/**
    \file IndicatorsCode/slow_stdev_calculator_port.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator_port.hpp"

namespace HFSAT {
void SlowStdevCalculatorPort::CollectShortCodes(std::vector<std::string>& t_shortcodes_affecting_this_indicator_,
                                                std::vector<std::string>& _ors_source_needed_vec_,
                                                const std::vector<const char*>& r_tokens_) {
  IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[3], t_shortcodes_affecting_this_indicator_);
}

SlowStdevCalculatorPort* SlowStdevCalculatorPort::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                    const std::vector<const char*>& r_tokens_,
                                                                    PriceType_t _basepx_pxtype_) {
  double t_min_stdev_value_factor_ = 1.0;
  double t_trend_history_msecs_ = 200 * 1000u;
  if (r_tokens_.size() > 4) {
    t_trend_history_msecs_ = atoi(r_tokens_[4]) * 1000u;
    if (r_tokens_.size() > 5) {
      t_min_stdev_value_factor_ = atof(r_tokens_[5]);
    }
  }

  return GetUniqueInstance(t_dbglogger_, r_watch_, (std::string)r_tokens_[3], t_trend_history_msecs_,
                           t_min_stdev_value_factor_);
}

SlowStdevCalculatorPort* SlowStdevCalculatorPort::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                    const std::string& _portfolio_descriptor_shortcode_,
                                                                    const unsigned int t_trend_history_msecs_,
                                                                    double t_min_stdev_value_factor_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _portfolio_descriptor_shortcode_ << ' ' << t_trend_history_msecs_ << ' '
              << t_min_stdev_value_factor_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, SlowStdevCalculatorPort*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new SlowStdevCalculatorPort(
        t_dbglogger_, r_watch_, concise_indicator_description_, _portfolio_descriptor_shortcode_,
        t_trend_history_msecs_, t_min_stdev_value_factor_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

SlowStdevCalculatorPort::SlowStdevCalculatorPort(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                 const std::string& concise_indicator_description_,
                                                 const std::string& _portfolio_descriptor_shortcode_,
                                                 const unsigned int t_trend_history_msecs_,
                                                 double t_min_stdev_value_factor_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_portfolio_price__(PCAPortPrice::GetUniqueInstance(t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_,
                                                              kPriceTypeMktSizeWPrice)),
      moving_avg_price_(0),
      last_price_recorded_(0),
      stdev_value_(0),
      current_indep_price_(0),
      min_unbiased_l2_norm_(1),
      slow_stdev_calculator_listener_ptr_vec_() {
  min_unbiased_l2_norm_ = t_min_stdev_value_factor_ * t_min_stdev_value_factor_ *
                          indep_portfolio_price__->min_price_increment() *
                          indep_portfolio_price__->min_price_increment();
  trend_history_msecs_ = std::max(100u, t_trend_history_msecs_);
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();

  indep_portfolio_price__->AddPriceChangeListener(this);
}

void SlowStdevCalculatorPort::OnPortfolioPriceChange(double _new_price_) {
  current_indep_price_ = _new_price_;

  if (!is_ready_) {
    is_ready_ = true;
    InitializeValues();
  } else if (!data_interrupted_) {  // maintain expwmov_avg of this price_type
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

      // broadcast to listeners
      for (auto i = 0u; i < slow_stdev_calculator_listener_ptr_vec_.size(); i++) {
        slow_stdev_calculator_listener_ptr_vec_[i]->OnStdevUpdate(stdev_value_);
      }

      indicator_value_ = stdev_value_;

      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void SlowStdevCalculatorPort::OnPortfolioPriceReset(double t_new_price_, double t_old_price_,
                                                    unsigned int is_data_interrupted_) {
  if (is_data_interrupted_ == 1u) {
    data_interrupted_ = true;
    NotifyIndicatorListeners(indicator_value_);
  } else if (is_data_interrupted_ == 2u) {
    InitializeValues();
    data_interrupted_ = false;
  } else if (is_data_interrupted_ == 0u) {
    // TODO

    // current_indep_price_ = t_new_price_ ; // = current_indep_price_ + ( t_new_price_ - t_old_price_ )
    // double delta_ = t_old_price_ - t_new_price_ ;
    // moving_avg_price_         -= delta_ ;
    // moving_avg_squared_price_ -= delta_ * ( delta_ + 2 * moving_avg_price_ );

    // double unadjusted_trend_ = current_indep_price_ - moving_avg_price_ ;
    // double unbiased_l2_norm_ = std::max ( min_unbiased_l2_norm_ , ( moving_avg_squared_price_ - ( moving_avg_price_ *
    // moving_avg_price_ ) ) ) ;
    // stdev_value_ = sqrt ( unbiased_l2_norm_ ) ;
    // indicator_value_ = ( unadjusted_trend_ / stdev_value_ );
    // last_price_recorded_ = current_indep_price_ ;
  }
}

void SlowStdevCalculatorPort::InitializeValues() {
  moving_avg_price_ = current_indep_price_;
  moving_avg_squared_price_ = current_indep_price_ * current_indep_price_;
  last_price_recorded_ = current_indep_price_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}
void SlowStdevCalculatorPort::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                      const int msecs_since_last_receive_) {
  data_interrupted_ = true;
}

void SlowStdevCalculatorPort::OnMarketDataResumed(const unsigned int _security_id_) {
  InitializeValues();
  data_interrupted_ = false;
}
}
