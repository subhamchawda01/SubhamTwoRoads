/**
    \file Indicators/stud_price_trend_diff.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_INDICATORS_VOLUME_WEIGHTED_STUD_PRICE_RETURNS_DIFF_H
#define BASE_INDICATORS_VOLUME_WEIGHTED_STUD_PRICE_RETURNS_DIFF_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvctrade/Indicators/slow_stdev_returns_calculator.hpp"
#include "dvctrade/Indicators/offline_returns_lrdb.hpp"
#include "dvctrade/Indicators/volume_ratio_calculator.hpp"
#include "dvctrade/Indicators/recent_scaled_volume_calculator.hpp"

namespace HFSAT {

/**
 * Scale the value of the indicator according to the volume of the source
 * Not adding it for source as volume of portfolio is not defined properly
 */
class VolumeWeightedStudPriceReturnsDiff : public CommonIndicator,
                                           public IndicatorListener,
                                           public VolumeRatioListener,
                                           public RecentScaledVolumeListener {
 protected:
  // variables
  const SecurityMarketView &dep_market_view_;
  const SecurityMarketView &indep_market_view_;

  SlowStdevReturnsCalculator *dep_stdev_trend_calculator_;
  SlowStdevReturnsCalculator *indep_stdev_trend_calculator_;

  RecentScaledVolumeCalculator *recent_scaled_volume_calculator_;
  VolumeRatioCalculator *volume_ratio_calculator_;

  const PriceType_t price_type_;

  // computational variables
  double volume_ratio_exponent_;

  double moving_avg_dep_;
  double last_dep_price_;
  double current_dep_price_;

  double moving_avg_indep_;
  double last_indep_price_;
  double current_indep_price_;

  double stdev_dep_;
  double stdev_indep_;

  double returns_indep_;
  double returns_dep_;

  double volume_ratio_;
  double scaled_volume_;
  double volume_factor_;

  bool dep_interrupted_;
  bool indep_interrupted_;

  int lrdb_sign_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string> &shortcodes_affecting_this_indicator,
                                std::vector<std::string> &ors_source_needed_vec,
                                const std::vector<const char *> &tokens);

  static VolumeWeightedStudPriceReturnsDiff *GetUniqueInstance(DebugLogger &_dbglogger_, const Watch &t_watch,
                                                               const std::vector<const char *> &tokens,
                                                               PriceType_t _basepx_pxtype_);

  static VolumeWeightedStudPriceReturnsDiff *GetUniqueInstance(DebugLogger &_dbglogger_, const Watch &t_watch,
                                                               const SecurityMarketView &dep_market_view,
                                                               const SecurityMarketView &indep_market_view,
                                                               double indicator_duration, double stdev_duration,
                                                               double volume_ratio_duration,
                                                               double volume_ratio_exponent, PriceType_t price_type);

 protected:
  VolumeWeightedStudPriceReturnsDiff(DebugLogger &_dbglogger_, const Watch &t_watch,
                                     const std::string &t_concise_indicator_description,
                                     const SecurityMarketView &dep_market_view,
                                     const SecurityMarketView &indep_market_view, double indicator_duration,
                                     double stdev_duration, double volume_ratio_duration, double volume_ratio_exponent,
                                     PriceType_t price_type);

 public:
  ~VolumeWeightedStudPriceReturnsDiff() {}

  // listener interface
  void OnMarketUpdate(const unsigned int t_security_id, const MarketUpdateInfo &market_update_info);
  inline void OnTradePrint(const unsigned int t_security_id, const TradePrintInfo &trade_print_info,
                           const MarketUpdateInfo &market_update_info) {
    OnMarketUpdate(t_security_id, market_update_info);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  void OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &_new_value_);
  inline void OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &new_value_decrease_,
                                const double &new_value_nochange_, const double &new_value_increase_) {
    return;
  }

  void OnVolumeRatioUpdate(const unsigned int security_id, const double &new_volume_ratio);
  void OnScaledVolumeUpdate(const unsigned int security_id, const double &new_scaled_volume);

  // functions
  static std::string VarName() { return "VolumeWeightedStudPriceReturnsDiff"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int t_security_id, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int t_security_id);

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_STUD_PRICE_RETURNS_DIFF_H
