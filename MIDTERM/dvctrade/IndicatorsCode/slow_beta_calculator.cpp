/**
    \file IndicatorsCode/slow_beta_calculator.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/slow_beta_calculator.hpp"

namespace HFSAT {

void SlowBetaCalculator::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                           std::vector<std::string>& _ors_source_needed_vec_,
                                           const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

SlowBetaCalculator* SlowBetaCalculator::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                          const std::vector<const char*>& r_tokens_,
                                                          PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_  _indep_market_view_ _msecs_
  return GetUniqueInstance(
      t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
      *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])), atoi(r_tokens_[5]));
}

SlowBetaCalculator* SlowBetaCalculator::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                          const SecurityMarketView& _dep_market_view_,
                                                          const SecurityMarketView& _indep_market_view_,
                                                          const unsigned int t_trend_history_secs_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep_market_view_.secname() << ' '
              << t_trend_history_secs_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, SlowBetaCalculator*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new SlowBetaCalculator(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                               _indep_market_view_, t_trend_history_secs_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

SlowBetaCalculator::SlowBetaCalculator(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                       const std::string& concise_indicator_description_,
                                       const SecurityMarketView& _dep_market_view_,
                                       const SecurityMarketView& _indep_market_view_,
                                       const unsigned int t_trend_history_secs_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_market_view_(_indep_market_view_),
      moving_dep_avg_price_(0),
      moving_dep_avg_squared_price_(0),
      moving_indep_avg_price_(0),
      moving_indep_avg_squared_price_(0),
      moving_dep_indep_avg_price_(0),
      last_dep_price_recorded_(0),
      last_indep_price_recorded_(0),
      last_dep_indep_price_recoreded_(0),
      beta_value_(0),
      current_dep_price_(0),
      current_indep_price_(0),
      current_dep_indep_price_(0),
      min_dep_unbiased_l2_norm_(
          _dep_market_view_.min_price_increment() *
          _dep_market_view_.min_price_increment()),  // TODO : to be updated dependng on volatility of product
      min_indep_unbiased_l2_norm_(
          _indep_market_view_.min_price_increment() *
          _indep_market_view_.min_price_increment()),  // TODO : to be updated dependng on volatility of product
      slow_beta_calculator_listener_ptr_vec_() {
  trend_history_msecs_ = std::max(1000, (int)round(1000 * t_trend_history_secs_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();
  dep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  indep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
}

void SlowBetaCalculator::OnMarketUpdate(const unsigned int _security_id_,
                                        const MarketUpdateInfo& _market_update_info_) {
  current_indep_price_ = indep_market_view_.mkt_size_weighted_price();
  current_dep_price_ = dep_market_view_.mkt_size_weighted_price();
  current_dep_indep_price_ = current_dep_price_ * current_indep_price_;

  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {  // maintain expwmov_avg of this price_type
    // maintain expwmov_beta of this price_type
    // typically value is ( current price - movavg ) / movbeta unless movbeta is less than a min price increment

    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      moving_dep_avg_price_ += inv_decay_sum_ * (current_dep_price_ - last_dep_price_recorded_);
      moving_indep_avg_price_ += inv_decay_sum_ * (current_indep_price_ - last_indep_price_recorded_);
      moving_dep_indep_avg_price_ += inv_decay_sum_ * (current_dep_indep_price_ - last_dep_indep_price_recoreded_);
      moving_dep_avg_squared_price_ += inv_decay_sum_ * ((current_dep_price_ * current_dep_price_) -
                                                         (last_dep_price_recorded_ * last_dep_price_recorded_));
      moving_indep_avg_squared_price_ += inv_decay_sum_ * ((current_indep_price_ * current_indep_price_) -
                                                           (last_indep_price_recorded_ * last_indep_price_recorded_));
      last_dep_price_recorded_ = current_dep_price_;
      last_indep_price_recorded_ = current_indep_price_;
      last_dep_indep_price_recoreded_ = current_dep_indep_price_;
    } else {  // new page(s)
      int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {
          moving_indep_avg_price_ =
              (current_indep_price_ * inv_decay_sum_) + (moving_indep_avg_price_ * decay_page_factor_);
          moving_dep_avg_price_ = (current_dep_price_ * inv_decay_sum_) + (moving_dep_avg_price_ * decay_page_factor_);
          moving_dep_indep_avg_price_ =
              (current_dep_indep_price_ * inv_decay_sum_) + (moving_dep_indep_avg_price_ * decay_page_factor_);
          moving_indep_avg_squared_price_ = (current_indep_price_ * current_indep_price_ * inv_decay_sum_) +
                                            (moving_indep_avg_squared_price_ * decay_page_factor_);
          moving_dep_avg_squared_price_ = (current_dep_price_ * current_dep_price_ * inv_decay_sum_) +
                                          (moving_dep_avg_squared_price_ * decay_page_factor_);
        } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
          moving_indep_avg_price_ =
              (current_indep_price_ * inv_decay_sum_) +
              (last_indep_price_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
              (moving_indep_avg_price_ * decay_vector_[num_pages_to_add_]);
          moving_dep_avg_price_ =
              (current_dep_price_ * inv_decay_sum_) +
              (last_dep_price_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
              (moving_dep_avg_price_ * decay_vector_[num_pages_to_add_]);
          moving_dep_indep_avg_price_ =
              (current_dep_indep_price_ * inv_decay_sum_) +
              (last_dep_indep_price_recoreded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
              (moving_dep_indep_avg_price_ * decay_vector_[num_pages_to_add_]);
          moving_indep_avg_squared_price_ = (current_indep_price_ * current_indep_price_ * inv_decay_sum_) +
                                            (last_indep_price_recorded_ * last_indep_price_recorded_ * inv_decay_sum_ *
                                             decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                                            (moving_indep_avg_squared_price_ * decay_vector_[num_pages_to_add_]);
          moving_dep_avg_squared_price_ = (current_dep_price_ * current_dep_price_ * inv_decay_sum_) +
                                          (last_dep_price_recorded_ * last_dep_price_recorded_ * inv_decay_sum_ *
                                           decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                                          (moving_dep_avg_squared_price_ * decay_vector_[num_pages_to_add_]);
        }
        last_dep_price_recorded_ = current_dep_price_;
        last_indep_price_recorded_ = current_indep_price_;
        last_dep_indep_price_recoreded_ = current_dep_indep_price_;
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }

      // only changing beta value on a new page ... can be done all the time, but saving computation
      // double unbiased_dep_l2_norm_ = std::max ( min_dep_unbiased_l2_norm_, ( moving_dep_avg_squared_price_ - (
      // moving_dep_avg_price_ * moving_dep_avg_price_ ) ) ) ;
      double unbiased_indep_l2_norm_ =
          std::max(min_indep_unbiased_l2_norm_,
                   (moving_indep_avg_squared_price_ - (moving_indep_avg_price_ * moving_indep_avg_price_)));
      beta_value_ =
          (moving_dep_indep_avg_price_ - moving_indep_avg_price_ * moving_dep_avg_price_) / unbiased_indep_l2_norm_;

      // broadcast to listeners
      for (auto i = 0u; i < slow_beta_calculator_listener_ptr_vec_.size(); i++) {
        slow_beta_calculator_listener_ptr_vec_[i]->OnBetaUpdate(indep_market_view_.security_id(), beta_value_);
      }
      indicator_value_ = beta_value_;
    }
    NotifyIndicatorListeners(indicator_value_);
  }
}

void SlowBetaCalculator::InitializeValues() {
  moving_dep_avg_price_ = current_dep_price_;
  moving_dep_avg_squared_price_ = current_dep_price_ * current_dep_price_;
  last_dep_price_recorded_ = current_dep_price_;

  moving_indep_avg_price_ = current_indep_price_;
  moving_indep_avg_squared_price_ = current_indep_price_ * current_indep_price_;
  last_indep_price_recorded_ = current_indep_price_;

  moving_dep_indep_avg_price_ = current_indep_price_ * current_dep_price_;
  last_dep_indep_price_recoreded_ = current_dep_price_ * current_indep_price_;

  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}
void SlowBetaCalculator::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                 const int msecs_since_last_receive_) {
  data_interrupted_ = true;
}

void SlowBetaCalculator::OnMarketDataResumed(const unsigned int _security_id_) {
  InitializeValues();
  data_interrupted_ = false;
}
}
