/**
    \file IndicatorsCode/scaled_trend_port.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/scaled_trend_port.hpp"

namespace HFSAT {

void ScaledTrendPort::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                        std::vector<std::string>& _ors_source_needed_vec_,
                                        const std::vector<const char*>& r_tokens_) {
  IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[3], _shortcodes_affecting_this_indicator_);
}

ScaledTrendPort* ScaledTrendPort::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                    const std::vector<const char*>& r_tokens_,
                                                    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _portfolio_descriptor_shortcode_ _num_levels_ _decay_factor_
  return GetUniqueInstance(t_dbglogger_, r_watch_, (std::string)r_tokens_[3], atof(r_tokens_[4]),
                           StringToPriceType_t(r_tokens_[5]));
}

ScaledTrendPort* ScaledTrendPort::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                    std::string _portfolio_descriptor_shortcode_,
                                                    double _fractional_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _portfolio_descriptor_shortcode_ << ' ' << _fractional_seconds_ << ' '
              << PriceType_t_To_String(_price_type_);

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, ScaledTrendPort*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new ScaledTrendPort(t_dbglogger_, r_watch_, concise_indicator_description_, _portfolio_descriptor_shortcode_,
                            _fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

ScaledTrendPort::ScaledTrendPort(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                 const std::string& concise_indicator_description_,
                                 std::string _portfolio_descriptor_shortcode_, double _fractional_seconds_,
                                 PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_portfolio_price__(
          PCAPortPrice::GetUniqueInstance(t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, _price_type_)),
      price_type_(_price_type_),
      moving_avg_price_(0),
      last_price_recorded_(0),
      stdev_value_(0),
      current_indep_price_(0),
      min_unbiased_l2_norm_(1) {
  min_unbiased_l2_norm_ =
      indep_portfolio_price__->min_price_increment() * indep_portfolio_price__->min_price_increment();
  trend_history_msecs_ = std::max(MIN_MSEC_HISTORY_INDICATORS, (int)round(1000 * _fractional_seconds_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();

  indep_portfolio_price__->AddPriceChangeListener(this);
}

void ScaledTrendPort::WhyNotReady() {
  if (!is_ready_) {
    if (!indep_portfolio_price__->is_ready()) {
      indep_portfolio_price__->WhyNotReady();
    }
  }
}

void ScaledTrendPort::OnPortfolioPriceChange(double _new_price_) {
  current_indep_price_ = _new_price_;

  if (!is_ready_) {
    is_ready_ = true;
    InitializeValues();
  } else {  // typically value is [ ( current price - movavg ) / movstdev ] unless movstdev is less than a min price
            // increment

    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      moving_avg_price_ += inv_decay_sum_ * (current_indep_price_ - last_price_recorded_);
      moving_avg_squared_price_ += inv_decay_sum_ * ((current_indep_price_ * current_indep_price_) -
                                                     (last_price_recorded_ * last_price_recorded_));
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
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }

      // only changing stdev value on a new page ... can be done all the time, but saving computation
      double unbiased_l2_norm_ =
          std::max(min_unbiased_l2_norm_, (moving_avg_squared_price_ - (moving_avg_price_ * moving_avg_price_)));
      stdev_value_ = sqrt(unbiased_l2_norm_);
    }

    last_price_recorded_ = current_indep_price_;

    double unadjusted_trend_ = (current_indep_price_ - moving_avg_price_);
    indicator_value_ = (unadjusted_trend_ / stdev_value_);

    if (data_interrupted_) indicator_value_ = 0;

    NotifyIndicatorListeners(indicator_value_);
  }
}

void ScaledTrendPort::OnPortfolioPriceReset(double t_new_price_, double t_old_price_,
                                            unsigned int is_data_interrupted_) {
  if (is_data_interrupted_ == 1u) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else if (is_data_interrupted_ == 2u) {
    InitializeValues();
    data_interrupted_ = false;
  } else if (is_data_interrupted_ == 0u) {
    current_indep_price_ = t_new_price_;  // = current_indep_price_ + ( t_new_price_ - t_old_price_ )
    double delta_ = t_old_price_ - t_new_price_;
    moving_avg_price_ -= delta_;
    moving_avg_squared_price_ -= delta_ * (delta_ + 2 * moving_avg_price_);

    double unadjusted_trend_ = current_indep_price_ - moving_avg_price_;
    double unbiased_l2_norm_ =
        std::max(min_unbiased_l2_norm_, (moving_avg_squared_price_ - (moving_avg_price_ * moving_avg_price_)));
    stdev_value_ = sqrt(unbiased_l2_norm_);
    indicator_value_ = (unadjusted_trend_ / stdev_value_);
    last_price_recorded_ = current_indep_price_;
  }
}

void ScaledTrendPort::InitializeValues() {
  moving_avg_price_ = current_indep_price_;
  moving_avg_squared_price_ = current_indep_price_ * current_indep_price_;
  last_price_recorded_ = current_indep_price_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  stdev_value_ = sqrt(min_unbiased_l2_norm_);
  indicator_value_ = 0;
}
void ScaledTrendPort::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void ScaledTrendPort::OnMarketDataResumed(const unsigned int _security_id_) {
  InitializeValues();
  data_interrupted_ = false;
}
}
