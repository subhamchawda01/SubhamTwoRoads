
#include "baseinfra/MarketAdapter/sgx_market_order_manager.hpp"
namespace HFSAT {

SGXMarketOrderManager::SGXMarketOrderManager(DebugLogger& dbglogger, const Watch& t_watch_,
                                             SecurityNameIndexer& t_sec_name_indexer_,
                                             const std::vector<MarketOrdersView*>& t_market_orders_view_map_)
    : BaseMarketOrderManager(dbglogger, t_watch_, t_sec_name_indexer_, t_market_orders_view_map_),
      previous_delete_position(0),
      previous_delete_order_id(0),
      previous_delete_size(0),
      sgx_order_mempool_(),
      sgx_markets_(t_sec_name_indexer_.NumSecurityId(), new SGXMarketPerSecurity()) {}

void SGXMarketOrderManager::OnOrderAdd(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                                       const uint32_t priority, const double price, const uint32_t size) {
  // check if the order exists in the order map if exists then the order_id is not unique hence print an error message

  MarketOrdersView* mov = market_orders_view_map_[security_id];
  SGXMarketPerSecurity* mkt = sgx_markets_[security_id];

  if (buysell == kTradeTypeBuy) {
    // check if the order exists in the order map if exists then the order_id is not unique hence print an error message
    if (mkt->order_id_bids_map_.find(order_id) != mkt->order_id_bids_map_.end()) {
      DBGLOG_TIME_CLASS_FUNC << "Order Exists "
                             << " OrderId: " << order_id << DBGLOG_ENDL_FLUSH;
      return;
    }
    // If the order is unique then add it in the bid_order_map
    SGXOrder* order = sgx_order_mempool_.Alloc();
    order->price = price;
    order->size = size;
    mkt->order_id_bids_map_[order_id] = order;
    /* Check to see if this add is part of order modification and if new size is less than previous size than maintain
     * the order's queue position. For more details see function SGXMarketOrderManager::OnOrderDelete */
    if (previous_delete_order_id == order_id && previous_delete_size > size) {
      mov->AddMarketBidPriority(order_id, price, size, previous_delete_position, true);
      return;
    }
    // Update the mov to reflect the current order
    // Since there is not prioirty order hence add the order to the normal back of the queue
    mov->AddMarketBidBack(order_id, price, size);
    previous_delete_order_id = 0;
    return;

  } else if (buysell == kTradeTypeSell) {
    // check if the order exists in the order map if exists then the order_id is not unique hence print an error message
    if (mkt->order_id_asks_map_.find(order_id) != mkt->order_id_asks_map_.end()) {
      DBGLOG_TIME_CLASS_FUNC << "Order Exists "
                             << " OrderId: " << order_id << DBGLOG_ENDL_FLUSH;
    }
    // If the order is unique then add it in the bid_order_map
    SGXOrder* order = sgx_order_mempool_.Alloc();
    order->price = price;
    order->size = size;
    mkt->order_id_asks_map_[order_id] = order;
    if (previous_delete_order_id == order_id && previous_delete_size > size) {
      mov->AddMarketAskPriority(order_id, price, size, previous_delete_position, true);
      return;
    }
    // Update the mov to reflect the current order
    // Since there is not prioirty order hence add the order to the normal back of the queue
    mov->AddMarketAskBack(order_id, price, size);
    previous_delete_order_id = 0;
    return;
  } else {
    DBGLOG_TIME_CLASS_FUNC << "ERROR"
                           << " OrderId: " << order_id << " The order to add type is neither buy nor sell "
                           << DBGLOG_ENDL_FLUSH;
  }
}

void SGXMarketOrderManager::OnOrderModify(const uint32_t security_id, const TradeType_t buysell,
                                          const uint64_t order_id, const uint32_t priority, const double price,
                                          const uint32_t size) {
  // check if the order exists in the order map if exists then the order_id is not unique hence print an error message

  MarketOrdersView* mov = market_orders_view_map_[security_id];
  SGXMarketPerSecurity* mkt = sgx_markets_[security_id];

  if (buysell == kTradeTypeBuy && mkt->order_id_bids_map_.find(order_id) != mkt->order_id_bids_map_.end()) {
    mkt->order_id_bids_map_[order_id]->size = size;
    mov->AddMarketBidPriority(order_id, price, size, priority, true);
    return;
  } else if (buysell == kTradeTypeSell && mkt->order_id_asks_map_.find(order_id) != mkt->order_id_asks_map_.end()) {
    mkt->order_id_asks_map_[order_id]->size = size;
    mov->AddMarketAskPriority(order_id, price, size, priority, true);
    return;
  } else {
    DBGLOG_TIME_CLASS_FUNC << "ERROR"
                           << " OrderId: " << order_id << " The order to add type is neither buy nor sell "
                           << DBGLOG_ENDL_FLUSH;
  }
}

void SGXMarketOrderManager::OnOrderDelete(const uint32_t security_id, const TradeType_t buysell,
                                          const uint64_t order_id) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  SGXMarketPerSecurity* mkt = sgx_markets_[security_id];
  if (buysell == kTradeTypeBuy) {
    // check if the order to be deleted is in the order map if not then raise an error
    if (mkt->order_id_bids_map_.find(order_id) == mkt->order_id_bids_map_.end()) {
      DBGLOG_TIME_CLASS_FUNC << "ERROR"
                             << " OrderId: " << order_id << " not present in bid map" << DBGLOG_ENDL_FLUSH;
      return;
    }

    // Order exists hence remove it from the sgx_market map
    SGXOrder* order = mkt->order_id_bids_map_[order_id];
    /*previous_delete_* variables are added to handle market modify orders as all market modifies are represented in the
     * exchange data as delete followed by an add immediately so whenever we encounter a delete w store its position and
     * order id and then whenever there is an add followed by the same order it and the size is less than previous
     * delete size we give the order its earlier position in the queue*/
    int order_int_price = mov->GetIntPx(order->price);
    ExchMarketOrder* delete_order = mov->FindMarketBid(order_id, order_int_price);
    if (delete_order != nullptr) {
      previous_delete_order_id = order_id;
      previous_delete_position = mov->GetBidPosition(delete_order);
      previous_delete_size = order->size;
    } else {
      DBGLOG_TIME_CLASS_FUNC << "ERROR"
                             << " OrderId: " << order_id << " not present in mov" << DBGLOG_ENDL_FLUSH;
    }
    // delete the order from mov
    mov->DeleteMarketBid(order_id, order->price, order->size);
    // delete the order from bid_mkt_vec map
    mkt->order_id_bids_map_.erase(order_id);
    // deallocate the mempool by removing the order pointer
    sgx_order_mempool_.DeAlloc(order);
  } else if (buysell == kTradeTypeSell) {
    // check if the order to be deleted is in the order map if not then raise an error
    if (mkt->order_id_asks_map_.find(order_id) == mkt->order_id_asks_map_.end()) {
      DBGLOG_TIME_CLASS_FUNC << "ERROR"
                             << " OrderId: " << order_id << " not present in ask map" << DBGLOG_ENDL_FLUSH;
      return;
    }

    SGXOrder* order = mkt->order_id_asks_map_[order_id];
    int order_int_price = mov->GetIntPx(order->price);
    ExchMarketOrder* delete_order = mov->FindMarketAsk(order_id, order_int_price);
    if (delete_order != nullptr) {
      previous_delete_order_id = order_id;
      previous_delete_position = mov->GetAskPosition(delete_order);
      previous_delete_size = order->size;
    } else {
      DBGLOG_TIME_CLASS_FUNC << "ERROR"
                             << " OrderId: " << order_id << " not present in mov" << DBGLOG_ENDL_FLUSH;
    }
    // delete the order from mov
    mov->DeleteMarketAsk(order_id, order->price, order->size);

    // delete the order from ask_mkt_vec map
    mkt->order_id_asks_map_.erase(order_id);
    // deallocate the mempool by removing the order pointer
    sgx_order_mempool_.DeAlloc(order);
  } else {
    DBGLOG_TIME_CLASS_FUNC << "ERROR the order to delete with "
                           << " OrderId: " << order_id << " is neither of sell or buy type" << DBGLOG_ENDL_FLUSH;
  }
}

