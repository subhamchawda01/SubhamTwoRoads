/**
    \file IndicatorsCode/simple_price_trend.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/simple_price_trend.hpp"

namespace HFSAT {

void SimplePriceTrend::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                         std::vector<std::string>& _ors_source_needed_vec_,
                                         const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

SimplePriceTrend* SimplePriceTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      const std::vector<const char*>& r_tokens_,
                                                      PriceType_t _basepx_pxtype_) {
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           (double)atof(r_tokens_[4]), _basepx_pxtype_);
}

SimplePriceTrend* SimplePriceTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      const SecurityMarketView& _indep_market_view_, double _alpha_,
                                                      PriceType_t _basepx_pxtype_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _alpha_ << ' ' << _basepx_pxtype_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, SimplePriceTrend*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new SimplePriceTrend(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _alpha_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

SimplePriceTrend::SimplePriceTrend(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                   const std::string& concise_indicator_description_,
                                   const SecurityMarketView& _indep_market_view_, double _alpha_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      alpha_(_alpha_),
      moving_avg_price_(0),
      current_indep_price_(0.0),
      last_indep_int_price_(0) {
  if (!indep_market_view_.subscribe_price_type(this, kPriceTypeMidprice)) {
    PriceType_t t_error_price_type_ = kPriceTypeMidprice;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
}

void SimplePriceTrend::OnMarketUpdate(const unsigned int _security_id_,
                                      const MarketUpdateInfo& cr_market_update_info_) {
  int current_indep_int_price_ = cr_market_update_info_.bestask_int_price_ + cr_market_update_info_.bestbid_int_price_;
  current_indep_price_ = SecurityMarketView::GetPriceFromType(kPriceTypeMidprice, cr_market_update_info_);

  if (!is_ready_) {
    if (indep_market_view_.is_ready() &&
        (indep_market_view_.spread_increments() < (2 * indep_market_view_.normal_spread_increments()))) {
      is_ready_ = true;

      moving_avg_price_ = current_indep_price_;
      indicator_value_ = 0;
    }
  } else if (!data_interrupted_) {
    if ((last_indep_int_price_ < current_indep_int_price_) || (last_indep_int_price_ > current_indep_int_price_)) {
      moving_avg_price_ = moving_avg_price_ * alpha_ + (1 - alpha_) * current_indep_price_;
      last_indep_int_price_ = current_indep_int_price_;
    }

    indicator_value_ = current_indep_price_ - moving_avg_price_;
  }

  NotifyIndicatorListeners(indicator_value_);
}

void SimplePriceTrend::InitializeValues() {
  indicator_value_ = 0;
  moving_avg_price_ = current_indep_price_;
}
// market_interrupt_listener interface
void SimplePriceTrend::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void SimplePriceTrend::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  }
}
}
