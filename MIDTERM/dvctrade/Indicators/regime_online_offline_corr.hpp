/**
    \file Indicators/regime_online_offline_corr.cpp

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
#include "dvctrade/Indicators/online_beta_trend.hpp"
#include "dvctrade/Indicators/stdev_ratio_normalised.hpp"
#include "dvctrade/Indicators/offline_returns_lrdb.hpp"

namespace HFSAT {

class RegimeOnlineOfflineCorr : public IndicatorListener, public CommonIndicator, public TimePeriodListener {
 protected:
  SecurityMarketView& dep_market_view_;
  // SecurityMarketView & indep_market_view_ ;
  std::string source_shortcode_;
  OnlineBetaTrend* p_online_beta_calculator_;
  StdevRatioNormalised* p_online_offline_stdev_ratio_;

  OfflineReturnsLRDB& lrdb_;
  LRInfo current_lrinfo_;
  int last_lrinfo_updated_msecs_;

  double online_beta_;
  double online_offline_stdev_ratio_;
  bool beta_updated_;
  bool stdev_ratio_updated_;

  double threshold_;
  double tolerance_;
  bool is_ready_ = false;
  bool lrdb_absent_ = false;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& r_shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& r_ors_source_needed_vec_,
                                const std::vector<const char*>& r_tokens_);

  static RegimeOnlineOfflineCorr* GetUniqueInstance(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                                    const std::vector<const char*>& r_tokens_,
                                                    PriceType_t t_basepx_pxtype_);

  static RegimeOnlineOfflineCorr* GetUniqueInstance(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                                    SecurityMarketView& r_dep_market_view_,
                                                    std::string r_source_shortcode_, const unsigned int _beta_secs_,
                                                    double t_threshold_, double t_tolerance_);

 protected:
  RegimeOnlineOfflineCorr(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                          const std::string& r_concise_indicator_description_, SecurityMarketView& r_dep_market_view_,
                          std::string r_source_shortcode_, const unsigned int _beta_secs_, double t_threshold_,
                          double t_tolerance_);

 public:
  ~RegimeOnlineOfflineCorr() {}

  // listener interface
  inline void OnMarketUpdate(const unsigned int t_security_id_, const MarketUpdateInfo& r_market_update_info_) {}
  inline void OnTradePrint(const unsigned int t_security_id_, const TradePrintInfo& r_trade_print_info_,
                           const MarketUpdateInfo& r_market_update_info_) {}

  inline void OnPortfolioPriceChange(double t_new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    if (p_online_beta_calculator_ != NULL) {
      market_update_manager_.AddMarketDataInterruptedListener(p_online_beta_calculator_);
    }
    if (p_online_offline_stdev_ratio_ != NULL) {
      market_update_manager_.AddMarketDataInterruptedListener(p_online_offline_stdev_ratio_);
    }
  }

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  void OnTimePeriodUpdate(const int num_pages_to_add_) { UpdateLRInfo(); }

  // functions
  static std::string VarName() { return "RegimeOnlineOfflineCorr"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}
  void OnMarketDataResumed(const unsigned int _security_id_) {}

  inline double OfflineBeta() const { return current_lrinfo_.lr_coeff_; }

 protected:
  void InitializeValues();
  void UpdateLRInfo();
  void WhyNotReady();
};
}
