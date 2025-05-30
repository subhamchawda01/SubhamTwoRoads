/**
    \file Indicators/stdev_l1_bias.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_STDEV_L1_BIAS_H
#define BASE_INDICATORS_STDEV_L1_BIAS_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvctrade/Indicators/price_portfolio.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"

namespace HFSAT {

/// Class returning current price minus moving average,
/// i.e. exponentially time-decaying moving average
class StdevL1Bias : public CommonIndicator, public IndicatorListener {
 protected:
  // variables
  const SecurityMarketView* indep_market_view_;

  std::string shortcode_;
  const PriceType_t price_type_;

  // computational variables
  double moving_avg_bias_;
  double moving_avg_bias_square_;
  double current_bias_;
  double last_bias_recorded_;
  double stdev_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static StdevL1Bias* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                        const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static StdevL1Bias* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_, std::string _shortcode_,
                                        double _fractional_seconds_, PriceType_t _price_type_);

 protected:
  StdevL1Bias(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
              std::string _shortcode_, double _fractional_seconds_, PriceType_t _price_type_);

 public:
  ~StdevL1Bias() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {}
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  double GetStdev() { return stdev_; }
  bool IsReady() { return is_ready_; }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  // functions
  static std::string VarName() { return "StdevL1Bias"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_STDEV_L1_BIAS_H
