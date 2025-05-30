// =====================================================================================
//
//       Filename:  client_tcp_thread.hpp
//
//    Description:  A client interface for TCP socket
//
//        Version:  1.0
//        Created:  09/14/2015 03:33:24 PM
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
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/lock.hpp"
#include "dvccode/Utils/tcp_server_socket_listener.hpp"

#define MAX_TCP_CLIENT_SOCKET_BUFFER_SIZE 65536

namespace HFSAT {
namespace Utils {

class ClientTCPThread : public HFSAT::Thread {
 private:
  int32_t client_fd_;
  int32_t server_fd_;
  bool keep_running_;
  bool requestless_mode_;
  std::vector<TCPServerSocketListener*> client_tcp_socket_listeners_vec_;

 public:
  ClientTCPThread(int32_t client_fd, int32_t server_fd, bool requestless_mode = false)
      : client_fd_(client_fd),
        server_fd_(server_fd),
        keep_running_(false),
        requestless_mode_(requestless_mode),
        client_tcp_socket_listeners_vec_() {}

  inline bool isRunning() { return keep_running_; }

  void AddClientSocketListener(TCPServerSocketListener* client_socket_listener) {
    HFSAT::VectorUtils::UniqueVectorAdd<TCPServerSocketListener*>(client_tcp_socket_listeners_vec_,
                                                                  client_socket_listener);
  }

  void NotifyClients(char* buffer, uint32_t const& length) {
    for (uint32_t listener_counter = 0; listener_counter < client_tcp_socket_listeners_vec_.size();
         listener_counter++) {
      client_tcp_socket_listeners_vec_[listener_counter]->OnClientRequest(client_fd_, buffer, length);
    }
  }

  inline int32_t WriteN(uint32_t const& packet_length, void* const packet) {
    int32_t written_length = -1;

    if (-1 != client_fd_) {
      written_length = write(client_fd_, packet, packet_length);
    }
    return written_length;
  }

  inline int32_t ReadN(uint32_t const& packet_length, void* packet) {
    int32_t read_length = -1;

    if (-1 != client_fd_) {
      read_length = read(client_fd_, packet, packet_length);
    }

    return read_length;
  }

  inline void Close() {
    if (-1 != server_fd_ && -1 != client_fd_) {
      shutdown(client_fd_, SHUT_RDWR);
      close(client_fd_);
      client_fd_ = -1;
    }
  }

  void CleanUp() {
    keep_running_ = false;
    Close();
  }

  void thread_main() {
    // Not expecting any input from client, only disseminate mode
    if (true == requestless_mode_) return;

    char buffer[MAX_TCP_CLIENT_SOCKET_BUFFER_SIZE];
    memset((void*)buffer, 0, MAX_TCP_CLIENT_SOCKET_BUFFER_SIZE);

    keep_running_ = true;

    while (keep_running_) {
      int32_t read_length = ReadN(MAX_TCP_CLIENT_SOCKET_BUFFER_SIZE, buffer);

      if (0 == read_length) {
        Close();
        keep_running_ = false;
        break;
      } else if (-1 == read_length && EAGAIN == errno) {
        continue;
      } else if (-1 == read_length) {
        Close();
        keep_running_ = false;
        break;
      } else {
        NotifyClients(buffer, read_length);
      }
    }
  }
};
}
}
