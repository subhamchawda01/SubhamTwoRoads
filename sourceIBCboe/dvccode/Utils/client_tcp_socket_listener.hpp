// =====================================================================================
//
//       Filename:  client_tcp_socket_listener.hpp
//
//    Description:  A consumer client listener tcp interface
//
//        Version:  1.0
//        Created:  09/15/2015 03:59:12 PM
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

#include <iostream>

namespace HFSAT {
namespace Utils {

class ClientTCPSocketListener {
 public:
  virtual ~ClientTCPSocketListener() = 0;
  virtual void NotifyClients(void* buffer, uint32_t const& length) = 0;
};
}
}
