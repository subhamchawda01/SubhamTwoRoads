// =====================================================================================
//
//       Filename:  BSESessionLogout.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/04/2012 12:36:04 PM
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

class BSESessionLogout {
 public:
  LogoutRequestT bse_session_logout_request_;

 public:
  BSESessionLogout() {
    // initialize logout message
    memset((void*)(&bse_session_logout_request_), 0, sizeof(LogoutRequestT));

    // fill up the Messageheader
    bse_session_logout_request_.MessageHeaderIn.BodyLen = sizeof(LogoutRequestT);
    bse_session_logout_request_.MessageHeaderIn.TemplateID = TID_LOGOUT_REQUEST;
    bse_session_logout_request_.RequestHeader.MsgSeqNum = 1;
  }

  void setBSESessionLogoutMessageSequence(const uint32_t& _msg_sequence_) {
    bse_session_logout_request_.RequestHeader.MsgSeqNum = _msg_sequence_;
  }


  // get MsgLength to write
  int getBSESessionLogoutRequestMsgLength() { return (sizeof(LogoutRequestT)); }

  LogoutRequestT *ProcessLogoutRequest(char const *msg_ptr) {
    bse_session_logout_request_.MessageHeaderIn.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_session_logout_request_.MessageHeaderIn.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_session_logout_request_.RequestHeader.MsgSeqNum = *((uint32_t *)(msg_ptr + 16));

    return &bse_session_logout_request_;
  }

};
}
}
