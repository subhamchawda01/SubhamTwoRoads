/**
    \file IndicatorsCode/stdev_l1_bias.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/stdev_l1_bias.hpp"

namespace HFSAT {

void StdevL1Bias::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                    std::vector<std::string>& _ors_source_needed_vec_,
                                    const std::vector<const char*>& r_tokens_) {
  IndicatorUtil::CollectShortcodeOrPortfolio(_shortcodes_affecting_this_indicator_, _ors_source_needed_vec_, r_tokens_);
}

StdevL1Bias* StdevL1Bias::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_->_fractional_seconds_ _price_type_
  // ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_, r_tokens_[3], atof(r_tokens_[4]), StringToPriceType_t(r_tokens_[5]));
}

StdevL1Bias* StdevL1Bias::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_, std::string _shortcode_,
                                            double _fractional_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;

  t_temp_oss_ << VarName() << ' ' << _shortcode_ << ' ' << _fractional_seconds_ << ' '
              << PriceType_t_To_String(_price_type_);

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, StdevL1Bias*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new StdevL1Bias(
        t_dbglogger_, r_watch_, concise_indicator_description_, _shortcode_, _fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

StdevL1Bias::StdevL1Bias(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                         const std::string& concise_indicator_description_, std::string _shortcode_,
                         double _fractional_seconds_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      shortcode_(_shortcode_),
      price_type_(_price_type_){
  trend_history_msecs_ = std::max(20, (int)round(1000 * _fractional_seconds_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();
  ShortcodeSecurityMarketViewMap::StaticCheckValid(_shortcode_);
  indep_market_view_ = (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(_shortcode_));
  moving_avg_bias_ = 0.5;
  moving_avg_bias_square_ = moving_avg_bias_ * moving_avg_bias_;
  last_bias_recorded_ = moving_avg_bias_;
  current_bias_ = moving_avg_bias_;
  stdev_ = 0.01 * moving_avg_bias_;
  if (!indep_market_view_->subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
}

void StdevL1Bias::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_->is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_->secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void StdevL1Bias::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  current_bias_ =
      (SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_) - indep_market_view_->bestbid_price()) /
      indep_market_view_->min_price_increment_;

  if (!is_ready_) {
    if (indep_market_view_->is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      moving_avg_bias_ += inv_decay_sum_ * (current_bias_ - last_bias_recorded_);
      moving_avg_bias_square_ +=
          inv_decay_sum_ * (current_bias_ * current_bias_ - last_bias_recorded_ * last_bias_recorded_);
    } else {
      int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {
          moving_avg_bias_ = (current_bias_ * inv_decay_sum_) + (moving_avg_bias_ * decay_vector_[1]);
          moving_avg_bias_square_ =
              (current_bias_ * current_bias_ * inv_decay_sum_) + (moving_avg_bias_square_ * decay_vector_[1]);
        } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
          moving_avg_bias_ = (current_bias_ * inv_decay_sum_) +
                             (last_bias_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                             (moving_avg_bias_ * decay_vector_[num_pages_to_add_]);
          moving_avg_bias_square_ = (current_bias_ * current_bias_ * inv_decay_sum_) +
                                    (last_bias_recorded_ * last_bias_recorded_ * inv_decay_sum_ *
                                     decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                                    (moving_avg_bias_square_ * decay_vector_[num_pages_to_add_]);
        }
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }
    }
    last_bias_recorded_ = current_bias_;
  }

  stdev_ = std::max(
      std::sqrt(moving_avg_bias_square_ - moving_avg_bias_ * moving_avg_bias_), 0.01);
  indicator_value_ = stdev_;
  NotifyIndicatorListeners(indicator_value_);
}

void StdevL1Bias::InitializeValues() {
  moving_avg_bias_ = 0.5;
  moving_avg_bias_square_ = moving_avg_bias_ * moving_avg_bias_;
  current_bias_ = moving_avg_bias_;
  last_bias_recorded_ = current_bias_;
  stdev_ = 0.01 * moving_avg_bias_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = stdev_;
}

// market_interrupt_listener interface
void StdevL1Bias::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_->security_id() == _security_id_) {
    data_interrupted_ = true;
  }
}

void StdevL1Bias::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_->security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  }
}
}
