/**
    \file Indicators/online_implied_yen.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_ONLINE_IMPLIED_YEN_H
#define BASE_INDICATORS_ONLINE_IMPLIED_YEN_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"
#include "dvctrade/Indicators/online_ratio_pairs.hpp"

namespace HFSAT {

/// Indicator that takes two SecurityMarketView objects dep, indep and
/// computes the diff of their prices at all points and
/// prints how the current spread differs from the past spread
class OnlineImpliedYen : public CommonIndicator, public IndicatorListener {
 protected:
  // computational variables
  double mkt_trend_;
  double ipld_trend_;

  SimpleTrend* market_trend_;
  OnlineRatioPairs* implied_trend_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static OnlineImpliedYen* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static OnlineImpliedYen* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             SecurityMarketView& _dollar_market_view_,
                                             SecurityMarketView& _yen_market_view_,
                                             SecurityMarketView& _currency_market_view_, double _fractional_seconds_,
                                             PriceType_t _price_type_);

 protected:
  OnlineImpliedYen(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                   SecurityMarketView& _dollar_market_view_, SecurityMarketView& _yen_market_view_,
                   SecurityMarketView& _currency_market_view_, double _fractional_seconds_, PriceType_t _price_type_);

 public:
  ~OnlineImpliedYen() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

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

  static std::string VarName() { return "OnlineImpliedYen"; }

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_ONLINE_IMPLIED_YEN_H
