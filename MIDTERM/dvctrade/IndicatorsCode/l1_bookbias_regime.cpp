/**
   \file IndicatorsCode/offline_breakout_adjusted_pairs.cpp

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

#include "dvctrade/Indicators/l1_bookbias_regime.hpp"

namespace HFSAT {

void L1BookbiasRegime::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                         std::vector<std::string>& _ors_source_needed_vec_,
                                         const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

L1BookbiasRegime* L1BookbiasRegime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      const std::vector<const char*>& r_tokens_,
                                                      PriceType_t _basepx_pxtype_) {
  // INDICATOR  _this_weight_  _indicator_string_  _dep_market_view_  _threshold_  _tolerance_
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), atof(r_tokens_[5]), _basepx_pxtype_);
}

L1BookbiasRegime* L1BookbiasRegime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      const SecurityMarketView& _dep_market_view_, double _thresh_,
                                                      double _tolerance_, PriceType_t _basepx_pxtype_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _thresh_ << ' ' << _tolerance_ << '\n';

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, L1BookbiasRegime*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new L1BookbiasRegime(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _thresh_,
                             _tolerance_, _basepx_pxtype_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

L1BookbiasRegime::L1BookbiasRegime(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                   const std::string& concise_indicator_description_,
                                   const SecurityMarketView& t_dep_market_view_, double t_thresh_, double t_tolerance_,
                                   PriceType_t t_basepx_pxtype_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_), dep_market_view_(t_dep_market_view_) {
  /// always subscribe to mktpx for this indicator
  if (!dep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice)) {
    PriceType_t t_error_price_type_ = kPriceTypeMktSizeWPrice;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
  tolerance_ = t_tolerance_;
  threshold_ = t_thresh_;
  InitializeValues();
}

void L1BookbiasRegime::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (dep_market_view_.bestask_int_price() - dep_market_view_.bestbid_int_price() > 1 && indicator_value_ != 1) {
    indicator_value_ = 1;
    NotifyIndicatorListeners(indicator_value_);
  } else if (dep_market_view_.bestask_int_price() - dep_market_view_.bestbid_int_price() == 1) {
    double t_dep_price_ = SecurityMarketView::GetPriceFromType(kPriceTypeMktSizeWPrice, _market_update_info_);
    if (((t_dep_price_ - dep_market_view_.bestbid_price() < threshold_ ||
          dep_market_view_.bestask_price() - t_dep_price_ < threshold_) &&
         indicator_value_ == 1) ||
        ((t_dep_price_ - dep_market_view_.bestbid_price() < threshold_ * (1 - tolerance_) ||
          dep_market_view_.bestask_price() - t_dep_price_ < threshold_ * (1 - tolerance_)) &&
         indicator_value_ == 3)) {
      indicator_value_ = 2;
      NotifyIndicatorListeners(indicator_value_);
    }

    if (((t_dep_price_ - dep_market_view_.bestbid_price() > threshold_ &&
          dep_market_view_.bestask_price() - t_dep_price_ > threshold_) &&
         indicator_value_ == 1) ||
        ((t_dep_price_ - dep_market_view_.bestbid_price() > threshold_ * (1 + tolerance_) &&
          dep_market_view_.bestask_price() - t_dep_price_ > threshold_ * (1 + tolerance_)) &&
         indicator_value_ == 2)) {
      indicator_value_ = 3;
      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void L1BookbiasRegime::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                    const MarketUpdateInfo& _market_update_info_) {
  OnMarketUpdate(_security_id_, _market_update_info_);
}

void L1BookbiasRegime::OnMarketDataResumed(const unsigned int _security_id_) {
  /// we're already using most stable model since interrupt.
}

void L1BookbiasRegime::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive) {
  /// move to most 'stable' model in case of data interrupt
  if (_security_id_ == dep_market_view_.security_id()) {
    indicator_value_ = 3;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void L1BookbiasRegime::InitializeValues() {
  indicator_value_ = 3;
  NotifyIndicatorListeners(indicator_value_);
  is_ready_ = true;
}
}
