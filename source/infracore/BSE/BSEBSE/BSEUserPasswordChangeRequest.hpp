// =====================================================================================
//
//       Filename:  BSEUserPasswordChangeRequest.hpp
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

class BSEUserPasswordChangeRequest {
 public:
  UserPasswordChangeRequestT bse_user_password_change_request_;

 public:
  BSEUserPasswordChangeRequest(const uint32_t& _sender_sub_id_) {
    // initialize password change message
    memset((void*)(&bse_user_password_change_request_), 0, sizeof(UserPasswordChangeRequestT));

    // fill up the Messageheader
    bse_user_password_change_request_.MessageHeaderIn.BodyLen = sizeof(UserPasswordChangeRequestT);
    bse_user_password_change_request_.MessageHeaderIn.TemplateID = TID_USER_PASSWORD_CHANGE_REQUEST;
    
    // fill up the RequestHeeader
    bse_user_password_change_request_.RequestHeader.SenderSubID = _sender_sub_id_;

  }

  void setBSEUserPasswordChangeMessageSequence(const uint32_t& _message_sequence_) {
    bse_user_password_change_request_.RequestHeader.MsgSeqNum = _message_sequence_;
  }

  // get MsgLength to write
  int getBSEUserPasswordChangeRequestMsgLength() { return (sizeof(UserPasswordChangeRequestT)); }

  // These fields would be read from ORS Config File
  void setBSEUserPasswordChangeStaticFields(const uint32_t _username_,const char* _new_password_, const uint8_t _new_password_fill_length_,
                                      const char* _password_, const uint8_t _password_fill_length_) {
    bse_user_password_change_request_.Username = _username_;
    memcpy((void*)(bse_user_password_change_request_.Password), (void*)(_password_), _password_fill_length_);
    memcpy((void*)(bse_user_password_change_request_.NewPassword), (void*)(_new_password_), _new_password_fill_length_);
  }

};
}
}
