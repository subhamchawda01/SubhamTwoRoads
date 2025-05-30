/**
    \file Indicators/volume_weighted_simple_trend.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_INDICATORS_VOLUME_WEIGHTED_SIMPLE_TREND_H
#define BASE_INDICATORS_VOLUME_WEIGHTED_SIMPLE_TREND_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvctrade/Indicators/volume_ratio_calculator.hpp"
#include "dvctrade/Indicators/recent_scaled_volume_calculator.hpp"

namespace HFSAT {

/// Class returning current price minus moving average,
/// i.e. exponentially time-decaying moving average
class VolumeWeightedSimpleTrend : public CommonIndicator,
                                  public VolumeRatioListener,
                                  public RecentScaledVolumeListener {
 protected:
  // variables
  const SecurityMarketView& indep_market_view_;

  const PriceType_t price_type_;

  // computational variables
  double moving_avg_price_;

  double last_price_recorded_;
  double current_indep_price_;

  RecentScaledVolumeCalculator& recent_scaled_volume_calculator_;
  VolumeRatioCalculator& volume_ratio_calculator_;

  double current_volume_ratio_;
  double scaled_volume_;
  double volume_factor_;
  double volume_ratio_exponent_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static VolumeWeightedSimpleTrend* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                      const std::vector<const char*>& _tokens_,
                                                      PriceType_t _basepx_pxtype_);

  static VolumeWeightedSimpleTrend* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                      const SecurityMarketView& _indep_market_view_,
                                                      double _fractional_seconds_, double _volume_ratio_duration_,
                                                      double _volume_ratio_exponent_, PriceType_t _price_type_);

 protected:
  VolumeWeightedSimpleTrend(DebugLogger& _dbglogger_, const Watch& _watch_,
                            const std::string& concise_indicator_description_,
                            const SecurityMarketView& _indep_market_view_, double _fractional_seconds_,
                            double _volume_ratio_duration_, double _volume_ratio_exponent_, PriceType_t _price_type_);

 public:
  ~VolumeWeightedSimpleTrend() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  void OnVolumeRatioUpdate(const unsigned int r_security_id_, const double& r_new_volume_ratio_);

  void OnScaledVolumeUpdate(const unsigned int r_security_id_, const double& r_new_scaled_volume_);

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  // functions
  static std::string VarName() { return "VolumeWeightedSimpleTrend"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_VOLUME_WEIGHTED_SIMPLE_TREND_H
