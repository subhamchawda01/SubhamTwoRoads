/**
   \file MarketAdapter/market_orders_notifier.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include <vector>

#include "baseinfra/MarketAdapter/order_defines.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

namespace HFSAT {

class QueuePositionChangeListener {
 public:
  virtual ~QueuePositionChangeListener(){};
  virtual void QueuePosChange(QueuePositionUpdate position_update_) = 0;
};

class MarketOrdersNotifier {
 private:
  DebugLogger &dbglogger_;
  const Watch &watch_;

  // <------ Order change listeners
  std::vector<QueuePositionChangeListener *> queue_pos_change_listeners_;
  std::vector<QueuePositionChangeListener *> l1_queue_pos_change_listeners_;

 public:
  MarketOrdersNotifier(DebugLogger &t_dbglogger_, const Watch &t_watch_);

  ~MarketOrdersNotifier(){};

  void AddQueuePositionChangeListener(QueuePositionChangeListener *queue_pos_listener_);
  void RemoveQueuePositionChangeListener(QueuePositionChangeListener *queue_pos_listener_);

  void AddL1QueuePosChangeListener(QueuePositionChangeListener *queue_pos_listener_);
  void RemoveL1QueuePosChangeListener(QueuePositionChangeListener *queue_pos_listener_);

  void NotifyQueuePosChange(QueuePositionUpdate queue_pos_, bool l1_update_);
  void NotifyQueuePosChange(QueuePositionUpdate queue_pos_);
  void NotifyL1QueuePosChange(QueuePositionUpdate queue_pos_);
};
}
