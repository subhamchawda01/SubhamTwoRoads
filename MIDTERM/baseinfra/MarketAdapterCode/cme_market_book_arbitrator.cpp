// =====================================================================================
//
//       Filename:  baseinfra/MarketAdapterCode/cme_market_book_arbitrator.cpp
//
//    Description:  Utility Functions for fetching the Sample Data values from the last 30 days
//
//        Version:  1.0
//        Created:  01/15/2016 12:38:39 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include "baseinfra/MarketAdapter/cme_market_book_arbitrator.hpp"

namespace HFSAT {

CMEMarketBookArbitrator::CMEMarketBookArbitrator(DebugLogger& dbglogger, Watch& watch,
                                                 SecurityNameIndexer& sec_name_indexer,
                                                 const std::vector<HFSAT::MarketOrdersView*>& sid_to_mov_ptr_map)
    : BaseMarketOrderManager(dbglogger, watch, sec_name_indexer, sid_to_mov_ptr_map),
      dbglogger_(dbglogger),
      watch_(watch),
      cme_order_mempool_(),
      cme_markets_(sec_name_indexer.NumSecurityId(), new CMEMarketPerSecurity()),
      market_orders_view_map_(sid_to_mov_ptr_map),
      multicast_sender_socket_(nullptr) {
  LoadLiveSourceProductCodes(cme_ls_shc_to_code_map_, DEF_CME_LS_PRODUCTCODE_SHORTCODE_);
  dbglogger_.SetNoLogs();
}

/**
 * The order got executed. Check if it was confirmed within certain time. If yes then the order must have been
 * aggressive in market in that case we can assume the level was cleared
 *
 * @param t_server_assigned_client_id
 * @param client_assigned_order_sequence
 * @param _server_assigned_order_sequence_
 * @param security_id
 * @param price
 * @param r_buysell
 * @param size_remaining
 * @param size_executed
 * @param client_position
 * @param global_position
 * @param r_int_price
 * @param t_server_assigned_message_sequence
 */
void CMEMarketBookArbitrator::OrderExecuted(const int t_server_assigned_client_id,
                                            const int client_assigned_order_sequence,
                                            const int t_server_assigned_order_sequence, const unsigned int security_id,
                                            const double price, const TradeType_t r_buysell, const int size_remaining,
                                            const int size_executed, const int client_position,
                                            const int global_position, const int r_int_price,
                                            const int32_t t_server_assigned_message_sequence,
                                            const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  uint64_t orderid = exchange_order_id;
  auto saos_iter = saos_orderid_map_.find(t_server_assigned_order_sequence);
  if (saos_iter != saos_orderid_map_.end()) {
    orderid = saos_iter->second;
  }

  auto iter = saos_order_map_.find(orderid);
  if (iter != saos_order_map_.end()) {
    if (watch_.tv() - iter->second->conf_time < ttime_t(0, MAX_CONF_EXEC_USECS_TO_CONSIDER)) {
      iter->second->exec_time = watch_.tv();
      DBGLOG_TIME_CLASS_FUNC_LINE << " CMEARB Exec SAOS: " << iter->first << " " << iter->second->ToString()
                                  << DBGLOG_ENDL_FLUSH;
      // Trigger for synthetic DEL messages
      if (nullptr != multicast_sender_socket_ &&
          PopulateCMELSCommonStruct(t_server_assigned_client_id, client_assigned_order_sequence,
                                    t_server_assigned_order_sequence, security_id, price, r_buysell, size_remaining,
                                    size_executed, client_position, global_position, r_int_price,
                                    t_server_assigned_message_sequence, exchange_order_id, time_set_by_server)) {
        multicast_sender_socket_->WriteN(sizeof(cme_ls_data_), &cme_ls_data_);
      } else {
        DBGLOG_TIME_CLASS_FUNC_LINE << " CMEARB MCast Sock null OR Prod code Not found! Will not multicast"
                                    << DBGLOG_ENDL_FLUSH;
      }
    }
  }
}

/**
 *
 * @param t_server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param _server_assigned_order_sequence_
 * @param _security_id_
 * @param _price_
 * @param r_buysell_
 * @param _size_remaining_
 * @param _size_executed_
 * @param _client_position_
 * @param _global_position_
 * @param r_int_price_
 * @param t_server_assigned_message_sequence
 */
void CMEMarketBookArbitrator::OrderConfirmed(
    const int t_server_assigned_client_id, const int t_client_assigned_order_sequence,
    const int t_server_assigned_order_sequence, const unsigned int t_security_id, const double price,
    const TradeType_t r_buysell, const int size_remaining, const int size_executed, const int _client_position,
    const int global_position, const int r_int_price, const int32_t t_server_assigned_message_sequence,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  CMEMarketPerSecurity* mkt = cme_markets_[t_security_id];

  auto order = new CMEORSOrder(exchange_order_id, t_server_assigned_order_sequence, price, size_remaining);
  order->conf_time = watch_.tv();

  // Put the order in map
  auto iter = saos_order_map_.find(exchange_order_id);
  if (iter == saos_order_map_.end()) {
    saos_order_map_[exchange_order_id] = order;
    saos_orderid_map_[t_server_assigned_order_sequence] = exchange_order_id;

    // Check if order is already in market data
    if (r_buysell == kTradeTypeBuy) {
      auto iter = mkt->order_id_bids_map_.find(exchange_order_id);
      if (iter != mkt->order_id_bids_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << " CMEARB AlreadyInMktData: " << GetTradeTypeChar(r_buysell) << " " << watch_.tv()
                               << " MktDelay: " << (watch_.tv() - mkt_bid_order_time_map_[exchange_order_id]).ToString()
                               << " " << order->ToString() << DBGLOG_ENDL_FLUSH;
      }
    } else if (r_buysell == kTradeTypeSell) {
      auto iter = mkt->order_id_asks_map_.find(exchange_order_id);
      if (iter != mkt->order_id_asks_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << " CMEARB AlreadyInMktData: " << GetTradeTypeChar(r_buysell) << " " << watch_.tv()
                               << " MktDelay: " << (watch_.tv() - mkt_ask_order_time_map_[exchange_order_id]).ToString()
                               << " " << order->ToString() << DBGLOG_ENDL_FLUSH;
      }
    }
  }
}

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
void CMEMarketBookArbitrator::OnOrderAdd(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                                         const uint32_t priority, const double price, const uint32_t size) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  CMEMarketPerSecurity* mkt = cme_markets_[security_id];

