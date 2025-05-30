/**
   \file MarketAdapterCode/market_order_level.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/market_order_level.hpp"

namespace HFSAT {

MktOrdLevel::MktOrdLevel(const Watch& watch)
    : last_activity_time_(0, 0), last_add_size_(0), ord_(), last_removed_order_(nullptr), watch_(watch) {}

MktOrdLevel::~MktOrdLevel() { ord_.clear(); }

int MktOrdLevel::NumOrders() { return ord_.size(); }

int MktOrdLevel::Size() {
  int size_ = 0;
  for (auto order_ : ord_) {
    size_ += order_->size_;
  }
  return size_;
}

bool MktOrdLevel::Empty() { return ord_.empty(); }

void MktOrdLevel::GiveTopOrderPriority() { top_order_ = true; }

int MktOrdLevel::GetTopOrderSize() {
  if (top_order_) {
    return ord_[0]->size_;
  }
  return 0;
}

void MktOrdLevel::RemoveTopOrderPriority() { top_order_ = false; }

bool MktOrdLevel::TopOrderExists() {
  if (top_order_) {
    return true;
  }

  return false;
}
/**
 *
 * Insert the given order into the mkt_orders_ vector at
 * appropriate position based on the priority of the order (assumes increasing order priority)
 * @param order_
 */

void MktOrdLevel::InsertExchOrder(ExchMarketOrder* order_) {
  if (ord_.size() == 0) {
    GiveTopOrderPriority();
  }
  int num_orders_ = ord_.size();

  if (order_->priority_ == 0 || num_orders_ == 0) {
    if (!ord_.empty()) {
      order_->priority_ = ord_[num_orders_ - 1]->priority_ + 1;
    }
    ord_.push_back(order_);
  } else if (ord_[num_orders_ - 1]->priority_ <= order_->priority_) {
    ord_.push_back(order_);
  } else if (ord_[0]->priority_ > order_->priority_) {
    ord_.insert(ord_.begin(), order_);
  } else {
    auto it_ = lower_bound(ord_.begin(), ord_.end(), order_, &ExchMarketOrder::LTExchOrder);
    ord_.insert(it_, order_);
  }
  last_activity_time_ = watch_.tv();
  last_add_size_ = order_->size_;
}

/**
 * Inserts order based on priority (assumes priority is the position of order)
 * @param order_
 */
void MktOrdLevel::InsertExchOrderAtPosition(ExchMarketOrder* order_) {
  int pos = order_->priority_ - 1;
  if (pos < 0) {
    // insert at begin
    ord_.insert(ord_.begin(), order_);
  } else if (pos >= (int32_t)ord_.size()) {
    // insert at begin
    ord_.push_back(order_);
  } else {
    // insert at correct position
    ord_.insert(ord_.begin() + pos, order_);
  }
  last_activity_time_ = watch_.tv();
  last_add_size_ = order_->size_;
}

int MktOrdLevel::GetPosition(ExchMarketOrder* t_order_) {
  int position_ = 0;

  for (auto order_ : ord_) {
    // Skip null orders: shouldn't actually happen though
    if (order_ == NULL) {
      continue;
    }

    if (order_->order_id_ == t_order_->order_id_) {
      return position_;
    } else {
      position_ += order_->size_;
    }
  }
  return -1;
}

/**
 * Returns the order which has position_udpate.position_ value same as the given position
 * @param position
 * @return
 */
ExchMarketOrder* MktOrdLevel::GetOrderAtPosition(int position) {
  int t_position = 0;
  for (auto order : ord_) {
    // skipp null orders
    if (order == nullptr) continue;

    t_position += order->size_;
    if (t_position >= position) return order;
  }
  return nullptr;
}

ExchMarketOrder* MktOrdLevel::FindOrder(int64_t t_order_id_) {
  for (auto order : ord_) {
    if (order->order_id_ == t_order_id_) {
      return order;
    }
  }
  return NULL;
}

bool MktOrdLevel::Remove(ExchMarketOrder* t_order_, QueuePositionUpdate* queue_pos,
                         SimpleMempool<ExchMarketOrder>* mkt_order_mempool) {
  queue_pos->action_ = QueuePositionUpdate::OrderInvalid;
  queue_pos->position_ = 0;

  if (TopOrderExists() && t_order_ == ord_[0]) {
    RemoveTopOrderPriority();
  }

  for (auto it_ = ord_.begin(); it_ != ord_.end();) {
    ExchMarketOrder* order_ = *it_;
    if (order_ == t_order_) {
      queue_pos->action_ = QueuePositionUpdate::OrderRemove;
      queue_pos->size_ = t_order_->size_;
      queue_pos->int_price_ = t_order_->int_price_;

      last_removed_order_ = *it_;
      it_ = ord_.erase(it_);
      return true;
    } else {
      queue_pos->position_ += order_->size_;
      it_++;
    }
  }

  // Order not found and hence not deleted
  return false;
}

void MktOrdLevel::Clear(SimpleMempool<ExchMarketOrder>* _mkt_order_mempool_) {
  last_activity_time_ = ttime_t(0, 0);
  last_add_size_ = 0;
  for (auto t_order_ : ord_) {
    if (t_order_ != last_removed_order_) {
      // Do not deallocate the last removed order, deallocate it only when it's overwritten by other order
      _mkt_order_mempool_->DeAlloc(t_order_);
    }
  }
  ord_.clear();
}

ttime_t MktOrdLevel::GetLastAdd(int* size_) {
  if (Empty()) {
    *size_ = 0;
    return ttime_t(0, 0);
  }
  *size_ = last_add_size_;
  return last_activity_time_;
}
}
