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
#include "dvctrade/Indicators/exponential_moving_average.hpp"

namespace HFSAT {

void ExponentialMovingAverage::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                 std::vector<std::string>& _ors_source_needed_vec_,
                                                 const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

ExponentialMovingAverage* ExponentialMovingAverage::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                      const std::vector<const char*>& r_tokens_,
                                                                      PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_ _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), StringToPriceType_t(r_tokens_[5]));
}

ExponentialMovingAverage* ExponentialMovingAverage::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                      const SecurityMarketView& _indep_market_view_,
                                                                      double _fractional_seconds_,
                                                                      PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _fractional_seconds_ << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, ExponentialMovingAverage*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new ExponentialMovingAverage(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_,
                                     _fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

ExponentialMovingAverage::ExponentialMovingAverage(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                   const std::string& concise_indicator_description_,
                                                   const SecurityMarketView& _indep_market_view_,
                                                   double _fractional_seconds_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      current_indep_price_(0),
      price_type_(_price_type_) {
  moving_avg_util_ = new MovingAverage(t_dbglogger_,r_watch_,_fractional_seconds_);
  if (!indep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
}

void ExponentialMovingAverage::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready())) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void ExponentialMovingAverage::OnMarketUpdate(const unsigned int _security_id_,
                                              const MarketUpdateInfo& _market_update_info_) {
  current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);

  if (!is_ready_) {
    if (indep_market_view_.is_ready()) {
      is_ready_ = true;
      InitializeValues();
      moving_avg_util_->CalculateValue(current_indep_price_); // First Call will initialize the values of the class
    }
  } else if (!data_interrupted_) {

	moving_avg_util_->CalculateValue(current_indep_price_);
    indicator_value_ = moving_avg_util_->moving_avg_;

    NotifyIndicatorListeners(indicator_value_);
  }
}

void ExponentialMovingAverage::InitializeValues() {
  indicator_value_ = current_indep_price_;
}

// market_interrupt_listener interface
void ExponentialMovingAverage::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                       const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = current_indep_price_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void ExponentialMovingAverage::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  }
}
}
