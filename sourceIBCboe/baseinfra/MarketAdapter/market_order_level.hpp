/**
   \file MarketAdapter/market_order_level.hpp

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

namespace HFSAT {

class MktOrdLevel {
 protected:
  ttime_t last_activity_time_;
  int last_add_size_;
  bool top_order_ = false;

 public:
  std::vector<ExchMarketOrder*> ord_;
  ExchMarketOrder* last_removed_order_;

  MktOrdLevel(const HFSAT::Watch&);
  ~MktOrdLevel();

  int NumOrders();
  int Size();
  bool Empty();

  // Sets the top order flag to true which helps in identifying if a price level(market order level) contains top order
  void GiveTopOrderPriority();
  // Returns the top order size at this market order level
  int GetTopOrderSize();
  // Removes the top order priority
  void RemoveTopOrderPriority();
  // Checks to see if this market order level contains a top order
  bool TopOrderExists();
  // Inserts exchange order at its correct position based on the queue size ahead
  void InsertExchOrder(ExchMarketOrder* order);

  void InsertExchOrderAtPosition(ExchMarketOrder* order);
  int GetPosition(ExchMarketOrder* order);

  ExchMarketOrder* GetOrderAtPosition(int position);

  ExchMarketOrder* FindOrder(int64_t t_order_id);
  bool Remove(ExchMarketOrder* order, QueuePositionUpdate* queue_pos,
              SimpleMempool<ExchMarketOrder>* mkt_order_mempool);

  void Clear(SimpleMempool<ExchMarketOrder>* _mkt_order_mempool_);

  ttime_t GetLastAdd(int* size);
  const Watch& watch_;
};
}
