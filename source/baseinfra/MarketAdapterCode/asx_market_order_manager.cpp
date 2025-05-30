/**
   \file MarketAdapterCode/asx_market_order_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/asx_market_order_manager.hpp"

namespace HFSAT {

AsxMarketOrderManager::AsxMarketOrderManager(DebugLogger& dbglogger, const Watch& t_watch_,
                                             SecurityNameIndexer& t_sec_name_indexer_,
                                             const std::vector<MarketOrdersView*>& t_market_orders_view_map_,
                                             bool is_ose_itch)
    : BaseMarketOrderManager(dbglogger, t_watch_, t_sec_name_indexer_, t_market_orders_view_map_),
      asx_order_mempool_(),
      asx_markets_(t_sec_name_indexer_.NumSecurityId(), new AsxMarketPerSecurity()),
      is_ose_itch_(is_ose_itch) {}

void AsxMarketOrderManager::OnOrderAdd(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                                       const uint32_t priority, const double price, const uint32_t size) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  AsxMarketPerSecurity* mkt = asx_markets_[security_id];

  switch (buysell) {
    case kTradeTypeBuy: {
      if (mkt->order_id_bids_map_.find(order_id) != mkt->order_id_bids_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " already present in bid map" << DBGLOG_ENDL_FLUSH;
        break;
      }

      AsxOrder* order = asx_order_mempool_.Alloc();
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
        break;
      }

      AsxOrder* order = asx_order_mempool_.Alloc();
      order->price = price;
      order->size = size;
      mkt->order_id_asks_map_[order_id] = order;

      mov->AddMarketAskPriority(order_id, price, size, priority, is_ose_itch_);
      break;
    }
    default: { break; }
  }
}

void AsxMarketOrderManager::OnOrderModify(const uint32_t security_id, const TradeType_t buysell,
                                          const uint64_t order_id, const uint32_t priority, const double price,
                                          const uint32_t size) {
  // This changes priority as well
  // So, do a delete followed by add
  AsxMarketPerSecurity* mkt = asx_markets_[security_id];
  switch (buysell) {
    case kTradeTypeBuy: {
      auto order = mkt->order_id_bids_map_.find(order_id);
      if (order == mkt->order_id_bids_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " is not present in bid map" << DBGLOG_ENDL_FLUSH;
        break;
      }

      order->second->size = size;  // Update size
      break;
    }
    case kTradeTypeSell: {
      auto order = mkt->order_id_asks_map_.find(order_id);
      if (order == mkt->order_id_asks_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " is not present in ask map" << DBGLOG_ENDL_FLUSH;
        break;
      }
      order->second->size = size;  // Update size
      break;
    }
    default: { break; }
  }
  // Delete
  OnOrderDelete(security_id, buysell, order_id);

  // Add the new order
  OnOrderAdd(security_id, buysell, order_id, priority, price, size);
}

void AsxMarketOrderManager::OnVolumeDelete(const uint32_t security_id, const TradeType_t buysell,
                                           const uint64_t order_id, const uint32_t size) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  AsxMarketPerSecurity* mkt = asx_markets_[security_id];

  switch (buysell) {
    case kTradeTypeBuy: {
      if (mkt->order_id_bids_map_.find(order_id) == mkt->order_id_bids_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " is not present in bid map" << DBGLOG_ENDL_FLUSH;
        break;
      }

      AsxOrder* order = mkt->order_id_bids_map_[order_id];

      mov->ModifyMarketBid(order_id, order->price, order->size, order_id, order->price, size);

      order->size = size;  // Update size
      break;
    }
    case kTradeTypeSell: {
      if (mkt->order_id_asks_map_.find(order_id) == mkt->order_id_asks_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " is not present in ask map" << DBGLOG_ENDL_FLUSH;
        break;
      }

      AsxOrder* order = mkt->order_id_asks_map_[order_id];

      mov->ModifyMarketAsk(order_id, order->price, order->size, order_id, order->price, size);

      order->size = size;  // Update size
      break;
    }
    default: { break; }
  }
}

void AsxMarketOrderManager::OnOrderDelete(const uint32_t security_id, const TradeType_t buysell,
                                          const uint64_t order_id) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  AsxMarketPerSecurity* mkt = asx_markets_[security_id];

  switch (buysell) {
    case kTradeTypeBuy: {
      if (mkt->order_id_bids_map_.find(order_id) == mkt->order_id_bids_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " is not present in bid map" << DBGLOG_ENDL_FLUSH;
        break;
      }

      AsxOrder* order = mkt->order_id_bids_map_[order_id];

      mov->DeleteMarketBid(order_id, order->price, order->size);

      // Remove/Dealloc order
      mkt->order_id_bids_map_.erase(order_id);
      asx_order_mempool_.DeAlloc(order);
      break;
    }
    case kTradeTypeSell: {
      if (mkt->order_id_asks_map_.find(order_id) == mkt->order_id_asks_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " is not present in ask map" << DBGLOG_ENDL_FLUSH;
        break;
      }

      AsxOrder* order = mkt->order_id_asks_map_[order_id];

      mov->DeleteMarketAsk(order_id, order->price, order->size);

      // Remove/Dealloc order
      mkt->order_id_asks_map_.erase(order_id);
      asx_order_mempool_.DeAlloc(order);
      break;
    }
    default: { break; }
  }
}

void AsxMarketOrderManager::OnOrderExec(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                                        const double _exec_price, const uint32_t size_exec,
                                        const uint32_t size_remaining) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  AsxMarketPerSecurity* mkt = asx_markets_[security_id];
  double exec_price = _exec_price;

  if (abs(exec_price - INVALID_EXEC_PRICE) < 0.01) {
    // price was provided as INVALID_PRICE (assume that trade happened at the best level)
    if (buysell == kTradeTypeBuy) {
      exec_price = mov->GetBestAskPrice();
    } else {
      exec_price = mov->GetBestBidPrice();
    }
  }

  switch (buysell) {
    case kTradeTypeBuy: {
      if (mkt->order_id_bids_map_.find(order_id) == mkt->order_id_bids_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " is not present in bid map" << DBGLOG_ENDL_FLUSH;
        break;
      }

      AsxOrder* order = mkt->order_id_bids_map_[order_id];
      if (mov->GetIntPx(exec_price) != mov->GetIntPx(order->price)) {
        // Skip order exec with price change updates for exec
        break;
      }

      mov->ExecMarketBid(order_id, exec_price, size_exec, false);
      break;
    }
    case kTradeTypeSell: {
      if (mkt->order_id_asks_map_.find(order_id) == mkt->order_id_asks_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " is not present in ask map" << DBGLOG_ENDL_FLUSH;
        break;
      }

      AsxOrder* order = mkt->order_id_asks_map_[order_id];
      if (mov->GetIntPx(exec_price) != mov->GetIntPx(order->price)) {
        // Skip order exec with price change updates for exec
        break;
      }

      mov->ExecMarketAsk(order_id, exec_price, size_exec, false);
      break;
    }
    default: { break; }
  }

  if (size_remaining > 0) {
    OnVolumeDelete(security_id, buysell, order_id, size_remaining);
  } else {
    OnOrderDelete(security_id, buysell, order_id);
  }
}

void AsxMarketOrderManager::OnOrderExecWithPx(const uint32_t security_id, const uint64_t bid_order_id,
                                              const uint64_t ask_order_id, const double exec_price,
                                              const uint32_t size_exec, const uint32_t bid_remaining,
                                              const uint32_t ask_remaining) {
  // handling ASX ITCH cases i.e cases after March 2017 when the ASX feed changed declaring the variables make the
  // updates feed agnostic
  const uint64_t order_id = bid_order_id;
  const uint64_t opposite_order_id = ask_order_id;
  const uint32_t size_remaining = bid_remaining;

  AsxMarketPerSecurity* mkt = asx_markets_[security_id];

  // if order_id is for sell side
  if (mkt->order_id_asks_map_.find(order_id) != mkt->order_id_asks_map_.end()) {
    OnOrderExec(security_id, kTradeTypeSell, order_id, exec_price, size_exec, size_remaining);
    if ((mkt->order_id_bids_map_.find(opposite_order_id) != mkt->order_id_bids_map_.end())) {
      OnOrderExec(security_id, kTradeTypeBuy, opposite_order_id, exec_price, size_exec,
                  mkt->order_id_bids_map_[opposite_order_id]->size - size_exec);
    } else {
      DBGLOG_TIME_CLASS_FUNC << "ERROR"
                             << " OrderId: " << opposite_order_id << " is not present in bid map" << DBGLOG_ENDL_FLUSH;
    }
  }
  // order_id is for buy side
  else if (mkt->order_id_bids_map_.find(order_id) != mkt->order_id_bids_map_.end()) {
    OnOrderExec(security_id, kTradeTypeBuy, order_id, exec_price, size_exec, size_remaining);
    if ((mkt->order_id_asks_map_.find(opposite_order_id) != mkt->order_id_asks_map_.end())) {
      OnOrderExec(security_id, kTradeTypeSell, opposite_order_id, exec_price, size_exec,
                  mkt->order_id_asks_map_[opposite_order_id]->size - size_exec);
    } else {
      DBGLOG_TIME_CLASS_FUNC << "ERROR"
                             << " OrderId: " << opposite_order_id << " is not present in ask map" << DBGLOG_ENDL_FLUSH;
    }
  } else {
    DBGLOG_TIME_CLASS_FUNC << "ERROR"
                           << " OrderId: " << order_id << " is not present in ask or bid map" << DBGLOG_ENDL_FLUSH;
  }
}

void AsxMarketOrderManager::OnOrderSpExec(const uint32_t security_id, const TradeType_t buysell,
                                          const uint64_t order_id, const double exec_price, const uint32_t size_exec,
                                          const uint32_t size_remaining) {
  AsxMarketPerSecurity* mkt = asx_markets_[security_id];
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

void AsxMarketOrderManager::ResetBook(const unsigned int security_id) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  AsxMarketPerSecurity* mkt = asx_markets_[security_id];

  for (auto order : mkt->order_id_bids_map_) {
    asx_order_mempool_.DeAlloc(order.second);
  }

  for (auto order : mkt->order_id_asks_map_) {
    asx_order_mempool_.DeAlloc(order.second);
  }

  mkt->Clear();
  mov->Reset();
}
}
