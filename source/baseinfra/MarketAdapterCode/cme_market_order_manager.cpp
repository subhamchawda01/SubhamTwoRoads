/**
   \file MarketAdapterCode/cme_market_order_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/cme_market_order_manager.hpp"

namespace HFSAT {

/**
 * Constructor class
 *
 * @param dbglogger
 * @param t_watch_
 * @param t_sec_name_indexer_
 * @param t_market_orders_view_map_
 * @param is_ose_itch
 */
CMEMarketOrderManager::CMEMarketOrderManager(DebugLogger& dbglogger, const Watch& t_watch_,
                                             SecurityNameIndexer& t_sec_name_indexer_,
                                             const std::vector<MarketOrdersView*>& t_market_orders_view_map_,
                                             bool is_ose_itch)
    : BaseMarketOrderManager(dbglogger, t_watch_, t_sec_name_indexer_, t_market_orders_view_map_),
      cme_order_mempool_(),
      cme_markets_(t_sec_name_indexer_.NumSecurityId(), new CMEMarketPerSecurity()),
      is_ose_itch_(is_ose_itch),
      last_update_was_exec_(false),
      last_agg_size_(0),
      last_agg_order_id_(0),
      last_was_agg_trade_(false) {}

/**
 *  Handling order add functionality
 *
 * @param security_id
 * @param buysell
 * @param order_id
 * @param priority
 * @param price
 * @param size
 */
void CMEMarketOrderManager::OnOrderAdd(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                                       const uint32_t priority, const double price, const uint32_t size) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  CMEMarketPerSecurity* mkt = cme_markets_[security_id];

  last_update_was_exec_ = false;

  if (dbglogger_.CheckLoggingLevel(BOOK_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "OnOrderAdd: " << security_id << " Type: " << GetTradeTypeChar(buysell)
                           << " OrderID: " << order_id << " Priority: " << priority << " px: " << price
                           << " size: " << size << DBGLOG_ENDL_FLUSH;
  }

  switch (buysell) {
    case kTradeTypeBuy: {
      if (mkt->order_id_bids_map_.find(order_id) != mkt->order_id_bids_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " already present in bid map" << DBGLOG_ENDL_FLUSH;
        std::cerr << "ERROR"
                  << " OrderId: " << order_id << " already present in bid map" << std::endl;
        // For now, Modifying these orders to new orders
        // Not removing and adding as that would always have incorrect queue position
        OnOrderModify(security_id, buysell, order_id, priority, price, size);
        break;
      }

      CMEOrder* order = cme_order_mempool_.Alloc();
      order->price = price;
      order->size = size;
      mkt->order_id_bids_map_[order_id] = order;

      mov->AddMarketBidPriority(order_id, price, size, priority, is_ose_itch_);
      break;
    }
    case kTradeTypeSell: {
      if (mkt->order_id_asks_map_.find(order_id) != mkt->order_id_asks_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " already present in ask map" << DBGLOG_ENDL_FLUSH;
        std::cerr << "ERROR"
                  << " OrderId: " << order_id << " already present in ask map" << std::endl;
        // For now, Modifying these orders to new orders
        // Not removing and adding as that would always have incorrect queue position
        OnOrderModify(security_id, buysell, order_id, priority, price, size);
        break;
      }

      CMEOrder* order = cme_order_mempool_.Alloc();
      order->price = price;
      order->size = size;
      mkt->order_id_asks_map_[order_id] = order;

      mov->AddMarketAskPriority(order_id, price, size, priority, is_ose_itch_);
      break;
    }
    default: { break; }
  }
}

/**
 * Order is modified
 *
 * @param security_id
 * @param buysell
 * @param order_id
 * @param priority
 * @param price
 * @param size
 */
