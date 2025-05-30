/**
    \file Indicators/online_ratio_pairs_filter.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_ONLINE_RATIO_PAIRS_FILTER_H
#define BASE_INDICATORS_ONLINE_RATIO_PAIRS_FILTER_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator.hpp"

namespace HFSAT {

/// Indicator that takes two SecurityMarketView objects dep, indep and
/// computes the ratio of their prices at all points and
/// prints how the current ratio differs from the past ratios
class OnlineRatioPairsFilter : public CommonIndicator, public SlowStdevCalculatorListener {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  const SecurityMarketView& indep_market_view_;

  PriceType_t price_type_;

  // computational variables
  double moving_avg_ratio_;
  double moving_avg_dep_;
  double moving_avg_indep_;

  double last_price_ratio_;
  double last_dep_price_;
  double last_indep_price_;

  double current_dep_price_;
  double current_indep_price_;
  double current_price_ratio_;

  double stdev_dep_;
  double stdev_indep_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static OnlineRatioPairsFilter* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                   const std::vector<const char*>& _tokens_,
                                                   PriceType_t _basepx_pxtype_);

  static OnlineRatioPairsFilter* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                   SecurityMarketView& _dep_market_view_,
                                                   SecurityMarketView& _indep_market_view_, double _fractional_seconds_,
                                                   PriceType_t _price_type_);

 protected:
  OnlineRatioPairsFilter(DebugLogger& _dbglogger_, const Watch& _watch_,
                         const std::string& concise_indicator_description_, SecurityMarketView& _dep_market_view_,
                         SecurityMarketView& _indep_market_view_, double _fractional_seconds_,
                         PriceType_t _price_type_);

 public:
  ~OnlineRatioPairsFilter() {}

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

  static std::string VarName() { return "OnlineRatioPairsFilter"; }

  void OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_);

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_ONLINE_RATIO_PAIRS_FILTER_H
