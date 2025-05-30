/**
    \file dvccode/Utils/udp_receiver_socket.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_UTILS_UDP_RECEIVER_SOCKET_H
#define BASE_UTILS_UDP_RECEIVER_SOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>  // Include this for memset()
#include <string>
#include <fcntl.h>   // Include this for fcntl
#include <iostream>  // Inlucde this for std::cerr

#include <sys/types.h>
#include <sys/socket.h>  // Include these for socket(), connect(), bind(), etc.
#include <netdb.h>       // Include this for getprotobyname()
#include <netinet/in.h>  // Include this for htonl(), htons(), etc.
#include <arpa/inet.h>
#include <netdb.h>  // Include this for getprotobyname()

namespace HFSAT {

class UDPReceiverSocket {
 protected:
  const std::string listen_ip_;
  const int listen_port_;
  int socket_file_descriptor_;
  bool connection_open_;

 public:
  UDPReceiverSocket(const std::string &listen_ip, const int listen_port)
      : listen_ip_(listen_ip), listen_port_(listen_port), socket_file_descriptor_(-1), connection_open_(false) {
    socket_file_descriptor_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_file_descriptor_ < 0) {
      printf("UDPReceiverSocket cannot open socket \n");
      exit(1);
    }

    struct sockaddr_in self_Addr;
    bzero(&self_Addr, sizeof(struct sockaddr_in));
    self_Addr.sin_family = AF_INET;
    self_Addr.sin_port = htons(listen_port_);
    inet_pton(AF_INET, listen_ip_.c_str(), &(self_Addr.sin_addr));
    bzero(&(self_Addr.sin_zero), 8);  // zero the rest of the struct

    if (bind(socket_file_descriptor_, (struct sockaddr *)&self_Addr, sizeof(self_Addr)) < 0) {
      printf("UDPReceiverSocket cannot bind %s:%d \n", listen_ip_.c_str(), listen_port_);
      exit(1);
    }
  }

  void SetNonBlocking() {
    int flags = fcntl(socket_file_descriptor_, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(socket_file_descriptor_, F_SETFL, flags);
  }

  inline int ReadN(unsigned int _len_, void *_dest_) {
    if (socket_file_descriptor_ != -1) {
      return recvfrom(socket_file_descriptor_, _dest_, _len_, 0, NULL, NULL);
    }
    return -1;
  }

  inline int socket_file_descriptor() const { return socket_file_descriptor_; }

  inline bool IsOpen() const { return connection_open_; }

  void Close() {
    if (socket_file_descriptor_ != -1) {
      shutdown(socket_file_descriptor_, SHUT_RDWR);
      close(socket_file_descriptor_);
      socket_file_descriptor_ = -1;
      connection_open_ = false;
    }
  }
};
}

#endif  // BASE_UTILS_UDP_RECEIVER_SOCKET_H
