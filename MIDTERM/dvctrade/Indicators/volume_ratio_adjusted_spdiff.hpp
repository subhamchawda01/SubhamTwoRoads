/**
    \file Indicators/volume_ratio_adjusted_spdiff.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#pragma once

#include "dvctrade/Indicators/recent_simple_volume_measure.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/offline_returns_lrdb.hpp"

namespace HFSAT {

class RSVMRatioAdjSPDiff : public IndicatorListener,
                           public CommonIndicator,
                           public SlowStdevCalculatorListener,
                           public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  const SecurityMarketView& indep_market_view_;

  RecentSimpleVolumeMeasure* p_dep_volume_;
  std::map<int, double> offline_dep_volume_;
  double avg_dep_volume_;
  double current_dep_volume_;
  double current_dep_volume_ratio_;
  RecentSimpleVolumeMeasure* p_indep_volume_;
  std::map<int, double> offline_indep_volume_;
  double avg_indep_volume_;
  double current_indep_volume_;
  double current_indep_volume_ratio_;

  int current_slot_;

  const PriceType_t price_type_;

  // computational variables
  double moving_avg_dep_;
  double last_dep_price_;
  double current_dep_price_;

  double moving_avg_indep_;
  double last_indep_price_;
  double current_indep_price_;

  double stdev_dep_;
  double stdev_indep_;

  bool dep_interrupted_;
  bool indep_interrupted_;

  double vol_factor_;
  int lrdb_sign_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static RSVMRatioAdjSPDiff* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                               const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static RSVMRatioAdjSPDiff* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                               const SecurityMarketView& _dep_market_view_,
                                               const SecurityMarketView& _indep_market_view_,
                                               double _fractional_seconds_, double _lrdb_sign_,
                                               PriceType_t _price_type_);

 protected:
  RSVMRatioAdjSPDiff(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                     const SecurityMarketView& _dep_market_view_, const SecurityMarketView& _indep_market_view_,
                     double _fractional_seconds_, double _lrdb_sign_, PriceType_t _price_type_);

 public:
  ~RSVMRatioAdjSPDiff() {}

  void OnTimePeriodUpdate(const int num_pages_to_add_);

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  void OnIndicatorUpdate(const unsigned int& indicator_index_, const double& new_value_);
  inline void OnIndicatorUpdate(const unsigned int& indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  // functions
  static std::string VarName() { return "RSVMRatioAdjSPDiff"; }

  void WhyNotReady();

  void OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_);

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}
