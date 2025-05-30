// =====================================================================================
//
//       Filename:  BSESessionPasswordChangeRequest.hpp
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

class BSESessionPasswordChangeRequest {
 private:
  SessionPasswordChangeRequestT bse_session_password_change_request_;

 public:
  BSESessionPasswordChangeRequest(const uint32_t& _sender_sub_id_) {
    // initialize password change message
    memset((void*)(&bse_session_password_change_request_), 0, sizeof(SessionPasswordChangeRequestT));

    // fill up the Messageheader
    bse_session_password_change_request_.MessageHeaderIn.BodyLen = sizeof(LogonRequestT);
    bse_session_password_change_request_.TemplateID = TID_SESSION_PASSWORD_CHANGE_REQUEST;
    
    // fill up the RequestHeeader
    bse_session_password_change_request_.RequestHeader.SenderSubID = _sender_sub_id_;

  }

  void setBSESessionPasswordChangeMessageSequence(const uint32_t& _message_sequence_) {
    bse_session_password_change_request_.RequestHeader.MsgSeqNum = _message_sequence_;
  }


  // These fields would be read from ORS Config File
  void setBSESessionPasswordChangeStaticFields(const uint32_t _party_session_id_,const char* _new_password_,
                                               const uint8_t _new_password_fill_length_,
                                               const char* _password_, const uint8_t _password_fill_length_) {
    bse_session_password_change_request_.PartyIDSessionID = _party_session_id_;
    memcpy((void*)(bse_session_password_change_request_.Password), (void*)(_password_), _password_fill_length_);
    memcpy((void*)(bse_session_password_change_request_.NewPassword), (void*)(_new_password_), _new_password_fill_length_);
  }

};
}
}
