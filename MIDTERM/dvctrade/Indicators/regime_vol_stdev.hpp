/**
    \file Indicators/regime_vol_stdev.hpp

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
#include "dvctrade/Indicators/volume_ratio_calculator_port.hpp"
#include "dvctrade/Indicators/volume_ratio_calculator.hpp"
#include "dvctrade/Indicators/l1_size_trend.hpp"
namespace HFSAT {

/// Class returning current price minus moving average,
/// i.e. exponentially time-decaying moving average
class RegimeVolStdev : public IndicatorListener, public CommonIndicator {
 protected:
  SecurityMarketView& indep_market_view_;
  SlowStdevCalculator* p_slow_stdev_calculator;
  VolumeRatioCalculator* volume_ratio_calculator_;

  double volume_switch_threshold_;
  double stdev_switch_threshold_;
  double tolerance_;

  double current_volume_ratio_;
  double current_stdev_ratio_;

  double avg_stdev_;
  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static RegimeVolStdev* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                           const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static RegimeVolStdev* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                           SecurityMarketView& _indep_market_view_, double fractional_sec,
                                           double volume_switch_threshold_, double stdev_switch_threshold_,
                                           double tolerance_);

 protected:
  RegimeVolStdev(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                 SecurityMarketView& _indep_market_view_, double fractional_sec, double volume_switch_threshold,
                 double stdev_switch_threshold_, double tolerance_);

 public:
  ~RegimeVolStdev() {}

  // listener interface
  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  }  ///< from CommonIndicator::SecurityMarketViewChangeListener
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
  }  ///< from CommonIndicator::SecurityMarketViewChangeListener

  inline void OnPortfolioPriceChange(double _new_price_) {}  ///< from CommonIndicator::PortfolioPriceChangeListener
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    if (p_slow_stdev_calculator != nullptr)
      market_update_manager_.AddMarketDataInterruptedListener(p_slow_stdev_calculator);
    if (volume_ratio_calculator_ != nullptr)
      market_update_manager_.AddMarketDataInterruptedListener(volume_ratio_calculator_);
  }

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);

  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  // functions
  static std::string VarName() { return "RegimeVolStdev"; }
  void InitializeValues();
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

  void WhyNotReady();
};
}
