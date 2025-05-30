/**
    \file ExecLogic/directional_agrressive_trading_modify.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#ifndef BASE_EXECLOGIC_DIRECTIONAL_AGGRESSIVE_TRADING_MODIFY_H
#define BASE_EXECLOGIC_DIRECTIONAL_AGGRESSIVE_TRADING_MODIFY_H

#include "dvctrade/ExecLogic/base_trading.hpp"

#define NON_BEST_LEVEL_ORDER_MANAGEMENT_INTERVAL_MSECS 1000

namespace HFSAT {

/// @brief Class to trade with a target price that is not dependant on top level sizes
///
/// Here we are trying to predit direction of trading. And compare the target price against the midprice.
class DirectionalAggressiveTradingModify : public virtual BaseTrading {
 protected:
 public:
  DirectionalAggressiveTradingModify(DebugLogger& _dbglogger_, const Watch& _watch_,
                                     const SecurityMarketView& _dep_market_view_, SmartOrderManager& _order_manager_,
                                     const std::string& _paramfilename_, const bool _livetrading_,
                                     MulticastSenderSocket* _p_strategy_param_sender_socket_,
                                     EconomicEventsManager& t_economic_events_manager_,
                                     const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_,
                                     const int _runtime_id_,
                                     const std::vector<std::string> _this_model_source_shortcode_vec_);

  /// destructor not made virtual ... please do so when making child classes
  ~DirectionalAggressiveTradingModify(){};

  static std::string StrategyName() {
    return "DirectionalAggressiveTradingModify";
  }  ///< used in trade_init to check which strategy is to be initialized

 protected:
  std::vector<int> bid_int_prices_to_place_at_;
  std::vector<int> bid_sizes_to_place_at_;
  std::vector<int> ask_int_prices_to_place_at_;
  std::vector<int> ask_sizes_to_place_at_;

  int cancel_asks_below_, cancel_bids_below_;
  int cancel_asks_above_, cancel_bids_above_;
  int cancel_asks_from_far_size_, cancel_bids_from_far_size_;

  int num_max_orders_;
  int our_bid_orders_, our_ask_orders_;
  int effective_bid_position_, effective_ask_position_;
  int effective_bid_position_to_keep_, effective_ask_position_to_keep_;

  int worst_case_long_position_, worst_case_short_position_;
  int last_bid_nonbest_level_om_, last_ask_nonbest_level_om_;

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

  void HandleORSIndicators() {}
  void TradeVarsForMultOrder();
  void PlaceCancelNonBestLevels() {}
  void CallPlaceCancelNonBestLevels() {}

  void PlaceNewBidOrders(int& improve_order_placed, int non_best_level_start_index, int total_size_so_far);
  void PlaceNewAskOrders(int& improve_order_placed, int non_best_level_start_index, int total_size_so_far);
  /// try modifying orders,
  /// Place orders at px_to_place_at_vec_ and cancel below price
  void PlaceCancelBidOrders();

  int PlaceModifyOrdersAtPrice(int size_to_place, std::vector<BaseOrder*>& order_vector,
                               std::vector<BaseOrder*>& orders_to_cancel, bool cancel_orders);

  bool CanPlaceAggressiveBid2() {
    int total_bid_size = order_manager_.SumBidSizes();
    int allowance_for_aggressive_buy_ =
        my_position_ + total_bid_size + current_tradevarset_.l1bid_trade_size_ - param_set_.worst_case_position_;

    if (param_set_.use_new_aggress_logic_) {
      allowance_for_aggressive_buy_ = my_position_ + total_bid_size + current_tradevarset_.l1bid_trade_size_ -
                                      std::max(param_set_.worst_case_position_, param_set_.max_position_);
    }
    return allowance_for_aggressive_buy_ < 0;
  }

  void TryPlacingAggressiveBid2(int& aggressive_order_placed) {
    if (top_ask_lift_ && CanPlaceAggressiveBid2() &&
        order_manager_.GetTotalBidSizeOrderedAtIntPx(best_nonself_ask_int_price_) <= 0) {
      // If there was no order
      bid_int_prices_to_place_at_.push_back(best_nonself_ask_int_price_);
      bid_sizes_to_place_at_.push_back(current_tradevarset_.l1bid_trade_size_);
      aggressive_order_placed += current_tradevarset_.l1bid_trade_size_;
      placed_bids_this_round_ = true;
    }
  }

  bool CanPlaceAggressiveAsk2() {
    int allowance_for_aggressive_sell_ = -my_position_ + order_manager_.SumAskSizes() +
                                         current_tradevarset_.l1ask_trade_size_ - param_set_.worst_case_position_;
    if (param_set_.use_new_aggress_logic_) {
      allowance_for_aggressive_sell_ = -my_position_ + order_manager_.SumAskSizes() +
                                       current_tradevarset_.l1ask_trade_size_ -
                                       std::max(param_set_.worst_case_position_, param_set_.max_position_);
    }
    return allowance_for_aggressive_sell_ < 0;
  }

  void TryPlacingAggressiveAsk2(int& aggressive_order_placed) {
    if (top_bid_hit_ && order_manager_.GetTotalAskSizeOrderedAtIntPx(best_nonself_bid_int_price_) <= 0) {
      // If there was no order
      ask_int_prices_to_place_at_.push_back(best_nonself_bid_int_price_);
      ask_sizes_to_place_at_.push_back(current_tradevarset_.l1ask_trade_size_);
      aggressive_order_placed += current_tradevarset_.l1ask_trade_size_;
      last_agg_sell_msecs_ = watch_.msecs_from_midnight();
      placed_asks_this_round_ = true;
    }
  }

  void TryPlacingImproveBid2(int& improve_order_placed) {
    if (top_bid_improve_ && order_manager_.GetTotalBidSizeOrderedAtIntPx(best_nonself_bid_int_price_ + 1) <= 0) {
      // If there was no improve order
      bid_int_prices_to_place_at_.push_back(best_nonself_bid_int_price_ + 1);
      bid_sizes_to_place_at_.push_back(current_tradevarset_.l1bid_trade_size_);
      improve_order_placed += current_tradevarset_.l1bid_trade_size_;
      last_agg_buy_msecs_ = watch_.msecs_from_midnight();
      placed_bids_this_round_ = true;
    }
  }

  void TryPlacingImproveAsk2(int& improve_order_placed) {
    if (top_ask_improve_ && order_manager_.GetTotalAskSizeOrderedAtIntPx(best_nonself_ask_int_price_ - 1) <= 0) {
      // If there was no improve order
      ask_int_prices_to_place_at_.push_back(best_nonself_ask_int_price_ - 1);
      ask_sizes_to_place_at_.push_back(current_tradevarset_.l1ask_trade_size_);
      improve_order_placed += current_tradevarset_.l1ask_trade_size_;
      last_agg_sell_msecs_ = watch_.msecs_from_midnight();
    }
  }

  void TryPlacingBestBid2(int& best_order_placed) {
    if (top_bid_place_ && CanPlaceBestBid2() &&
        order_manager_.GetTotalBidSizeOrderedAtIntPx(best_nonself_bid_int_price_) <= 0) {
      // If there was no improve order
      bid_int_prices_to_place_at_.push_back(best_nonself_bid_int_price_);
      bid_sizes_to_place_at_.push_back(current_tradevarset_.l1bid_trade_size_);
      best_order_placed += current_tradevarset_.l1bid_trade_size_;
      placed_bids_this_round_ = true;
    }
  }

  void TryPlacingBestAsk2(int& best_order_placed) {
    if (top_ask_place_ && CanPlaceBestAsk2() &&
        order_manager_.GetTotalAskSizeOrderedAtIntPx(best_nonself_ask_int_price_) <= 0) {
      // If there was no improve order
      ask_int_prices_to_place_at_.push_back(best_nonself_ask_int_price_);
      ask_sizes_to_place_at_.push_back(current_tradevarset_.l1ask_trade_size_);
      best_order_placed += current_tradevarset_.l1ask_trade_size_;
      placed_asks_this_round_ = true;
    }
  }

  bool CanPlaceBestBid2() {
    return ((stdev_ <= param_set_.low_stdev_lvl_ ||
             (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_place_)));
  }

  bool CanPlaceBestAsk2() {
    return (stdev_ <= param_set_.low_stdev_lvl_ ||
            (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_place_));
  }

  int CanPlaceNonBestLevelBidOrders() {
    int start_index = -1;
    if (my_position_ <= (param_set_.max_position_ / 2) ||
        param_set_.ignore_max_pos_check_for_non_best_) {  // position is not very long

      start_index = std::max(start_index, (int)param_set_.min_distance_for_non_best_);

      if (param_set_.min_size_ahead_for_non_best_ > 0) {
        unsigned int _size_ahead_ = 0;
        int i = 0;
        for (;
             _size_ahead_ <= param_set_.min_size_ahead_for_non_best_ && i <= (int)param_set_.max_distance_for_non_best_;
             i++) {
          _size_ahead_ += dep_market_view_.bid_size(i);
        }
        start_index = std::max(start_index, i);
      }
    }
    return start_index;
  }

  int CanPlaceNonBestLevelAskOrders() {
    int start_index = -1;
    if (my_position_ >= (-param_set_.max_position_ / 2) ||
        param_set_.ignore_max_pos_check_for_non_best_) {  // position is not too short

      start_index = std::max(start_index, (int)param_set_.min_distance_for_non_best_);
      if (param_set_.min_size_ahead_for_non_best_ > 0) {
        unsigned int _size_ahead_ = 0;
        int i = 0;
        for (;
             _size_ahead_ <= param_set_.min_size_ahead_for_non_best_ && i <= (int)param_set_.max_distance_for_non_best_;
             i++) {
          _size_ahead_ += dep_market_view_.bid_size(i);
        }
        start_index = std::max(start_index, i);
      }
    }
    return start_index;
  }

  char GetOrderLevelIndicator(TradeType_t order_side, int int_order_px);
  void PlaceCancelAskOrders();
};
}
#endif  // BASE_EXECLOGIC_DIRECTIONAL_AGGRESSIVE_TRADING_H
