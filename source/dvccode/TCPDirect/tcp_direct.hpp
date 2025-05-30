// =====================================================================================
//
//       Filename:  tcp_direct.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  04/24/2018 05:44:10 AM
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
#include <zf/zf.h>
#include "zf_utils.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/time.h>
#include <inttypes.h>
#include <iostream>

#include <string>
#include <sstream>

#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"

#define MAX_LISTENERS 8

namespace Solarflare {

class TCPDirect {

 private:
  struct zf_attr *attr_;
  struct zf_stack *stack_;
  std::vector<struct zfur *> udp_receive_zockets_;
  std::vector<SimpleExternalDataLiveListener *> zocket_listeners_;

  TCPDirect()
      : attr(NULL), stack(NULL), udp_receive_zockets_(), zocket_listeners_() {}

  TCPDirect(TCPDirect const &disabled_copy_constructor) = 0;

 public:
  static TCPDirect &GetUniqueInstance() {
    static TCPDirect unique_instance;
    return unique_instance;
  }

  void init() {
    // Initialize tcpdirect lib and allocate a stack
    ZF_TRY(zf_init());
    ZF_TRY(zf_attr_alloc(&attr_));
    ZF_TRY(zf_stack_alloc(attr_, &stack_));
  }

  bool CreateZocketAndAddToMuxer(std::string ip, int32_t port,
                                 SimpleExternalDataLiveListener *listener) {

    if (MAX_LISTENERS == zocket_listeners_.size()) return false;

    std::ostringstream t_temp_oss;
    t_temp_oss << ip << ":" << port;

    struct addrinfo *mcast_info;

    if (0 != getaddrinfo_hostport(t_temp_oss.str(), NULL, &mcast_info)) {
      std::cerr << "FAILED TO LOOKUP MCAST ADDRESS : " << t_temp_oss.str()
                << std::endl;
      return false;
    }

    int32_t ret = -1;

    struct zfur *udp_receive_zocket = NULL;
    ZF_TRY(ret = zfur_alloc(&udp_receive_zocket, stack_, attr_));
    if (0 != ret) return false;

    ZF_TRY(ret = zfur_addr_bind(udp_receive_zocket, mcast_info->ai_addr,
                                mcast_info->ai_addrlen, NULL, 0, 0));
    if (0 != ret) return false;

    return true;
  }
};
}
