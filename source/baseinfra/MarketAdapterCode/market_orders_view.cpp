/**
   \file MarketAdapterCode/market_orders_view.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/market_orders_view.hpp"
#include "dvccode/CDef/security_definitions.hpp"

namespace HFSAT {

std::map<std::string, MarketOrdersView*> MarketOrdersView::SMM_desc_map_;

MarketOrdersView* MarketOrdersView::GetUniqueInstance(DebugLogger& dbglogger, const Watch& watch,
                                                      const unsigned int security_id) {
  std::ostringstream temp_oss;
  temp_oss << "MarketOrdersView " << security_id;
  std::string mov_desc(temp_oss.str());

  if (SMM_desc_map_.find(mov_desc) == SMM_desc_map_.end()) {
    SMM_desc_map_[mov_desc] = new MarketOrdersView(dbglogger, watch, security_id);
  }

  return SMM_desc_map_[mov_desc];
}

void MarketOrdersView::RemoveUniqueInstance(const unsigned int security_id) {
  std::ostringstream temp_oss;
  temp_oss << "MarketOrdersView " << security_id;
  std::string mov_desc(temp_oss.str());

  auto iter = SMM_desc_map_.find(mov_desc);
  if (iter != SMM_desc_map_.end()) {
    delete SMM_desc_map_[mov_desc];
    SMM_desc_map_.erase(iter);
  }
}
MarketOrdersView::MarketOrdersView(DebugLogger& dbglogger, const Watch& watch, const unsigned int security_id)
    : dbglogger_(dbglogger),
      watch_(watch),
      market_book_(new MarketBook(dbglogger, watch)),
      market_orders_notifier_(new MarketOrdersNotifier(dbglogger, watch)) {
  auto& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();

  std::string shortcode = sec_name_indexer.GetShortcodeFromId(security_id);
  int tradingdate = watch.YYYYMMDD();
  double min_price_inc = SecurityDefinitions::GetContractMinPriceIncrement(shortcode, tradingdate);

  fast_px_converter_ = new FastPriceConvertor(min_price_inc);
}

// <-------- Market Order book function ---------->

// Add market bid based on the priority of the order
void MarketOrdersView::AddMarketBidPriority(int64_t order_id, double price, int size, int64_t priority,
                                            bool position_based) {
  if (size <= 0) {
    DBGLOG_TIME_CLASS_FUNC << "Invalid size in add market bid. size: " << size << DBGLOG_ENDL_FLUSH;
    // Ignore invalid sized orders
    return;
  }

  int bid_int_price = GetIntPx(price);

  ExchMarketOrder* order = market_book_->GetExchMarketOrder(order_id, price, size, bid_int_price, priority);

  QueuePositionUpdate queue_pos;
  bool l1_update = false;
  if (!market_book_->AddBid(order, &queue_pos, position_based, l1_update)) {
    return;
  }

  // Notify the queue position listeners
  market_orders_notifier_->NotifyQueuePosChange(queue_pos, l1_update);
}

// Add market ask based on the priority of the order
void MarketOrdersView::AddMarketAskPriority(int64_t order_id, double price, int size, int64_t priority,
                                            bool position_based) {
  if (size <= 0) {
    DBGLOG_TIME_CLASS_FUNC << "Invalid size in add market ask. size: " << size << DBGLOG_ENDL_FLUSH;
    // Ignore invalid sized orders
    return;
  }

  int ask_int_price = GetIntPx(price);

  ExchMarketOrder* order = market_book_->GetExchMarketOrder(order_id, price, size, ask_int_price, priority);

  QueuePositionUpdate queue_pos;
  bool l1_update = false;
  if (!market_book_->AddAsk(order, &queue_pos, position_based, l1_update)) {
    return;
  }

  // Notify the queue position listeners
  market_orders_notifier_->NotifyQueuePosChange(queue_pos, l1_update);
}

void MarketOrdersView::AddMarketBidBack(int64_t order_id, double price, int size) {
  AddMarketBidPriority(order_id, price, size, 0);
}

void MarketOrdersView::AddMarketAskBack(int64_t order_id, double price, int size) {
  AddMarketAskPriority(order_id, price, size, 0);
}

void MarketOrdersView::ModifyMarketBid(int64_t prev_order_id, double prev_price, int prev_size, int64_t new_order_id,
                                       double price, int size) {
  int prev_int_price = GetIntPx(prev_price);
  int int_price = GetIntPx(price);

  // Modify can change the priority of the order
  // Order priority ( queue pos ) changes if order size increases
  // or if the order price changes

  ExchMarketOrder* order = market_book_->FindMarketBid(prev_order_id, prev_int_price, prev_size);

  if (order == nullptr) {
    AddMarketBidBack(new_order_id, price, size);
  } else if (size > order->size_ || order->int_price_ != int_price) {
    // Priority change is simply removing the prev order and adding new one
    QueuePositionUpdate queue_pos;
    bool l1_update = false;
    l1_update = market_book_->RemoveMarketBid(order, &queue_pos);
    market_orders_notifier_->NotifyQueuePosChange(queue_pos, l1_update);

    ExchMarketOrder* order = market_book_->GetExchMarketOrder(new_order_id, price, size, int_price, 0);

    if (!market_book_->AddBid(order, &queue_pos, false, l1_update)) {
      return;
    }

    market_orders_notifier_->NotifyQueuePosChange(queue_pos, l1_update);
  } else {
    int size_diff = order->size_ - size;
    order->size_ = size;

    int position = market_book_->GetBidPosition(order);
    if (position >= 0) {
      QueuePositionUpdate queue_pos;
      queue_pos.action_ = QueuePositionUpdate::OrderRemove;
      queue_pos.buysell_ = kTradeTypeBuy;
      queue_pos.int_price_ = int_price;
      queue_pos.position_ = position;
      queue_pos.size_ = size_diff;

      bool l1_update = market_book_->IsBidIntPriceL1(int_price);
      market_orders_notifier_->NotifyQueuePosChange(queue_pos, l1_update);
    } else if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "ERROR: -ve modify position." << DBGLOG_ENDL_FLUSH;
    }
  }
}

void MarketOrdersView::ModifyMarketAsk(int64_t t_prev_order_id_, double t_prev_price_, int t_prev_size_,
                                       int64_t t_new_order_id_, double t_price_, int t_size_) {
  int prev_int_price_ = GetIntPx(t_prev_price_);
  int int_price_ = GetIntPx(t_price_);

  // Modify can change the priority of the order
  // Order priority ( queue pos ) changes if order size increases
  // or if the order price changes

  ExchMarketOrder* order_ = market_book_->FindMarketAsk(t_prev_order_id_, prev_int_price_, t_prev_size_);

  if (order_ == nullptr) {
    AddMarketAskBack(t_new_order_id_, t_price_, t_size_);
  } else if (t_size_ > order_->size_ || order_->int_price_ != int_price_) {
    // Priority change is simply removing the prev order and adding new one
    QueuePositionUpdate queue_pos_;
    bool l1_update_ = false;
    l1_update_ = market_book_->RemoveMarketAsk(order_, &queue_pos_);
    market_orders_notifier_->NotifyQueuePosChange(queue_pos_, l1_update_);

    ExchMarketOrder* this_order_ = market_book_->GetExchMarketOrder(t_new_order_id_, t_price_, t_size_, int_price_, 0);

    if (!market_book_->AddAsk(this_order_, &queue_pos_, false, l1_update_)) {
      return;
    }
    market_orders_notifier_->NotifyQueuePosChange(queue_pos_, l1_update_);
  } else {
    // priority doesn't change,  keep the order as it was
    int size_diff_ = order_->size_ - t_size_;
    order_->size_ = t_size_;

    int position_ = market_book_->GetAskPosition(order_);
    if (position_ >= 0) {
      QueuePositionUpdate queue_pos_;
      queue_pos_.action_ = QueuePositionUpdate::OrderRemove;
      queue_pos_.buysell_ = kTradeTypeSell;
      queue_pos_.int_price_ = int_price_;
      queue_pos_.position_ = position_;
      queue_pos_.size_ = size_diff_;

      bool l1_update_ = market_book_->IsAskIntPriceL1(int_price_);
      market_orders_notifier_->NotifyQueuePosChange(queue_pos_, l1_update_);
    } else if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "ERROR: -ve modify position." << DBGLOG_ENDL_FLUSH;
    }
  }
}

void MarketOrdersView::DeleteMarketBid(int64_t order_id, double price, int size) {
  int bid_int_price = GetIntPx(price);

  ExchMarketOrder* this_order = market_book_->FindMarketBid(order_id, bid_int_price, size);

  if (this_order != nullptr) {
    QueuePositionUpdate queue_pos;
    bool l1_update = market_book_->RemoveMarketBid(this_order, &queue_pos);

    // Notify the queue position listeners
    market_orders_notifier_->NotifyQueuePosChange(queue_pos, l1_update);
  }
}

void MarketOrdersView::DeleteMarketAsk(int64_t order_id, double price, int size) {
  int ask_int_price = GetIntPx(price);

  ExchMarketOrder* order = market_book_->FindMarketAsk(order_id, ask_int_price, size);

  if (order != nullptr) {
    QueuePositionUpdate queue_pos;
    bool l1_update = market_book_->RemoveMarketAsk(order, &queue_pos);

    // Notify the queue position listeners
    market_orders_notifier_->NotifyQueuePosChange(queue_pos, l1_update);
  }
}

/**
 *
 * @param order_id
 * @param price
 * @param size
 * @param agg_order
 * @return
 */
