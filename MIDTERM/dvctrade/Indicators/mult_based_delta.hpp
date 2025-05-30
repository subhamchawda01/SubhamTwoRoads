/**
    \file Indicators/offline_computed_returns_mult.hpp

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

class MultBasedDelta : public IndicatorListener, public CommonIndicator, public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView &dep_market_view_;
  const SecurityMarketView &indep1_market_view_;
  const SecurityMarketView &indep2_market_view_;

  double dep_ret_trend_;
  double indep1_ret_trend_;
  double indep2_ret_trend_;
  SimpleReturns *dep_ret_indicator_;
  SimpleReturns *indep1_ret_indicator_;
  SimpleReturns *indep2_ret_indicator_;
  PriceType_t price_type_;
  std::vector<bool> readiness_required_vec_;

  // functions
 protected:
  MultBasedDelta(DebugLogger &_dbglogger_, const Watch &_watch_, const std::string &concise_indicator_description_,
                 SecurityMarketView &_dep_market_view_, SecurityMarketView &_indep1_market_view_,
                 SecurityMarketView &_indep2_market_view_, double _fractional_seconds_, PriceType_t _price_type_);

 public:
  static void CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                std::vector<std::string> &_ors_source_needed_vec_,
                                const std::vector<const char *> &_tokens_);

  static MultBasedDelta *GetUniqueInstance(DebugLogger &_dbglogger_, const Watch &_watch_,
                                           const std::vector<const char *> &_tokens_, PriceType_t _basepx_pxtype_);

  static MultBasedDelta *GetUniqueInstance(DebugLogger &_dbglogger_, const Watch &_watch_,
                                           SecurityMarketView &_dep_market_view_,
                                           SecurityMarketView &_indep1_market_view_,
                                           SecurityMarketView &_indep2_market_view_, double _fractional_seconds_,
                                           PriceType_t _price_type_);

  ~MultBasedDelta() {}

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_) {}

  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                           const MarketUpdateInfo &_market_update_info_){};
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};
  inline void SubscribeDataInterrupts(MarketUpdateManager &market_update_manager_) {
    if (indep1_ret_indicator_ != NULL) {
      market_update_manager_.AddMarketDataInterruptedListener(indep1_ret_indicator_);
    }
    if (indep2_ret_indicator_ != NULL) {
      market_update_manager_.AddMarketDataInterruptedListener(indep2_ret_indicator_);
    }
  }

  void OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &_new_value_);
  inline void OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &new_value_decrease_,
                                const double &new_value_nochange_, const double &new_value_increase_) {
    return;
  }

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

  /// Used in ModelCreator to see which variable is in the model file
  static std::string VarName() { return "MultBasedDelta"; }

  void WhyNotReady();

  /// market_interrupt_listener interface
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}
