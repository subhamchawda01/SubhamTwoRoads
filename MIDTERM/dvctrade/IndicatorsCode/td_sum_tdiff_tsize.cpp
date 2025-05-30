/**
    \file IndicatorsCode/td_sum_tdiff_tsize.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvctrade/Indicators/indicator_util.hpp"

#include "dvctrade/Indicators/td_sum_tdiff_tsize.hpp"

namespace HFSAT {

void TDSumTDiffTSize::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                        std::vector<std::string>& _ors_source_needed_vec_,
                                        const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

TDSumTDiffTSize* TDSumTDiffTSize::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                    const std::vector<const char*>& r_tokens_,
                                                    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), _basepx_pxtype_);
}

TDSumTDiffTSize* TDSumTDiffTSize::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                    SecurityMarketView& _indep_market_view_,
                                                    double _fractional_seconds_, PriceType_t _basepx_pxtype_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _fractional_seconds_ << " "
              << PriceType_t_To_String(_basepx_pxtype_);

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, TDSumTDiffTSize*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new TDSumTDiffTSize(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _fractional_seconds_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

TDSumTDiffTSize::TDSumTDiffTSize(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                 const std::string& concise_indicator_description_,
                                 SecurityMarketView& _indep_market_view_, double _fractional_seconds_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      time_decayed_trade_info_manager_(*(TimeDecayedTradeInfoManager::GetUniqueInstance(
          t_dbglogger_, r_watch_, _indep_market_view_, _fractional_seconds_))) {
  time_decayed_trade_info_manager_.compute_sumtdiffsz();
  _indep_market_view_.subscribe_tradeprints(this);
}

void TDSumTDiffTSize::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  indicator_value_ = time_decayed_trade_info_manager_.sumtdiffsz_;

  if (data_interrupted_) indicator_value_ = 0;

  NotifyIndicatorListeners(indicator_value_);
}

void TDSumTDiffTSize::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                   const MarketUpdateInfo& _market_update_info_) {
  indicator_value_ = time_decayed_trade_info_manager_.sumtdiffsz_;

  if (data_interrupted_) indicator_value_ = 0;

  NotifyIndicatorListeners(indicator_value_);
}

void TDSumTDiffTSize::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else
    return;
}

void TDSumTDiffTSize::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
    time_decayed_trade_info_manager_.InitializeValues();
  } else
    return;
}
}
