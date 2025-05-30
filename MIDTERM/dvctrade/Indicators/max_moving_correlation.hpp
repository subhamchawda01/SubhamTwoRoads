/**
    \file Indicators/online_computed_pairs.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/slow_corr_calculator.hpp"

namespace HFSAT {

/// Indicator that takes two SecurityMarketView objects dep, indep and
/// computes roughly OnlineLRCoeff ( dep, indep ) * indep_price_trend - dep_price_trend
/// For the computation of OnlineLRCoeff ( dep_price_changes, indep_price_changes ) it uses the mean zero math
/// Sum ( dep * indep ) / Sum ( indep * indep )
class MaxMovingCorrelation : public CommonIndicator, public IndicatorListener {
 protected:
  // variables
  SecurityMarketView &dep_market_view_;
  std::vector<SecurityMarketView *> indep_market_view_vec_;

  std::vector<double> moving_correlation_vec_;

  std::vector<SlowCorrCalculator *> slow_corr_calculator_vec_;

  std::vector<double> indep_corr_vec_;
  int max_indep_corr_idx_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                std::vector<std::string> &_ors_source_needed_vec_,
                                const std::vector<const char *> &_tokens_);

  static MaxMovingCorrelation *GetUniqueInstance(DebugLogger &_dbglogger_, const Watch &_watch_,
                                                 const std::vector<const char *> &_tokens_,
                                                 PriceType_t _basepx_pxtype_);

  static MaxMovingCorrelation *GetUniqueInstance(DebugLogger &_dbglogger_, const Watch &_watch_,
                                                 SecurityMarketView &_dep_market_view_,
                                                 std::vector<SecurityMarketView *> &_indep_market_view_vec_,
                                                 double _fractional_seconds_);

 protected:
  MaxMovingCorrelation(DebugLogger &_dbglogger_, const Watch &_watch_,
                       const std::string &concise_indicator_description_, SecurityMarketView &_dep_market_view_,
                       std::vector<SecurityMarketView *> &_indep_market_view_vec_, double _fractional_seconds_);

 public:
  ~MaxMovingCorrelation() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_){};

  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                           const MarketUpdateInfo &_market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  /// Used in ModelCreator to see which shortcodes are core
  bool GetReadinessRequired(const std::string &r_dep_shortcode_, const std::vector<const char *> &tokens_) const {
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

  void OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &_new_value_);
  inline void OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &new_value_decrease_,
                                const double &new_value_nochange_, const double &new_value_increase_) {
    return;
  }

  static std::string VarName() { return "MaxMovingCorrelation"; }

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}