int MarketOrdersView::ExecMarketBid(int64_t order_id, double price, int size, bool agg_order) {
  int bid_int_price = GetIntPx(price);

  ExchMarketOrder* order = market_book_->FindMarketBid(order_id, bid_int_price, size);

  if (!agg_order && order == nullptr) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR"
                           << " nullptr order in Exec Bid"
                           << " Size: " << size << " Price: " << price << " OrderId: " << order_id << DBGLOG_ENDL_FLUSH;

    return -1;
  }

  QueuePositionUpdate queue_pos;
  int position = -1;
  if (order) position = market_book_->GetBidPosition(order);

  if (agg_order || position >= 0) {
    queue_pos.action_ = QueuePositionUpdate::OrderExec;
    queue_pos.buysell_ = kTradeTypeBuy;
    queue_pos.int_price_ = bid_int_price;
    queue_pos.position_ = position;
    queue_pos.size_ = size;
    queue_pos.agg_order_ = agg_order;

    market_book_->ExecMarketBid(order, &queue_pos);

    market_orders_notifier_->NotifyQueuePosChange(queue_pos, true);
  } else if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR: -ve exec position." << DBGLOG_ENDL_FLUSH;
  }

  if (order != nullptr) {
    if (size <= order->size_) {
      return (order->size_ - size);
    }
  }

  return -1;  // -1 represents order not found case
}

