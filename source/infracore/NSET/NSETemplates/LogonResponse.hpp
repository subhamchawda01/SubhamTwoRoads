// =====================================================================================
//
//       Filename:  LogonRequest.hpp
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
 * Generic  interface for nse logon response trimmed template.
 * Please extend this for both derivatives and cash market.
 */

class NSELogonResponse {
 protected:
  ProcessedLogonResponse processed_logon_response_;

 public:
  NSELogonResponse() { memset((void *)&processed_logon_response_, 0, sizeof(ProcessedLogonResponse)); }
  virtual ~NSELogonResponse() {}

  virtual ProcessedLogonResponse *ProcessLogon(char const *msg_ptr) { return &processed_logon_response_; }
  virtual int32_t GetLogonErrorMsgLength() { return 0; }
  virtual int32_t GetLogonErrorMsgOffset() { return 0; }
};
}
}
