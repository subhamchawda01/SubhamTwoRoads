/**
    \file ExecLogic/directional_agrressive_trading_modify.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#ifndef BASE_EXECLOGIC_DIRECTIONAL_AGGRESSIVE_TRADING_SINGLE_ORDER_H
#define BASE_EXECLOGIC_DIRECTIONAL_AGGRESSIVE_TRADING_SINGLE_ORDER_H

#include "dvctrade/ExecLogic/base_trading.hpp"

#define NON_BEST_LEVEL_ORDER_MANAGEMENT_INTERVAL_MSECS 1000

namespace HFSAT {

/// @brief Class to trade with a target price that is not dependant on top level sizes
///
/// Here we are trying to predit direction of trading. And compare the target price against the midprice.
class DirectionalAggressiveTradingSingleOrder : public virtual BaseTrading {
 protected:
 public:
  DirectionalAggressiveTradingSingleOrder(
      DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
      SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
      MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
      const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int _runtime_id_,
      const std::vector<std::string> _this_model_source_shortcode_vec_);

  /// destructor not made virtual ... please do so when making child classes
  ~DirectionalAggressiveTradingSingleOrder(){};

  static std::string StrategyName() {
    return "DirectionalAggressiveTradingSingleOrder";
  }  ///< used in trade_init to check which strategy is to be initialized

 protected:
  /// target prices
  int bid_int_price_to_place_at_;
  int ask_int_price_to_place_at_;

  // current prices
  int existing_bid_int_price_;
  int existing_ask_int_price_;

  int worst_case_long_position_, worst_case_short_position_;
  bool unsequenced_bid_order_present_;
  bool unsequenced_ask_order_present_;

  void ComputeExistingBidPrices();
  void ComputeExistingAskPrices();

  void TradingLogic();  ///< All the strategy based trade execution is written here

  void PrintFullStatus();

  bool CanBidAggress();
  bool CanAskAggress();

  bool CanBidImprove();
  bool CanAskImprove();

  bool CanKeepBidImprove();
  bool CanKeepAskImprove();

  bool CanPlaceBid();
  bool CanPlaceAsk();

  bool CanKeepBid();
  bool CanKeepAsk();

  void SetBidPlaceDirectives();
  void SetAskPlaceDirectives();

  void PlaceCancelNonBestLevels() {}
  void CallPlaceCancelNonBestLevels() {}

  void PlaceCancelBidOrders();
  void PlaceCancelAskOrders();

  void GetFlatTradingLogic();  ///< Trade Selection and execution during getflat mode

  void SendTradeAndLog(int int_price, int size, TradeType_t buysell);

  char GetOrderLevelIndicator(TradeType_t order_side, int int_order_px);
};
}
#endif  // BASE_EXECLOGIC_DIRECTIONAL_AGGRESSIVE_TRADING_H
