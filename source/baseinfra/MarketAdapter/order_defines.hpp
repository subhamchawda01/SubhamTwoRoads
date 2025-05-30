/**
   \file MarketAdapter/order_defines.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include <sys/types.h>
#include "dvccode/CDef/defines.hpp"

namespace HFSAT {

struct ExchMarketOrder {
  double price_;
  int int_price_;
  int size_;
  int64_t order_id_;
  uint64_t priority_;

  // Less than comparison of ExchMarketOrders based on priority
  static bool LTExchOrder(ExchMarketOrder* lhs, ExchMarketOrder* rhs) { return (lhs->priority_ < rhs->priority_); }

  // Greater than comparison of ExchMarketOrders based on priority
  static bool GTExchOrder(ExchMarketOrder* lhs, ExchMarketOrder* rhs) { return (lhs->priority_ > rhs->priority_); }
};

struct QueuePositionUpdate {
  enum OrderChangeType_t { OrderInvalid = 0, OrderAdd, OrderRemove, OrderExec };

  // 0 - invalid (ignore), 1 - Add, 2 - Remove, 3 - Exec
  OrderChangeType_t action_;
  TradeType_t buysell_;
  int int_price_;
  int size_;
  int position_;
  bool agg_order_;

  // Constructors
  QueuePositionUpdate(OrderChangeType_t action, TradeType_t buysell, int int_price, int size, int position,
                      bool agg_order)
      : action_(action),
        buysell_(buysell),
        int_price_(int_price),
        size_(size),
        position_(position),
        agg_order_(agg_order) {}

  QueuePositionUpdate()
      : action_(OrderInvalid),
        buysell_(kTradeTypeNoInfo),
        int_price_(kInvalidIntPrice),
        size_(0),
        position_(-1),
        agg_order_(false) {}
};

struct SelfOrder {
  int size_;
  int qsa_;
  int qsb_;
  int qoa_;
  int qob_;

  SelfOrder(int size, int qsa, int qsb, int qoa, int qob) : size_(size), qsa_(qsa), qsb_(qsb), qoa_(qoa), qob_(qob) {}
  SelfOrder() : size_(0), qsa_(0), qsb_(0), qoa_(0), qob_(0) {}
};
}
