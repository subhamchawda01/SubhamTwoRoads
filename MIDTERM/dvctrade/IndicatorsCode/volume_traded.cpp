/**
    \file IndicatorsCode/volume_traded.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"

#include "dvctrade/Indicators/volume_traded.hpp"

namespace HFSAT {

void VolumeTraded::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                     std::vector<std::string>& _ors_source_needed_vec_,
                                     const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

VolumeTraded* VolumeTraded::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                              const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ t_indep_market_view_ _fractional_seconds_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);  ///< called to make sure the shortcode is valid
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])));
}

VolumeTraded* VolumeTraded::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                              SecurityMarketView& t_indep_market_view_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_indep_market_view_.secname();
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, VolumeTraded*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new VolumeTraded(t_dbglogger_, r_watch_, concise_indicator_description_, t_indep_market_view_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

VolumeTraded::VolumeTraded(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                           const std::string& concise_indicator_description_, SecurityMarketView& t_indep_market_view_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(t_indep_market_view_) {
  t_indep_market_view_.subscribe_tradeprints(this);
}

void VolumeTraded::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                const MarketUpdateInfo& _market_update_info_) {
  if (indep_market_view_.is_ready()) {
    is_ready_ = true;
  }

  indicator_value_ += _trade_print_info_.size_traded_;
  if (std::isnan(indicator_value_)) {
    std::cerr << __PRETTY_FUNCTION__ << " nan " << std::endl;
    exit(1);
  }
  if (data_interrupted_) indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}
void VolumeTraded::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else
    return;
}

void VolumeTraded::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  } else
    return;
}
}
