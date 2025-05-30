/**
    \file Indicators/regime_online_offline_stdev_ratio.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "dvctrade/Indicators/stdev_ratio_normalised.hpp"

namespace HFSAT {

class RegimeOnlineOfflineStdevRatio : public IndicatorListener, public CommonIndicator {
 protected:
  SecurityMarketView& dep_market_view_;
  std::string source_shortcode_;
  StdevRatioNormalised* p_stdev_ratio_calculator_;

  double threshold_;
  double tolerance_;
  bool is_ready_ = false;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& r_shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& r_ors_source_needed_vec_,
                                const std::vector<const char*>& r_tokens_);

  static RegimeOnlineOfflineStdevRatio* GetUniqueInstance(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                                          const std::vector<const char*>& r_tokens_,
                                                          PriceType_t t_basepx_pxtype_);

  static RegimeOnlineOfflineStdevRatio* GetUniqueInstance(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                                          SecurityMarketView& r_dep_market_view_,
                                                          std::string r_source_shortcode_, double _stdev_duration_,
                                                          double _threshold_, double _tolerance_);

 protected:
  RegimeOnlineOfflineStdevRatio(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                const std::string& r_concise_indicator_description_,
                                SecurityMarketView& r_dep_market_view_, std::string r_source_shortcode_,
                                double _stdev_duration_, double _threshold_, double _tolerance_);

 public:
  ~RegimeOnlineOfflineStdevRatio() {}

  // listener interface
  inline void OnMarketUpdate(const unsigned int t_security_id_, const MarketUpdateInfo& r_market_update_info_) {}
  inline void OnTradePrint(const unsigned int t_security_id_, const TradePrintInfo& r_trade_print_info_,
                           const MarketUpdateInfo& r_market_update_info_) {}

  inline void OnPortfolioPriceChange(double t_new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    if (p_stdev_ratio_calculator_ != NULL) {
      market_update_manager_.AddMarketDataInterruptedListener(p_stdev_ratio_calculator_);
    }
  }

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  // functions
  static std::string VarName() { return "RegimeOnlineOfflineStdevRatio"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}
  void OnMarketDataResumed(const unsigned int _security_id_) {}

 protected:
  void InitializeValues();
  void WhyNotReady();
};
}
