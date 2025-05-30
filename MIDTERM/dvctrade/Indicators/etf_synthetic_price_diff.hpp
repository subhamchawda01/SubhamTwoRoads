/**
    \file Indicators/etf_synthetic_price.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"

namespace HFSAT {

/// Class returning current price minus moving average,
/// i.e. exponentially time-decaying moving average
class ETFSyntheticPriceDiff : public CommonIndicator, public IndicatorListener {
 protected:
  // variables
  SecurityMarketView* dep_market_view_;
  SecurityMarketView* indep_market_view_;
  CommonIndicator* synthetic_index_;

  const PriceType_t price_type_;

  // computational variables
  double current_indep_dep_price_diff_;
  double moving_avg_indep_dep_price_diff_;
  double last_indep_dep_price_diff_recorded_;

  double currrent_dep_price_;

  double current_indep_synthetic_price_;
  bool dep_ready_;

  ETFSyntheticPriceDiff(DebugLogger& dbglogger, const Watch& watch, const std::string& indicator_description,
                        SecurityMarketView* dep_market_view, SecurityMarketView* indep_market_view,
                        double fractional_seconds, PriceType_t price_type);

  void InitializeValues();
  void ComputeIndicatorValue();

 public:
  static void CollectShortCodes(std::vector<std::string>& shortcodes_affecting_this_indicator,
                                std::vector<std::string>& ors_source_needed_vec,
                                const std::vector<const char*>& tokens);

  static ETFSyntheticPriceDiff* GetUniqueInstance(DebugLogger& dbglogger, const Watch& watch,
                                                  const std::vector<const char*>& tokens, PriceType_t basepx_pxtype);

  static ETFSyntheticPriceDiff* GetUniqueInstance(DebugLogger& dbglogger, const Watch& watch,
                                                  SecurityMarketView* dep_market_view,
                                                  SecurityMarketView* indep_market_view, double fractional_seconds,
                                                  PriceType_t price_type);

  ~ETFSyntheticPriceDiff() {}

  // listener interface
  void OnMarketUpdate(const unsigned int sec_id, const MarketUpdateInfo& market_update_info);
  inline void OnTradePrint(const unsigned int sec_id, const TradePrintInfo& trade_print_info,
                           const MarketUpdateInfo& market_update_info) {
    OnMarketUpdate(sec_id, market_update_info);
  }

  inline void OnPortfolioPriceChange(double new_price) {}
  inline void OnPortfolioPriceReset(double new_price, double old_price, unsigned int is_data_interrupted_) {}

  void OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value);
  void OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value_decrease,
                         const double& new_value_nochange, const double& new_value_increase) {}

  // functions
  static std::string VarName() { return "ETFSyntheticPriceDiff"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int security_id, const int msecs_since_last_receive);
  void OnMarketDataResumed(const unsigned int security_id);
};
}
