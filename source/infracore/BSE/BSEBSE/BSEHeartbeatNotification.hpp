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

class BSEHeartbeatNotification {
 private:
  HeartbeatNotificationT bse_heartbeat_response_;

 public:
  BSEHeartbeatNotification() { memset((void*)(&bse_heartbeat_response_), 0, sizeof(HeartbeatNotificationT)); }

  HeartbeatNotificationT *ProcessHeartbeatNotification(char const *msg_ptr) {
    bse_heartbeat_response_.MessageHeaderOut.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_heartbeat_response_.MessageHeaderOut.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_heartbeat_response_.NotifHeader.SendingTime = *((uint64_t *)(msg_ptr + 8));

    return &bse_heartbeat_response_;
  }

};
}
}
