// =====================================================================================
//
//       Filename:  BSEUserLogon.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/04/2012 12:55:20 PM
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

class BSEUserLogon {
 public:
  UserLoginRequestT bse_user_logon_request_;

 public:
  BSEUserLogon() {
    // initialize user logon message
    memset((void*)(&bse_user_logon_request_), 0, sizeof(UserLoginRequestT));

    // fill up the Messageheader
    bse_user_logon_request_.MessageHeaderIn.BodyLen = sizeof(UserLoginRequestT);
    bse_user_logon_request_.MessageHeaderIn.TemplateID = TID_USER_LOGIN_REQUEST;
  }

  void setBSEUserLogonMessageSequence(const uint32_t& _message_sequence_) {
    bse_user_logon_request_.RequestHeader.MsgSeqNum = _message_sequence_;
  }

  // These fields would be read from ORS Config File
  void setBSEUserLogonStaticFields(const uint32_t& _user_id_, const char* _password_,
                                   const uint8_t& _password_fill_length_) {
    bse_user_logon_request_.Username = _user_id_;
    memcpy((void*)(bse_user_logon_request_.Password), (void*)(_password_), _password_fill_length_);
  }

  int getBSEUserLogonMsgLength() { return (sizeof(UserLoginRequestT)); }

  UserLoginRequestT *ProcessUserLoginRequest(char const *msg_ptr) {
    bse_user_logon_request_.MessageHeaderIn.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_user_logon_request_.MessageHeaderIn.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_user_logon_request_.RequestHeader.MsgSeqNum = *((uint32_t *)(msg_ptr + 16));
    bse_user_logon_request_.Username = *((uint32_t *)(msg_ptr + 24));
    memcpy((void*)bse_user_logon_request_.Password,
           (void*)(msg_ptr + 28),LEN_PASSWORD);

    return &bse_user_logon_request_;
  }

};
}
}
