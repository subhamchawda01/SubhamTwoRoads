/**
    \file dvccode/Utils/tcp_server_socket.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_UTILS_TCP_SERVER_SOCKET_H
#define BASE_UTILS_TCP_SERVER_SOCKET_H

#include <stdlib.h>  // Include this for exit()
#include <stdio.h>
#include <unistd.h>
#include <string.h>  // Include this for bzero()
#include <sys/time.h>

#include <sys/types.h>
#include <sys/socket.h>  // Include these for socket(), connect(), bind(), etc.
#include <netinet/in.h>  // Include this for htonl(), htons(), etc.
#include <arpa/inet.h>
#include <netdb.h>  // Include this for getprotobyname()
#include <string>
#include "dvccode/Utils/strict_join.hpp"

namespace HFSAT {

class TCPServerSocket {
 protected:
  int server_port_;
  int socket_file_descriptor_;
  std::string interface_ip_addr_;

 public:
  TCPServerSocket(const int server_port, const std::string &interface_ip_addr = "0.0.0.0")
      : server_port_(server_port), socket_file_descriptor_(-1), interface_ip_addr_(interface_ip_addr) {
    /* socket creation */
    socket_file_descriptor_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_file_descriptor_ < 0) {
      printf("%s: cannot open socket \n", "TCPServer");
      exit(1);
    }

    /* create address structure to bind */
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);

    //! If interface_ip_addr_ name is not provided, then
    //! Set interface_ip_addr_ to INADDR_ANY(0.0.0.0)
    if (SocketJoinType::IsStrictJoin() && strcmp(interface_ip_addr_.c_str(), "0.0.0.0") == 0) {
      serv_addr.sin_addr.s_addr = inet_addr(SocketJoinType::GetStrictJoinIp().c_str());  //==> htonl(INADDR_ANY);
    } else {
      serv_addr.sin_addr.s_addr = inet_addr(interface_ip_addr_.c_str());
    }

    int reuse = 1;
    if (setsockopt(socket_file_descriptor_, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
      perror("Setting SO_REUSEADDR error");
      close(socket_file_descriptor_);
      exit(1);
    }

    if (bind(socket_file_descriptor_, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in)) < 0) {
      printf("%s cannot bind port number %d \n", "TCPServer", server_port);
      shutdown(socket_file_descriptor_, SHUT_RDWR);
      close(socket_file_descriptor_);
      exit(1);
    }

/* listen */
#define LISTENQUE 10
    if (listen(socket_file_descriptor_, LISTENQUE) < 0) {
      fprintf(stderr, "%s: Error calling listen()\n", "TCPServer");
      shutdown(socket_file_descriptor_, SHUT_RDWR);
      close(socket_file_descriptor_);
      exit(EXIT_FAILURE);
    }
#undef LISTENQUE
  }

  /// accept incoming connections
  int Accept() {
    int connected_socket_file_descriptor_ = -1;
    if ((connected_socket_file_descriptor_ = accept(socket_file_descriptor_, NULL, NULL)) < 0) {
      fprintf(stderr, "%s Error calling accept()\n", "TCPServer");
      shutdown(socket_file_descriptor_, SHUT_RDWR);
      close(socket_file_descriptor_);
    }
    return connected_socket_file_descriptor_;
  }

  inline int socket_file_descriptor() { return socket_file_descriptor_; }

  /// write _len_ bytes of data from _src_ to socket if connection_open_
  inline int WriteN(const unsigned int _len_, const void *_src_) const {
    if (socket_file_descriptor_ != -1) {
      return write(socket_file_descriptor_, _src_, _len_);
    }
    return -1;
  }

  inline int WriteNToSocket(int _connected_socket_file_descriptor_, const unsigned int _len_, const void *_src_) const {
    if (_connected_socket_file_descriptor_ != -1) {
      return write(_connected_socket_file_descriptor_, _src_, _len_);
    }
    return -1;
  }

  /// read upto _len_ bytes of data from socket to _dest_ if connection_open_
  inline int ReadNFromSocket(int connected_socket_file_descriptor_, const unsigned int _len_, void *_dest_) {
    if (connected_socket_file_descriptor_ != -1) {
      return read(connected_socket_file_descriptor_, _dest_, _len_);
    }
    return -1;
  }

  /// read upto _len_ bytes of data from socket to _dest_ if connection_open_
  inline int ReadN(const unsigned int _len_, void *_dest_) {
    if (socket_file_descriptor_ != -1) {
      return read(socket_file_descriptor_, _dest_, _len_);
    }
    return -1;
  }

  /// accept a new TCP client connection and read exactly _len_ bytes of data from socket to _dest_ if connection_open_
  // Use with caution, as this is a blocking call (nd will wait for exact number of bytes)
  inline int ReadNFixed(const unsigned int _len_, void *_dest_) {
    if (socket_file_descriptor_ <= 0) {
      // Invalid fd
      return -1;
    }
    int fd = Accept();
    if (fd < 0) {
      std::cerr << "Failed to accept new client: " << errno << ", " << strerror(errno) << "\n";
      return -1;
    }
    size_t offset = 0;
    while (offset < _len_) {
      int read_len = read(fd, ((char *)_dest_) + offset, _len_ - offset);
      if (read_len < 0) {
        // Socket disconnected or network error
        return -1;
      }
      offset += read_len;
    }
    close(fd);
    return _len_;
  }

  inline bool IsOpen() const { return (socket_file_descriptor_ != -1); }

  void Close() {
    if (socket_file_descriptor_ != -1) {
      shutdown(socket_file_descriptor_, SHUT_RDWR);
      close(socket_file_descriptor_);
      socket_file_descriptor_ = -1;
    }
  }
};
}

#endif  // BASE_UTILS_TCP_SERVER_SOCKET_H
