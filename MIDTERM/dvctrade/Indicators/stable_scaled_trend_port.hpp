/**
    \file Indicators/stable_scaled_trend_port.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator_port.hpp"

namespace HFSAT {

/// Indicator that computes the trend ( current price - moving average )
/// then scales it by the standard deviation, to see what multiple of the stdev is this move
/// Is very similar to ScaledTrend, except that for computation of unbiased_l2_norm_
/// ScaledTrend uses the same time period as the one used in computation of price change
/// StableScaledTrendPort uses a flat time period of 300 seconds.
class StableScaledTrendPort : public CommonIndicator, public SlowStdevCalculatorPortListener {
 protected:
  // variables
  PCAPortPrice* const indep_portfolio_price__;

  const PriceType_t price_type_;

  SlowStdevCalculatorPort& slow_stdev_calculator_;
  double stable_stdev_value_;
  double fast_math_multiplier_;

  // computational variables
  double moving_avg_price_;

  double last_price_recorded_;
  double current_indep_price_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static StableScaledTrendPort* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                  const std::vector<const char*>& _tokens_,
                                                  PriceType_t _basepx_pxtype_);

  static StableScaledTrendPort* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                  const std::string& _portfolio_descriptor_shortcode_,
                                                  double _fractional_seconds_, PriceType_t _price_type_);

 protected:
  StableScaledTrendPort(DebugLogger& _dbglogger_, const Watch& _watch_,
                        const std::string& concise_indicator_description_,
                        const std::string& _portfolio_descriptor_shortcode_, double _fractional_seconds_,
                        PriceType_t _price_type_);

 public:
  ~StableScaledTrendPort() {}

  // listener interface
  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {}

  void OnPortfolioPriceChange(double _new_price_);
  void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_);

  void OnStdevUpdate(const double& _new_stdev_value_);

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    market_update_manager_.AddMarketDataInterruptedListener(indep_portfolio_price__);
  }

  // functions
  static std::string VarName() { return "StableScaledTrendPort"; }

  void WhyNotReady();

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}
