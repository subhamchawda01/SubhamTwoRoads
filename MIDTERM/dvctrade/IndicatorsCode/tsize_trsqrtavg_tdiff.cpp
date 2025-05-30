/**
    \file IndicatorsCode/tsize_trsqrtavg_tdiff.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/tsize_trsqrtavg_tdiff.hpp"

namespace HFSAT {

void TSizeTRSqrtAvgTDiff::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                            std::vector<std::string>& _ors_source_needed_vec_,
                                            const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

TSizeTRSqrtAvgTDiff* TSizeTRSqrtAvgTDiff::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                            const std::vector<const char*>& r_tokens_,
                                                            PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _num_trades_halflife_
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           (unsigned int)(std::max(atoi(r_tokens_[4]), 1)), _basepx_pxtype_);
}

TSizeTRSqrtAvgTDiff* TSizeTRSqrtAvgTDiff::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                            SecurityMarketView& _indep_market_view_,
                                                            unsigned int _num_trades_halflife_,
                                                            PriceType_t _basepx_pxtype_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _num_trades_halflife_ << " "
              << PriceType_t_To_String(_basepx_pxtype_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, TSizeTRSqrtAvgTDiff*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new TSizeTRSqrtAvgTDiff(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _num_trades_halflife_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

TSizeTRSqrtAvgTDiff::TSizeTRSqrtAvgTDiff(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                         const std::string& concise_indicator_description_,
                                         SecurityMarketView& _indep_market_view_, unsigned int _num_trades_halflife_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      trade_decayed_trade_info_manager_(*(TradeDecayedTradeInfoManager::GetUniqueInstance(
          t_dbglogger_, r_watch_, _indep_market_view_, _num_trades_halflife_))) {
  trade_decayed_trade_info_manager_.compute_sumtdiffsz();
  trade_decayed_trade_info_manager_.compute_sumsqrtsz();
  _indep_market_view_.subscribe_tradeprints(this);
}

/// Should remove this function.
/// Since this is trade decayed, on a market change there should not be any difference in the values.
/// In the long term it will be a better dea to make this a subscription mechanism as well
void TSizeTRSqrtAvgTDiff::OnMarketUpdate(const unsigned int _security_id_,
                                         const MarketUpdateInfo& _market_update_info_) {
#define MIN_SIGNIFICANT_SUM_SQRT_SZ_TRADED 0.10
  // need to compare against a low value since otherwise there would be weird values as the denominator recedes in value
  if (trade_decayed_trade_info_manager_.sumsqrtsz_ >= MIN_SIGNIFICANT_SUM_SQRT_SZ_TRADED) {
    indicator_value_ = trade_decayed_trade_info_manager_.sumtdiffsz_ / trade_decayed_trade_info_manager_.sumsqrtsz_;
  } else {
    indicator_value_ = 0;
  }
#undef MIN_SIGNIFICANT_SUM_SQRT_SZ_TRADED

  if (std::isnan(indicator_value_)) {
    std::cerr << __PRETTY_FUNCTION__ << " nan " << std::endl;
    exit(1);
  }

  if (data_interrupted_) indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void TSizeTRSqrtAvgTDiff::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                       const MarketUpdateInfo& _market_update_info_) {
#define MIN_SIGNIFICANT_SUM_SQRT_SZ_TRADED 0.10
  // need to compare against a low value since otherwise there would be weird values as the denominator recedes in value
  if (trade_decayed_trade_info_manager_.sumsqrtsz_ >= MIN_SIGNIFICANT_SUM_SQRT_SZ_TRADED) {
    indicator_value_ = trade_decayed_trade_info_manager_.sumtdiffsz_ / trade_decayed_trade_info_manager_.sumsqrtsz_;
  } else {
    indicator_value_ = 0;
  }
#undef MIN_SIGNIFICANT_SUM_SQRT_SZ_TRADED

  if (std::isnan(indicator_value_)) {
    std::cerr << __PRETTY_FUNCTION__ << " nan " << std::endl;
    exit(1);
  }

  if (data_interrupted_) indicator_value_ = 0;

  NotifyIndicatorListeners(indicator_value_);
}

void TSizeTRSqrtAvgTDiff::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                  const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else
    return;
}

void TSizeTRSqrtAvgTDiff::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
    trade_decayed_trade_info_manager_.InitializeValues();
  } else
    return;
}
}
