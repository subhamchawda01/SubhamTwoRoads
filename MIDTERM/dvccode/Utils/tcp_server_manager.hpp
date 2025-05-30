// =====================================================================================
//
//       Filename:  tcp_server_manager.hpp
//
//    Description:  A TCP Server Manager
//
//        Version:  1.0
//        Created:  09/10/2015 04:57:06 PM
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
#include <fstream>
#include <sys/time.h>
#include <cerrno>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/Utils/tcp_server_socket.hpp"
#include "dvccode/Utils/client_tcp_thread.hpp"
#include "dvccode/Utils/thread.hpp"
#include <ifaddrs.h>

namespace HFSAT {
namespace Utils {

class TCPServerManager : public HFSAT::Thread {
 private:
  bool keep_running_;
  HFSAT::TCPServerSocket* tcp_server_socket_;
  HFSAT::DebugLogger& dbglogger_;
  int32_t server_connection_port_;
  bool is_requestless_mode_;
  HFSAT::Lock server_connection_lock_;
  std::map<int32_t, ClientTCPThread*> socket_fd_to_client_tcp_threads_;
  TCPServerSocketListener* callback_handler_;
  std::string interface_name_;

 public:
  TCPServerManager(int32_t server_port, HFSAT::DebugLogger& dbglogger, bool requestless_mode = false,
                   std::string interface_name = "")
      : keep_running_(false),
        tcp_server_socket_(NULL),
        dbglogger_(dbglogger),
        server_connection_port_(server_port),
        is_requestless_mode_(requestless_mode),
        server_connection_lock_(),
        socket_fd_to_client_tcp_threads_() {
    if (strcmp(interface_name.c_str(), "") == 0) {
      tcp_server_socket_ = new HFSAT::TCPServerSocket(server_connection_port_);
    } else {
      struct ifaddrs *ifap, *ifa;
      char* interface_ip_addr;
      getifaddrs(&ifap);
      bool found = false;
      for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if ((ifa->ifa_addr->sa_family == AF_INET) && strcmp(ifa->ifa_name, interface_name.c_str()) == 0) {
          found = true;
          interface_ip_addr = inet_ntoa(((struct sockaddr_in*)ifa->ifa_addr)->sin_addr);
          printf("Interface: %s\tAddress: %s\n", ifa->ifa_name,
                 inet_ntoa(((struct sockaddr_in*)ifa->ifa_addr)->sin_addr));
          break;
        }
      }
      if (!found) {
        interface_ip_addr = const_cast<char*>("0.0.0.0");
      }
      tcp_server_socket_ = new HFSAT::TCPServerSocket(server_connection_port_, interface_ip_addr);
    }

    if (-1 == tcp_server_socket_->socket_file_descriptor()) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO OPEN TCP SERVER PORT : " << server_connection_port_
                                   << " SYSTEMERROR : " << strerror(errno) << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      exit(-1);
    }
  }

  void SubscribeForUpdates(TCPServerSocketListener* tcp_server_socket_listener) {
    callback_handler_ = tcp_server_socket_listener;
  }

  void CleanUp() {
    keep_running_ = false;
    sleep(1);

    server_connection_lock_.LockMutex();
    for (auto& itr : socket_fd_to_client_tcp_threads_) {
      if (NULL != itr.second) {
        (itr.second)->CleanUp();
        delete itr.second;
        itr.second = NULL;
      }
    }
    server_connection_lock_.UnlockMutex();

    if (NULL != tcp_server_socket_) {
      tcp_server_socket_->Close();
      delete tcp_server_socket_;
      tcp_server_socket_ = NULL;
    }
  }

  void RespondToClient(int32_t client_fd, char const* packet_buffer, int32_t const& packet_length) {
    server_connection_lock_.LockMutex();

    if (socket_fd_to_client_tcp_threads_.end() == socket_fd_to_client_tcp_threads_.find(client_fd)) {
      server_connection_lock_.UnlockMutex();
      return;
    }

    int32_t written_length = socket_fd_to_client_tcp_threads_[client_fd]->WriteN(packet_length, (void*)packet_buffer);

    if (written_length < packet_length) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "FAILED TO SEND COMPLETE MESSAGE TO CLIENT, WROTE " << written_length
                                   << " INSTEAD OF : " << packet_length << " SYSTEMERROR : " << strerror(errno)
                                   << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
    }

    server_connection_lock_.UnlockMutex();
  }

  void RespondToAllClients(char const* packet_buffer, int32_t const& packet_length) {
    server_connection_lock_.LockMutex();

    for (auto& clients : socket_fd_to_client_tcp_threads_) {
      int32_t written_length = (clients.second)->WriteN(packet_length, (void*)packet_buffer);

      if (written_length < packet_length) {
        DBGLOG_CLASS_FUNC_LINE_ERROR << "FAILED TO SEND COMPLETE MESSAGE TO CLIENT, WROTE " << written_length
                                     << " INSTEAD OF : " << packet_length << " SYSTEMERROR : " << strerror(errno)
                                     << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
      }
    }

    server_connection_lock_.UnlockMutex();
  }

  // This is just to notify client that request has failed and only response server could send is an error
  void RespondFailureToClient(int32_t client_fd) {
    server_connection_lock_.LockMutex();
    if (socket_fd_to_client_tcp_threads_.end() == socket_fd_to_client_tcp_threads_.find(client_fd)) {
      server_connection_lock_.UnlockMutex();
      return;
    }

    // Mark Invalid Read
    int32_t failure_data_length = -2;
    int32_t written_length = socket_fd_to_client_tcp_threads_[client_fd]->WriteN(4, (void*)&failure_data_length);

    DBGLOG_CLASS_FUNC_LINE_ERROR << " FAILED, WROTE : " << written_length << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    if (written_length < 4) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "FAILED TO SEND COMPLETE MESSAGE TO CLIENT, WROTE " << written_length
                                   << " INSTEAD OF 4, SYSTEMERROR : " << strerror(errno) << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
    }

    server_connection_lock_.UnlockMutex();
  }

  std::vector<uint32_t> GetClientFDList() {
    std::vector<uint32_t> fd_list;
    for (auto pair : socket_fd_to_client_tcp_threads_) {
      fd_list.push_back(pair.first);
    }
    return fd_list;
  }

  void thread_main() {
    keep_running_ = true;

    while (keep_running_) {
      // Since client fd is local we are fine with race condition here
      int32_t client_fd = tcp_server_socket_->Accept();

      if (-1 == client_fd) {
        DBGLOG_CLASS_FUNC_LINE_ERROR << "TCP SERVER MANAGER HAS FAILED TO ACCEPT CONNECTION, SYSTEMERROR : "
                                     << strerror(errno) << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
        continue;
      }

      // Add client thread atomically
      server_connection_lock_.LockMutex();

      ClientTCPThread* new_client_thread =
          new ClientTCPThread(client_fd, tcp_server_socket_->socket_file_descriptor(), is_requestless_mode_);
      socket_fd_to_client_tcp_threads_[client_fd] = new_client_thread;

      new_client_thread->AddClientSocketListener(callback_handler_);

      server_connection_lock_.UnlockMutex();

      DBGLOG_CLASS_FUNC_LINE_INFO << "ACCEPTED NEW CLIENT THREAD ON : " << client_fd << DBGLOG_ENDL_NOFLUSH;

      new_client_thread->run();
    }
  }
};
}
}
