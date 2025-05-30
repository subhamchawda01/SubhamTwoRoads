/**
    \file Indicators/avg_abs_trend.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 354, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_AVG_ABS_TREND_H
#define BASE_INDICATORS_AVG_ABS_TREND_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"

namespace HFSAT {

/**
 *
 */
class AvgAbsTrend : public CommonIndicator, public IndicatorListener {
 protected:
  // variables
  const SecurityMarketView& indep_market_view_;

  const PriceType_t price_type_;

  // computational variables
  double moving_avg_abs_trend_;

  double last_abs_trend_recorded_;
  double current_indep_abs_trend_;
  SimpleTrend* p_indep_indicator_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static AvgAbsTrend* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                        const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static AvgAbsTrend* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                        const SecurityMarketView& _indep_market_view_, double _trend_seconds_,
                                        double _average_seconds_, PriceType_t _price_type_);

 protected:
  AvgAbsTrend(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
              const SecurityMarketView& _indep_market_view_, double _trend_seconds_, double _average_seconds_,
              PriceType_t _price_type_);

 public:
  ~AvgAbsTrend() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {}

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  // functions
  static std::string VarName() { return "AvgAbsTrend"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_AVG_ABS_TREND_H
