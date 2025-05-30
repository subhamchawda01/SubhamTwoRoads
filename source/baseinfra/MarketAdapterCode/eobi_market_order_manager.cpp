/**
   \file MarketAdapterCode/eobi_market_order_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/eobi_market_order_manager.hpp"

namespace HFSAT {

EOBIMarketOrderManager::EOBIMarketOrderManager(DebugLogger& dbglogger, const Watch& t_watch_,
                                               SecurityNameIndexer& t_sec_name_indexer_,
                                               const std::vector<MarketOrdersView*>& t_market_orders_view_map_)
    : BaseMarketOrderManager(dbglogger, t_watch_, t_sec_name_indexer_, t_market_orders_view_map_),
      eobi_order_mempool_(),
      eobi_markets_(t_sec_name_indexer_.NumSecurityId(), new EOBIMarketPerSecurity()) {}

void EOBIMarketOrderManager::OnOrderAdd(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                                        const uint32_t priority, const double price, const uint32_t size) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  EOBIMarketPerSecurity* mkt = eobi_markets_[security_id];

  switch (buysell) {
    case kTradeTypeBuy: {
      if (mkt->order_id_bids_map_.find(order_id) != mkt->order_id_bids_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " already present in bid map" << DBGLOG_ENDL_FLUSH;
        break;
      }

      EOBIOrder* order = eobi_order_mempool_.Alloc();
      order->price = price;
      order->size = size;
      mkt->order_id_bids_map_[order_id] = order;
      mov->AddMarketBidBack(order_id, price, size);
      break;
    }
    case kTradeTypeSell: {
      if (mkt->order_id_asks_map_.find(order_id) != mkt->order_id_asks_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " already present in ask map" << DBGLOG_ENDL_FLUSH;
        break;
      }

      EOBIOrder* order = eobi_order_mempool_.Alloc();
      order->price = price;
      order->size = size;
      mkt->order_id_asks_map_[order_id] = order;
      mov->AddMarketAskBack(order_id, price, size);
      break;
    }
    default: { break; }
  }
}

void EOBIMarketOrderManager::OnOrderModifyWithPrevOrderId(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                          const int64_t t_new_order_id_, const double t_price_,
                                                          const uint32_t t_size_, const int64_t t_prev_order_id_,
                                                          const double t_prev_price_, const uint32_t t_prev_size_) {
  // This changes priority as well
  // So, do a delete followed by add
  MarketOrdersView* mov_ = market_orders_view_map_[t_security_id_];
  EOBIMarketPerSecurity* mkt = eobi_markets_[t_security_id_];
  // If the order gets changed, simply delete and add the order
  // instead of leaving this decision to market_orders_view
  if (t_new_order_id_ != t_prev_order_id_) {
    OnOrderDelete(t_security_id_, t_buysell_, t_prev_order_id_);

    OnOrderAdd(t_security_id_, t_buysell_, t_new_order_id_, 0, t_price_, t_size_);
  } else {
    switch (t_buysell_) {
      case kTradeTypeBuy: {
        if (mkt->order_id_bids_map_.find(t_new_order_id_) != mkt->order_id_bids_map_.end()) {
          EOBIOrder* order = mkt->order_id_bids_map_[t_new_order_id_];

          order->size = int(t_size_);
          mov_->ModifyMarketBid(t_prev_order_id_, t_prev_price_, t_prev_size_, t_new_order_id_, t_price_, t_size_);
        }
      } break;
      case kTradeTypeSell: {
        if (mkt->order_id_asks_map_.find(t_new_order_id_) != mkt->order_id_asks_map_.end()) {
          EOBIOrder* order = mkt->order_id_asks_map_[t_new_order_id_];

          order->size = int(t_size_);
          mov_->ModifyMarketAsk(t_prev_order_id_, t_prev_price_, t_prev_size_, t_new_order_id_, t_price_, t_size_);
        }
      } break;
      default:
        break;
    }
  }
}

// Deletes a part of the order
void EOBIMarketOrderManager::OnVolumeDelete(const uint32_t security_id, const TradeType_t buysell,
                                            const uint64_t order_id, const uint32_t size) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  EOBIMarketPerSecurity* mkt = eobi_markets_[security_id];

  switch (buysell) {
    case kTradeTypeBuy: {
      if (mkt->order_id_bids_map_.find(order_id) == mkt->order_id_bids_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " is not present in bid map" << DBGLOG_ENDL_FLUSH;
        break;
      }

      EOBIOrder* order = mkt->order_id_bids_map_[order_id];

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

      EOBIOrder* order = mkt->order_id_asks_map_[order_id];

      mov->ModifyMarketAsk(order_id, order->price, order->size, order_id, order->price, size);

      order->size = size;  // Update size
      break;
    }
    default: { break; }
  }
}

// Deletes the entire order
void EOBIMarketOrderManager::OnOrderDelete(const uint32_t security_id, const TradeType_t buysell,
                                           const uint64_t order_id) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  EOBIMarketPerSecurity* mkt = eobi_markets_[security_id];

  switch (buysell) {
    case kTradeTypeBuy: {
      if (mkt->order_id_bids_map_.find(order_id) == mkt->order_id_bids_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " is not present in bid map" << DBGLOG_ENDL_FLUSH;
        break;
      }

      EOBIOrder* order = mkt->order_id_bids_map_[order_id];

      mov->DeleteMarketBid(order_id, order->price, order->size);

      // Remove/Dealloc order
      mkt->order_id_bids_map_.erase(order_id);
      eobi_order_mempool_.DeAlloc(order);
      break;
    }
    case kTradeTypeSell: {
      if (mkt->order_id_asks_map_.find(order_id) == mkt->order_id_asks_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " is not present in ask map" << DBGLOG_ENDL_FLUSH;
        break;
      }

      EOBIOrder* order = mkt->order_id_asks_map_[order_id];

      mov->DeleteMarketAsk(order_id, order->price, order->size);

      // Remove/Dealloc order
      mkt->order_id_asks_map_.erase(order_id);
      eobi_order_mempool_.DeAlloc(order);
      break;
    }
    default: { break; }
  }
}

void EOBIMarketOrderManager::OnOrderExec(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                                         const double _exec_price, const uint32_t size_exec,
                                         const uint32_t size_remaining) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  EOBIMarketPerSecurity* mkt = eobi_markets_[security_id];

  switch (buysell) {
    case kTradeTypeBuy: {
      if (mkt->order_id_bids_map_.find(order_id) == mkt->order_id_bids_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " is not present in bid map" << DBGLOG_ENDL_FLUSH;
        break;
      }

      EOBIOrder* order = mkt->order_id_bids_map_[order_id];

      if (mov->GetIntPx(_exec_price) != mov->GetIntPx(order->price)) {
        // Skip order exec with price change updates for exec
        break;
      }

      mov->ExecMarketBid(order_id, _exec_price, size_exec, false);
      if ((size_remaining) == 0) {
        // delete the order
        OnOrderDelete(security_id, buysell, order_id);
      } else if ((size_remaining) == 1) {
        // partial execution
        OnVolumeDelete(security_id, buysell, order_id, (order->size - size_exec));
      }
      break;
    }
    case kTradeTypeSell: {
      if (mkt->order_id_asks_map_.find(order_id) == mkt->order_id_asks_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << order_id << " is not present in ask map" << DBGLOG_ENDL_FLUSH;
        break;
      }

      EOBIOrder* order = mkt->order_id_asks_map_[order_id];
      if (mov->GetIntPx(_exec_price) != mov->GetIntPx(order->price)) {
        // Skip order exec with price change updates for exec
        break;
      }

      mov->ExecMarketAsk(order_id, _exec_price, size_exec, false);
      if ((size_remaining) == 0) {
        // delete the order
        OnOrderDelete(security_id, buysell, order_id);
      } else if ((size_remaining) == 1) {
        // partial execution
        OnVolumeDelete(security_id, buysell, order_id, (order->size - size_exec));
      }
      break;
    }
    default: { break; }
  }
}

void EOBIMarketOrderManager::ResetBook(const unsigned int security_id) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  EOBIMarketPerSecurity* mkt = eobi_markets_[security_id];

  for (auto order : mkt->order_id_bids_map_) {
    eobi_order_mempool_.DeAlloc(order.second);
  }

  for (auto order : mkt->order_id_asks_map_) {
    eobi_order_mempool_.DeAlloc(order.second);
  }

  mkt->Clear();
  mov->Reset();
}
}
