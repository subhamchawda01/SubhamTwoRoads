/**
    \file Indicators/stdev_based_regime.hpp

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

#include "dvctrade/Indicators/stdev_ratio_calculator.hpp"

#include "dvctrade/OfflineUtils/periodic_l1norm_utils.hpp"

#define NUM_DAYS_HISTORY 20

namespace HFSAT {

class StdevRatioRegime : public CommonIndicator, public StdevRatioListener, public TimePeriodListener {
 protected:
  // variables
  SecurityMarketView &dep_market_view_;

  std::vector<SecurityMarketView *> indep_market_view_vec_;

  int trend_history_msecs_;

  double switch_threshold_;
  double inverse_switch_threshold_;

  double dep_Stdev_ratio_;
  double indep_Stdev_ratio_;

  double dep_source_Stdev_ratio_;

  StdevRatioCalculator *dep_Stdev_ratio_calculator_;
  StdevRatioCalculator *indep_Stdev_ratio_calculator_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string> &shortcodes_affecting_this_indicator,
                                std::vector<std::string> &ors_source_needed_vec,
                                const std::vector<const char *> &tokens);

  static StdevRatioRegime *GetUniqueInstance(DebugLogger &dbglogger, const Watch &watch,
                                             const std::vector<const char *> &tokens, PriceType_t basepx_pxtype);

  static StdevRatioRegime *GetUniqueInstance(DebugLogger &dbglogger, const Watch &watch,
                                             SecurityMarketView &dep_market_view, const std::string &indep_name,
                                             double fractional_seconds, double switch_threshold, bool scaled_stdev);

 protected:
  StdevRatioRegime(DebugLogger &dbglogger, const Watch &watch, const std::string &concise_indicator_description,
                   SecurityMarketView &dep_market_view, const std::string &indep_name, double fractional_seconds,
                   double switch_threshold, bool scaled_stdev);

 public:
  ~StdevRatioRegime() {}

  // listener interface
  void OnMarketUpdate(const unsigned int security_id, const MarketUpdateInfo &market_update_info){};
  inline void OnTradePrint(const unsigned int security_id, const TradePrintInfo &trade_print_info,
                           const MarketUpdateInfo &market_update_info) {
    OnMarketUpdate(security_id, market_update_info);
  }

  void OnTimePeriodUpdate(const int num_pages_to_add);

  void OnStdevRatioUpdate(const unsigned int index_to_send, const double &r_new_scaled_Stdev_value);

  inline void OnPortfolioPriceChange(double new_price){};
  inline void OnPortfolioPriceReset(double t_new_price, double t_old_price, unsigned int is_data_interrupted_){};

  void OnStdevUpdate(const unsigned int security_id, const double &new_stdev_value){};

  // void OnIndicatorUpdate(const unsigned int &indicator_index, const double &new_value);

  inline void OnIndicatorUpdate(const unsigned int &indicator_index, const double &new_value_decrease,
                                const double &new_value_nochange, const double &new_value_increase) {
    return;
  }

  // functions
  static std::string VarName() { return "StdevRatioRegime"; }

  void OnMarketDataInterrupted(const unsigned int security_id, const int msecs_since_last_receive);
  void OnMarketDataResumed(const unsigned int security_id);

 protected:
  void ComputeRegime();
  void InitializeValues(){};
};
}