  if (dbglogger_.CheckLoggingLevel(BOOK_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "OnOrderAdd: " << security_id << " Type: " << GetTradeTypeChar(buysell)
                           << " OrderID: " << order_id << " Priority: " << priority << " px: " << price
                           << " size: " << size << DBGLOG_ENDL_FLUSH;
  }

  switch (buysell) {
    case kTradeTypeBuy: {
      if (mkt->order_id_bids_map_.find(order_id) != mkt->order_id_bids_map_.end()) {
        if (dbglogger_.CheckLoggingLevel(BOOK_ERROR)) {
          DBGLOG_TIME_CLASS_FUNC << "ERROR"
                                 << " OrderId: " << order_id << " already present in bid map" << DBGLOG_ENDL_FLUSH;
        }
        break;
      }

      CMEOrder* order = cme_order_mempool_.Alloc();
      order->price = price;
      order->size = size;
      mkt->order_id_bids_map_[order_id] = order;
      mkt_bid_order_time_map_[order_id] = watch_.tv();
      mov->AddMarketBidPriority(order_id, price, size, priority, false);
      break;
    }
    case kTradeTypeSell: {
      if (mkt->order_id_asks_map_.find(order_id) != mkt->order_id_asks_map_.end()) {
        if (dbglogger_.CheckLoggingLevel(BOOK_ERROR)) {
          DBGLOG_TIME_CLASS_FUNC << "ERROR"
                                 << " OrderId: " << order_id << " already present in ask map" << DBGLOG_ENDL_FLUSH;
        }
        break;
      }

      CMEOrder* order = cme_order_mempool_.Alloc();
      order->price = price;
      order->size = size;
      mkt->order_id_asks_map_[order_id] = order;
      mkt_ask_order_time_map_[order_id] = watch_.tv();
      mov->AddMarketAskPriority(order_id, price, size, priority, false);
      break;
    }
    default: { break; }
  }

  auto self_iter = saos_order_map_.find(order_id);
  if (self_iter != saos_order_map_.end()) {
    DBGLOG_TIME_CLASS_FUNC << " CMEARB RemoveOnAddMktData: " << GetTradeTypeChar(buysell) << " " << watch_.tv()
                           << " MktDelay: " << (watch_.tv() - self_iter->second->conf_time).ToString() << " "
                           << self_iter->second->ToString() << DBGLOG_ENDL_FLUSH;
    saos_order_map_.erase(self_iter);
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
void CMEMarketBookArbitrator::OnOrderModify(const uint32_t security_id, const TradeType_t buysell,
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

  if (dbglogger_.CheckLoggingLevel(BOOK_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "OnOrderModify: " << security_id << " Type: " << GetTradeTypeChar(buysell)
                           << " OrderID: " << order_id << " Priority: " << priority << " px: " << price
                           << " size: " << size << DBGLOG_ENDL_FLUSH;
  }

  switch (buysell) {
    case kTradeTypeBuy: {
      auto iter = mkt->order_id_bids_map_.find(order_id);
      if (iter == mkt->order_id_bids_map_.end()) {
        if (dbglogger_.CheckLoggingLevel(BOOK_ERROR)) {
          DBGLOG_TIME_CLASS_FUNC << "ERROR"
                                 << " OrderId: " << order_id << " not present in bid map" << DBGLOG_ENDL_FLUSH;
        }
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
        if (dbglogger_.CheckLoggingLevel(BOOK_ERROR)) {
          DBGLOG_TIME_CLASS_FUNC << "ERROR"
                                 << " OrderId: " << order_id << " not present in ask map" << DBGLOG_ENDL_FLUSH;
        }
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
void CMEMarketBookArbitrator::OnVolumeDelete(const uint32_t security_id, const TradeType_t buysell,
                                             const uint64_t order_id, const uint32_t size) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  CMEMarketPerSecurity* mkt = cme_markets_[security_id];

  switch (buysell) {
    case kTradeTypeBuy: {
      auto iter = mkt->order_id_bids_map_.find(order_id);
      if (iter == mkt->order_id_bids_map_.end()) {
        if (dbglogger_.CheckLoggingLevel(BOOK_ERROR)) {
          DBGLOG_TIME_CLASS_FUNC << "ERROR"
                                 << " OrderId: " << order_id << " is not present in bid map" << DBGLOG_ENDL_FLUSH;
        }
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
        if (dbglogger_.CheckLoggingLevel(BOOK_ERROR)) {
          DBGLOG_TIME_CLASS_FUNC << "ERROR: "
                                 << " OrderId: " << order_id
                                 << " volume deleted is more than order-size, deleting the order" << DBGLOG_ENDL_FLUSH;
        }
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
        if (dbglogger_.CheckLoggingLevel(BOOK_ERROR)) {
          DBGLOG_TIME_CLASS_FUNC << "ERROR"
                                 << " OrderId: " << order_id << " is not present in ask map" << DBGLOG_ENDL_FLUSH;
        }
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
        if (dbglogger_.CheckLoggingLevel(BOOK_ERROR)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << "ERROR: "
                                      << " OrderId: " << order_id
                                      << " volume deleted is more than order-size, deleting the order"
                                      << DBGLOG_ENDL_FLUSH;
        }

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
void CMEMarketBookArbitrator::OnOrderDelete(const uint32_t security_id, const TradeType_t buysell,
                                            const uint64_t order_id) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  CMEMarketPerSecurity* mkt = cme_markets_[security_id];

  if (dbglogger_.CheckLoggingLevel(BOOK_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "OnOrderDelete: " << security_id << " Type: " << GetTradeTypeChar(buysell)
                           << " OrderID: " << order_id << " Priority: " << DBGLOG_ENDL_FLUSH;
  }

  switch (buysell) {
    case kTradeTypeBuy: {
      if (mkt->order_id_bids_map_.find(order_id) == mkt->order_id_bids_map_.end()) {
        if (dbglogger_.CheckLoggingLevel(BOOK_ERROR)) {
          DBGLOG_TIME_CLASS_FUNC << "ERROR"
                                 << " OrderId: " << order_id << " is not present in bid map" << DBGLOG_ENDL_FLUSH;
        }
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
        if (dbglogger_.CheckLoggingLevel(BOOK_ERROR)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << "ERROR"
                                      << " OrderId: " << order_id << " is not present in ask map" << DBGLOG_ENDL_FLUSH;
        }
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
void CMEMarketBookArbitrator::OnOrderExec(const uint32_t security_id, const TradeType_t buysell,
                                          const uint64_t order_id, const double _exec_price, const uint32_t size_exec,
                                          const uint32_t size_remaining) {
  // buysell, exec-price, size remaining are useless in cme

  MarketOrdersView* mov = market_orders_view_map_[security_id];
  CMEMarketPerSecurity* mkt = cme_markets_[security_id];

  if (dbglogger_.CheckLoggingLevel(BOOK_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "OnOrderModify: " << security_id << " Type: " << GetTradeTypeChar(buysell)
                           << " OrderID: " << order_id << " Priority: "
                           << " size: " << size_exec << DBGLOG_ENDL_FLUSH;
  }

  TradeType_t this_buysell = buysell;
  if (this_buysell == kTradeTypeNoInfo) {
    auto bid_iter = mkt->order_id_bids_map_.find(order_id);
    if (bid_iter != mkt->order_id_bids_map_.end()) {
      this_buysell = kTradeTypeBuy;

      CMEOrder* order = bid_iter->second;
      mov->ExecMarketBid(order_id, order->price, size_exec, false);

      // Send the size-executed here
      OnVolumeDelete(security_id, this_buysell, order_id, size_exec);
    }
  }

  if (this_buysell == kTradeTypeNoInfo) {
    auto ask_iter = mkt->order_id_asks_map_.find(order_id);
    if (ask_iter != mkt->order_id_asks_map_.end()) {
      this_buysell = kTradeTypeSell;

      CMEOrder* order = ask_iter->second;

      mov->ExecMarketAsk(order_id, order->price, size_exec, false);

      // Send the size-executed here
      OnVolumeDelete(security_id, this_buysell, order_id, size_exec);
    }
  }

  auto self_iter = saos_order_map_.find(order_id);
  if (self_iter != saos_order_map_.end()) {
    if (self_iter->second->exec_time != ttime_t(0, 0)) {
      DBGLOG_TIME_CLASS_FUNC << " CMEARB ExecInMarketData: " << GetTradeTypeChar(this_buysell) << " " << watch_.tv()
                             << " MktDelay: " << (watch_.tv() - self_iter->second->exec_time).ToString() << " "
                             << self_iter->second->ToString() << DBGLOG_ENDL_FLUSH;
    }
  }
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
void CMEMarketBookArbitrator::OnOrderExecWithPx(const uint32_t security_id, const uint64_t bid_order_id,
                                                const uint64_t ask_order_id, const double exec_price,
                                                const uint32_t size_exec, const uint32_t bid_remaining,
                                                const uint32_t ask_remaining) {
  OnOrderExec(security_id, kTradeTypeBuy, bid_order_id, exec_price, size_exec, bid_remaining);
  OnOrderExec(security_id, kTradeTypeSell, ask_order_id, exec_price, size_exec, ask_remaining);
}

void CMEMarketBookArbitrator::OnOrderSpExec(const uint32_t security_id, const TradeType_t buysell,
                                            const uint64_t order_id, const double exec_price, const uint32_t size_exec,
                                            const uint32_t size_remaining) {
  CMEMarketPerSecurity* mkt = cme_markets_[security_id];
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
void CMEMarketBookArbitrator::ResetBook(const unsigned int security_id) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  CMEMarketPerSecurity* mkt = cme_markets_[security_id];

  for (auto order : mkt->order_id_bids_map_) {
    cme_order_mempool_.DeAlloc(order.second);
  }

  for (auto order : mkt->order_id_asks_map_) {
    cme_order_mempool_.DeAlloc(order.second);
  }

  mkt->Clear();
  mov->Reset();
}

void CMEMarketBookArbitrator::SetMultiSenderSocket(HFSAT::MulticastSenderSocket* mcast_sock) {
  multicast_sender_socket_ = mcast_sock;
}

// This part of code is copied from CombinedShmMulticaster
// It loads product codes for popular CME products
void CMEMarketBookArbitrator::LoadLiveSourceProductCodes(std::map<std::string, int>& shc_to_code_map,
                                                         const char* file_name) {
  std::ifstream shc_code_file;
  shc_code_file.open(file_name, std::ifstream::in);

  if (!shc_code_file.is_open()) {
    std::cerr << "Could not open " << file_name << std::endl;
    exit(-1);
  }
  if (shc_code_file.is_open()) {
    char line[1024];
    while (!shc_code_file.eof()) {
      bzero(line, 1024);
      shc_code_file.getline(line, 1024);
      if (strlen(line) == 0 || strstr(line, "#") != NULL) continue;
      HFSAT::PerishableStringTokenizer st_(line, 1024);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() < 2) {
        std::cerr << "Malformatted line in " << file_name << std::endl;
        exit(-1);
      }

      std::string shc = tokens_[1];
      int code = atoi(tokens_[0]);

      shc_to_code_map[shc] = code;
    }
    shc_code_file.close();
    std::cout << "Loaded product codes for " << shc_to_code_map.size() << " securities" << std::endl;
  }
}

bool CMEMarketBookArbitrator::PopulateCMELSCommonStruct(
    const int t_server_assigned_client_id, const int client_assigned_order_sequence,
    const int t_server_assigned_order_sequence, const unsigned int security_id, const double price,
    const TradeType_t r_buysell, const int size_remaining, const int size_executed, const int client_position,
    const int global_position, const int r_int_price, const int32_t t_server_assigned_message_sequence,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  cme_ls_data_.msg_ = CME_MDS::CME_DELTA;
  // Get the product code
  std::string shc = HFSAT::SecurityNameIndexer::GetUniqueInstance().GetShortcodeFromId(security_id);
  if (cme_ls_shc_to_code_map_.find(shc) == cme_ls_shc_to_code_map_.end()) {
    DBGLOG_TIME_CLASS_FUNC << "SHC: " << shc << " not found in CME livesource -> contract code map"
                           << DBGLOG_ENDL_FLUSH;
    return false;
  }

  cme_ls_data_.data_.cme_dels_.contract_code_ = (uint8_t)cme_ls_shc_to_code_map_[shc];
  cme_ls_data_.data_.cme_dels_.action_ = '9';  // synthetic delete
  cme_ls_data_.data_.cme_dels_.price_ = price;
  cme_ls_data_.data_.cme_dels_.type_ =
      (r_buysell == TradeType_t::kTradeTypeBuy) ? '0' : (r_buysell == TradeType_t::kTradeTypeSell) ? '1' : '2';
  cme_ls_data_.data_.cme_dels_.intermediate_ = false;

  // What to fill here, we do not have any info?????
  cme_ls_data_.data_.cme_dels_.level_ = 1;

  return true;
}
}
