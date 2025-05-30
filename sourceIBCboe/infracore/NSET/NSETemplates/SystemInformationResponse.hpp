// =====================================================================================
//
//       Filename:  SystemInformationResponse.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/30/2015 08:32:04 AM
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

#pragma once

#include "infracore/NSET/NSETemplates/DataDefines.hpp"

namespace HFSAT {
namespace NSE {

class SystemInfoResponse {
 protected:
  ProcessedSystemInformationResponse processed_system_info_response;

 public:
  SystemInfoResponse() {
    memset((void*)&processed_system_info_response, 0, sizeof(ProcessedSystemInformationResponse));
  }
  virtual ~SystemInfoResponse() {}

  virtual ProcessedSystemInformationResponse* ProcessSystemInfoResponse(char const* msg_ptr) {
    return &processed_system_info_response;
  }
};
}
}
