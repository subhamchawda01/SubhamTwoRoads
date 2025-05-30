// =====================================================================================
//
//       Filename:  tcp_server_socket_listener.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  09/21/2015 07:49:26 AM
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

class TCPServerSocketListener {
 public:
  virtual ~TCPServerSocketListener() {}
  virtual void OnClientRequest(int32_t client_fd, char* buffer, uint32_t const& length) = 0;
};
}
}
