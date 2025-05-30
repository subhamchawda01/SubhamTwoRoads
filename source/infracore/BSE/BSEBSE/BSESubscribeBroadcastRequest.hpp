// =====================================================================================
//
//       Filename:  BSESubscribeBroadcastRequest.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/04/2012 11:24:47 AM
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

#include "infracore/BSE/BSEBSE/BSEMessageDefs.hpp"

namespace HFSAT {
namespace BSE {

class BSESubscribeBroadcastRequest {
 private:
  SubscribeRequestT bse_subscribe_broadcast_request_;

 public:
  BSESubscribeBroadcastRequest(const uint32_t& _sender_sub_id_) {
    // initialize password change message
    memset((void*)(&bse_subscribe_broadcast_request_), 0, sizeof(SubscribeRequestT));

    // fill up the Messageheader
    bse_subscribe_broadcast_request_.MessageHeaderIn.BodyLen = sizeof(SubscribeRequestT);
    bse_subscribe_broadcast_request_.TemplateID = TID_SUBSCRIBE_REQUEST;
    
  }

  void setBSESubscribeBroadcastMessageSequence(const uint32_t& _message_sequence_) {
    bse_subscribe_broadcast_request_.RequestHeader.MsgSeqNum = _message_sequence_;
  }


  // a single call should be enough to fill all the dynamic fields
  void setBSESubscribeBroadcastDynamicFields(const int32_t& _subscription_scope_, const uint8_t& _refappl_id_) {
    bse_subscribe_broadcast_request_.SubscriptionScope = _subscription_scope_,;
    bse_subscribe_broadcast_request_.RefApplID = _refappl_id_;
  }

};
}
}
