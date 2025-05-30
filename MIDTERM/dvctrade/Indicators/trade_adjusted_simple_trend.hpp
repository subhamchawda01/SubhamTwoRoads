/**
    \file Indicators/trade_adjusted_simple_trend.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_TRADE_ADJUSTED_SIMPLE_TREND_H
#define BASE_INDICATORS_TRADE_ADJUSTED_SIMPLE_TREND_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/TradeUtils/time_decayed_trade_info_manager.hpp"

namespace HFSAT {

/// Class returning current trade adjusted price minus moving average,
/// i.e. exponentially time-decaying moving average
class TradeAdjustedSimpleTrend : public CommonIndicator, public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& indep_market_view_;

  TimeDecayedTradeInfoManager& time_decayed_trade_info_manager_;

  const PriceType_t price_type_;

  // computational variables
  double moving_avg_price_;

  double last_price_recorded_;
  double current_indep_price_;

  double stale_price_;  // price seen at last big time period update. The aim is to see whether trade price accentuates
                        // trend or not

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static TradeAdjustedSimpleTrend* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                     const std::vector<const char*>& _tokens_,
                                                     PriceType_t _basepx_pxtype_);

  static TradeAdjustedSimpleTrend* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                     SecurityMarketView& _indep_market_view_,
                                                     double _fractional_seconds_, double _trade_seconds_,
                                                     PriceType_t _price_type_);

 protected:
  TradeAdjustedSimpleTrend(DebugLogger& _dbglogger_, const Watch& _watch_,
                           const std::string& concise_indicator_description_, SecurityMarketView& _indep_market_view_,
                           double _fractional_seconds_, double _trade_seconds_, PriceType_t _price_type_);

 public:
  ~TradeAdjustedSimpleTrend() {}

  // listener interface

  /// currently called by watch ... TODO ... call this on select timeout in livetrading
  void OnTimePeriodUpdate(const int num_pages_to_add_);

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  static std::string VarName() { return "TradeAdjustedSimpleTrend"; }

  void WhyNotReady();
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_TRADE_ADJUSTED_SIMPLE_TREND_H