void SGXMarketOrderManager::OnOrderExec(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                                        const double exec_price, const uint32_t size_exec,
                                        const uint32_t size_remaining) {
  // SGX does not send the exec_price and size_remaining so i am taking this argument -1 by default . these value has to
  // be computed from the order map
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  SGXMarketPerSecurity* mkt = sgx_markets_[security_id];
  double execution_price_;
  if (buysell == kTradeTypeBuy) {
    // Check if the order exists in the map or not
    if (mkt->order_id_bids_map_.find(order_id) == mkt->order_id_bids_map_.end()) {
      DBGLOG_TIME_CLASS_FUNC << "ERROR"
                             << " OrderId: " << order_id << " not present in bid map" << DBGLOG_ENDL_FLUSH;
      return;
    }
    SGXOrder* order = mkt->order_id_bids_map_[order_id];
    execution_price_ = order->price;
    if (mov->GetBestBidIntPrice() != mov->GetIntPx(execution_price_)) {
      // skip the order that leads to price change
      return;
    }
    // execute the order
    mov->ExecMarketBid(order_id, execution_price_, size_exec, false);
    // check for remaining size
    if ((order->size - size_exec) == 0) {
      // delete the order
      OnOrderDelete(security_id, buysell, order_id);
    } else if ((order->size - size_exec) > 0) {
      // partial execution
      OnVolumeDelete(security_id, buysell, order_id, (order->size - size_exec));
    }
  } else if (buysell == kTradeTypeSell) {
    if (mkt->order_id_asks_map_.find(order_id) == mkt->order_id_asks_map_.end()) {
      DBGLOG_TIME_CLASS_FUNC << "ERROR"
                             << " OrderId: " << order_id << " not present in ask map" << DBGLOG_ENDL_FLUSH;
      return;
    }
    SGXOrder* order = mkt->order_id_asks_map_[order_id];
    execution_price_ = order->price;
    if (mov->GetBestAskIntPrice() != mov->GetIntPx(execution_price_)) {
      // skip the order that leads to price change
      return;
    }
    mov->ExecMarketAsk(order_id, execution_price_, size_exec, false);
    if ((order->size - size_exec) == 0) {
      // delete the order
      OnOrderDelete(security_id, buysell, order_id);
    } else if ((order->size - size_exec) > 0) {
      // partial execution
      OnVolumeDelete(security_id, buysell, order_id, (order->size - size_exec));
    }
  } else {
    DBGLOG_TIME_CLASS_FUNC << "ERROR the order to delete with "
                           << " OrderId: " << order_id << " is neither of sell or buy type" << DBGLOG_ENDL_FLUSH;
    return;
  }
}
void SGXMarketOrderManager::OnVolumeDelete(const uint32_t security_id, const TradeType_t buysell,
                                           const uint64_t order_id, const uint32_t size_remaining) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  SGXMarketPerSecurity* mkt = sgx_markets_[security_id];
  if (buysell == kTradeTypeBuy) {
    if (mkt->order_id_bids_map_.find(order_id) == mkt->order_id_bids_map_.end()) {
      DBGLOG_TIME_CLASS_FUNC << "ERROR"
                             << " OrderId: " << order_id << " is not present in bid map" << DBGLOG_ENDL_FLUSH;
      return;
    }
    SGXOrder* order = mkt->order_id_bids_map_[order_id];
    mov->ModifyMarketBid(order_id, order->price, order->size, order_id, order->price, size_remaining);
    order->size = size_remaining;
  } else if (buysell == kTradeTypeSell) {
    if (mkt->order_id_asks_map_.find(order_id) == mkt->order_id_asks_map_.end()) {
      DBGLOG_TIME_CLASS_FUNC << "ERROR"
                             << " OrderId: " << order_id << " not present in ask map" << DBGLOG_ENDL_FLUSH;
      return;
    }
    SGXOrder* order = mkt->order_id_asks_map_[order_id];
    mov->ModifyMarketAsk(order_id, order->price, order->size, order_id, order->price, size_remaining);
    order->size = size_remaining;
  }
}

void SGXMarketOrderManager::ResetBook(const unsigned int security_id) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  SGXMarketPerSecurity* mkt = sgx_markets_[security_id];
  for (auto order : mkt->order_id_bids_map_) {
    sgx_order_mempool_.DeAlloc(order.second);
  }

  for (auto order : mkt->order_id_asks_map_) {
    sgx_order_mempool_.DeAlloc(order.second);
  }
  mkt->Clear();
  mov->Reset();
}
}
