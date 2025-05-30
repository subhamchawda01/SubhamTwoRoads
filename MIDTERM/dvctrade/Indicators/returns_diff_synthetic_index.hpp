/**
    \file Indicators/returns_diff_synthetic_index.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvctrade/Indicators/simple_returns.hpp"
#include "dvctrade/Indicators/synthetic_index.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"

namespace HFSAT {

/// Class returning current price minus moving average,
/// i.e. exponentially time-decaying moving average
class ReturnsDiffSyntheticIndex : public IndicatorListener, public CommonIndicator {
 protected:
  // variables
  const SecurityMarketView& indep_market_view_;
  SyntheticIndex* synth_index;
  SimpleReturns* returns_index;

  const PriceType_t price_type_;

  // computational variables
  double moving_avg_synthetic_price_;
  double synth_index_value_;
  double returns_index_value_;

  double last_synthetic_price_recorded_;
  double current_synthetic_price_;

  TimeDecayCalculator time_decay_calculator_;

  int use_fut_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static ReturnsDiffSyntheticIndex* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                      const std::vector<const char*>& _tokens_,
                                                      PriceType_t _basepx_pxtype_);

  static ReturnsDiffSyntheticIndex* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                      const SecurityMarketView& _indep_market_view_,
                                                      double _fractional_seconds_, int _use_fut_,
                                                      PriceType_t _price_type_);

 protected:
  ReturnsDiffSyntheticIndex(DebugLogger& _dbglogger_, const Watch& _watch_,
                            const std::string& concise_indicator_description_,
                            const SecurityMarketView& _indep_market_view_, double _fractional_seconds_, int _use_fut_,
                            PriceType_t _price_type_);

 public:
  ~ReturnsDiffSyntheticIndex() {}

  // listener interface
  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    if (synth_index != NULL) market_update_manager_.AddMarketDataInterruptedListener(synth_index);

    if (returns_index != NULL) market_update_manager_.AddMarketDataInterruptedListener(returns_index);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  // functions
  static std::string VarName() { return "ReturnsDiffSyntheticIndex"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}
