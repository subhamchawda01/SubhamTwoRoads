/**
    \file Indicators/regime_slow_stdev.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator.hpp"
namespace HFSAT {

/// Class returning current price minus moving average,
/// i.e. exponentially time-decaying moving average
class RegimeSlowStdev : public IndicatorListener, public CommonIndicator {
 protected:
  SecurityMarketView& indep_market_view_;
  SlowStdevCalculator* p_slow_stdev_calculator;

  double switch_threshold_;
  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static RegimeSlowStdev* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                            const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static RegimeSlowStdev* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            SecurityMarketView& _indep_market_view_, double fractional_sec,
                                            double switch_threshold);

 protected:
  RegimeSlowStdev(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                  SecurityMarketView& _indep_market_view_, double fractional_sec, double switch_threshold);

 public:
  ~RegimeSlowStdev() {}

  // listener interface
  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  }  ///< from CommonIndicator::SecurityMarketViewChangeListener
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
  }  ///< from CommonIndicator::SecurityMarketViewChangeListener

  inline void OnPortfolioPriceChange(double _new_price_) {}  ///< from CommonIndicator::PortfolioPriceChangeListener
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    if (p_slow_stdev_calculator != NULL)
      market_update_manager_.AddMarketDataInterruptedListener(p_slow_stdev_calculator);
  }

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);

  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  // functions
  static std::string VarName() { return "RegimeSlowStdev"; }

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

  void WhyNotReady();
};
}
