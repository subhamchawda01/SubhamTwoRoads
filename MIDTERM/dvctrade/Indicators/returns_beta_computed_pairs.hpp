/**
    \file Indicators/returns_beta_computed_pairs.hpp

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

namespace HFSAT {

/// From returns series of dep and indep the following were computed offline
/// correlation, LR coeff
class ReturnsBetaComputedPairs : public IndicatorListener, public CommonIndicator, public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  const SecurityMarketView& indep_market_view_;

  double dep_price_returns_;
  double indep_price_returns_;

  double returns_beta_;
  double beta_change_factor_;
  double beta_change_multiplier_;
  double beta_reversion_price_;
  double current_projected_returns_;
  double current_dep_price_;

  SimpleReturns* p_dep_indicator_;
  SimpleReturns* p_indep_indicator_;

  // functions
 protected:
  ReturnsBetaComputedPairs(DebugLogger& _dbglogger_, const Watch& _watch_,
                           const std::string& concise_indicator_description_, SecurityMarketView& _dep_market_view_,
                           SecurityMarketView& _indep_market_view_, double _fractional_seconds_,
                           double _beta_change_factor_, PriceType_t _price_type_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static ReturnsBetaComputedPairs* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                     const std::vector<const char*>& _tokens_,
                                                     PriceType_t _basepx_pxtype_);

  static ReturnsBetaComputedPairs* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                     SecurityMarketView& _dep_market_view_,
                                                     SecurityMarketView& _indep_market_view_,
                                                     double _fractional_seconds_, double _beta_change_factor_,
                                                     PriceType_t _price_type_);

  ~ReturnsBetaComputedPairs() {}

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_){};

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
    if (_security_id_ == dep_market_view_.security_id()) {
      current_dep_price_ = SecurityMarketView::GetPriceFromType(kPriceTypeMidprice, _market_update_info_);
      beta_change_multiplier_ = beta_change_factor_ * (current_dep_price_ - beta_reversion_price_);
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
  static std::string VarName() { return "ReturnsBetaComputedPairs"; }

  void WhyNotReady();

  /// market_interrupt_listener interface
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  bool InitializeBetaValues();
};
}
