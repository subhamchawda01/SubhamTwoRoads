/**
    \file Indicators/stable_scaled_trend.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/slow_stdev_trend_calculator.hpp"

namespace HFSAT {

/// Indicator that computes the trend ( current price - moving average )
/// then scales it by the standard deviation of the trend, to see what multiple of the stdev is this move
/// Is very similar to ScaledTrend, except that for computation of unbiased_l2_norm_
/// ScaledTrend uses the same time period as the one used in computation of price change
/// StabledScaledTredVersion2 takes that time period as one of the input.
class StableScaledTrendVersion2 : public CommonIndicator, public IndicatorListener {
 protected:
  // variables
  SecurityMarketView& indep_market_view_;

  const PriceType_t price_type_;

  SlowStdevTrendCalculator* slow_stdev_trend_calculator_;
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

  static StableScaledTrendVersion2* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                      const std::vector<const char*>& _tokens_,
                                                      PriceType_t _basepx_pxtype_);

  static StableScaledTrendVersion2* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                      SecurityMarketView& _indep_market_view_,
                                                      double _fractional_seconds_, double _stdev_duration_,
                                                      PriceType_t _price_type_);

 protected:
  StableScaledTrendVersion2(DebugLogger& _dbglogger_, const Watch& _watch_,
                            const std::string& concise_indicator_description_, SecurityMarketView& _indep_market_view_,
                            double _fractional_seconds_, double _stdev_duration_, PriceType_t _price_type_);

 public:
  ~StableScaledTrendVersion2() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  static std::string VarName() { return "StableScaledTrendVersion2"; }

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}
