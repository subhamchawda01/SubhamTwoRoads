/**
    \file Indicators/offline_low_correlation_pairs_port.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_OFFLINE_LOW_CORRELATION_PAIRS_PORT_H
#define BASE_INDICATORS_OFFLINE_LOW_CORRELATION_PAIRS_PORT_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/pcaport_price.hpp"
#include "dvctrade/Indicators/offline_returns_lrdb.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"
#include "dvctrade/Indicators/simple_trend_port.hpp"

namespace HFSAT {

/// Indicator very much like OfflineComputedPairsPort and OfflineLowCorrelationPairs
class OfflineLowCorrelationPairsPort : public IndicatorListener,
                                       public CommonIndicator,
                                       public SlowStdevCalculatorListener,
                                       public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;

  PCAPortPrice& indep_portfolio_price_;

  double dep_price_trend_;
  double indep_price_trend_;

  SimpleTrend* p_dep_indicator_;
  SimpleTrendPort* p_indep_indicator_;

  OfflineReturnsLRDB& lrdb_;
  int last_lrinfo_updated_msecs_;
  LRInfo current_lrinfo_;
  double current_projection_multiplier_;
  double current_projected_trend_;
  double sd_adj_current_projected_trend_;

  /// Right now made the stdev comparison on dependant
  /// TODO make a SlowStdevCalculatorPort ( portfolio version of SlowStdevCalculator )
  /// and change the comparison of magnitude from stdev of deopendant to
  /// stdev of indep_portfolio
  SlowStdevCalculator& slow_dep_stdev_calculator_;
  double stable_dep_stdev_value_;
  double sqrt_time_factor_;  ///< assuming that the stdev of price change for time _fractional_seconds_ = stdev of price
  /// change for 300 seconds * sqrt ( _fractional_seconds_ / 300 )
  double stdev_high_mult_factor_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& t_shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& t_ors_source_needed_vec_,
                                const std::vector<const char*>& r_tokens_);

  static OfflineLowCorrelationPairsPort* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                           const std::vector<const char*>& r_tokens_,
                                                           PriceType_t _basepx_pxtype_);

  static OfflineLowCorrelationPairsPort* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                           SecurityMarketView& r_dep_market_view_,
                                                           std::string t_portfolio_descriptor_shortcode_,
                                                           double t_fractional_seconds_, PriceType_t t_price_type_);

 protected:
  OfflineLowCorrelationPairsPort(DebugLogger& _dbglogger_, const Watch& _watch_,
                                 const std::string& concise_indicator_description_,
                                 SecurityMarketView& _dep_market_view_, std::string _portfolio_descriptor_shortcode_,
                                 double _fractional_seconds_, PriceType_t _price_type_);

 public:
  ~OfflineLowCorrelationPairsPort() {}

  void OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_);

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_) { UpdateLRInfo(); }

  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_){};
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};
  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    market_update_manager_.AddMarketDataInterruptedListener(&indep_portfolio_price_);
    if (p_indep_indicator_ != NULL) market_update_manager_.AddMarketDataInterruptedListener(p_indep_indicator_);
  }
  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);
  // functions

  /// Used in ModelCreator to see which variable is in the model file
  static std::string VarName() { return "OfflineLowCorrelationPairsPort"; }

 protected:
  void InitializeValues();
  void UpdateLRInfo();

  /// function to compute current_projection_multiplier_.  This is same in OfflineLowCorrelationPairsPort as
  /// OfflineComputedPairsPort
  inline void ComputeMultiplier() { current_projection_multiplier_ = current_lrinfo_.lr_coeff_; }
};
}

#endif  // BASE_INDICATORS_OFFLINE_LOW_CORRELATION_PAIRS_PORT_H
