/**
    \file Indicators/second_derivative.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_SECOND_DERIVATIVE_H
#define BASE_INDICATORS_SECOND_DERIVATIVE_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"

namespace HFSAT {

/// Class returning current price minus moving average,
/// i.e. exponentially time-decaying moving average
class SecondDerivative : public IndicatorListener, public CommonIndicator {
 protected:
  const SecurityMarketView& indep_market_view_;

  double st_trend_value_;
  double lt_trend_value_;

  std::vector<CommonIndicator*> indicator_vec_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static SecondDerivative* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static SecondDerivative* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                             const SecurityMarketView& _indep_market_view_,
                                             double _fractional_st_seconds_, double _fractional_lt_seconds_,
                                             PriceType_t _price_type_);

 protected:
  SecondDerivative(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                   const SecurityMarketView& _indep_market_view_, double _fractional_st_seconds_,
                   double _fractional_lt_seconds_, PriceType_t _price_type_);

 public:
  ~SecondDerivative() {}

  // listener interface
  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  }  ///< from CommonIndicator::SecurityMarketViewChangeListener
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
  }                                                          ///< from CommonIndicator::SecurityMarketViewChangeListener
  inline void OnPortfolioPriceChange(double _new_price_) {}  ///< from CommonIndicator::PortfolioPriceChangeListener
  inline void OnPortfolioPriceReset(double _new_price_, double _old_price_, unsigned int is_data_interrupted_) {
  }  ///< from CommonIndicator::PortfolioPriceChangeListener
  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    for (auto i = 0u; i < indicator_vec_.size(); i++) {
      if (indicator_vec_[i] != NULL) {
        market_update_manager_.AddMarketDataInterruptedListener(indicator_vec_[i]);
      }
    }
  }

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
  // functions

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  // functions
  static std::string VarName() { return "SecondDerivative"; }

  void WhyNotReady();

 protected:
};
}

#endif  // BASE_INDICATORS_SECOND_DERIVATIVE_H