int MarketOrdersView::ExecMarketAsk(int64_t t_order_id_, double t_price_, int t_size_, bool agg_order) {
  int ask_int_price_ = GetIntPx(t_price_);

  ExchMarketOrder* this_order_ = market_book_->FindMarketAsk(t_order_id_, ask_int_price_, t_size_);

  if (!agg_order && this_order_ == nullptr) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR"
                           << " nullptr order in Exec Ask"
                           << " Size: " << t_size_ << " Price: " << t_price_ << " OrderId: " << t_order_id_
                           << DBGLOG_ENDL_FLUSH;

    return -1;
  }

  QueuePositionUpdate queue_pos_;
  int position_ = -1;
  if (this_order_) position_ = market_book_->GetAskPosition(this_order_);

  if (agg_order || position_ >= 0) {
    queue_pos_.action_ = QueuePositionUpdate::OrderExec;
    queue_pos_.buysell_ = kTradeTypeSell;
    queue_pos_.int_price_ = ask_int_price_;
    queue_pos_.position_ = position_;
    queue_pos_.size_ = t_size_;
    queue_pos_.agg_order_ = agg_order;

    market_book_->ExecMarketAsk(this_order_, &queue_pos_);

    market_orders_notifier_->NotifyQueuePosChange(queue_pos_, true);
  } else if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR: -ve exec position." << DBGLOG_ENDL_FLUSH;
  }

  if (this_order_ != nullptr) {
    if (t_size_ <= this_order_->size_) {
      return (this_order_->size_ - t_size_);
    }
  }

  return -1;  // -1 represents order not found case
}

