/**
    \file IndicatorsCode/moving_avg_bidask_spread.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/moving_avg_bidask_spread.hpp"

namespace HFSAT {

void MovingAvgBidAskSpread::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                              std::vector<std::string>& _ors_source_needed_vec_,
                                              const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

MovingAvgBidAskSpread* MovingAvgBidAskSpread::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                const std::vector<const char*>& r_tokens_,
                                                                PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_ _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]));
}

MovingAvgBidAskSpread* MovingAvgBidAskSpread::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                const SecurityMarketView* _indep_market_view_,
                                                                double _fractional_seconds_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_->secname() << ' ' << _fractional_seconds_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, MovingAvgBidAskSpread*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new MovingAvgBidAskSpread(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _fractional_seconds_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

MovingAvgBidAskSpread::MovingAvgBidAskSpread(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                             const std::string& concise_indicator_description_,
                                             const SecurityMarketView* _indep_market_view_, double _fractional_seconds_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      moving_avg_bidask_spread_(0),
      last_bidask_spread_recorded_(0),
      current_bidask_spread_(0) {
  trend_history_msecs_ = std::max(20, (int)round(1000 * _fractional_seconds_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();
  _indep_market_view_->subscribe_price_type(this, kPriceTypeMktSizeWPrice);
}

void MovingAvgBidAskSpread::OnMarketUpdate(const unsigned int _security_id_,
                                           const MarketUpdateInfo& cr_market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_->is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else {
    current_bidask_spread_ = cr_market_update_info_.spread_increments_;
    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      moving_avg_bidask_spread_ += inv_decay_sum_ * (current_bidask_spread_ - last_bidask_spread_recorded_);
    } else {  // new page(s)
      int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {
          moving_avg_bidask_spread_ =
              (current_bidask_spread_ * inv_decay_sum_) + (moving_avg_bidask_spread_ * decay_page_factor_);
        } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
          moving_avg_bidask_spread_ =
              (current_bidask_spread_ * inv_decay_sum_) +
              (last_bidask_spread_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
              (moving_avg_bidask_spread_ * decay_vector_[num_pages_to_add_]);
        }
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }
    }

    last_bidask_spread_recorded_ = current_bidask_spread_;
    indicator_value_ = moving_avg_bidask_spread_;
    if (data_interrupted_) indicator_value_ = 1;

    NotifyIndicatorListeners(indicator_value_);
  }
}

void MovingAvgBidAskSpread::InitializeValues() {
  current_bidask_spread_ = indep_market_view_->spread_increments();
  moving_avg_bidask_spread_ = current_bidask_spread_;

  last_bidask_spread_recorded_ = current_bidask_spread_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 1;
}

// market_interrupt_listener interface
void MovingAvgBidAskSpread::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                    const int msecs_since_last_receive_) {
  if (indep_market_view_->security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 1;
    NotifyIndicatorListeners(indicator_value_);
  } else
    return;
}

void MovingAvgBidAskSpread::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_->security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  } else
    return;
}
}
