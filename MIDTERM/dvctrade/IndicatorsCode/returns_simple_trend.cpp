/**
    \file IndicatorsCode/returns_simple_trend.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
// INDICATOR 1.00 ReturnsSimpleTrend ES_0 100 100 OfflineMixMMS
//#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/returns_simple_trend.hpp"

namespace HFSAT {

void ReturnsSimpleTrend::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                           std::vector<std::string>& _ors_source_needed_vec_,
                                           const std::vector<const char*>& r_tokens_) {
  if (r_tokens_.size() >= 7u)  // doesnt mean indicator string syntax is valid .
  {
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  }
}

ReturnsSimpleTrend* ReturnsSimpleTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                          const std::vector<const char*>& r_tokens_,
                                                          PriceType_t _base_price_type_) {
  if (r_tokens_.size() < 7u) {
    ExitVerbose(kExitErrorCodeGeneral, "ReturnsSimpleTrend needs 7 tokens");
    return NULL;
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), atof(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
}

ReturnsSimpleTrend* ReturnsSimpleTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                          const SecurityMarketView& _smv_, double _returns_window_,
                                                          double _trend_history_secs_, PriceType_t t_price_type_) {
  std::ostringstream t_temp_oss_;

  t_temp_oss_ << VarName() << ' ' << _smv_.secname() << ' ' << _returns_window_ << ' ' << _trend_history_secs_ << ' '
              << t_price_type_;

  std::string concise_indicator_description_(t_temp_oss_.str());
  static std::map<std::string, ReturnsSimpleTrend*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new ReturnsSimpleTrend(t_dbglogger_, r_watch_, concise_indicator_description_, _smv_, _returns_window_,
                               _trend_history_secs_, t_price_type_);
  }

  return concise_indicator_description_map_[concise_indicator_description_];
}

ReturnsSimpleTrend::ReturnsSimpleTrend(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                       const std::string& concise_indicator_description_,
                                       const SecurityMarketView& _smv_, double _returns_window_,
                                       double _trend_history_secs_, PriceType_t _price_type_)
    : IndicatorListener(),
      CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      moving_avg_returns_(0.0),
      last_return_recorded_(0.0),
      current_return_(0.0) {
  trend_history_msecs_ = std::max(20, (int)round(1000 * _trend_history_secs_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();
  returns_ = SimpleReturns::GetUniqueInstance(t_dbglogger_, r_watch_, _smv_, _returns_window_, _price_type_);
  returns_->add_unweighted_indicator_listener(1u, this);
}

void ReturnsSimpleTrend::OnIndicatorUpdate(const unsigned int& indicator_index_, const double& new_value_) {
  current_return_ = new_value_;
  if (!is_ready_) {
    is_ready_ = true;
    InitializeValues();
  } else if (!data_interrupted_) {
    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      moving_avg_returns_ += inv_decay_sum_ * (current_return_ - last_return_recorded_);
    } else {
      int num_pages_to_add_ = ((int)floor(watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {
          moving_avg_returns_ = (current_return_ * inv_decay_sum_) + (moving_avg_returns_ * decay_vector_[1]);
        } else {
          moving_avg_returns_ = (current_return_ * inv_decay_sum_) +
                                (last_return_recorded_ * inv_decay_sum_ * decay_vector_sums_[num_pages_to_add_ - 1]) +
                                (moving_avg_returns_ * decay_vector_[num_pages_to_add_]);
        }
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }
      indicator_value_ = current_return_ - moving_avg_returns_;
      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void ReturnsSimpleTrend::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                 const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void ReturnsSimpleTrend::OnMarketDataResumed(const unsigned int _security_id_) {
  InitializeValues();
  data_interrupted_ = false;
}

void ReturnsSimpleTrend::InitializeValues() {
  moving_avg_returns_ = current_return_;
  last_return_recorded_ = current_return_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}

}