void CMEMarketOrderManager::OnOrderModify(const uint32_t security_id, const TradeType_t buysell,
                                          const uint64_t order_id, const uint32_t priority, const double price,
                                          const uint32_t size) {
  if (dbglogger_.CheckLoggingLevel(BOOK_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "OnOrderModify: " << security_id << " Type: " << GetTradeTypeChar(buysell)
                           << " OrderID: " << order_id << " Priority: " << priority << " px: " << price
                           << " size: " << size << DBGLOG_ENDL_FLUSH;
  }

  // This changes priority as well
  // So, do a delete followed by add

  /* we will need to keep track of the fact that order-added is because of modify
  // Reasons :
  // In case of modify if the priority of order remains same then it would remain at it's place
  // this can be helpful in conditions where the order which is just ahead of us and we are either first/last in the
  // queue
   * */

  MarketOrdersView* mov = market_orders_view_map_[security_id];
  CMEMarketPerSecurity* mkt = cme_markets_[security_id];

  last_update_was_exec_ = false;
  if (dbglogger_.CheckLoggingLevel(BOOK_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "OnOrderModify: " << security_id << " Type: " << GetTradeTypeChar(buysell)
                           << " OrderID: " << order_id << " Priority: " << priority << " px: " << price
                           << " size: " << size << DBGLOG_ENDL_FLUSH;
  }

  switch (buysell) {
    case kTradeTypeBuy: {
      auto iter = mkt->order_id_bids_map_.find(order_id);
      if (iter == mkt->order_id_bids_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " not present in bid map" << DBGLOG_ENDL_FLUSH;
        break;
      }

      auto* order = iter->second;

      mov->ModifyMarketBid(order_id, order->price, order->size, order_id, price, size);

      order->price = price;
      order->size = size;
      break;
    }
    case kTradeTypeSell: {
      auto iter = mkt->order_id_asks_map_.find(order_id);
      if (iter == mkt->order_id_asks_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " not present in ask map" << DBGLOG_ENDL_FLUSH;
        break;
      }

      auto* order = iter->second;
      mov->ModifyMarketAsk(order_id, order->price, order->size, order_id, price, size);
      order->price = price;
      order->size = size;
      break;
    }
    default: { break; }
  }
}

/**
 * Delete the given size from the order, if the size is equal to order-size then remove order completely
 * @param security_id
 * @param buysell
 * @param order_id
 * @param size
 */
void CMEMarketOrderManager::OnVolumeDelete(const uint32_t security_id, const TradeType_t buysell,
                                           const uint64_t order_id, const uint32_t size) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  CMEMarketPerSecurity* mkt = cme_markets_[security_id];

  last_update_was_exec_ = false;

  switch (buysell) {
    case kTradeTypeBuy: {
      auto iter = mkt->order_id_bids_map_.find(order_id);
      if (iter == mkt->order_id_bids_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " is not present in bid map" << DBGLOG_ENDL_FLUSH;
        break;
      }

      CMEOrder* order = iter->second;

      if (order->size == (int)size) {
        // If the trade size is same as oder size, order should be deleted
        mov->DeleteMarketBid(order_id, order->price, order->size);

        // Remove/Dealloc order
        mkt->order_id_bids_map_.erase(order_id);
        cme_order_mempool_.DeAlloc(order);
      } else if (order->size > (int)size) {
        mov->ModifyMarketBid(order_id, order->price, order->size, order_id, order->price, order->size - size);

        order->size = order->size - size;  // Update size
      } else {
        DBGLOG_TIME_CLASS_FUNC << "ERROR: "
                               << " OrderId: " << order_id
                               << " volume deleted is more than order-size, deleting the order" << DBGLOG_ENDL_FLUSH;
        // If the trade size is same as oder size, order should be deleted
        mov->DeleteMarketBid(order_id, order->price, order->size);

        // Remove/Dealloc order
        mkt->order_id_bids_map_.erase(order_id);
        cme_order_mempool_.DeAlloc(order);
      }
      break;
    }
    case kTradeTypeSell: {
      auto iter = mkt->order_id_asks_map_.find(order_id);
      if (iter == mkt->order_id_asks_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " is not present in ask map" << DBGLOG_ENDL_FLUSH;
        break;
      }

      CMEOrder* order = iter->second;

      if (order->size == (int)size) {
        // If the trade size is same as order size, order should be deleted
        mov->DeleteMarketAsk(order_id, order->price, order->size);

        // Remove/Dealloc order
        mkt->order_id_asks_map_.erase(order_id);
        cme_order_mempool_.DeAlloc(order);

      } else if (order->size > (int)size) {
        mov->ModifyMarketAsk(order_id, order->price, order->size, order_id, order->price, order->size - size);
        order->size = order->size - size;  // Update size
      } else {
        DBGLOG_TIME_CLASS_FUNC_LINE << "ERROR: "
                                    << " OrderId: " << order_id
                                    << " volume deleted is more than order-size, deleting the order"
                                    << DBGLOG_ENDL_FLUSH;

        mov->DeleteMarketAsk(order_id, order->price, order->size);

        // Remove/De-alloc order
        mkt->order_id_asks_map_.erase(order_id);
        cme_order_mempool_.DeAlloc(order);
      }

      break;
    }
    default: { break; }
  }
}

/**
 * Order deleted from market
 *
 * @param security_id
 * @param buysell
 * @param order_id
 */
void CMEMarketOrderManager::OnOrderDelete(const uint32_t security_id, const TradeType_t buysell,
                                          const uint64_t order_id) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  CMEMarketPerSecurity* mkt = cme_markets_[security_id];

  last_update_was_exec_ = false;
  if (dbglogger_.CheckLoggingLevel(BOOK_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "OnOrderDelete: " << security_id << " Type: " << GetTradeTypeChar(buysell)
                           << " OrderID: " << order_id << " Priority: " << DBGLOG_ENDL_FLUSH;
  }

  switch (buysell) {
    case kTradeTypeBuy: {
      if (mkt->order_id_bids_map_.find(order_id) == mkt->order_id_bids_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " is not present in bid map" << DBGLOG_ENDL_FLUSH;
        break;
      }

      CMEOrder* order = mkt->order_id_bids_map_[order_id];

      mov->DeleteMarketBid(order_id, order->price, order->size);

      // Remove/Dealloc order
      mkt->order_id_bids_map_.erase(order_id);
      cme_order_mempool_.DeAlloc(order);
      break;
    }
    case kTradeTypeSell: {
      if (mkt->order_id_asks_map_.find(order_id) == mkt->order_id_asks_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC_LINE << "ERROR"
                                    << " OrderId: " << order_id << " is not present in ask map" << DBGLOG_ENDL_FLUSH;
        break;
      }

      CMEOrder* order = mkt->order_id_asks_map_[order_id];

      mov->DeleteMarketAsk(order_id, order->price, order->size);

      // Remove/Dealloc order
      mkt->order_id_asks_map_.erase(order_id);
      cme_order_mempool_.DeAlloc(order);
      break;
    }
    default: { break; }
  }
}

/**
 * Order executed,
 * here this is passive exec, In most of the instances this is redundant message as we have cumulative trade message
 * faster
 * ( in case of pro-rata, it can be used to verify the actual order size which was executed)
 *
 * @param security_id
 * @param buysell
 * @param order_id
 * @param _exec_price
 * @param size_exec
 * @param size_remaining
 */
void CMEMarketOrderManager::OnOrderExec(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                                        const double _exec_price, const uint32_t size_exec,
                                        const uint32_t size_remaining) {
  // buysell, exec-price, size remaining are useless in cme

  MarketOrdersView* mov = market_orders_view_map_[security_id];
  CMEMarketPerSecurity* mkt = cme_markets_[security_id];

  if (dbglogger_.CheckLoggingLevel(BOOK_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "OnOrderExec: " << security_id << " Type: " << GetTradeTypeChar(buysell)
                           << " OrderID: " << order_id << " Priority: "
                           << " size: " << size_exec << DBGLOG_ENDL_FLUSH;
  }

  if (!last_update_was_exec_) {
    // If last message was not a trade message, it means this exec is aggressor side
    // This would probably mean that someone modified their price from best to aggressive
    // so we are receiving the exec message
    // Skip this message, process the passive trade updates
    last_update_was_exec_ = true;
    last_agg_size_ = size_exec;
    last_agg_order_id_ = order_id;
    last_was_agg_trade_ = true;
    return;
  }

  TradeType_t this_buysell = buysell;
  if (this_buysell == kTradeTypeNoInfo) {
    auto bid_iter = mkt->order_id_bids_map_.find(order_id);
    if (bid_iter != mkt->order_id_bids_map_.end()) {
      this_buysell = kTradeTypeBuy;

      CMEOrder* order = bid_iter->second;

      if (last_was_agg_trade_) {
        // Last trade was for aggressor side, take the price from this update and notify for aggressive side
        mov->ExecMarketBid(last_agg_order_id_, order->price, last_agg_size_, true);
      }

      mov->ExecMarketBid(order_id, order->price, size_exec, false);

      // Send the size-executed here
      OnVolumeDelete(security_id, this_buysell, order_id, size_exec);
    } else {
      // We couldn't find the order in book, meaning it's aggressive order
      mov->ExecMarketBid(order_id, 0, size_exec, true);
    }
  }

  if (this_buysell == kTradeTypeNoInfo) {
    auto ask_iter = mkt->order_id_asks_map_.find(order_id);
    if (ask_iter != mkt->order_id_asks_map_.end()) {
      this_buysell = kTradeTypeSell;

      CMEOrder* order = ask_iter->second;

      if (last_was_agg_trade_) {
        // Last trade was for aggressor side, take the price from this update and notify for aggressive side
        mov->ExecMarketAsk(last_agg_order_id_, order->price, last_agg_size_, true);
      }

      mov->ExecMarketAsk(order_id, order->price, size_exec, false);

      // Send the size-executed here
      OnVolumeDelete(security_id, this_buysell, order_id, size_exec);
    }
  }

  last_update_was_exec_ = true;
  last_was_agg_trade_ = false;
}

/**
 * Not sure if it's going to be used
 * @param security_id
 * @param bid_order_id
 * @param ask_order_id
 * @param exec_price
 * @param size_exec
 * @param bid_remaining
 * @param ask_remaining
 */
void CMEMarketOrderManager::OnOrderExecWithPx(const uint32_t security_id, const uint64_t bid_order_id,
                                              const uint64_t ask_order_id, const double exec_price,
                                              const uint32_t size_exec, const uint32_t bid_remaining,
                                              const uint32_t ask_remaining) {
  OnOrderExec(security_id, kTradeTypeBuy, bid_order_id, exec_price, size_exec, bid_remaining);
  OnOrderExec(security_id, kTradeTypeSell, ask_order_id, exec_price, size_exec, ask_remaining);
}

void CMEMarketOrderManager::OnOrderSpExec(const uint32_t security_id, const TradeType_t buysell,
                                          const uint64_t order_id, const double exec_price, const uint32_t size_exec,
                                          const uint32_t size_remaining) {
  CMEMarketPerSecurity* mkt = cme_markets_[security_id];
  last_update_was_exec_ = true;

  // TODO: add accurate handling for SpExChain msgs, for now we are considering both sides
  if (mkt->order_id_asks_map_.find(order_id) != mkt->order_id_asks_map_.end()) {
    // Size changed due to the semantics of this message for ASX spreads
    OnOrderExec(security_id, kTradeTypeSell, order_id, exec_price,
                (mkt->order_id_asks_map_[order_id])->size - size_remaining, size_remaining);
  } else if (mkt->order_id_bids_map_.find(order_id) != mkt->order_id_bids_map_.end()) {
    // Size changed due to the semantics of this message for ASX spreads
    OnOrderExec(security_id, kTradeTypeBuy, order_id, exec_price,
                (mkt->order_id_bids_map_[order_id])->size - size_remaining, size_remaining);
  } else {
    // The side doesn't matter in this case, as we anyways won't process this msg (we'll just log it)
    OnOrderExec(security_id, kTradeTypeSell, order_id, exec_price, size_exec, size_remaining);
  }
}

/**
 * Reset the book to original state
 * @param security_id
 */
void CMEMarketOrderManager::ResetBook(const unsigned int security_id) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  CMEMarketPerSecurity* mkt = cme_markets_[security_id];

  last_update_was_exec_ = false;

  for (auto order : mkt->order_id_bids_map_) {
    cme_order_mempool_.DeAlloc(order.second);
  }

  for (auto order : mkt->order_id_asks_map_) {
    cme_order_mempool_.DeAlloc(order.second);
  }

  mkt->Clear();
  mov->Reset();
}

void CMEMarketOrderManager::OnResetBegin(const unsigned int security_id) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  CMEMarketPerSecurity* mkt = cme_markets_[security_id];

  last_update_was_exec_ = false;

  for (auto order : mkt->order_id_bids_map_) {
    cme_order_mempool_.DeAlloc(order.second);
  }

  for (auto order : mkt->order_id_asks_map_) {
    cme_order_mempool_.DeAlloc(order.second);
  }

  mkt->Clear();
  mov->Reset();
}

void CMEMarketOrderManager::OnResetEnd(const unsigned int security_id) {}
}
