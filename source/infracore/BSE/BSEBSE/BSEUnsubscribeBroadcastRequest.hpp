// =====================================================================================
//
//       Filename:  BSEUnsubscribeBroadcastRequest.hpp
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

class BSEUnsubscribeBroadcastRequest {
 private:
  UnsubscribeRequestT bse_unsubscribe_broadcast_request_;

 public:
  BSEUnsubscribeBroadcastRequest(const uint32_t& _sender_sub_id_) {
    // initialize password change message
    memset((void*)(&bse_unsubscribe_broadcast_request_), 0, sizeof(UnsubscribeRequestT));

    // fill up the Messageheader
    bse_unsubscribe_broadcast_request_.MessageHeaderIn.BodyLen = sizeof(UnsubscribeRequestT);
    bse_unsubscribe_broadcast_request_.TemplateID = TID_SUBSCRIBE_REQUEST;
    
  }

  void setBSEUnsubscribeBroadcastMessageSequence(const uint32_t& _message_sequence_) {
    bse_unsubscribe_broadcast_request_.RequestHeader.MsgSeqNum = _message_sequence_;
  }


  // a single call should be enough to fill all the dynamic fields
  void setBSEUnsubscribeBroadcastDynamicFields(const int32_t& _refapplsub_id_) {
    bse_unsubscribe_broadcast_request_.RefApplSubID = _refapplsub_id_;
  }

};
}
}
