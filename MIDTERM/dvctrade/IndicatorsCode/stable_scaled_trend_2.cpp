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
#include "dvctrade/Indicators/stable_scaled_trend_2.hpp"

namespace HFSAT {

void StableScaledTrend2::CollectShortCodes(std::vector<std::string>& t_shortcodes_affecting_this_indicator_,
                                           std::vector<std::string>& _ors_source_needed_vec_,
                                           const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(t_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

StableScaledTrend2* StableScaledTrend2::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                          const std::vector<const char*>& r_tokens_,
                                                          PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ t_indep_market_view_ _fractional_seconds_ _price_type_
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), StringToPriceType_t(r_tokens_[5]));
}

StableScaledTrend2* StableScaledTrend2::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                          SecurityMarketView& t_indep_market_view_,
                                                          double _fractional_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_indep_market_view_.secname() << ' ' << _fractional_seconds_ << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, StableScaledTrend2*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new StableScaledTrend2(t_dbglogger_, r_watch_, concise_indicator_description_, t_indep_market_view_,
                               _fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

StableScaledTrend2::StableScaledTrend2(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                       const std::string& concise_indicator_description_,
                                       SecurityMarketView& t_indep_market_view_, double _fractional_seconds_,
                                       PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(t_indep_market_view_),
      price_type_(_price_type_),
      stable_stdev_value_(1),
      moving_avg_price_(0),
      last_price_recorded_(0),
      current_indep_price_(0) {
  trend_history_msecs_ = std::max(MIN_MSEC_HISTORY_INDICATORS, (int)round(1000 * _fractional_seconds_));
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
  stable_stdev_value_ = PcaWeightsManager::GetUniqueInstance().GetShortcodeStdevs(indep_market_view_.shortcode());
}

void StableScaledTrend2::OnMarketUpdate(const unsigned int _security_id_,
                                        const MarketUpdateInfo& _market_update_info_) {
  current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  }

  else {
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
      int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) /
                                         page_width_msecs_);  ///< will always be > 0
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
}

void StableScaledTrend2::InitializeValues() {
  moving_avg_price_ = current_indep_price_;
  last_price_recorded_ = current_indep_price_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}

// market_interrupt_listener interface
void StableScaledTrend2::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                 const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void StableScaledTrend2::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  }
}
}
