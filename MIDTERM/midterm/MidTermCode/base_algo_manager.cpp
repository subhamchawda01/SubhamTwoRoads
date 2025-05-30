// =====================================================================================
//
//       Filename:  base_algo_manager.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Thursday 07 January 2016 02:50:58  GMT
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
#include "midterm/MidTerm/base_algo_manager.hpp"

namespace MIDTERM {

void BaseAlgoManager::SubscribeOrderListener(MidTermOrderListener *instance_) {
  HFSAT::VectorUtils::UniqueVectorAdd(order_listeners, instance_);
}

void BaseAlgoManager::NotifyOrderConfirmedListeners(OrderResponse response_) {
  for (auto i : order_listeners) {
    i->OnOrderConfirmed(response_);
  }
}
void BaseAlgoManager::NotifyOrderExecutedListeners(OrderResponse response_) {
  for (auto i : order_listeners) {
    i->OnOrderExecuted(response_);
  }
}
void BaseAlgoManager::NotifyOrderCancelledListeners(OrderResponse response_) {
  for (auto i : order_listeners) {
    i->OnOrderCancelled(response_);
  }
}
void BaseAlgoManager::NotifyOrderCancelRejectedListeners(
    OrderResponse response_) {
  for (auto i : order_listeners) {
    i->OnOrderCancelRejected(response_);
  }
}
void BaseAlgoManager::NotifyOrderRejectedListeners(OrderResponse response_) {
  for (auto i : order_listeners) {
    i->OnOrderRejected(response_);
  }
}

void BaseAlgoManager::ProcessSingleResponse() {
  switch (order_response_.order_response_type) {
  case HFSAT::ORRType_t::kORRType_Cxld: {
    // Case when cancelled on exchange
    // Need to place fresh SendOrder
    // Very rare to get a cancel order
    dbglogger_ << "Cancelled..Printing order response..\n";
    dbglogger_ << order_response_.ToString() << DBGLOG_ENDL_FLUSH;
    // send the order
    NotifyOrderCancelledListeners(order_response_);
  } break;

  case HFSAT::ORRType_t::kORRType_CxRe: {
    // Case when cancel request gets rejected
    dbglogger_ << "Cancel Rejected..Printing order response..\n";
    dbglogger_ << order_response_.ToString() << DBGLOG_ENDL_FLUSH;
    NotifyOrderCancelRejectedListeners(order_response_);
  } break;

  case HFSAT::ORRType_t::kORRType_Exec: {
    // Case when executed on exchange
    // Need to extract execution price
    dbglogger_ << "Executed..Printing order response..\n";
    dbglogger_ << order_response_.ToString() << DBGLOG_ENDL_FLUSH;
    NotifyOrderExecutedListeners(order_response_);
  } break;

  case HFSAT::ORRType_t::kORRType_Rejc: {
    // Case when rejected on an OrderSend
    // Need to place fresh SendOrder
    dbglogger_ << "Rejected..Printing order response..\n";
    dbglogger_ << order_response_.ToString() << DBGLOG_ENDL_FLUSH;
    NotifyOrderRejectedListeners(order_response_);
  } break;

  default: {
    // dbglogger_ << "Confirmed.. Printing order response..\n";
    // dbglogger_ << order_response_.ToString() << DBGLOG_ENDL_FLUSH;
    // Don't need to do anything
    NotifyOrderConfirmedListeners(order_response_);
  } break;
  }
}
}
