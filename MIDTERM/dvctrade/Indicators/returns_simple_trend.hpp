/**
    \file Indicators/returns_simple_trend.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_RETURNS_SIMPLE_TREND_H
#define BASE_INDICATORS_RETURNS_SIMPLE_TREND_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/BaseUtils/curve_utils.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "dvctrade/Indicators/simple_returns.hpp"

namespace HFSAT {
/* returns window && trend window */
class ReturnsSimpleTrend : public IndicatorListener, public CommonIndicator {
 protected:
  SimpleReturns* returns_;

  // readiness && interrupts handlers

  // computational variables
  double moving_avg_returns_;

  double last_return_recorded_;
  double current_return_;

  ReturnsSimpleTrend(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& _concise_indicator_description_,
                     const SecurityMarketView& _dep_market_view_, double returns_window_, double _trend_history_secs_,
                     PriceType_t _price_type_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static ReturnsSimpleTrend* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                               const std::vector<const char*>& _tokens_, PriceType_t _base_price_type_);

  static ReturnsSimpleTrend* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                               const SecurityMarketView& _dep_market_view_, double returns_window_,
                                               double _trend_history_secs_, PriceType_t t_price_type_);

  ~ReturnsSimpleTrend() {}

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {}

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  // subscribing at this indicators level ( OnMarketDataInterrupted )
  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    if (!returns_) market_update_manager_.AddMarketDataInterruptedListener(returns_);
  }

  void OnIndicatorUpdate(const unsigned int& indicator_index_, const double& new_value_);
  inline void OnIndicatorUpdate(const unsigned int& indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  // functions
  static std::string VarName() { return "ReturnsSimpleTrend"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}

#endif
