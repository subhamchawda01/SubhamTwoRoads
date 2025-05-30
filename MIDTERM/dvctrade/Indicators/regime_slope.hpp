/*
 * regime_slope_indicator.hpp
 *
 *  Created on: 11-Nov-2015
 *      Author: raghuram
 */

#ifndef DVCTRADE_REGIME_SLOPE_INDICATOR_HPP_
#define DVCTRADE_REGIME_SLOPE_INDICATOR_HPP_

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "dvctrade/Indicators/stdev_ratio_normalised.hpp"
#include "dvctrade/Indicators/cmvf_l1_price.hpp"

namespace HFSAT {

class RegimeSlope : public IndicatorListener, public CommonIndicator {
 protected:
  SecurityMarketView& indep1_market_view_;
  SecurityMarketView& indep2_market_view_;
  SecurityMarketView& indep3_market_view_;
  PriceType_t price_type_;

  double threshold_;
  double tolerance_;

  // unsigned int indicator_value_ = 1;
  double first_maturity_price_ = 0;
  double second_maturity_price_ = 0;

  bool indicator1_ready_ = false;
  bool indicator2_ready_ = false;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& r_shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& r_ors_source_needed_vec_,
                                const std::vector<const char*>& r_tokens_);

  static RegimeSlope* GetUniqueInstance(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                        const std::vector<const char*>& r_tokens_, PriceType_t t_basepx_pxtype_);

  static RegimeSlope* GetUniqueInstance(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                        SecurityMarketView& _indep1_market_view_,
                                        SecurityMarketView& _indep2_market_view_,
                                        SecurityMarketView& _indep3_market_view_, double _threshold_,
                                        double _tolerance_, PriceType_t _price_type_);

 protected:
  RegimeSlope(DebugLogger& r_dbglogger_, const Watch& r_watch_, const std::string& r_concise_indicator_description_,
              SecurityMarketView& _indep1_market_view_, SecurityMarketView& _indep2_market_view_,
              SecurityMarketView& _indep3_market_view_, double _threshold_, double _tolerance_,
              PriceType_t _price_type_);

 public:
  ~RegimeSlope() {}

  // listener interface
  inline void OnMarketUpdate(const unsigned int t_security_id_, const MarketUpdateInfo& r_market_update_info_) {}
  inline void OnTradePrint(const unsigned int t_security_id_, const TradePrintInfo& r_trade_print_info_,
                           const MarketUpdateInfo& r_market_update_info_) {}

  inline void OnPortfolioPriceChange(double t_new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    /*
          if (p_stdev_ratio_calculator_ != NULL) {
      market_update_manager_.AddMarketDataInterruptedListener(p_stdev_ratio_calculator_);
    }
    */
  }

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  // functions
  static std::string VarName() { return "RegimeSlope"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}
  void OnMarketDataResumed(const unsigned int _security_id_) {}

 protected:
  void InitializeValues();
  void WhyNotReady();
};
}

#endif /* DVCTRADE_REGIME_SLOPE_INDICATOR_HPP_ */
