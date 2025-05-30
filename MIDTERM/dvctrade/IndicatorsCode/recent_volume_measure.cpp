/**
    \file IndicatorsCode/recent_volume_measure.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvctrade/Indicators/indicator_util.hpp"

#include "dvctrade/Indicators/recent_volume_measure.hpp"

namespace HFSAT {

void RecentVolumeMeasure::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                            std::vector<std::string>& _ors_source_needed_vec_,
                                            const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

RecentVolumeMeasure* RecentVolumeMeasure::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                            const std::vector<const char*>& r_tokens_,
                                                            PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]));
}

RecentVolumeMeasure* RecentVolumeMeasure::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                            const SecurityMarketView& _indep_market_view_,
                                                            double _fractional_seconds_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _fractional_seconds_;

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, RecentVolumeMeasure*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new RecentVolumeMeasure(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _fractional_seconds_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

RecentVolumeMeasure::RecentVolumeMeasure(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                         const std::string& concise_indicator_description_,
                                         const SecurityMarketView& _indep_market_view_, double _fractional_seconds_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      decay_minute_factor_(MathUtils::CalcDecayFactor(std::max(1, (int)ceil((_fractional_seconds_) / (60.0))))),
      pages_since_last_decay_(0) {
  r_watch_.subscribe_BigTimePeriod(this);
  _indep_market_view_.subscribe_tradeprints(this);
}

void RecentVolumeMeasure::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if (indep_market_view_.is_ready()) {
    is_ready_ = true;
  }

  pages_since_last_decay_ += num_pages_to_add_;
  if (pages_since_last_decay_ >= 1) {
    indicator_value_ *= decay_minute_factor_;
    // broadcast to listeners
    for (auto i = 0u; i < recent_volume_measure_listener_ptr_vec_.size(); i++) {
      recent_volume_measure_listener_ptr_vec_[i]->OnVolumeUpdate(indep_market_view_.security_id(), indicator_value_);
    }
    pages_since_last_decay_ = 0;
  }
  NotifyIndicatorListeners(indicator_value_);
}

void RecentVolumeMeasure::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                       const MarketUpdateInfo& _market_update_info_) {
  if (indep_market_view_.is_ready()) {
    is_ready_ = true;
  }

  indicator_value_ += (1 - decay_minute_factor_) * (double)_trade_print_info_.size_traded_;

  // // broadcast to listeners
  // for ( unsigned int i = 0 ; i < recent_volume_measure_listener_ptr_vec_.size ( ) ; i ++ )
  //   {
  // 	recent_volume_measure_listener_ptr_vec_[i]->OnVolumeUpdate ( indep_market_view_.security_id(), indicator_value_
  // ) ;
  //   }
}
void RecentVolumeMeasure::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                  const int msecs_since_last_receive_) {
  data_interrupted_ = true;
}

void RecentVolumeMeasure::OnMarketDataResumed(const unsigned int _security_id_) { data_interrupted_ = false; }
}
