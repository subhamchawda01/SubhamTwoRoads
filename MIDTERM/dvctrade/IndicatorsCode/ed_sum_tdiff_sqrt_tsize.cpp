/**
   \file IndicatorsCode/ed_sum_tdiff_sqrt_tsize.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/ed_sum_tdiff_sqrt_tsize.hpp"

namespace HFSAT {

void EDSumTDiffSqrtTSize::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                            std::vector<std::string>& _ors_source_needed_vec_,
                                            const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

EDSumTDiffSqrtTSize* EDSumTDiffSqrtTSize::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                            const std::vector<const char*>& r_tokens_,
                                                            PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _num_trades_halflife_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atoi(r_tokens_[4]), _basepx_pxtype_);
}

EDSumTDiffSqrtTSize* EDSumTDiffSqrtTSize::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                            SecurityMarketView& _indep_market_view_,
                                                            int _num_trades_halflife_, PriceType_t _basepx_pxtype_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _num_trades_halflife_ << " "
              << PriceType_t_To_String(_basepx_pxtype_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, EDSumTDiffSqrtTSize*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new EDSumTDiffSqrtTSize(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _num_trades_halflife_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

EDSumTDiffSqrtTSize::EDSumTDiffSqrtTSize(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                         const std::string& concise_indicator_description_,
                                         SecurityMarketView& _indep_market_view_, int _num_trades_halflife_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      event_decayed_trade_info_manager_(*(EventDecayedTradeInfoManager::GetUniqueInstance(
          t_dbglogger_, r_watch_, _indep_market_view_, _num_trades_halflife_))) {
  event_decayed_trade_info_manager_.compute_sumtdiffsqrtsz();
  _indep_market_view_.subscribe_tradeprints(this);
}

void EDSumTDiffSqrtTSize::OnMarketUpdate(const unsigned int _security_id_,
                                         const MarketUpdateInfo& _market_update_info_) {
  indicator_value_ = data_interrupted_ ? 0 : event_decayed_trade_info_manager_.sumtdiffsqrtsz_;
  NotifyIndicatorListeners(indicator_value_);
}

void EDSumTDiffSqrtTSize::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                       const MarketUpdateInfo& _market_update_info_) {
  indicator_value_ = data_interrupted_ ? 0 : event_decayed_trade_info_manager_.sumtdiffsqrtsz_;
  NotifyIndicatorListeners(indicator_value_);
}

void EDSumTDiffSqrtTSize::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                  const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else
    return;
}

void EDSumTDiffSqrtTSize::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
    event_decayed_trade_info_manager_.InitializeValues();
  } else
    return;
}
}
