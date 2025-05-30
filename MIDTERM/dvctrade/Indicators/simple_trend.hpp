/**
    \file Indicators/simple_trend.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_SIMPLE_TREND_H
#define BASE_INDICATORS_SIMPLE_TREND_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvctrade/Indicators/price_portfolio.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"

namespace HFSAT {

/// Class returning current price minus moving average,
/// i.e. exponentially time-decaying moving average
class SimpleTrend : public CommonIndicator, public IndicatorListener {
 protected:
  // variables
  const SecurityMarketView* indep_market_view_;

  std::string shortcode_;
  const PriceType_t price_type_;

  // computational variables
  double moving_avg_price_;

  double last_price_recorded_;
  double current_indep_price_;
  bool is_indep_portfolio_;
  PricePortfolio* indep_portfolio_;
  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static SimpleTrend* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                        const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static SimpleTrend* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_, std::string _shortcode_,
                                        double _fractional_seconds_, PriceType_t _price_type_);

 protected:
  SimpleTrend(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
              std::string _shortcode_, double _fractional_seconds_, PriceType_t _price_type_);

 public:
  ~SimpleTrend() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void UpdateComputedVariables();
  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  double GetMovingAverage() { return moving_avg_price_; }
  bool IsReady() { return is_ready_; }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  // functions
  static std::string VarName() { return "SimpleTrend"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_SIMPLE_TREND_H
