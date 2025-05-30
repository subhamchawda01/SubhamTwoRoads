/**
   \file MarketAdapter/market_book.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include <vector>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/CommonDataStructures/simple_mempool.hpp"
#include "baseinfra/MarketAdapter/order_defines.hpp"
#include "baseinfra/MarketAdapter/market_order_level.hpp"

namespace HFSAT {

class MarketBook {
 private:
  DebugLogger& dbglogger_;
  const Watch& watch_;

  SimpleMempool<ExchMarketOrder>* mkt_order_mempool_;

  bool bid_initialized_;
  bool ask_initialized_;

  std::vector<MktOrdLevel*> bid_levels_;
  std::vector<MktOrdLevel*> ask_levels_;

  int bid_adjustment_;
  int ask_adjustment_;

  int top_bid_index_;
  int top_ask_index_;

  int book_levels_;

  void InitializeBidLevels(int int_price);
  void InitializeAskLevels(int int_price);

  // Returns bid index into the market bid market orders give the int_price
  inline int GetBidIndex(int int_price) { return int_price - bid_adjustment_; }

  // Returns ask index into the market ask market orders give the int_price
  inline int GetAskIndex(int int_price) { return book_levels_ - (int_price - ask_adjustment_); }

  // Returns bid int price given the bid market orders index
  inline int GetBidIntPrice(int bid_index) { return bid_index + bid_adjustment_; }

  // Returns ask int price given the bid market orders index
  inline int GetAskIntPrice(int ask_index) { return book_levels_ - (ask_index - ask_adjustment_); }

  int GetBestBidIndex(int int_price);
  int GetBestAskIndex(int int_price);

  // Returns true if top bid/ask index is changed
  bool AdjustTopBidIndex(int index);
  bool AdjustTopAskIndex(int index);

  int GetBidLevelSize(int index);
  int GetAskLevelSize(int index);

 public:
  MarketBook(DebugLogger& dbglogger, const Watch& watch);
  virtual ~MarketBook();

  ExchMarketOrder* GetExchMarketOrder(int64_t order_id, double price, int size, int int_price, uint64_t priority);

  // Add bid/ask to the book
  // Returns true if there is a change in l1 because of this
  // position_based = true => given order->priority_ is position-based (priority is the current position in queue)
  virtual bool AddBid(ExchMarketOrder* order, QueuePositionUpdate* queue_pos, bool position_based, bool& l1_update);
  virtual bool AddAsk(ExchMarketOrder* order, QueuePositionUpdate* queue_pos, bool position_based, bool& l1_update);

  ExchMarketOrder* FindMarketBid(int64_t t_order_id_, int t_int_price_);

  ExchMarketOrder* FindMarketAsk(int64_t t_order_id_, int t_int_price_);

  ExchMarketOrder* FindMarketBid(int64_t t_order_id_, int t_int_price_, int t_size_);

  ExchMarketOrder* FindMarketAsk(int64_t t_order_id_, int t_int_price_, int t_size_);

  // Returns true if there is a change in l1
  virtual bool RemoveMarketBid(ExchMarketOrder* t_order_, QueuePositionUpdate* queue_pos_);
  virtual bool RemoveMarketAsk(ExchMarketOrder* t_order_, QueuePositionUpdate* queue_pos_);

  virtual void ExecMarketBid(ExchMarketOrder* order, QueuePositionUpdate* queue_pos);
  virtual void ExecMarketAsk(ExchMarketOrder* order, QueuePositionUpdate* queue_pos);

  void Reset();

  int GetBidPosition(ExchMarketOrder* t_order_);
  int GetAskPosition(ExchMarketOrder* t_order_);

  bool IsBidIntPriceL1(int int_price_);
  bool IsAskIntPriceL1(int int_price_);

  std::string GetMarketString();
  bool IsReady();

  int GetBidSize(int int_price_);
  int GetAskSize(int int_price_);

  int GetBestBidIntPrice();
  int GetBestAskIntPrice();

  std::vector<ExchMarketOrder*>& GetBidOrders(int int_price_);
  std::vector<ExchMarketOrder*>& GetAskOrders(int int_price_);

  ExchMarketOrder* GetLastRemovedBidOrder(int int_price);
  ExchMarketOrder* GetLastRemovedAskOrder(int int_price);

  ExchMarketOrder* GetBidOrderAtPositionPrice(int position, int int_price);
  ExchMarketOrder* GetAskOrderAtPositionPrice(int position, int int_price);

  ttime_t GetLastBidAdd(int int_price_, int* size_);
  ttime_t GetLastAskAdd(int int_price_, int* size_);

  // Returns the size of Top Order (Top Order is the order which creates a price level) at the int_price level
  int GetTopOrderSizeBidLevels(int int_price);
  int GetTopOrderSizeAskLevels(int int_price);

  // Removes top order flag from the bid int_price level
  void RemoveBidTopOrder(int int_price);
  // Removes top order flag from the ask int_price level
  void RemoveAskTopOrder(int int_price);
};
}
