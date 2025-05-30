/**
    \file IndicatorsCode/recent_simple_range_measure.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvctrade/Indicators/indicator_util.hpp"

#include "dvctrade/Indicators/recent_simple_range_measure.hpp"

namespace HFSAT {

RecentSimpleRangeMeasure* RecentSimpleRangeMeasure::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                      const SecurityMarketView& _indep_market_view_,
                                                                      unsigned int t_seconds_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << t_seconds_;

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, RecentSimpleRangeMeasure*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new RecentSimpleRangeMeasure(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, t_seconds_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

RecentSimpleRangeMeasure::RecentSimpleRangeMeasure(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                   const std::string& concise_indicator_description_,
                                                   const SecurityMarketView& _indep_market_view_,
                                                   unsigned int t_seconds_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      range_calc_seconds_(std::max(1u, t_seconds_)),
      decay_second_factor_(MathUtils::CalcDecayFactor(std::max(1u, t_seconds_))),
      running_max_value_(kInvalidPrice),
      running_min_value_(kInvalidPrice) {
  r_watch_.subscribe_BigTimePeriod(this);
  indep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
}

void RecentSimpleRangeMeasure::OnTimePeriodUpdate(const int num_pages_to_add_) {
  double current_price_ = indep_market_view_.market_update_info_.mkt_size_weighted_price_;
  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      running_max_value_ = running_min_value_ = current_price_;
      indicator_value_ = 0;
      DBGLOG_TIME_CLASS_FUNC << " ready" << DBGLOG_ENDL_FLUSH;
    }
  } else if (!data_interrupted_) {
    if (current_price_ > running_max_value_) {
      running_max_value_ = current_price_;
    } else {
      running_max_value_ = (decay_second_factor_ * running_max_value_) + ((1 - decay_second_factor_) * current_price_);
    }
    if (current_price_ < running_min_value_) {
      running_min_value_ = current_price_;
    } else {
      running_min_value_ = (decay_second_factor_ * running_min_value_) + ((1 - decay_second_factor_) * current_price_);
    }
    indicator_value_ = running_max_value_ - running_min_value_;
    // DBGLOG_TIME_CLASS_FUNC << " iv:" << indicator_value_ << " " << running_min_value_ << " " << running_max_value_ <<
    // " " << current_price_ << DBGLOG_ENDL_FLUSH ;

    // pages_since_last_broadcast_ += num_pages_to_add_;
    // if ( pages_since_last_broadcast_ >= 60 )
    {
      // broadcast to listeners
      for (auto i = 0u; i < recent_range_measure_listener_ptr_vec_.size(); i++) {
        recent_range_measure_listener_ptr_vec_[i]->OnRangeUpdate(indep_market_view_.security_id(), indicator_value_);
      }
      // pages_since_last_broadcast_ = 0;
    }
  }
}
void RecentSimpleRangeMeasure::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                       const int msecs_since_last_receive_) {
  data_interrupted_ = true;
}

void RecentSimpleRangeMeasure::OnMarketDataResumed(const unsigned int _security_id_) { data_interrupted_ = false; }
}