int MarketOrdersView::ExecMarketBidHidden(int64_t order_id, double price, int size_hidden_, int size_traded_) {
  int bid_int_price = GetIntPx(price);

  ExchMarketOrder* order = market_book_->FindMarketBid(order_id, bid_int_price, size_hidden_);

  if (order == nullptr) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR"
                           << " nullptr order in Exec Bid"
                           << " Size: " << size_traded_ << " Price: " << price << " OrderId: " << order_id
                           << DBGLOG_ENDL_FLUSH;

    return -1;
  }

  QueuePositionUpdate queue_pos;
  int position = -1;
  if (order) position = market_book_->GetBidPosition(order);

  if (position >= 0) {
    int size_to_requeue_ = std::min(size_hidden_, size_traded_);

    // First add the hidden order back to the queue at top
    queue_pos.action_ = QueuePositionUpdate::OrderAdd;
    queue_pos.buysell_ = kTradeTypeBuy;
    queue_pos.int_price_ = bid_int_price;
    queue_pos.position_ = 0;
    queue_pos.size_ = size_to_requeue_;
    market_orders_notifier_->NotifyQueuePosChange(queue_pos, true);

    // Delete the hidden order size from the position of order
    queue_pos.action_ = QueuePositionUpdate::OrderRemove;
    queue_pos.buysell_ = kTradeTypeBuy;
    queue_pos.int_price_ = bid_int_price;
    queue_pos.position_ = position;
    queue_pos.size_ = size_to_requeue_;
    market_orders_notifier_->NotifyQueuePosChange(queue_pos, true);

    // Now send the exec notifications
    queue_pos.action_ = QueuePositionUpdate::OrderExec;
    queue_pos.buysell_ = kTradeTypeBuy;
    queue_pos.int_price_ = bid_int_price;
    queue_pos.position_ = 0;
    // Size traded will be greater than size hidden if it is the single order  at that level
    queue_pos.size_ = size_traded_;
    market_orders_notifier_->NotifyQueuePosChange(queue_pos, true);

    // Even if size traded is more than order size (packet drop scenario),
    // then also delete the order

    if (size_traded_ >= order->size_) {
      bool l1_update = market_book_->RemoveMarketBid(order, &queue_pos);
      queue_pos.position_ = 0;
      market_orders_notifier_->NotifyQueuePosChange(queue_pos, l1_update);
      return 0;
    } else {
      order->size_ = order->size_ - size_traded_;
      queue_pos.action_ = QueuePositionUpdate::OrderRemove;
      queue_pos.buysell_ = kTradeTypeBuy;
      queue_pos.int_price_ = bid_int_price;
      queue_pos.position_ = 0;
      queue_pos.size_ = size_traded_;
      market_orders_notifier_->NotifyQueuePosChange(queue_pos, true);
      return order->size_;
    }
  } else if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR: -ve modify position." << DBGLOG_ENDL_FLUSH;
  }

  return -1;  // -1 represents order not found case
}

