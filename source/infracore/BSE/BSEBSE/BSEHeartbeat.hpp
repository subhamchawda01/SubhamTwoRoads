// =====================================================================================
//
//       Filename:  BSEHeartbeat.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/04/2012 12:49:17 PM
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

class BSEHeartbeat {
  public:
  HeartbeatT bse_heartbeat_request_;

 public:
  BSEHeartbeat() {
    // initialize heartbeat message
    memset((void*)(&bse_heartbeat_request_), 0, sizeof(HeartbeatT));

    // fill up the Messageheader
    bse_heartbeat_request_.MessageHeaderIn.BodyLen = sizeof(HeartbeatT);
    bse_heartbeat_request_.MessageHeaderIn.TemplateID = TID_HEARTBEAT;
  }

  // get MsgLength to write
  int getBSEHeartbeatMsgLength() { return (sizeof(BSEHeartbeat)); }

  HeartbeatT *ProcessHeartbeatRequest(char const *msg_ptr) {
    bse_heartbeat_request_.MessageHeaderIn.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_heartbeat_request_.MessageHeaderIn.TemplateID = *((uint16_t *)(msg_ptr + 4)); 
    return &bse_heartbeat_request_;
  }
};
}
}
