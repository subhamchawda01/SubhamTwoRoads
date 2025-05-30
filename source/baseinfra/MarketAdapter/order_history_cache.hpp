/**
    \file MarketAdapter/order_history_cache.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2017
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#pragma once

#include <vector>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/Utils/flat_hash_map.hpp"
#include "dvccode/CommonDataStructures/lockfree_simple_mempool.hpp"

namespace HFSAT {

typedef struct {
  bool is_buy_order;
  int32_t order_size;
  uint64_t order_id;
  double order_price;
} OrderDetailsStruct;

class OrderHistory {
 public:
  OrderHistory(int num_security_id);
  ~OrderHistory();
  void AddOrderToOrderHistory(const uint32_t t_security_id_, const TradeType_t t_buysell_, const double t_price_,
                              const uint32_t t_size_, const uint64_t t_order_id_);
  bool IsOrderSeen(const uint32_t t_security_id_, const uint64_t t_order_id_);
  ska::flat_hash_map<uint64_t, OrderDetailsStruct *>::iterator DeleteOrderFromHistory(const uint32_t t_security_id_, const uint64_t t_order_id_);
  OrderDetailsStruct* GetOrderDetails(const uint32_t t_security_id_, const uint64_t t_order_id_);
  std::vector<OrderDetailsStruct*>& GetOrderCache(const uint32_t t_security_id_);
  ska::flat_hash_map<uint64_t, OrderDetailsStruct*>& GetOrderMaps(const uint32_t t_security_id_);

  void UpdateOrderId(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t t_prev_order_id_, 
                     const uint64_t t_new_order_id_, const double t_price_, const uint32_t t_size_);

  // debug functions -
  int GetSizeAtPrice(int security_id, double px, TradeType_t buysell);

 private:
  int GetHashIndex(uint64_t t_order_id_);

 private:
  LockFreeSimpleMempool<OrderDetailsStruct> order_struct_mempool_;
  std::vector<std::vector<OrderDetailsStruct*>> sid_to_order_cache_;
  std::vector<std::vector<int>> sid_to_order_count_cache;
  std::vector<ska::flat_hash_map<uint64_t, OrderDetailsStruct*>> sid_to_order_history_;
};
}
