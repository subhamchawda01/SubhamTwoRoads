/**
    \file Indicators/smooth_zscore.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_INDICATORS_SMOOTH_ZSCORE_H
#define BASE_INDICATORS_SMOOTH_ZSCORE_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "dvctrade/Indicators/smooth_trend.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator.hpp"

namespace HFSAT {

class SmoothZscore : public IndicatorListener, public CommonIndicator, public SlowStdevCalculatorListener {
 protected:
  // variables
  const SecurityMarketView& indep_market_view_;
  SlowStdevCalculator& slow_stdev_calculator_;
  double stable_stdev_value_;
  SmoothTrend* p_indicator_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static SmoothZscore* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                         const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static SmoothZscore* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                         const SecurityMarketView& _indep_market_view_, double _lt_fractional_seconds_,
                                         double _st_fractional_secs_, PriceType_t _price_type_);

 protected:
  SmoothZscore(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
               const SecurityMarketView& _indep_market_view_, double _lt_fractional_secs_,
               double _st_fractional_seconds_, PriceType_t _price_type_);

 public:
  ~SmoothZscore() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {}

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    if (p_indicator_ != NULL) market_update_manager_.AddMarketDataInterruptedListener(p_indicator_);
  }

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }
  // functions
  static std::string VarName() { return "SmoothZscore"; }

  void WhyNotReady();

  void OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_);

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_STUDENT_PRICE_H
