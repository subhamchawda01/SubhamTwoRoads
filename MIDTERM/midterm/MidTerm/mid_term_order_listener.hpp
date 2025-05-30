// =====================================================================================
//
//       Filename:  mid_term_order_listener.hpp
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
#pragma once
#include "midterm/MidTerm/mid_term_order_routing_defines.hpp"

using namespace std;

namespace MIDTERM {
class MidTermOrderListener {
public:
  virtual ~MidTermOrderListener() {}
  virtual void OnOrderConfirmed(OrderResponse) = 0;
  virtual void OnOrderExecuted(OrderResponse) = 0;
  virtual void OnOrderCancelled(OrderResponse) = 0;
  virtual void OnOrderCancelRejected(OrderResponse) = 0;
  virtual void OnOrderRejected(OrderResponse) = 0;
};
}
