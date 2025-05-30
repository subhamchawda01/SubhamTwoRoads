/**
    \file Indicators/slow_stdev_trend_calculator.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_INDICATORS_SLOW_STDEV_TREND_CALCULATOR_H
#define BASE_INDICATORS_SLOW_STDEV_TREND_CALCULATOR_H

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"
#include "dvctrade/Indicators/simple_trend_port.hpp"

namespace HFSAT {

// Class returning stdev of trend of price of shortcode or portfolio
class SlowStdevTrendCalculator : public CommonIndicator, public IndicatorListener {
 protected:
  // variables
  const SecurityMarketView* indep_market_view_;
  std::string source_shortcode_;
  CommonIndicator* indep_trend_indicator_;
  PriceType_t price_type_;
  bool is_price_portfolio_;
  // computational variables
  double moving_avg_trend_;
  double moving_avg_squared_trend_;

  double last_trend_recorded_;
  double stdev_value_;

  double current_indep_trend_;
  /// since sqrt of positive number unbiased_l2_norm_ is a denominator, we should have a min threshold
  /// below which there has been so little movement that a BollingerBand sort of math would not make sense
  /// and hence the movement is best described as independant of unbiased_l2_norm_
  double min_unbiased_l2_norm_;

  // functions
 public:
  static SlowStdevTrendCalculator* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                     std::string _source_shortcode_,
                                                     const unsigned int _indicator_duration_,
                                                     const unsigned int t_trend_history_secs_,
                                                     PriceType_t _t_price_type_,
                                                     double t_min_stdev_value_factor_ = 0.01);

 protected:
  SlowStdevTrendCalculator(DebugLogger& _dbglogger_, const Watch& _watch_,
                           const std::string& concise_indicator_description_, std::string _source_shortcode_,
                           const unsigned int _indicator_duration_, const unsigned int t_trend_history_secs_,
                           PriceType_t _t_price_type_, const double t_min_stdev_value_factor_);

 public:
  ~SlowStdevTrendCalculator() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double _new_price_, double _old_price_, unsigned int is_data_interrupted_) {}

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);

  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  // functions
  static std::string VarName() { return "SlowStdevTrendCalculator"; }
  inline double stdev_value() const { return stdev_value_; }
  inline unsigned int trend_history_msecs() const { return trend_history_msecs_; }

  static SlowStdevTrendCalculator* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                     const std::vector<const char*>& r_tokens_,
                                                     PriceType_t _basepx_pxtype_);
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& r_tokens_);

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_SLOW_STDEV_TREND_CALCULATOR_H
