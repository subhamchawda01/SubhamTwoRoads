// =====================================================================================
//
//       Filename:  BSESessionLogon.hpp
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

class BSESessionRegistrationRequest {
 public:
  SessionRegistrationRequestT bse_session_registration_request_;

 public:
  BSESessionRegistrationRequest() {
    // initialize session registration message
    memset((void*)(&bse_session_registration_request_), 0, sizeof(SessionRegistrationRequestT));

    // fill up the Messageheader
    bse_session_registration_request_.MessageHeaderIn.BodyLen = sizeof(SessionRegistrationRequestT);
    bse_session_registration_request_.MessageHeaderIn.TemplateID = TID_SECURE_REGISTRATION;

    // fill up the request struct
    bse_session_registration_request_.RequestHeader.MsgSeqNum = 1;

  }

  // These fields would be read from ORS Config File
  void setBSESessionLogonStaticFields(const uint32_t _party_session_id_) {
    bse_session_registration_request_.PartyIDSessionID = _party_session_id_;
  }

  // get MsgLength to write
  int getBSESessionLogonMsgLength() { return (sizeof(SessionRegistrationRequestT)); }


};
}
}
