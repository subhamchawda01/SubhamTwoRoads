/**
    \file IndicatorsCode/trend_adjusted_self_position.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/risk_indicator_util.hpp"
#include "dvctrade/Indicators/trend_adjusted_self_position.hpp"

namespace HFSAT {

void TrendAdjustedSelfPosition::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                  std::vector<std::string>& _ors_source_needed_vec_,
                                                  const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_ors_source_needed_vec_, (std::string)r_tokens_[3]);
}

TrendAdjustedSelfPosition* TrendAdjustedSelfPosition::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                        const Watch& r_watch_,
                                                                        const std::vector<const char*>& r_tokens_,
                                                                        PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_  _indicator_string_  _indep_prom_order_manager_shortcode_  _fractional_seconds_
  // _trend_factor_
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodePromOrderManagerMap::StaticGetPromOrderManager(r_tokens_[3])), atof(r_tokens_[4]),
                           atof(r_tokens_[5]), _basepx_pxtype_);
}

TrendAdjustedSelfPosition* TrendAdjustedSelfPosition::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, PromOrderManager& _indep_prom_order_manager_,
    const double _fractional_seconds_, const double _trend_factor_, PriceType_t _basepx_pxtype_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_prom_order_manager_.secname() << ' ' << _fractional_seconds_ << ' '
              << _trend_factor_ << " " << PriceType_t_To_String(_basepx_pxtype_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, TrendAdjustedSelfPosition*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new TrendAdjustedSelfPosition(t_dbglogger_, r_watch_, concise_indicator_description_,
                                      _indep_prom_order_manager_, _fractional_seconds_, _trend_factor_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

TrendAdjustedSelfPosition::TrendAdjustedSelfPosition(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                     const std::string& concise_indicator_description_,
                                                     PromOrderManager& _indep_prom_order_manager_,
                                                     const double _fractional_seconds_, const double _trend_factor_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      trend_factor_(_trend_factor_),
      moving_avg_position_(0),
      last_position_recorded_(0),
      current_indep_position_(0) {
  trend_history_msecs_ = std::max(MIN_MSEC_HISTORY_INDICATORS, (int)round(1000 * _fractional_seconds_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();
  _indep_prom_order_manager_.AddGlobalPositionChangeListener(this);
}

void TrendAdjustedSelfPosition::OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_) {
  current_indep_position_ = _new_global_position_;

  if (!is_ready_) {
    is_ready_ = true;
    indicator_value_ = 0;
  } else {
    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      moving_avg_position_ += inv_decay_sum_ * (current_indep_position_ - last_position_recorded_);
      last_position_recorded_ = current_indep_position_;
    } else {
      unsigned int num_pages_to_add_ = (unsigned int)(std::max(
          0, (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_)));
      if (num_pages_to_add_ >= decay_vector_.size()) {
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {
          moving_avg_position_ = (current_indep_position_ * inv_decay_sum_) + (moving_avg_position_ * decay_vector_[1]);
        } else {  // ( num_pages_to_add_ >= 2 ) && ( num_pages_to_add_ < decay_vector_.size ( ) )
          moving_avg_position_ =
              (current_indep_position_ * inv_decay_sum_) +
              (last_position_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
              (moving_avg_position_ * decay_vector_[num_pages_to_add_]);
        }
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }
    }

    indicator_value_ = (current_indep_position_) + ((current_indep_position_ - moving_avg_position_) * trend_factor_);

    NotifyIndicatorListeners(indicator_value_);
  }
}

void TrendAdjustedSelfPosition::InitializeValues() {
  moving_avg_position_ = current_indep_position_;
  last_position_recorded_ = current_indep_position_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}
void TrendAdjustedSelfPosition::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                        const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void TrendAdjustedSelfPosition::OnMarketDataResumed(const unsigned int _security_id_) { data_interrupted_ = false; }
}
