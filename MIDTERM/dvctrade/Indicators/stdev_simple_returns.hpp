/**
    \file Indicators/stdev_simple_returns.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
       Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_INDICATORS_STDEV_SIMPLE_RETURNS_H
#define BASE_INDICATORS_STDEV_SIMPLE_RETURNS_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator.hpp"

namespace HFSAT {

/// Class returning current price minus moving average,
/// i.e. exponentially time-decaying moving average
class StdevSimpleReturns : public SlowStdevCalculatorListener, public CommonIndicator {
 protected:
  // variables
  const SecurityMarketView& indep_market_view_;

  // computational variables
  double moving_avg_stdev_;

  double last_stdev_recorded_;
  double current_stdev_;

  SlowStdevCalculator* p_slow_stdev_calculator;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static StdevSimpleReturns* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                               const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static StdevSimpleReturns* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                               const SecurityMarketView& _indep_market_view_,
                                               double _stdev_fractional_seconds_, double _trend_fractional_seconds_,
                                               PriceType_t _price_type_);

 protected:
  StdevSimpleReturns(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                     const SecurityMarketView& _indep_market_view_, double _stdev_fractional_seconds_,
                     double _trend_fractional_seconds_);

 public:
  ~StdevSimpleReturns() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
  void OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_);

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  // functions
  static std::string VarName() { return "StdevSimpleReturns"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_STDEV_SIMPLE_RETURNS_H
