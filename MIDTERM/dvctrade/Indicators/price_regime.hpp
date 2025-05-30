/*
 * price_regime.hpp
 *
 *  Created on: 04-Mar-2016
 *      Author: raghuram
 */

#ifndef DVCTRADE_INDICATORS_PRICE_REGIME_HPP_
#define DVCTRADE_INDICATORS_PRICE_REGIME_HPP_

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"

namespace HFSAT {

class PriceRegime : public IndicatorListener, public CommonIndicator {
 protected:
  SecurityMarketView& sec_market_view_;

  double half_life_ = 0;
  std::vector<double> threshold_vec_;  // should be sorted vector
  double tolerance_ = 0.05;
  double current_price_ = 0;
  PriceType_t price_type_;
  bool indicator_ready_ = false;
  unsigned int current_regime_index_ = 1;
  int lower_bound_ = 0;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& r_shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& r_ors_source_needed_vec_,
                                const std::vector<const char*>& r_tokens_);

  static PriceRegime* GetUniqueInstance(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                        const std::vector<const char*>& r_tokens_, PriceType_t t_basepx_pxtype_);

  static PriceRegime* GetUniqueInstance(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                        SecurityMarketView& _sec_market_view_, double _half_life_,
                                        std::vector<double> _threshold_vec_, double _tolerance_,
                                        PriceType_t _price_type_);

 protected:
  PriceRegime(DebugLogger& r_dbglogger_, const Watch& r_watch_, const std::string& r_concise_indicator_description_,
              SecurityMarketView& _sec_market_view_, double _half_life_, std::vector<double> _threshold_vec_,
              double _tolerance_, PriceType_t _price_type_);

 public:
  ~PriceRegime() {}

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
  static std::string VarName() { return "PriceRegime"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}
  void OnMarketDataResumed(const unsigned int _security_id_) {}

 protected:
  void InitializeValues();
  void WhyNotReady();
};
}

#endif /* DVCTRADE_INDICATORS_PRICE_REGIME_HPP_ */
