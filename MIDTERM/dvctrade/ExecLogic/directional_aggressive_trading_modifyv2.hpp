/**
    \file ExecLogic/directional_agrressive_trading_modifyv2.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#ifndef BASE_EXECLOGIC_DIRECTIONAL_AGGRESSIVE_TRADING_MODIFYV2_H
#define BASE_EXECLOGIC_DIRECTIONAL_AGGRESSIVE_TRADING_MODIFYV2_H

#include "dvctrade/ExecLogic/base_trading.hpp"

#define NON_BEST_LEVEL_ORDER_MANAGEMENT_INTERVAL_MSECS 1000

namespace HFSAT {

/// @brief Class to trade with a target price that is not dependant on top level sizes
///
/// Here we are trying to predit direction of trading. And compare the target price against the midprice.
class DirectionalAggressiveTradingModifyV2 : public virtual BaseTrading {
 private:
  const SecurityMarketView& dep_market_view_;

 public:
  DirectionalAggressiveTradingModifyV2(DebugLogger& _dbglogger_, const Watch& _watch_,
                                       const SecurityMarketView& _dep_market_view_, SmartOrderManager& _order_manager_,
                                       const std::string& _paramfilename_, const bool _livetrading_,
                                       MulticastSenderSocket* _p_strategy_param_sender_socket_,
                                       EconomicEventsManager& t_economic_events_manager_,
                                       const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_,
                                       const int _runtime_id_,
                                       const std::vector<std::string> _this_model_source_shortcode_vec_);

  /// destructor not made virtual ... please do so when making child classes
  ~DirectionalAggressiveTradingModifyV2(){};

  static std::string StrategyName() {
    return "DirectionalAggressiveTradingModifyV2";
  }  ///< used in trade_init to check which strategy is to be initialized

 protected:
  void TradingLogic();  ///< All the strategy based trade execution is written here

  void PrintFullStatus();
  bool CanBidAggress();
  bool CanAskAggress();
  bool CanBidImprove();
  bool CanAskImprove();

  bool CanPlaceBid();
  bool CanPlaceAsk();

  void SetBidPlaceDirectives();
  void SetAskPlaceDirectives();

  void PlaceModifyBidOrders();
  void PlaceModifyAskOrders();

  bool CanPlaceBestBid2();
  bool CanPlaceBestAsk2();

  bool WillExceedWorstCasePos(TradeType_t buysell, int order_size);

  void SendTradeAndLog(const double price, const int intpx, int size_requested, TradeType_t t_buysell,
                       char placed_at_level_indicator);
  void ModifyOrder(BaseOrder* order_to_modify, double price, int int_price, int size, TradeType_t t_buysell,
                   char placed_at_level_indicator);

  void ModifyBestBidOrdersToNonBest(int best_nonself_bid_int_price_);
  void ModifyBestAskOrdersToNonBest(int best_nonself_ask_int_price_);

  char GetOrderLevelIndicator(TradeType_t order_side, int int_order_px);

  // overriding it to empty function
  void CallPlaceCancelNonBestLevels() override {}
  void ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool _conservative_close_) override {
    printf("#send: %d #canceled: %d #modified: %d\n", order_manager_.SendOrderCount(), order_manager_.CxlOrderCount(),
           order_manager_.ModifyOrderCount());
    BaseTrading::ReportResults(trades_writer_, _conservative_close_);
  }
};
}
#endif  // BASE_EXECLOGIC_DIRECTIONAL_AGGRESSIVE_TRADING_MODIFYV2_H
