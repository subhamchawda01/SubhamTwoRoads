/**
    \file Indicators/offline_breakout_adjusted_pairs.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"
#include "dvctrade/Indicators/offline_returns_lrdb.hpp"
#include "dvctrade/Indicators/volume_ratio_calculator.hpp"

namespace HFSAT {

/// From returns series of dep and indep the following were computed offline
/// correlation, LR coeff
/// Multiplier used in proj return :
/// From price changes, we compute the coefficient and corerlation through Single Linear Regression
/// < lr_coeff_, lr_correlation_ > = GetSLRCoeffCorrelation ( dep_price_change_vector_, indep_price_change_vector_ )
/// Using projected return as indep_price_trend_ * lr_coeff_ makes
/// stdev ( projected return real ) = ( fabs ( lr_correlation_ ) * stdev ( dep_historical ) * stdev ( dep_real ) / stdev
/// ( indep_historical ) )
/// Hence using multiplier = lr_coeff_ / fabs ( lr_correlation_ )
/// makes stdev ( projected return real ) = ( stdev ( dep_historical ) * stdev ( indep_real ) / stdev ( indep_historical
/// ) )
/// similar to stdev ( dep_real )
/// Slight Improvement: instead of dividing by fabs ( lr_correlation_ )
/// divide by std::max ( 0.05, fabs ( lr_correlation_ ) )
/// TODO : study the constant 0.05, perhaps values as high as 0.25 might be better
///
/// Multiplying indep returns and (lr_coeff_/lr_correlation_) it gets the projected return of the dependant
/// and subtracting the realized return of the dependant it computes a prediction for the future
/// For computation of returns exponentially-time-decayed moving average is used, same as simple_trend
///
/// TODO, possible improvements, divide correlation into bands, <-0.5, -0.5 to 0.5, >0.5 and discretize the
/// space. Possibly when the correlation is weak instead of making the variable purely a trend reversing indicator,
/// include correlation into the answer
class OfflineBreakoutAdjustedPairs2 : public IndicatorListener,
                                      public CommonIndicator,
                                      public VolumeRatioListener,
                                      public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  const SecurityMarketView& indep_market_view_;

  double dep_price_trend_;
  double indep_price_trend_;
  double trend_weight_;

  VolumeRatioCalculator& dep_volume_ratio_calculator_;
  VolumeRatioCalculator& indep_volume_ratio_calculator_;

  OfflineReturnsLRDB& lrdb_;
  int last_lrinfo_updated_msecs_;
  LRInfo current_lrinfo_;
  double current_projection_multiplier_;
  double current_projected_trend_;

  double dep_volume_ratio_;
  double indep_volume_ratio_;

  unsigned int pred_mode_;

  SimpleTrend* p_dep_indicator_;
  SimpleTrend* p_indep_indicator_;

 protected:
  OfflineBreakoutAdjustedPairs2(DebugLogger& _dbglogger_, const Watch& _watch_,
                                const std::string& concise_indicator_description_,
                                const SecurityMarketView& _dep_market_view_,
                                const SecurityMarketView& _indep_market_view_, double _fractional_seconds_,
                                int t_volume_measure_seconds_, double t_trend_weight_, PriceType_t _price_type_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static OfflineBreakoutAdjustedPairs2* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                          const std::vector<const char*>& _tokens_,
                                                          PriceType_t _basepx_pxtype_);

  static OfflineBreakoutAdjustedPairs2* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                          const SecurityMarketView& _dep_market_view_,
                                                          const SecurityMarketView& _indep_market_view_,
                                                          double _fractional_seconds_, int t_volume_measure_seconds_,
                                                          double t_trend_weight_, PriceType_t _price_type_);

  ~OfflineBreakoutAdjustedPairs2() {}

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_) { UpdateLRInfo(); }

  void OnVolumeRatioUpdate(const unsigned int r_security_id_, const double& r_new_volume_ratio_);

  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_){};
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  // functions
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

  static std::string VarName() { return "OfflineBreakoutAdjustedPairs2"; }

  void WhyNotReady();

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    if (p_dep_indicator_ != NULL) {
      market_update_manager_.AddMarketDataInterruptedListener(p_dep_indicator_);
    }
    if (p_indep_indicator_ != NULL) {
      market_update_manager_.AddMarketDataInterruptedListener(p_indep_indicator_);
    }
  }

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
  inline double FabsCorrelation() const { return std::max(0.05, fabs(current_lrinfo_.lr_correlation_)); }

 protected:
  void InitializeValues();
  void UpdateLRInfo();

  /// Multiplier :
  /// From price changes, we compute the coefficient and corerlation through Single Linear Regression
  /// < lr_coeff_, lr_correlation_ > = GetSLRCoeffCorrelation ( dep_price_change_vector_, indep_price_change_vector_ )
  /// For OfflineBreakoutAdjustedPairs2 lr_coeff_/fabs(lr_correlation_) is sort of the lr_coeff_ as it would be if indep
  /// and dep are either perfectly correlated or
  /// perfectly anti-correlated ( correation = -1 )
  inline void ComputeMultiplier() {
    current_projection_multiplier_ =
        (current_lrinfo_.lr_coeff_ / std::max(0.05, fabs(current_lrinfo_.lr_correlation_)));
  }
};
}
