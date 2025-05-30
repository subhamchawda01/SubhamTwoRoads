/**
    \file Indicators/online_computed_pairs.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_ONLINE_BETA_COMPUTED_PAIRS_H
#define BASE_INDICATORS_ONLINE_BETA_COMPUTED_PAIRS_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/online_beta.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"

#define ONLINE_BETA_DURATION 900.0

namespace HFSAT {

/// Indicator that takes two SecurityMarketView objects dep, indep and
/// computes roughly OnlineLRCoeff ( dep, indep ) * indep_price_trend - dep_price_trend
/// For the computation of OnlineLRCoeff ( dep_price_changes, indep_price_changes ) it uses the mean zero math
/// Sum ( dep * indep ) / Sum ( indep * indep )
class OnlineBetaComputedPairs : public CommonIndicator, public IndicatorListener {
 protected:
  // variables
  SecurityMarketView &dep_market_view_;
  SecurityMarketView &indep_market_view_;

  double dep_price_trend_;
  double indep_price_trend_;

  double beta_value_;
  OnlineBeta *beta_indicator_;
  double current_projected_trend_;
  SimpleTrend *p_dep_indicator_;
  SimpleTrend *p_indep_indicator_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                std::vector<std::string> &_ors_source_needed_vec_,
                                const std::vector<const char *> &_tokens_);

  static OnlineBetaComputedPairs *GetUniqueInstance(DebugLogger &_dbglogger_, const Watch &_watch_,
                                                    const std::vector<const char *> &_tokens_,
                                                    PriceType_t _basepx_pxtype_);

  static OnlineBetaComputedPairs *GetUniqueInstance(DebugLogger &_dbglogger_, const Watch &_watch_,
                                                    SecurityMarketView &_dep_market_view_,
                                                    SecurityMarketView &_indep_market_view_,
                                                    double _fractional_seconds_, double _beta_fractional_secs_,
                                                    PriceType_t _price_type_);

 protected:
  OnlineBetaComputedPairs(DebugLogger &_dbglogger_, const Watch &_watch_,
                          const std::string &concise_indicator_description_, SecurityMarketView &_dep_market_view_,
                          SecurityMarketView &_indep_market_view_, double _fractional_seconds_,
                          double _beta_fractional_secs_, PriceType_t _price_type_);

 public:
  ~OnlineBetaComputedPairs() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_){};

  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                           const MarketUpdateInfo &_market_update_info_){};
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};
  inline void SubscribeDataInterrupts(MarketUpdateManager &market_update_manager_) {
    if (p_indep_indicator_ != NULL) {
      market_update_manager_.AddMarketDataInterruptedListener(p_indep_indicator_);
    }
  }

  void OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &_new_value_);
  inline void OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &new_value_decrease_,
                                const double &new_value_nochange_, const double &new_value_increase_) {
    return;
  }

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

  void WhyNotReady();
  static std::string VarName() { return "OnlineBetaComputedPairs"; }

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
};
}

#endif  // BASE_INDICATORS_ONLINE_BETA_COMPUTED_PAIRS_H
