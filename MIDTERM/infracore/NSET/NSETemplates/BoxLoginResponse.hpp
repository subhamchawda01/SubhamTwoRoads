// =====================================================================================
//
//       Filename:  BoxLoginRequest.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/29/2015 05:47:46 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 353, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#include "infracore/NSET/NSETemplates/RequestPacket.hpp"
#include "infracore/NSET/NSETemplates/RequestHeader.hpp"

namespace HFSAT {
namespace NSE {

/*
 * Generic  interface for nse gr response trimmed template.
 * Please extend this for both derivatives and cash market.
 */

class NSEBoxLoginResponse {
 protected:
  ProcessedBoxLoginResponse processed_boxlogin_response_;

 public:
  NSEBoxLoginResponse() { memset((void *)&processed_boxlogin_response_, 0, sizeof(ProcessedBoxLoginResponse)); }
  ~NSEBoxLoginResponse() {}

  ProcessedBoxLoginResponse *ProcessBoxLoginResponse(char const *msg_ptr) { 
    processed_boxlogin_response_.box_id = ntoh16(*((int16_t *)(msg_ptr + 0)));
    return &processed_boxlogin_response_; 
  }
  int32_t GetBoxLoginErrorMsgLength() { return 0; }
  int32_t GetBoxLoginErrorMsgOffset() { return 0; }
};
}
}
