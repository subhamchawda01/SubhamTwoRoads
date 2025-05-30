// =====================================================================================
//
//       Filename:  tcp_direct_client_zocket.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  08/01/2018 05:11:43 AM
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

#include <zf/zf.h>
#include <zf/zf_utils.h>

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <vector>
#include <sstream>

#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"

namespace HFSAT {
namespace Utils {

struct cfg {
  int size;
};

struct rx_msg {
  struct zft_msg msg;
  struct iovec iov[1];
};

class TCPDirectClientZocket {
 private:
  bool keep_me_running_;
  struct zfur_msg rcv_buffer_;
  struct zf_attr* attr_;
  struct zf_stack* stack_;
  struct zf_muxer_set* muxer_;
  struct zft* zock_;
  int32_t packet_count_;

  TCPDirectClientZocket(TCPDirectClientZocket const& disabled_copy_constructor) = delete;
  TCPDirectClientZocket() {}

 public:
  static TCPDirectClientZocket& GetUniqueInstance() {
    static TCPDirectClientZocket unique_instance;
    return unique_instance;
  }

  void init() {
    // Initialize tcpdirect lib and allocate a stack
    ZF_TRY(zf_init());
    ZF_TRY(zf_attr_alloc(&attr_));
    ZF_TRY(zf_stack_alloc(attr_, &stack_));
    ZF_TRY(zf_muxer_alloc(stack_, &muxer_));
  }

  //      bool CreateTCPDirectClientZocketAndAddToMuxer( std::string ip, int32_t port, SimpleExternalDataLiveListener *
  //      listener ) {
  bool CreateTCPDirectClientZocketAndAddToMuxer(std::string ip, int32_t port) {
    std::ostringstream t_temp_oss;
    t_temp_oss << ip << ":" << port;

    struct addrinfo* ai;
    if (getaddrinfo_hostport(t_temp_oss.str().c_str(), NULL, &ai) != 0) {
      std::cerr << "FAILED TO LOOKUP ADDRESS : " << t_temp_oss.str() << std::endl;
      exit(-1);
    }

    struct zft_handle* tcp_handle;
    ZF_TRY(zft_alloc(stack_, attr_, &tcp_handle));

    // Connect
    ZF_TRY(zft_connect(tcp_handle, ai->ai_addr, ai->ai_addrlen, &zock_));
    while (TCP_SYN_SENT == zft_state(zock_)) zf_reactor_perform(stack_);
    ZF_TEST(TCP_ESTABLISHED == zft_state(zock_));

    struct epoll_event event = {.events = EPOLLIN};
    ZF_TRY(zf_muxer_add(muxer_, zft_to_waitable(zock_), &event));

    std::cout << "CONNECTION ESTABLISHED : " << std::endl;

    return true;
  }

  void RunLiveDispatcher() {

    struct rx_msg msg;
    const int max_iov = sizeof(msg.iov) / sizeof(msg.iov[0]);

    struct epoll_event ev;


    ZF_TEST(zf_muxer_wait(muxer_, &ev, 1, -1) == 1);
    ZF_TEST(ev.events & EPOLLIN);


    msg.msg.iovcnt = max_iov;
    zft_zc_recv(zock_, &msg.msg, 0);
    ZF_TEST(msg.msg.iovcnt == 1);
    ZF_TEST(zft_zc_recv_done(zock_, &msg.msg) == 1);

    std::cout << "RECEIVED : " << msg.msg.iov[0].iov_len << " " << *((char*)(msg.msg.iov[0].iov_base))
              << " LEFT OVER : " << msg.msg.pkts_left << std::endl;
  }

  void DoReactorPerform() { zf_reactor_perform(stack_); }

  int32_t SendPacket(char* send_buf, int32_t length) {
    //        size_t space ;
    //        int32_t sp = zft_send_space(zock_, &space);

    //        std::cout << sp << " " << space << std::endl ;

    //        ZF_TEST(zft_send_single(zock_, send_buf, length, 0) == (int32_t)length);
    int32_t testval = zft_send_single(zock_, send_buf, length, 0);
    packet_count_++;

    //        if( packet_count_ == 40 ) {
    //          packet_count_= 0 ;
    //          zf_reactor_perform(stack_);
    //        }

    //        std::cout << " REACTOR : " << rec_status << std::endl;

    std::cout << " SENT PACKETS : " << packet_count_ << std::endl;
    return testval;
  }
};
}
}