int MarketOrdersView::ExecMarketAskHidden(int64_t t_order_id_, double t_price_, int size_hidden_, int size_traded_) {
  int ask_int_price = GetIntPx(t_price_);

  ExchMarketOrder* order = market_book_->FindMarketAsk(t_order_id_, ask_int_price, size_hidden_);

  if (order == nullptr) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR"
                           << " nullptr order in Exec Ask"
                           << " Size: " << size_traded_ << " Price: " << t_price_ << " OrderId: " << t_order_id_
                           << DBGLOG_ENDL_FLUSH;
    return -1;
  }

  QueuePositionUpdate queue_pos;
  int position = -1;
  if (order) position = market_book_->GetAskPosition(order);

  if (position >= 0) {
    // First add the hidden order back to the queue at top
    queue_pos.action_ = QueuePositionUpdate::OrderAdd;
    queue_pos.buysell_ = kTradeTypeSell;
    queue_pos.int_price_ = ask_int_price;
    queue_pos.position_ = 0;
    queue_pos.size_ = size_hidden_;
    market_orders_notifier_->NotifyQueuePosChange(queue_pos, true);

    // Delete the hidden order size from the position of order
    queue_pos.action_ = QueuePositionUpdate::OrderRemove;
    queue_pos.buysell_ = kTradeTypeSell;
    queue_pos.int_price_ = ask_int_price;
    queue_pos.position_ = position;
    queue_pos.size_ = size_hidden_;
    market_orders_notifier_->NotifyQueuePosChange(queue_pos, true);

    // Now send the exec notifications
    queue_pos.action_ = QueuePositionUpdate::OrderExec;
    queue_pos.buysell_ = kTradeTypeSell;
    queue_pos.int_price_ = ask_int_price;
    queue_pos.position_ = 0;
    // Size traded will be greater than size hidden if it is the single order  at that level
    queue_pos.size_ = size_traded_;
    market_orders_notifier_->NotifyQueuePosChange(queue_pos, true);

    // Even if size traded is more than order size (packet drop scenario),
    // then also delete the order

    if (size_traded_ >= order->size_) {
      bool l1_update = market_book_->RemoveMarketAsk(order, &queue_pos);
      queue_pos.position_ = 0;
      market_orders_notifier_->NotifyQueuePosChange(queue_pos, l1_update);
      return 0;
    } else {
      order->size_ = order->size_ - size_traded_;
      queue_pos.action_ = QueuePositionUpdate::OrderRemove;
      queue_pos.buysell_ = kTradeTypeSell;
      queue_pos.int_price_ = ask_int_price;
      queue_pos.position_ = 0;
      queue_pos.size_ = size_traded_;
      market_orders_notifier_->NotifyQueuePosChange(queue_pos, true);
      return order->size_;
    }
  } else if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR: -ve modify position." << DBGLOG_ENDL_FLUSH;
  }

  return -1;  // -1 represents order not found case
}

void MarketOrdersView::Reset() { market_book_->Reset(); }

// <-------- Call Listeners ----------->

void MarketOrdersView::SubscribeL1QueuePosChange(QueuePositionChangeListener* queue_pos_change_listener_) {
  market_orders_notifier_->AddL1QueuePosChangeListener(queue_pos_change_listener_);
}

void MarketOrdersView::SubscribeQueuePosChange(QueuePositionChangeListener* queue_pos_change_listener_) {
  market_orders_notifier_->AddQueuePositionChangeListener(queue_pos_change_listener_);
}

std::string MarketOrdersView::GetMarketString() { return market_book_->GetMarketString(); }

bool MarketOrdersView::IsReady() { return market_book_->IsReady(); }

int MarketOrdersView::GetBidSize(int int_price_, int64_t order_id_) {
  ExchMarketOrder* order_ = market_book_->FindMarketBid(order_id_, int_price_);

  if (order_ != nullptr) {
    return order_->size_;
  }

  return 0;
}

int MarketOrdersView::GetAskSize(int int_price_, int64_t order_id_) {
  ExchMarketOrder* order_ = market_book_->FindMarketAsk(order_id_, int_price_);

  if (order_ != nullptr) {
    return order_->size_;
  }

  return 0;
}

