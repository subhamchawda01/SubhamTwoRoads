/**
    \file Indicators/offline_low_correlation_pairs.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_OFFLINE_LOW_CORRELATION_PAIRS_H
#define BASE_INDICATORS_OFFLINE_LOW_CORRELATION_PAIRS_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"
#include "dvctrade/Indicators/offline_returns_lrdb.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator.hpp"

namespace HFSAT {

/// Very similar to OfflineComputedPairs, but focusing on pairs whose correlation has
/// a lowish magnitude like <= 0.60, it might be best to return 0 when the independant
/// has had a price change that is not outstandingly high in magnitude.
/// In case the independant has moved a lot more than normal for that duration
/// the return value will be same as OfflineComputedPairs
class OfflineLowCorrelationPairs : public IndicatorListener,
                                   public CommonIndicator,
                                   public SlowStdevCalculatorListener,
                                   public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  const SecurityMarketView& indep_market_view_;

  double dep_price_trend_;
  double indep_price_trend_;

  OfflineReturnsLRDB& lrdb_;
  int last_lrinfo_updated_msecs_;
  LRInfo current_lrinfo_;
  double current_projection_multiplier_;
  double current_projected_trend_;

  SlowStdevCalculator& slow_stdev_calculator_;
  SimpleTrend* p_dep_indicator_;
  SimpleTrend* p_indep_indicator_;

  double stable_stdev_value_;
  double sqrt_time_factor_;  ///< assuming that the stdev of price change for time _fractional_seconds_ = stdev of price
  /// change for 300 seconds * sqrt ( _fractional_seconds_ / 300 )
  double stdev_high_mult_factor_;

  // functions
 protected:
  OfflineLowCorrelationPairs(DebugLogger& _dbglogger_, const Watch& _watch_,
                             const std::string& concise_indicator_description_, SecurityMarketView& _dep_market_view_,
                             SecurityMarketView& _indep_market_view_, double _fractional_seconds_,
                             PriceType_t _price_type_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static OfflineLowCorrelationPairs* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                       const std::vector<const char*>& _tokens_,
                                                       PriceType_t _basepx_pxtype_);

  static OfflineLowCorrelationPairs* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                       SecurityMarketView& _dep_market_view_,
                                                       SecurityMarketView& _indep_market_view_,
                                                       double _fractional_seconds_, PriceType_t _price_type_);

  ~OfflineLowCorrelationPairs() {}

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_) { UpdateLRInfo(); }

  void OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_);

  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_){};
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};
  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    if (p_indep_indicator_ != NULL) market_update_manager_.AddMarketDataInterruptedListener(p_indep_indicator_);
  }

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  /// Used in ModelCreator to see which shortcodes are core
  bool GetReadinessRequired(const std::string& r_dep_shortcode_, const std::vector<const char*>& tokens_) const {
    std::vector<std::string> core_shortcodes_;
    GetCoreShortcodes(r_dep_shortcode_, core_shortcodes_);
    // here tokes 3 and 4 are important
    // if ( ( tokens_.size() > 3u ) &&
    // 	   ( VectorUtils::LinearSearchValue ( core_shortcodes_, std::string(tokens_[3]) ) ) )
    // 	{ return true ; }
    if ((tokens_.size() > 4u) && (VectorUtils::LinearSearchValue(core_shortcodes_, std::string(tokens_[4])))) {
      return true;
    }
    return false;
  }

  /// Used in ModelCreator to see which variable is in the model file
  static std::string VarName() { return "OfflineLowCorrelationPairs"; }

  void WhyNotReady();

  /// market_interrupt_listener interface
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

  inline double FabsCorrelation() const { return std::max(0.05, fabs(current_lrinfo_.lr_correlation_)); }

 protected:
  void InitializeValues();
  void UpdateLRInfo();

  /// function to compute current_projection_multiplier_.  This is same in OfflineLowCorrelationPairs as
  /// OfflineComputedPairs
  inline void ComputeMultiplier() { current_projection_multiplier_ = current_lrinfo_.lr_coeff_; }
};
}

#endif  // BASE_INDICATORS_OFFLINE_LOW_CORRELATION_PAIRS_H
