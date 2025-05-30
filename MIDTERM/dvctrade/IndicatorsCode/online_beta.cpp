/**
   \file IndicatorsCode/moving_covariance_cutoff.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 351, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/online_beta.hpp"

namespace HFSAT {
void OnlineBeta::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                   std::vector<std::string>& _ors_source_needed_vec_,
                                   const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

OnlineBeta* OnlineBeta::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                          const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_  _indep_market_view_ t_trend_history_secs_
  // _t_price_type_
  if (r_tokens_.size() < 7) {
    std::cerr << "insufficient arguments to INDICATOR OnlineBeta, currect syntax : _this_weight_ _indicator_string_ "
                 "_dep_market_view_  _indep_market_view_ t_trend_history_secs_ _t_price_type_\n";
    exit(1);
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           atoi(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
}

OnlineBeta* OnlineBeta::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                          const SecurityMarketView& _dep_market_view_,
                                          const SecurityMarketView& _indep_market_view_,
                                          const unsigned int t_trend_history_secs_, PriceType_t _t_price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep_market_view_.secname() << ' '
              << t_trend_history_secs_ << ' ' << _t_price_type_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OnlineBeta*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new OnlineBeta(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _indep_market_view_,
                       t_trend_history_secs_, _t_price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OnlineBeta::OnlineBeta(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                       const std::string& concise_indicator_description_, const SecurityMarketView& _dep_market_view_,
                       const SecurityMarketView& _indep_market_view_, const unsigned int t_trend_history_secs_,
                       PriceType_t _t_price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_market_view_(_indep_market_view_),
      price_type_(_t_price_type_),
      moving_covariance_(0),
      dep_slow_stdev_calculator_(*(SlowStdevCalculator::GetUniqueInstance(
          t_dbglogger_, r_watch_, _dep_market_view_.shortcode(), t_trend_history_secs_ * 1000u))),
      indep_slow_stdev_calculator_(*(SlowStdevCalculator::GetUniqueInstance(
          t_dbglogger_, r_watch_, _indep_market_view_.shortcode(), t_trend_history_secs_ * 1000u))),
      dep_std_dev_(1.00),
      indep_std_dev_(1.00),
      dep_updated_(false),
      indep_updated_(false),
      last_dep_price_recorded_(0),
      last_indep_price_recorded_(0),
      current_dep_price_(0),
      current_indep_price_(0),
      dep_moving_avg_price_(0),
      indep_moving_avg_price_(0),
      dep_indep_moving_avg_price_(0) {
  trend_history_msecs_ = std::max(100u, 1000u * t_trend_history_secs_);
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();
  dep_slow_stdev_calculator_.AddSlowStdevCalculatorListener(this);
  indep_slow_stdev_calculator_.AddSlowStdevCalculatorListener(this);
  dep_market_view_.subscribe_price_type(this, _t_price_type_);
  indep_market_view_.subscribe_price_type(this, _t_price_type_);
}

void OnlineBeta::OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_) {
  if (_security_id_ == dep_market_view_.security_id())
    dep_std_dev_ = _new_stdev_value_;
  else
    indep_std_dev_ = _new_stdev_value_;
}

void OnlineBeta::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    current_dep_price_ = dep_market_view_.GetPriceFromType(price_type_, _market_update_info_);
    dep_updated_ = true;
  }
  if (_security_id_ == indep_market_view_.security_id()) {
    current_indep_price_ = indep_market_view_.GetPriceFromType(price_type_, _market_update_info_);
    indep_updated_ = true;
  }

  if (dep_updated_ && indep_updated_) {
    if (!is_ready_) {
      if (indep_market_view_.is_ready_complex(2) && dep_market_view_.is_ready_complex(2)) {
        is_ready_ = true;
        InitializeValues();
      }
    } else {
      if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
        dep_moving_avg_price_ += inv_decay_sum_ * (current_dep_price_ - last_dep_price_recorded_);
        indep_moving_avg_price_ += inv_decay_sum_ * (current_indep_price_ - last_indep_price_recorded_);
        dep_indep_moving_avg_price_ += inv_decay_sum_ * (current_dep_price_ * current_indep_price_ -
                                                         last_dep_price_recorded_ * last_indep_price_recorded_);
      } else {  // new page(s)
        int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
        if (num_pages_to_add_ >= (int)decay_vector_.size()) {
          InitializeValues();
        } else {
          if (num_pages_to_add_ == 1) {
            dep_moving_avg_price_ =
                (current_dep_price_ * inv_decay_sum_) + (dep_moving_avg_price_ * decay_page_factor_);
            indep_moving_avg_price_ =
                (current_indep_price_ * inv_decay_sum_) + (indep_moving_avg_price_ * decay_page_factor_);
            dep_indep_moving_avg_price_ = (current_dep_price_ * current_indep_price_ * inv_decay_sum_) +
                                          (dep_indep_moving_avg_price_ * decay_page_factor_);
          } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
            dep_moving_avg_price_ =
                (current_dep_price_ * inv_decay_sum_) +
                (last_dep_price_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                (dep_moving_avg_price_ * decay_vector_[num_pages_to_add_]);
            indep_moving_avg_price_ =
                (current_indep_price_ * inv_decay_sum_) +
                (last_indep_price_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                (indep_moving_avg_price_ * decay_vector_[num_pages_to_add_]);
            dep_indep_moving_avg_price_ = inv_decay_sum_ * current_dep_price_ * current_indep_price_ +
                                          (last_dep_price_recorded_ * last_indep_price_recorded_ * inv_decay_sum_ *
                                           decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                                          (dep_indep_moving_avg_price_ * decay_vector_[num_pages_to_add_]);
          }
          last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
        }
      }
      last_indep_price_recorded_ = current_indep_price_;
      last_dep_price_recorded_ = current_dep_price_;

      moving_covariance_ = dep_indep_moving_avg_price_ - dep_moving_avg_price_ * indep_moving_avg_price_;

      indicator_value_ = moving_covariance_ /
                         (indep_std_dev_ *
                          indep_std_dev_);  //// beta = corr*std_dep/std_indep; corr = covariance /(std_dep*std_indep);
      if (data_interrupted_) {
        indicator_value_ = 0;
      }
      NotifyIndicatorListeners((indicator_value_));
    }
  }
}

void OnlineBeta::InitializeValues() {
  moving_covariance_ = 0;
  dep_moving_avg_price_ = current_dep_price_;
  indep_moving_avg_price_ = current_indep_price_;
  dep_indep_moving_avg_price_ = current_dep_price_ * current_indep_price_;
  last_indep_price_recorded_ = current_indep_price_;
  last_dep_price_recorded_ = current_dep_price_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - (watch_.msecs_from_midnight() % page_width_msecs_);
  indicator_value_ = 0;
}

void OnlineBeta::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void OnlineBeta::OnMarketDataResumed(const unsigned int _security_id_) {
  InitializeValues();
  data_interrupted_ = false;
}
}
