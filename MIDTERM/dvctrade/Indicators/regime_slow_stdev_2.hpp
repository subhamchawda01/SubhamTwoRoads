/**
    \file Indicators/regime_slow_stdev_2.cpp

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
#include "dvctrade/Indicators/slow_stdev_calculator_port.hpp"
#include "dvctrade/Indicators/pca_weights_manager.hpp"

namespace HFSAT {

/// Class returning current price minus moving average,
/// i.e. exponentially time-decaying moving average
class RegimeSlowStdev2 : public IndicatorListener, public CommonIndicator {
 protected:
  SecurityMarketView* indep_market_view_;
  SlowStdevCalculator* p_st_slow_stdev_calculator_;
  SlowStdevCalculator* p_lt_slow_stdev_calculator_;

  SlowStdevCalculatorPort* p_st_slow_stdev_calculator_port_;
  SlowStdevCalculatorPort* p_lt_slow_stdev_calculator_port_;

  std::string indep_name_;

  double switch_threshold_;

  double lt_st_factor_;

  bool st_volatile_;
  bool lt_volatile_;

  double avg_stdev_;
  double tolerance_;
  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& shortcodes_affecting_this_indicator,
                                std::vector<std::string>& ors_source_needed_vec_,
                                const std::vector<const char*>& tokens);

  static RegimeSlowStdev2* GetUniqueInstance(DebugLogger& dbglogger, const Watch& watch,
                                             const std::vector<const char*>& tokens, PriceType_t basepx_pxtype);

  static RegimeSlowStdev2* GetUniqueInstance(DebugLogger& t_dbglogger, const Watch& r_watch, std::string indep_name,
                                             double st_fractional_secs, double lt_fractional_secs,
                                             double switch_threshold, double tolerance, bool normalize_stdev);

 protected:
  RegimeSlowStdev2(DebugLogger& dbglogger, const Watch& watch, const std::string& t_concise_indicator_description,
                   std::string indep_name, double st_fractional_secs, double lt_fractional_secs,
                   double switch_threshold, double tolerance, bool normalize_stdev);

 public:
  ~RegimeSlowStdev2() {}

  // listener interface
  inline void OnMarketUpdate(const unsigned int security_id, const MarketUpdateInfo& market_update_info) {
  }  ///< from CommonIndicator::SecurityMarketViewChangeListener
  inline void OnTradePrint(const unsigned int security_id, const TradePrintInfo& trade_print_info,
                           const MarketUpdateInfo& market_update_info) {
  }  ///< from CommonIndicator::SecurityMarketViewChangeListener

  inline void OnPortfolioPriceChange(double new_price) {}  ///< from CommonIndicator::PortfolioPriceChangeListener
  inline void OnPortfolioPriceReset(double t_new_price, double t_old_price, unsigned int is_data_interrupted_){};

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    if (p_st_slow_stdev_calculator_ != nullptr) {
      market_update_manager_.AddMarketDataInterruptedListener(p_st_slow_stdev_calculator_);
    }

    if (p_lt_slow_stdev_calculator_ != nullptr) {
      market_update_manager_.AddMarketDataInterruptedListener(p_lt_slow_stdev_calculator_);
    }

    if (p_st_slow_stdev_calculator_port_ != nullptr) {
      market_update_manager_.AddMarketDataInterruptedListener(p_st_slow_stdev_calculator_port_);
    }

    if (p_lt_slow_stdev_calculator_port_ != nullptr) {
      market_update_manager_.AddMarketDataInterruptedListener(p_lt_slow_stdev_calculator_port_);
    }
  }

  void OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value);

  inline void OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value_decrease,
                                const double& new_value_nochange, const double& new_value_increase) {
    return;
  }

  // functions
  static std::string VarName() { return "RegimeSlowStdev2"; }

  void OnMarketDataInterrupted(const unsigned int security_id, const int msecs_since_last_receive);
  void OnMarketDataResumed(const unsigned int security_id);

  void WhyNotReady();
};
}
