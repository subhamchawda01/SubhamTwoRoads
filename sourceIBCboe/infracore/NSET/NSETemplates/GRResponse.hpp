// =====================================================================================
//
//       Filename:  GRRequest.hpp
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

class NSEGRResponse {
 protected:
  ProcessedGRResponse processed_gr_response_;

 public:
  NSEGRResponse() { memset((void *)&processed_gr_response_, 0, sizeof(ProcessedGRResponse)); }
  ~NSEGRResponse() {}

  ProcessedGRResponse *ProcessGRResponse(char const *msg_ptr) {
    processed_gr_response_.box_id = ntoh16(*((int16_t *)(msg_ptr + 0)));
    memcpy((void *)processed_gr_response_.ip, (void *)(msg_ptr + 8), 16);
    processed_gr_response_.port = ntoh32(*((int32_t *)(msg_ptr + 24)));
    memcpy((void *)processed_gr_response_.signon_key, (void *)(msg_ptr + 28), 8);
    memcpy((void *)processed_gr_response_.cryptographic_key, (void *)(msg_ptr + 36), 32);
    memcpy((void *)processed_gr_response_.cryptographic_iv, (void *)(msg_ptr + 68), 16);

    return &processed_gr_response_;
  }
  int32_t GetGRErrorMsgLength() { return 0; }
  int32_t GetGRErrorMsgOffset() { return 0; }
};
}
}
