/**
   \file MarketAdapterCode/market_orders_notifier.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/market_orders_notifier.hpp"

#include "dvccode/CommonDataStructures/vector_utils.hpp"

namespace HFSAT {

MarketOrdersNotifier::MarketOrdersNotifier(DebugLogger& t_dbglogger_, const Watch& t_watch_)
    : dbglogger_(t_dbglogger_), watch_(t_watch_) {}

void MarketOrdersNotifier::AddQueuePositionChangeListener(QueuePositionChangeListener* queue_pos_listener_) {
  if (queue_pos_listener_ != NULL) {
    VectorUtils::UniqueVectorAdd(queue_pos_change_listeners_, queue_pos_listener_);
  }
}

void MarketOrdersNotifier::RemoveQueuePositionChangeListener(QueuePositionChangeListener* queue_pos_listener_) {
  if (queue_pos_listener_ != NULL) {
    VectorUtils::UniqueVectorRemove(queue_pos_change_listeners_, queue_pos_listener_);
  }
}

void MarketOrdersNotifier::AddL1QueuePosChangeListener(QueuePositionChangeListener* queue_pos_listener_) {
  if (queue_pos_listener_ != NULL) {
    VectorUtils::UniqueVectorAdd(l1_queue_pos_change_listeners_, queue_pos_listener_);
  }
}

void MarketOrdersNotifier::RemoveL1QueuePosChangeListener(QueuePositionChangeListener* queue_pos_listener_) {
  if (queue_pos_listener_ != NULL) {
    VectorUtils::UniqueVectorRemove(l1_queue_pos_change_listeners_, queue_pos_listener_);
  }
}

void MarketOrdersNotifier::NotifyQueuePosChange(QueuePositionUpdate queue_pos_, bool l1_update_) {
  if (l1_update_) {
    NotifyL1QueuePosChange(queue_pos_);
  }

  NotifyQueuePosChange(queue_pos_);
}

void MarketOrdersNotifier::NotifyQueuePosChange(QueuePositionUpdate queue_pos_) {
  for (size_t i = 0; i < queue_pos_change_listeners_.size(); i++) {
    queue_pos_change_listeners_[i]->QueuePosChange(queue_pos_);
  }
}

void MarketOrdersNotifier::NotifyL1QueuePosChange(QueuePositionUpdate queue_pos_) {
  for (size_t i = 0; i < l1_queue_pos_change_listeners_.size(); i++) {
    l1_queue_pos_change_listeners_[i]->QueuePosChange(queue_pos_);
  }
}
}
