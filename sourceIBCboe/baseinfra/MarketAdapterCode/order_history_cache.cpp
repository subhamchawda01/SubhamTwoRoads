/**
    \file MarketAdapter/order_history_cache.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2017
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include <math.h>
#include "baseinfra/MarketAdapter/order_history_cache.hpp"
#define NSE_HASH_CACHE_SIZE 8

namespace HFSAT {

OrderHistory::OrderHistory(int num_security_id)
    : order_struct_mempool_(2048),
      sid_to_order_cache_(num_security_id, std::vector<OrderDetailsStruct *>(pow(2, NSE_HASH_CACHE_SIZE))),
      sid_to_order_count_cache(num_security_id, std::vector<int>(pow(2, NSE_HASH_CACHE_SIZE))),
      sid_to_order_history_(num_security_id, ska::flat_hash_map<uint64_t, OrderDetailsStruct *>()) {}

OrderHistory::~OrderHistory() {}

/*
* Returns an index which can be used to access the cache directly
* Simply calculates the last NSE_HASH_CACHE_SIZE bits of the order id
*/
int OrderHistory::GetHashIndex(uint64_t t_order_id_) {
  int hash_index = t_order_id_ & ((1 << NSE_HASH_CACHE_SIZE) - 1);
  return hash_index;
}

// Adds order to OrderHistory
// Add first to cache. If cache entry is occupied, adds to map
void OrderHistory::AddOrderToOrderHistory(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                          const double t_price_, const uint32_t t_size_, const uint64_t t_order_id_) {
  OrderDetailsStruct *t_order_ = order_struct_mempool_.Alloc();

  t_order_->is_buy_order = (t_buysell_ == kTradeTypeBuy ? true : false);
  t_order_->order_size = t_size_;
  t_order_->order_id = t_order_id_;
  t_order_->order_price = t_price_;

  int hash_index = GetHashIndex(t_order_id_);
  if (sid_to_order_cache_[t_security_id_][hash_index] == NULL) {
    sid_to_order_cache_[t_security_id_][hash_index] = t_order_;
  } else {
    sid_to_order_history_[t_security_id_].insert(std::pair<uint64_t, OrderDetailsStruct *>(t_order_id_, t_order_));
  }
  ++sid_to_order_count_cache[t_security_id_][hash_index];
}

// Returns true iff order is in order history.
// Looks first in the cache, and then in the map
bool OrderHistory::IsOrderSeen(const uint32_t t_security_id_, const uint64_t t_order_id_) {
  if (t_order_id_ == 0) return false;

  int hash_index = GetHashIndex(t_order_id_);
  if (sid_to_order_count_cache[t_security_id_][hash_index] == 0) {
    return false;
  }
  if (sid_to_order_cache_[t_security_id_][hash_index] != NULL) {
    OrderDetailsStruct *t_order_ = sid_to_order_cache_[t_security_id_][hash_index];
    if (t_order_->order_id == t_order_id_) return true;
  }

  ska::flat_hash_map<uint64_t, OrderDetailsStruct *>::iterator t_mapiter_ =
      sid_to_order_history_[t_security_id_].find(t_order_id_);
  if (t_mapiter_ != sid_to_order_history_[t_security_id_].end()) {
    return true;
  }

  return false;
}

// Gets order details from the history
OrderDetailsStruct *OrderHistory::GetOrderDetails(const uint32_t t_security_id_, const uint64_t t_order_id_) {
  int hash_index = GetHashIndex(t_order_id_);
  if (sid_to_order_cache_[t_security_id_][hash_index] != NULL) {
    OrderDetailsStruct *t_order_ = sid_to_order_cache_[t_security_id_][hash_index];
    if (t_order_->order_id == t_order_id_) return sid_to_order_cache_[t_security_id_][hash_index];
  }

  if (sid_to_order_history_[t_security_id_].find(t_order_id_) != sid_to_order_history_[t_security_id_].end()) {
    return sid_to_order_history_[t_security_id_][t_order_id_];
  }
  return NULL;
}

std::vector<OrderDetailsStruct *> &OrderHistory::GetOrderCache(const uint32_t t_security_id_) {
  return sid_to_order_cache_[t_security_id_];
}

ska::flat_hash_map<uint64_t, OrderDetailsStruct *> &OrderHistory::GetOrderMaps(const uint32_t t_security_id_) {
  return sid_to_order_history_[t_security_id_];
}

// Delete order from history
// Looks first in the cache, and then in the map
ska::flat_hash_map<uint64_t, OrderDetailsStruct *>::iterator OrderHistory::DeleteOrderFromHistory(const uint32_t t_security_id_, const uint64_t t_order_id_) {
  int hash_index = GetHashIndex(t_order_id_);
  ska::flat_hash_map<uint64_t, OrderDetailsStruct *>::iterator t_iter_ = sid_to_order_history_[t_security_id_].end();
  if (sid_to_order_cache_[t_security_id_][hash_index] != NULL) {
    OrderDetailsStruct *t_order_ = sid_to_order_cache_[t_security_id_][hash_index];
    if (t_order_->order_id == t_order_id_) {
      sid_to_order_cache_[t_security_id_][hash_index] = NULL;
      --sid_to_order_count_cache[t_security_id_][hash_index];
      order_struct_mempool_.DeAlloc(t_order_);
      return t_iter_;
    }
  }

  ska::flat_hash_map<uint64_t, OrderDetailsStruct *>::iterator t_mapiter_;
  t_mapiter_ = sid_to_order_history_[t_security_id_].find(t_order_id_);
  if (t_mapiter_ != sid_to_order_history_[t_security_id_].end()) {
    OrderDetailsStruct *t_order_ = (*t_mapiter_).second;
    t_iter_ = sid_to_order_history_[t_security_id_].erase(t_mapiter_);
    --sid_to_order_count_cache[t_security_id_][hash_index];
    order_struct_mempool_.DeAlloc(t_order_);
  }
  return t_iter_;
}


void OrderHistory::UpdateOrderId(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t t_prev_order_id_, 
				 const uint64_t t_new_order_id_, const double t_price_, const uint32_t t_size_){
  DeleteOrderFromHistory(t_security_id_, t_prev_order_id_);
  AddOrderToOrderHistory(t_security_id_, t_buysell_, t_price_, t_size_, t_new_order_id_);
}

// Returns total size of specified buysell in OrderHistory
int OrderHistory::GetSizeAtPrice(int security_id_, double px, TradeType_t buysell) {
  int ret_size_ = 0;
  auto vec_it_ = sid_to_order_cache_[security_id_].begin();
  while (vec_it_ != sid_to_order_cache_[security_id_].end()) {
    OrderDetailsStruct *t_order_ = *vec_it_;
    if (t_order_ != NULL && t_order_->is_buy_order == (buysell == HFSAT::kTradeTypeBuy) &&
        fabs(t_order_->order_price - px) < 1e-5) {
      ret_size_ += t_order_->order_size;
    }
    vec_it_++;
  }

  auto map_it_ = sid_to_order_history_[security_id_].begin();
  while (map_it_ != sid_to_order_history_[security_id_].end()) {
    OrderDetailsStruct *t_order_ = map_it_->second;
    if (t_order_->is_buy_order == (buysell == HFSAT::kTradeTypeBuy) && fabs(t_order_->order_price - px) < 1e-5) {
      ret_size_ += t_order_->order_size;
    }
    map_it_++;
  }

  // set to invalid size if size is 0 -- kInvalidSize
  if (ret_size_ == 0) {
    ret_size_ = -1;
  }
  return ret_size_;
}
}
