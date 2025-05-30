/**
   \file MarketAdapter/market_orders_view.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include <vector>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "baseinfra/MarketAdapter/market_orders_notifier.hpp"
#include "baseinfra/MarketAdapter/market_book.hpp"

namespace HFSAT {

class MarketOrdersView {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;

  MarketBook* market_book_;

  // Pointer to the notifier class - which maintains the subscribers
  // for various events like change in l1
  MarketOrdersNotifier* market_orders_notifier_;

  // Double price to int price converter
  FastPriceConvertor* fast_px_converter_;
  static std::map<std::string, MarketOrdersView*> SMM_desc_map_;

 public:
  static MarketOrdersView* GetUniqueInstance(DebugLogger& dbglogger, const Watch& watch,
                                             const unsigned int security_id);
  static void RemoveUniqueInstance(const unsigned int security_id);

  MarketOrdersView(DebugLogger& dbglogger, const Watch& t_watch, const unsigned int security_id);

  void NotifyMarketOrderListeners();

  void SetMarketBook(MarketBook* market_book) { market_book_ = market_book; }

  // Returns int price for the given double price
  inline int GetIntPx(double price) { return fast_px_converter_->GetFastIntPx(price); }

  // Returns double price for the given int price
  inline double GetDoublePx(int price) { return fast_px_converter_->GetDoublePx(price); }

  // Add market bid based on the priority of the order (position_based => given priority is current queue position)
  void AddMarketBidPriority(int64_t order_id, double price, int size, int64_t priority, bool position_based = false);

  // Add market ask based on the priority of the order (position_based => given priority is current queue position)
  void AddMarketAskPriority(int64_t order_id, double price, int size, int64_t priority, bool position_based = false);

  // Add market bid to the back of the price level
  void AddMarketBidBack(int64_t order_id, double price, int size);

  // Add market ask to the back of the price level
  void AddMarketAskBack(int64_t order_id, double price, int size);

  int GetTopOrderSize(int int_price);

  void ModifyMarketBid(int64_t prev_order_id, double prev_price, int prev_size, int64_t new_order_id, double price,
                       int size);
  void ModifyMarketAsk(int64_t prev_order_id, double prev_price, int prev_size, int64_t new_order_id, double price,
                       int size);

  void DeleteMarketBid(int64_t order_id, double price, int size);
  void DeleteMarketAsk(int64_t order_id, double price, int size);

  int ExecMarketBid(int64_t order_id, double price, int size, bool agg_order);
  int ExecMarketAsk(int64_t order_id, double price, int size, bool agg_order);

  // Handling for executing hidden orders which can affect positions
  // In case of hidden orders we get a modify first on which we will put
  // modified order at the end of the queue and then a trade for that order.
  // Acc. to current logic, we will give fills for all order above this
  // order in the queue which shouldn't happen.

  int ExecMarketBidHidden(int64_t order_id, double price, int size_hidden_, int size_traded_);
  int ExecMarketAskHidden(int64_t order_id, double price, int size_hidden_, int size_traded_);

  void Reset();

  void SubscribeQueuePosChange(QueuePositionChangeListener* queue_pos_change_listener);
  void SubscribeL1QueuePosChange(QueuePositionChangeListener* queue_pos_change_listener);

  std::string GetMarketString();
  bool IsReady();

  int GetBidSize(int int_price, int64_t order_id);
  int GetAskSize(int int_price, int64_t order_id);

  int GetSize(TradeType_t buysell, int int_price);
  int GetOrderCount(TradeType_t buysell, int int_price);

  int GetBidSize(int int_price);
  int GetAskSize(int int_price);
  int GetBidOrderCount(int int_price);
  int GetAskOrderCount(int int_price);

  int GetBestBidIntPrice();
  int GetBestAskIntPrice();

  double GetBestBidPrice();
  double GetBestAskPrice();

  /*wrapper function to get bid order using its price and order id from the market book */
  ExchMarketOrder* FindMarketBid(int64_t t_order_id_, int t_int_price_) {
    return market_book_->FindMarketBid(t_order_id_, t_int_price_);
  }

  /*wrapper function to get ask order using its price and order id from the market book */
  ExchMarketOrder* FindMarketAsk(int64_t t_order_id_, int t_int_price_) {
    return market_book_->FindMarketAsk(t_order_id_, t_int_price_);
  };

  /*wrapper function to get bid order position from the market book */
  int GetBidPosition(ExchMarketOrder* t_order_) { return market_book_->GetBidPosition(t_order_); }

  /*wrapper function to get ask order position from the market book */
  int GetAskPosition(ExchMarketOrder* t_order_) { return market_book_->GetAskPosition(t_order_); }

  std::vector<ExchMarketOrder*>& GetBidOrders(int int_price);
  std::vector<ExchMarketOrder*>& GetAskOrders(int int_price);

  ExchMarketOrder* GetLastRemovedBidOrder(int int_price);
  ExchMarketOrder* GetLastRemovedAskOrder(int int_price);

  ExchMarketOrder* GetBidOrderAtPositionPrice(int position, int int_price);
  ExchMarketOrder* GetAskOrderAtPositionPrice(int position, int int_price);

  // These are wrappers around market_book class functions to provide access to details of top order to market_view
  // class objects
  int GetTopOrderSizeBidLevels(int int_price);
  int GetTopOrderSizeAskLevels(int int_price);
  void RemoveBidTopOrder(int int_price);
  void RemoveAskTopOrder(int int_price);

  unsigned int GetBidPosition(int64_t order_id, double price, int size);
  unsigned int GetAskPosition(int64_t order_id, double price, int size);

  ttime_t GetLastAdd(TradeType_t buysell, int int_price, int* size);
  ttime_t GetLastBidAdd(int int_price, int* size);
  ttime_t GetLastAskAdd(int int_price, int* size);
};
static inline std::vector<MarketOrdersView*>& sid_to_market_orders_view_map() {
  static std::vector<MarketOrdersView*> sid_to_market_orders_view_map_;
  return sid_to_market_orders_view_map_;
}
}
