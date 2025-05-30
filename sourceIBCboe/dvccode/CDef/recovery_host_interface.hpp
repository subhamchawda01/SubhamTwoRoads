// =====================================================================================
//
//       Filename:  recovery_host_interface.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  03/15/2016 10:20:57 AM
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

namespace HFSAT {
namespace CDef {

class BaseRecoveryHostInterface {
 public:
  virtual bool IsSecurityAvailableForRecovery(std::string const& security_id) = 0;
  virtual bool FetchProcessedLiveOrdersMap(std::string const& security_id, char* data_packet,
                                           int32_t const& MAX_BUFFER_SIZE) = 0;

  virtual ~BaseRecoveryHostInterface() {}
};
}
}
