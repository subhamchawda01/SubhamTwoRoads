/**
    \file ExecLogic/jp400_pmm.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 354, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_JP400_PMM_H
#define BASE_JP400_PMM_H

#include "dvctrade/ExecLogic/base_trading.hpp"

namespace HFSAT {

/**
 * This is a Market Making ExecLogic for putting quotes on JP400_0 of OSE.
 * The requirement is to keep orders of size 10 within 4 ticks for greater than 50% of the times during second session
 * Logic is as Follows :
 * It is 99% correlated with TOPIX, so TOPIX is being used as indep/source.
 * Projected Price of JP400 = moving avg px of JP400 + stdev ratio * trend of Topix
 * If spread of JP > 4 ticks then GetFlat
 * If (procted px - best bid > 4 ticks || best ask - projected px > 4 ticks) then dont trade
 * else if projected px > best ask then place at best bid and 4 ticks above best bid.
 * else if projected px is between best bid and ask then place similar to FEU3MM
 * other cases are symetric
 */

class JP400PMM : public BaseTrading, public IndicatorListener {
 protected:
 public:
  static void CollectORSShortCodes(DebugLogger& _dbglogger_, const std::string& r_dep_shortcode_,
                                   std::vector<std::string>& source_shortcode_vec_,
                                   std::vector<std::string>& ors_source_needed_vec_);

  JP400PMM(DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
           SmartOrderManager& _order_manager_, const SecurityMarketView& _indep_market_view_,
           const std::string& _paramfilename_, const bool _livetrading_,
           MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
           const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int _runtime_id_,
           const std::vector<std::string> _this_model_source_shortcode_vec_);

  ~JP400PMM() {}  ///< destructor not made virtual ... please do so when making child classes

  static std::string StrategyName() {
    return "JP400PMM";
  }  ///< used in trade_init to check which strategy is to be initialized

 protected:
  virtual void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  virtual void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                            const MarketUpdateInfo& _market_update_info_);
  void OnTimePeriodUpdate(const int num_pages_to_add_);

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  void TradingLogic();  ///< All the strategy based trade execution is written here
  void PrintFullStatus();
  bool ShouldTrade();
  bool ShouldQuote();
  bool IsProjectedBetweenBidAsk();
  inline void CallPlaceCancelNonBestLevels();
  void ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool conservative_close_);

  const SecurityMarketView& indep_market_view_;  // TOPIX_0
  SimpleTrend* p_dep_indicator_;
  SimpleTrend* p_indep_indicator_;
  double beta_;
  double dep_trend_;
  double indep_trend_;
  double best_indep_bid_price_;
  int best_indep_bid_int_price_;
  int best_indep_bid_size_;

  double best_indep_ask_price_;
  int best_indep_ask_int_price_;
  int best_indep_ask_size_;
  int indep_dep_int_price_ratio_;

  bool status_;
  int total_quoted_msecs_;
  double quote_start_time_;
  double quote_end_time_;
  bool is_quoted_;
  double projected_price_;
  int bid_place_int_price_;
  int ask_place_int_price_;
  ;
};
}
#endif  // BASE_EXECLOGIC_PRICE_BASED_TRADING_H
