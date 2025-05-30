/**
    \file Indicators/stdev_adjusted_returns_diff_port.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_INDICATORS_STDEV_ADJUSTED_RETURNS_DIFF_PORT_H
#define BASE_INDICATORS_STDEV_ADJUSTED_RETURNS_DIFF_PORT_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/pcaport_price.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvctrade/Indicators/slow_stdev_returns_calculator.hpp"
#include "dvctrade/Indicators/offline_returns_lrdb.hpp"

namespace HFSAT {
class StdevAdjustedReturnsDiffPort : public CommonIndicator, public IndicatorListener {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  PCAPortPrice* indep_portfolio_price_;

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

  double returns_dep_;
  double returns_indep_;

  bool dep_interrupted_;
  bool indep_interrupted_;

  int lrdb_sign_;

  bool stdev_dep_updated_;
  bool stdev_indep_updated_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static StdevAdjustedReturnsDiffPort* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                         const std::vector<const char*>& _tokens_,
                                                         PriceType_t _basepx_pxtype_);

  static StdevAdjustedReturnsDiffPort* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                         const SecurityMarketView& _dep_market_view_,
                                                         const std::string& _portfolio_descriptor_shortcode_,
                                                         double _fractional_seconds_, double _stdev_duration_,
                                                         PriceType_t _price_type_);

 protected:
  StdevAdjustedReturnsDiffPort(DebugLogger& _dbglogger_, const Watch& _watch_,
                               const std::string& concise_indicator_description_,
                               const SecurityMarketView& _dep_market_view_,
                               const std::string& _portfolio_descriptor_shortcode_, double _fractional_seconds_,
                               double _stdev_duration_, PriceType_t _price_type_);

 public:
  ~StdevAdjustedReturnsDiffPort() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_);
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_);

  // functions
  static std::string VarName() { return "StdevAdjustedReturnsDiffPort"; }

  void WhyNotReady();

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    market_update_manager_.AddMarketDataInterruptedListener(indep_portfolio_price_);
  }

  void UpdateComputedVariables();

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_STDEV_ADJUSTED_RETURNS_DIFF_PORT_H
