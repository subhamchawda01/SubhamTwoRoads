/**
   \file Indicators/source_switch_regime_port.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator_port.hpp"
#include "dvctrade/Indicators/volume_ratio_calculator_port.hpp"
#include "dvctrade/Indicators/recent_scaled_volume_calculator_port.hpp"
#include "dvctrade/OfflineUtils/periodic_stdev_utils.hpp"

#define NUM_DAYS_HISTORY 20

namespace HFSAT {

/// Indicates the regime ( 1 or 2) based on self volatility and source volatility.
class SrcSwitchRegPort : public CommonIndicator,
                         public VolumeRatioListener,
                         public RecentScaledVolumeListener,
                         public SlowStdevCalculatorListener,
                         public SlowStdevCalculatorPortListener,
                         public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  PCAPortPrice& indep_portfolio_price_;

  SlowStdevCalculatorPort& slow_stdev_calculator_port_;
  SlowStdevCalculator& slow_stdev_calculator_;

  double port_stdev_;
  double dep_stdev_;

  VolumeRatioCalculator& volume_ratio_calculator_;
  RecentScaledVolumeCalculator& recent_scaled_volume_calculator_;

  VolumeRatioCalculatorPort& volume_ratio_calculator_port_;
  RecentScaledVolumeCalculatorPort& recent_scaled_volume_calculator_port_;

  double volume_exponent_;

  double current_volume_ratio_dep_;
  double scaled_volume_dep_;
  double volume_factor_dep_;

  double current_volume_ratio_port_;
  double scaled_volume_port_;
  double volume_factor_port_;

  double dep_mean_stdev_;
  double port_mean_stdev_;

  double port_thresh_;
  double dep_thresh_;
  double tolerance_;

 protected:
  SrcSwitchRegPort(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                   const SecurityMarketView& _dep_market_view_, const std::string& _portfolio_descriptor_shortcode_,
                   double _trend_duration_, double _port_thresh_fact_, double _dep_thresh_fact_,
                   double _volume_exponent_, double tolerance_, PriceType_t _price_type_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static SrcSwitchRegPort* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static SrcSwitchRegPort* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             const SecurityMarketView& _dep_market_view_,
                                             const std::string& _portfolio_descriptor_shortcode_,
                                             double _trend_duration_, double _port_thresh_fact_,
                                             double _dep_thresh_fact_, double _volume_exponent_, double tolerance_,
                                             PriceType_t _price_type_);

  ~SrcSwitchRegPort() {}

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_);
  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_){};
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  void OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_);
  void OnStdevUpdate(const double& _new_stdev_value_);

  void OnVolumeRatioUpdate(const unsigned int r_security_id_, const double& r_new_volume_ratio_);
  void OnScaledVolumeUpdate(const unsigned int r_security_id_, const double& r_new_scaled_volume_);

  // functions
  /// Used in ModelCreator to see which shortcodes are core
  bool GetReadinessRequired(const std::string& r_dep_shortcode_, const std::vector<const char*>& tokens_) const {
    std::vector<std::string> core_shortcodes_;
    GetCoreShortcodes(r_dep_shortcode_, core_shortcodes_);
    // here tokes 3 and 4 are important
    // if ( ( tokens_.size() > 3u ) &&
    // 	   ( VectorUtils::LinearSearchValue ( core_shortcodes_, std::string(tokens_[3]) ) ) )
    // 	{ return true ; }
    if ((tokens_.size() > 4u) && (VectorUtils::LinearSearchValue(core_shortcodes_, std::string(tokens_[4])))) {
      return true;
    }
    return false;
  }

  static std::string VarName() { return "SrcSwitchRegPort"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}
