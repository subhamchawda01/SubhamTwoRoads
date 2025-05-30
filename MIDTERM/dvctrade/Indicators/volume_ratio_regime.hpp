/**
    \file Indicators/volume_based_regime.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include "dvccode/ExternalData/external_time_listener.hpp"

#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"

#include "dvctrade/Indicators/core_shortcodes.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"

#include "dvctrade/Indicators/volume_ratio_calculator.hpp"
#include "dvctrade/Indicators/volume_ratio_calculator_port2.hpp"

#include "dvctrade/OfflineUtils/periodic_l1norm_utils.hpp"

#define NUM_DAYS_HISTORY 20

namespace HFSAT {

class VolumeRatioRegime : public CommonIndicator, public VolumeRatioListener, public TimePeriodListener {
 protected:
  // variables
  SecurityMarketView &dep_market_view_;

  std::vector<SecurityMarketView *> indep_market_view_vec_;

  int trend_history_msecs_;

  double switch_threshold_;
  double inverse_switch_threshold_;

  double dep_volume_ratio_;
  double indep_volume_ratio_;

  double dep_source_volume_ratio_;

  VolumeRatioCalculator *dep_volume_ratio_calculator_;
  VolumeRatioCalculator *indep_volume_ratio_calculator_;
  VolumeRatioCalculatorPort2 *indep_volume_ratio_calculator_port_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                std::vector<std::string> &_ors_source_needed_vec_,
                                const std::vector<const char *> &_tokens_);

  static VolumeRatioRegime *GetUniqueInstance(DebugLogger &_dbglogger_, const Watch &_watch_,
                                              const std::vector<const char *> &_tokens_, PriceType_t _basepx_pxtype_);

  static VolumeRatioRegime *GetUniqueInstance(DebugLogger &_dbglogger_, const Watch &_watch_,
                                              SecurityMarketView &_dep_market_view_, const std::string &indep_name,
                                              double _fractional_seconds_, double switch_threshold,
                                              PriceType_t _price_type_);

 protected:
  VolumeRatioRegime(DebugLogger &_dbglogger_, const Watch &_watch_, const std::string &concise_indicator_description_,
                    SecurityMarketView &_dep_market_view_, const std::string &indep_name, double _fractional_seconds_,
                    double switch_threshold, PriceType_t _price_type_);

 public:
  ~VolumeRatioRegime() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                           const MarketUpdateInfo &_market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  void OnTimePeriodUpdate(const int num_pages_to_add_);

  void OnVolumeRatioUpdate(const unsigned int index_to_send_, const double &r_new_scaled_volume_value_);

  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  void OnStdevUpdate(const unsigned int _security_id_, const double &_new_stdev_value_){};

  // void OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &_new_value_);

  inline void OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &new_value_decrease_,
                                const double &new_value_nochange_, const double &new_value_increase_) {
    return;
  }

  // functions
  static std::string VarName() { return "VolumeRatioRegime"; }

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void ComputeRegime();
  void InitializeValues(){};
};
}
