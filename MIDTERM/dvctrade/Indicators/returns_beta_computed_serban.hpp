/**
    \file Indicators/returns_beta_computed_serban.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/simple_returns.hpp"
#include "dvctrade/Indicators/offline_returns_lrdb.hpp"

namespace HFSAT {

class ReturnsBetaComputedSerban : public IndicatorListener, public CommonIndicator, public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  const SecurityMarketView& indep_market_view_;

  double dep_price_trend_;
  double indep_price_trend_;

  double returns_beta_;
  double current_projected_trend_;

  SimpleReturns* p_dep_indicator_;
  SimpleReturns* p_indep_indicator_;

  double current_projected_spread_;     // y(t) = alpha* st_i - st_d
  double indep_trend_momentum_factor_;  // the AR(1) coefficient that best matches p_i(t+1) - p_i(t) = coeff *
                                        // short_term_indep_trend_indicator_;
  SimpleReturns* short_term_indep_trend_indicator_;  // proxy for p_i(t) - p_i(t-1)
  double short_term_indep_trend_;                    // indicator value of above : proxy for p_i(t) - p_i(t-1)

  double current_dep_price_;

  double spread_reversion_factor_;       // c1 where spread(t+1) - spread(t) = c1*-spread(t) + arma(spread)
  double spread_trend_momentum_factor_;  // arma part of spread modeling

  // computational variables -- for spread projection
  double moving_avg_spread_;

  double last_spread_recorded_;

  // functions
 protected:
  ReturnsBetaComputedSerban(DebugLogger& _dbglogger_, const Watch& _watch_,
                            const std::string& concise_indicator_description_, SecurityMarketView& _dep_market_view_,
                            SecurityMarketView& _indep_market_view_, double t_fractional_seconds_,
                            double t_indep_trend_momentum_fractional_seconds_, double t_indep_trend_momentum_factor_,
                            double t_spread_reversion_factor_, double t_spread_trend_momentum_fractional_seconds_,
                            double t_spread_trend_momentum_factor_, PriceType_t _price_type_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static ReturnsBetaComputedSerban* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                      const std::vector<const char*>& _tokens_,
                                                      PriceType_t _basepx_pxtype_);

  static ReturnsBetaComputedSerban* GetUniqueInstance(
      DebugLogger& _dbglogger_, const Watch& _watch_, SecurityMarketView& _dep_market_view_,
      SecurityMarketView& _indep_market_view_, double t_fractional_seconds_,
      double t_indep_trend_momentum_fractional_seconds_, double t_indep_trend_momentum_factor_,
      double t_spread_reversion_factor_, double t_spread_trend_momentum_fractional_seconds_,
      double t_spread_trend_momentum_factor_, PriceType_t _price_type_);

  ~ReturnsBetaComputedSerban() {}

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_){};

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
    if (_security_id_ == dep_market_view_.security_id()) {
      current_dep_price_ = SecurityMarketView::GetPriceFromType(kPriceTypeMidprice, _market_update_info_);
    }
  }
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
  static std::string VarName() { return "ReturnsBetaComputedSerban"; }

  void WhyNotReady();

  /// market_interrupt_listener interface
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
  bool InitializeBetaValues();
};
}
