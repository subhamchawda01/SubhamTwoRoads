/**
    \file IndicatorsCode/simple_trend.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/adaptive_moving_average.hpp"

namespace HFSAT {

void AdaptiveMovingAverage::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                              std::vector<std::string>& _ors_source_needed_vec_,
                                              const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

AdaptiveMovingAverage* AdaptiveMovingAverage::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                const std::vector<const char*>& r_tokens_,
                                                                PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_ _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), StringToPriceType_t(r_tokens_[5]));
}

AdaptiveMovingAverage* AdaptiveMovingAverage::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                const SecurityMarketView& _indep_market_view_,
                                                                double _fractional_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _fractional_seconds_ << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, AdaptiveMovingAverage*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new AdaptiveMovingAverage(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_,
                                  _fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

AdaptiveMovingAverage::AdaptiveMovingAverage(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                             const std::string& concise_indicator_description_,
                                             const SecurityMarketView& _indep_market_view_, double _fractional_seconds_,
                                             PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      price_type_(_price_type_),
      moving_avg_price_(0),
      last_price_recorded_(0),
      current_indep_price_(0) {
  trend_history_msecs_ = std::max(20, (int)round(1000 * _fractional_seconds_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();
  if (!indep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
  slow_stdev_ =
      SlowStdevCalculator::GetUniqueInstance(t_dbglogger_, r_watch_, indep_market_view_.shortcode(), 300000u, .01);
  if (slow_stdev_ == NULL) {
    indicator_value_ = 0;
    is_ready_ = true;
    return;
  }
  slow_stdev_->add_unweighted_indicator_listener(1u, this);
}

void AdaptiveMovingAverage::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void AdaptiveMovingAverage::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (!is_ready_) {
    InitializeHist();
    is_ready_ = true;
    indicator_value_ = 0;
  } else {
    slow_stdev_value_ = _new_value_;
    if (hist_stdev_ > 0) {
      factor_ = std::max(0.2, std::min(5.0, slow_stdev_value_ / hist_stdev_));
    } else {
      factor_ = 1;
    }
  }
}

void AdaptiveMovingAverage::OnMarketUpdate(const unsigned int _security_id_,
                                           const MarketUpdateInfo& _market_update_info_) {
  current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);

  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
      InitializeHist();
    }
  } else if (!data_interrupted_) {
    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      moving_avg_price_ += factor_ * inv_decay_sum_ * (current_indep_price_ - last_price_recorded_);
    } else {
      int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {
          moving_avg_price_ =
              (current_indep_price_ * factor_ * inv_decay_sum_) + (moving_avg_price_ * (1 - factor_ * inv_decay_sum_));
        } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
          moving_avg_price_ =
              (current_indep_price_ * factor_ * inv_decay_sum_) +
              (last_price_recorded_ * factor_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
              (moving_avg_price_ *
               (1 - factor_ * inv_decay_sum_ - factor_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]));
        }
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }
    }

    last_price_recorded_ = current_indep_price_;
    indicator_value_ = moving_avg_price_;

    // if ( data_interrupted_ )
    //  { // not sure we will need this ... since by definition data is already iterrupted
    //    indicator_value_ = 0;
    //  }

    NotifyIndicatorListeners(indicator_value_);
  }
}

void AdaptiveMovingAverage::InitializeValues() {
  moving_avg_price_ = current_indep_price_;
  last_price_recorded_ = current_indep_price_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = current_indep_price_;
}

// market_interrupt_listener interface
void AdaptiveMovingAverage::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                    const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = current_indep_price_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void AdaptiveMovingAverage::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  }
}
void AdaptiveMovingAverage::InitializeHist() {
  hist_stdev_ = HFSAT::SampleDataUtil::GetAvgForPeriod(indep_market_view_.shortcode(), watch_.YYYYMMDD(), 60,
                                                       trading_start_mfm_, trading_end_mfm_, "STDEV");
  slow_stdev_value_ = hist_stdev_;
}
}
