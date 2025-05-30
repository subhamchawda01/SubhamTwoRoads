// =====================================================================================
//
//       Filename:  ors_fake_onload_tcp_warmup_thread.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  04/12/2017 04:38:58 PM
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
#include <cstdlib>
#include "dvccode/Utils/tcp_client_socket.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/settings.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "onload/extensions.h"

namespace HFSAT {
namespace Utils {

class OnloadFakeTCPWarmupThread : public HFSAT::Thread {
 private:
  bool keep_running_;
  std::vector<HFSAT::TCPClientSocket*> socket_vec_;

  OnloadFakeTCPWarmupThread() : keep_running_(true) {}

  OnloadFakeTCPWarmupThread(OnloadFakeTCPWarmupThread& disabled_copy_constructor) = delete;

 public:
  static OnloadFakeTCPWarmupThread& GetUniqueInstance() {
    static OnloadFakeTCPWarmupThread unique_instance;
    return unique_instance;
  }

  void SubscribeSocketForOnloadTCPWarmup(HFSAT::TCPClientSocket* tcp_client_socket) {
    if (onload_fd_check_feature(tcp_client_socket->socket_file_descriptor(), ONLOAD_FD_FEAT_MSG_WARM) > 0) {
      socket_vec_.push_back(tcp_client_socket);
      std::cout << "ONLOAD MSG WARM FEATURE IS SUPPORTED FOR FD : " << tcp_client_socket->socket_file_descriptor()
                << std::endl;
    } else {
      std::cerr << "ONLOAD MSG WARM FEATURE IS NOT SUPPORTED FOR FD : " << tcp_client_socket->socket_file_descriptor()
                << std::endl;
    }
  }

  void StopThread() { keep_running_ = false; }

  void thread_main() {
    char buffer[10];

    while (keep_running_) {
      // Try Sending a fake TCP warmpup packet at every 4msec
      HFSAT::usleep(4000);

      for (auto& socket_itr : socket_vec_) {
        send(socket_itr->socket_file_descriptor(), buffer, 10, ONLOAD_MSG_WARM);
      }
    }
  }
};
}
}
