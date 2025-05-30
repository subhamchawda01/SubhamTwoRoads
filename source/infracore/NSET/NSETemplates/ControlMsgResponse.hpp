// =====================================================================================
//
//       Filename:  ControlMsgResponse.hpp
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

#include "infracore/NSET/NSETemplates/ResponsePacket.hpp"
#include "infracore/NSET/NSETemplates/ResponseHeader.hpp"

namespace HFSAT {
namespace NSE {

/*
 * Generic  interface for nse Control msg response trimmed template.
 * Please extend this for both derivatives and cash market.
 */

class NSEControlMsgResponse {
 protected:
   ProcessedControlMsgResponse processed_control_msg_response_;

 public:
  NSEControlMsgResponse() { memset((void *)&processed_control_msg_response_, 0, sizeof(ProcessedControlMsgResponse)); }
  ~NSEControlMsgResponse() {}

  ProcessedControlMsgResponse *ProcessControlMsgResponse(char const *msg_ptr) {

    processed_control_msg_response_.trade_id = ntoh32(*((int32_t *)(msg_ptr + 0)));
    memcpy((void *)processed_control_msg_response_.action_code, (void *)(msg_ptr + 4), 3);
    processed_control_msg_response_.msg_len = ntoh16(*((int16_t *)(msg_ptr + 8)));
    memcpy((void *)processed_control_msg_response_.msg, (void *)(msg_ptr + 10), 240);
    return &processed_control_msg_response_;
  }
};
}
}
