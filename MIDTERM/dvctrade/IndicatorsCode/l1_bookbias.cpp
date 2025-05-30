/**
   \file IndicatorsCode/l1_bookbias.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include <fstream>
#include <strings.h>
#include <stdlib.h>
#include <sstream>

#include "dvccode/CDef/math_utils.hpp"

#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"

#include "dvctrade/Indicators/l1_bookbias.hpp"

namespace HFSAT {

void L1Bookbias::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                   std::vector<std::string>& _ors_source_needed_vec_,
                                   const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

L1Bookbias* L1Bookbias::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                          const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR  _this_weight_  _indicator_string_  _dep_market_view_  _threshold_  _tolerance_
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           _basepx_pxtype_);
}

L1Bookbias* L1Bookbias::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                          const SecurityMarketView& _dep_market_view_, PriceType_t _basepx_pxtype_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << '\n';

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, L1Bookbias*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new L1Bookbias(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _basepx_pxtype_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

L1Bookbias::L1Bookbias(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                       const std::string& concise_indicator_description_, const SecurityMarketView& t_dep_market_view_,
                       PriceType_t _basepx_pxtype_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_), dep_market_view_(t_dep_market_view_) {
  /// always subscribe to mktpx for this indicator
  if (!dep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice)) {
    PriceType_t t_error_price_type_ = kPriceTypeMktSizeWPrice;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
  InitializeValues();
}

void L1Bookbias::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  double t_dep_mkt_price_ = SecurityMarketView::GetPriceFromType(kPriceTypeMktSizeWPrice, _market_update_info_);
  double t_dep_mid_price_ = SecurityMarketView::GetPriceFromType(kPriceTypeMidprice, _market_update_info_);
  indicator_value_ = fabs(t_dep_mkt_price_ - t_dep_mid_price_) / dep_market_view_.min_price_increment();
  NotifyIndicatorListeners(indicator_value_);
}

void L1Bookbias::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                              const MarketUpdateInfo& _market_update_info_) {
  OnMarketUpdate(_security_id_, _market_update_info_);
}

void L1Bookbias::OnMarketDataResumed(const unsigned int _security_id_) {
  /// we're already using most stable model since interrupt.
}

void L1Bookbias::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive) {
  /// move to most 'stable' model in case of data interrupt
  if (_security_id_ == dep_market_view_.security_id()) {
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void L1Bookbias::InitializeValues() {
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
  is_ready_ = true;
}
}
