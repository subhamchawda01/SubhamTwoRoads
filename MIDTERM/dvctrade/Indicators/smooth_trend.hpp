/**
    \file Indicators/smooth_trend.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_INDICATORS_SMOOTH_TREND_H
#define BASE_INDICATORS_SMOOTH_TREND_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"

namespace HFSAT {

class SmoothTrend : public CommonIndicator {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  const PriceType_t price_type_;

  int lt_trend_history_msecs_;
  int st_trend_history_msecs_;

  // computational variables
  int lt_last_new_page_msecs_;
  int lt_page_width_msecs_;
  double lt_decay_page_factor_;
  std::vector<double> lt_decay_vector_;
  double lt_inv_decay_sum_;
  std::vector<double> lt_decay_vector_sums_;

  int st_last_new_page_msecs_;
  int st_page_width_msecs_;
  double st_decay_page_factor_;
  std::vector<double> st_decay_vector_;
  double st_inv_decay_sum_;
  std::vector<double> st_decay_vector_sums_;

  double lt_moving_avg_;
  double st_moving_avg_;
  double last_dep_price_;
  double current_dep_price_;

  bool dep_interrupted_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static SmoothTrend* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                        const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static SmoothTrend* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                        const SecurityMarketView& _dep_market_view_, double _lt_fractional_secs_,
                                        double _st_fractional_secs_, PriceType_t _price_type_);

 protected:
  SmoothTrend(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
              const SecurityMarketView& _dep_market_view_, double _lt_fractional_secs_, double _st_fractional_seconds_,
              PriceType_t _price_type_);

 public:
  ~SmoothTrend() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  // functions
  static std::string VarName() { return "SmoothTrend"; }

  void WhyNotReady();

  void OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_);

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void SetTimeDecayWeights();
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_STUDENT_PRICE_H
