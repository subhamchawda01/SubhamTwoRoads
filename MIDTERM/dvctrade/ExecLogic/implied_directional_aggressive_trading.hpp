/**
    \file ExecLogic/implied_directional_agrressive_trading.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_EXECLOGIC_IMPLIED_DIRECTIONAL_AGGRESSIVE_TRADING_H
#define BASE_EXECLOGIC_IMPLIED_DIRECTIONAL_AGGRESSIVE_TRADING_H

#include "dvctrade/ExecLogic/base_trading.hpp"
#include "dvctrade/ExecLogic/implied_price_calculator.hpp"

namespace HFSAT {

/// @brief Class to trade with a target price that is not dependant on top level sizes
///
/// Here we are trying to predit direction of trading. And compare the target price against the midprice.
class ImpliedDirectionalAggressiveTrading : public virtual BaseTrading {
 protected:
 public:
  ImpliedDirectionalAggressiveTrading(DebugLogger& _dbglogger_, const Watch& _watch_,
                               const SecurityMarketView& _dep_market_view_,
                               const std::vector<SecurityMarketView*> _indep_mkt_view_vec_,
                               SmartOrderManager& _order_manager_,
                               const std::string& _paramfilename_, const bool _livetrading_,
                               MulticastSenderSocket* _p_strategy_param_sender_socket_,
                               EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                               const int t_trading_end_utc_mfm_, const int _runtime_id_,
                               const std::vector<std::string> _this_model_source_shortcode_vec_);

  /// destructor not made virtual ... please do so when making child classes
  ~ImpliedDirectionalAggressiveTrading(){};

  static std::string StrategyName() {
    return "ImpliedDirectionalAggressiveTrading";
  }  ///< used in trade_init to check which strategy is to be initialized

  static void CollectORSShortCodes(DebugLogger& _dbglogger_, const std::string& r_dep_shortcode_,
                                                             std::vector<std::string>& source_shortcode_vec_,
                                                             std::vector<std::string>& ors_source_needed_vec_);


  void OnMarketUpdate(const unsigned int security_id, const MarketUpdateInfo& market_update_info);

  void OnTradePrint(const unsigned int security_id, const TradePrintInfo& trade_print_info,
                      const MarketUpdateInfo& market_update_info);

 protected:
  std::vector<double> px_to_place_at_vec_;
  std::vector<int> int_px_to_place_at_vec_;
  std::vector<int> size_to_place_vec_;
  std::vector<char> order_level_indicator_vec_;
  const std::vector<SecurityMarketView *> indep_mkt_view_vec_;
  double implied_bid_price_;
  double implied_ask_price_;
  int implied_bid_size_;
  int implied_ask_size_;
  double implied_mkt_price_;
  ImpliedPriceCalculator *implied_price_calculator_;
  //int cancel_asks_below_, cancel_bids_below_;
  //int cancel_asks_above_, cancel_bids_above_;
  //int cancel_asks_from_far_size_, cancel_bids_from_far_size_;

  void TradingLogic();  ///< All the strategy based trade execution is written here

  void PrintFullStatus();
  bool CanBidAggress();
  bool CanAskAggress();
  bool CanBidImprove();
  bool CanAskImprove();

  bool CanPlaceBid();
  bool CanPlaceAsk();
  void HandleORSIndicators();
  void CallTradingLogic();
};
}
#endif  // BASE_EXECLOGIC_IMPLIED_DIRECTIONAL_AGGRESSIVE_TRADING_H
