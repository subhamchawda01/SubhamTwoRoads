/**
    \file Indicators/offline_computed_serban_port.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/offline_returns_lrdb.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"
#include "dvctrade/Indicators/simple_trend_port.hpp"

namespace HFSAT {

/// From returns series of dep and indep the following were computed offline
/// correlation, LR coeff
/// Multiplier used in proj return :
/// From price changes, we compute the coefficient and corerlation through Single Linear Regression
/// < lr_coeff_, lr_correlation_ > = GetSLRCoeffCorrelation ( dep_price_change_vector_, indep_price_change_vector_ )
/// Using projected return as indep_price_trend_ * lr_coeff_ makes
/// stdev ( projected return real ) = ( fabs ( lr_correlation_ ) * stdev ( dep_historical ) * stdev ( dep_real ) / stdev
/// ( indep_historical ) )
/// similar to fabs ( lr_correlation_ ) * stdev ( dep_real )
///
/// Multiplying indep returns and lrcoeff it gets the projected return of the dependant
/// and subtracting the realized return of the dependant it computes a spread
/// For computation of returns exponentially-time-decayed moving average is used, same as simple_trend
///
/// Future values of the spread are not just projected to the mean, but alos a momentum factor is applied to it.
class OfflineComputedSerbanPort : public IndicatorListener, public CommonIndicator, public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  PCAPortPrice& indep_portfolio_price_;

  double dep_price_trend_;
  double indep_price_trend_;

  OfflineReturnsLRDB& lrdb_;
  int last_lrinfo_updated_msecs_;
  LRInfo current_lrinfo_;
  double current_projection_multiplier_;
  double current_projected_trend_;

  SimpleTrend* p_dep_indicator_;
  SimpleTrendPort* p_indep_indicator_;

  double current_projected_spread_;     // y(t) = alpha* st_i - st_d
  double indep_trend_momentum_factor_;  // the AR(1) coefficient that best matches p_i(t+1) - p_i(t) = coeff *
                                        // short_term_indep_trend_indicator_;
  SimpleTrendPort* short_term_indep_trend_indicator_;  // proxy for p_i(t) - p_i(t-1)
  double short_term_indep_trend_;                      // indicator value of above : proxy for p_i(t) - p_i(t-1)

  double spread_reversion_factor_;       // c1 where spread(t+1) - spread(t) = c1*-spread(t) + arma(spread)
  double spread_trend_momentum_factor_;  // arma part of spread modeling
  int spread_trend_history_msecs_;       // to compute moving average of spread

  // computational variables -- for spread projection
  double moving_avg_spread_;

  int last_new_page_msecs_;
  int page_width_msecs_;

  double decay_page_factor_;
  std::vector<double> decay_vector_;
  double inv_decay_sum_;
  std::vector<double> decay_vector_sums_;

  double last_spread_recorded_;

  // functions
 protected:
  OfflineComputedSerbanPort(DebugLogger& _dbglogger_, const Watch& _watch_,
                            const std::string& concise_indicator_description_, SecurityMarketView& _dep_market_view_,
                            const std::string& _portfolio_descriptor_shortcode_, double t_fractional_seconds_,
                            double t_indep_trend_momentum_fractional_seconds_, double t_indep_trend_momentum_factor_,
                            double t_spread_reversion_factor_, double t_spread_trend_momentum_fractional_seconds_,
                            double t_spread_trend_momentum_factor_, PriceType_t _price_type_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static OfflineComputedSerbanPort* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                      const std::vector<const char*>& _tokens_,
                                                      PriceType_t _basepx_pxtype_);

  static OfflineComputedSerbanPort* GetUniqueInstance(
      DebugLogger& _dbglogger_, const Watch& _watch_, SecurityMarketView& _dep_market_view_,
      const std::string& _portfolio_descriptor_shortcode_, double t_fractional_seconds_,
      double t_indep_trend_momentum_fractional_seconds_, double t_indep_trend_momentum_factor_,
      double t_spread_reversion_factor_, double t_spread_trend_momentum_fractional_seconds_,
      double t_spread_trend_momentum_factor_, PriceType_t _price_type_);

  ~OfflineComputedSerbanPort() {}

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_) { UpdateLRInfo(); }

  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_){};
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    if (p_indep_indicator_ != NULL) {
      market_update_manager_.AddMarketDataInterruptedListener(p_indep_indicator_);
    }
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
  static std::string VarName() { return "OfflineComputedSerbanPort"; }

  void WhyNotReady();

  /// market_interrupt_listener interface
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}
  void OnMarketDataResumed(const unsigned int _security_id_) {}

  inline double FabsCorrelation() const { return std::max(0.05, fabs(current_lrinfo_.lr_correlation_)); }

 protected:
  void InitializeValues();
  void UpdateLRInfo();

  /// function to compute current_projection_multiplier_.   This is the main difference between OfflineCorradjustedPairs
  /// and OfflineComputedSerbanPort
  inline void ComputeMultiplier() { current_projection_multiplier_ = current_lrinfo_.lr_coeff_; }
};
}
