// =====================================================================================
//
//       Filename:  BSEUserLogout.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/05/2012 06:04:23 AM
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

class BSEUserLogout {
 public:
  UserLogoutRequestT bse_user_logout_request_;

 public:
  BSEUserLogout() {
    // initialize user logout message
    memset((void*)(&bse_user_logout_request_), 0, sizeof(UserLogoutRequestT));

    // fill up the Messageheader
    bse_user_logout_request_.MessageHeaderIn.BodyLen = sizeof(UserLogoutRequestT);
    bse_user_logout_request_.MessageHeaderIn.TemplateID = TID_USER_LOGOUT_REQUEST;
    bse_user_logout_request_.RequestHeader.MsgSeqNum = 1;
  }

  void setBSEUserLogoutStaticFields(const uint32_t _username_) { bse_user_logout_request_.Username = _username_; }

  void setBSEUserLogoutMessageSeqeunce(const uint32_t& _message_sequence_) {
    bse_user_logout_request_.RequestHeader.MsgSeqNum = _message_sequence_;
  }

  int getBSEUserLogoutMsgLength() { return (sizeof(UserLogoutRequestT)); }

  UserLogoutRequestT *ProcessUserLogoutRequest(char const *msg_ptr) {
    bse_user_logout_request_.MessageHeaderIn.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_user_logout_request_.MessageHeaderIn.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_user_logout_request_.RequestHeader.MsgSeqNum = *((uint32_t *)(msg_ptr + 16));
    bse_user_logout_request_.Username = *((uint32_t *)(msg_ptr + 24));

    return &bse_user_logout_request_;
  }

};
}
}