ttime_t MarketOrdersView::GetLastAdd(TradeType_t buysell_, int int_price_, int* size_) {
  if (buysell_ == kTradeTypeBuy) {
    return GetLastBidAdd(int_price_, size_);
  } else if (buysell_ == kTradeTypeSell) {
    return GetLastAskAdd(int_price_, size_);
  }
  return ttime_t(0, 0);
}

ttime_t MarketOrdersView::GetLastBidAdd(int int_price_, int* size_) {
  return market_book_->GetLastBidAdd(int_price_, size_);
}

ttime_t MarketOrdersView::GetLastAskAdd(int int_price_, int* size_) {
  return market_book_->GetLastAskAdd(int_price_, size_);
}

int MarketOrdersView::GetSize(TradeType_t buysell_, int int_price_) {
  if (buysell_ == kTradeTypeBuy) {
    return GetBidSize(int_price_);
  } else if (buysell_ == kTradeTypeSell) {
    return GetAskSize(int_price_);
  }
  return 0;
}

/**
 * Return the number of orders at given price
 * @param buysell
 * @param int_price
 * @return
 */
int MarketOrdersView::GetOrderCount(TradeType_t buysell, int int_price) {
  if (buysell == kTradeTypeBuy) {
    return GetBidOrderCount(int_price);
  } else if (buysell == kTradeTypeSell) {
    return GetAskOrderCount(int_price);
  }
  return 0;
}

/**
 * return number of orders at bid side
 * @param int_price_
 * @return
 */
int MarketOrdersView::GetBidSize(int int_price_) { return market_book_->GetBidSize(int_price_); }

int MarketOrdersView::GetAskSize(int int_price_) { return market_book_->GetAskSize(int_price_); }

int MarketOrdersView::GetBidOrderCount(int int_price) { return market_book_->GetBidOrders(int_price).size(); }

int MarketOrdersView::GetAskOrderCount(int int_price) { return market_book_->GetAskOrders(int_price).size(); }

int MarketOrdersView::GetBestBidIntPrice() { return market_book_->GetBestBidIntPrice(); }

int MarketOrdersView::GetBestAskIntPrice() { return market_book_->GetBestAskIntPrice(); }

double MarketOrdersView::GetBestBidPrice() { return GetDoublePx(market_book_->GetBestBidIntPrice()); }

double MarketOrdersView::GetBestAskPrice() { return GetDoublePx(market_book_->GetBestAskIntPrice()); }

int MarketOrdersView::GetTopOrderSizeAskLevels(int int_price) {
  return market_book_->GetTopOrderSizeAskLevels(int_price);
}
int MarketOrdersView::GetTopOrderSizeBidLevels(int int_price) {
  return market_book_->GetTopOrderSizeBidLevels(int_price);
}

void MarketOrdersView::RemoveBidTopOrder(int int_price) { market_book_->RemoveBidTopOrder(int_price); }

void MarketOrdersView::RemoveAskTopOrder(int int_price) { market_book_->RemoveAskTopOrder(int_price); }

std::vector<ExchMarketOrder*>& MarketOrdersView::GetBidOrders(int int_price_) {
  return market_book_->GetBidOrders(int_price_);
}

std::vector<ExchMarketOrder*>& MarketOrdersView::GetAskOrders(int int_price_) {
  return market_book_->GetAskOrders(int_price_);
}

ExchMarketOrder* MarketOrdersView::GetLastRemovedBidOrder(int int_price) {
  return market_book_->GetLastRemovedBidOrder(int_price);
}

ExchMarketOrder* MarketOrdersView::GetLastRemovedAskOrder(int int_price) {
  return market_book_->GetLastRemovedAskOrder(int_price);
}

ExchMarketOrder* MarketOrdersView::GetBidOrderAtPositionPrice(int position, int int_price) {
  return market_book_->GetBidOrderAtPositionPrice(position, int_price);
}

ExchMarketOrder* MarketOrdersView::GetAskOrderAtPositionPrice(int position, int int_price) {
  return market_book_->GetAskOrderAtPositionPrice(position, int_price);
}
}
