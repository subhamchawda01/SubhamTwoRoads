// =====================================================================================
//
//       Filename:  tcp_client_socket.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  03/06/2015 10:40:16 AM
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

#include <stdlib.h>  // Include this for exit()
#include <stdio.h>
#include <unistd.h>
#include <string.h>  // Include this for bzero()
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>  // Include these for socket(), connect(), bind(), etc.
#include <netinet/in.h>  // Include this for htonl(), htons(), etc.
#include <arpa/inet.h>
#include <netdb.h>    // Include this for getprotobyname()
#include <ifaddrs.h>  // Include this for getifaddrs and comparison with "bond0", "eth5"
#include <fcntl.h>
#include <string>
#include <assert.h>
#include <errno.h>
#include <vector>

#include "dvccode/CDef/fwd_decl.hpp"

namespace HFSAT {

class TCPClientSocket {
 protected:
  int socket_file_descriptor_;
  unsigned int offset_;  // Used for Fixed size reads

 public:
  TCPClientSocket(bool use_eth5_ = false)  // By default use bond0
      : socket_file_descriptor_(-1),
        offset_(0) {
    socket_file_descriptor_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_file_descriptor_ < 0) {
      fprintf(stderr, "%s: cannot open socket \n", "TCPClient");
      exit(1);
    }
  }
  void SetNonBlocking() {
    int flags = fcntl(socket_file_descriptor_, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(socket_file_descriptor_, F_SETFL, flags);
  }

  inline void setReceiveTimeout(unsigned int milliSec) {
    if (milliSec < 0) {
      fprintf(stderr, "timeout value set to negative");
      close(socket_file_descriptor_);
      socket_file_descriptor_ = -1;
      return;
    }

    struct timeval tv;

    tv.tv_sec = milliSec / 1000;
    tv.tv_usec = (milliSec % 1000) * 1000;

    if ((setsockopt(socket_file_descriptor_, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(struct timeval))) ==
        -1) {
      fprintf(stderr, "set timeout failed on setsockopt\n\n");
      close(socket_file_descriptor_);
      socket_file_descriptor_ = -1;
    }
  }

  inline void SetSendingTimeout(unsigned int _msecs_) {
    if (_msecs_ < 0) {
      fprintf(stderr, "timeout value set to negative");
      close(socket_file_descriptor_);
      socket_file_descriptor_ = -1;
      return;
    }

    struct timeval tv;
    tv.tv_sec = _msecs_ / 1000;
    tv.tv_usec = (_msecs_ % 1000) * 1000;

    if ((setsockopt(socket_file_descriptor_, SOL_SOCKET, SO_SNDTIMEO, (struct timeval *)&tv, sizeof(struct timeval))) ==
        -1) {
      fprintf(stderr, "set timeout failed on setsockopt\n\n");
      close(socket_file_descriptor_);
      socket_file_descriptor_ = -1;
    }
  }

  void Connect(const std::string &_ors_ip_, const int _ors_port_) {
    struct sockaddr_in ors_Addr_;
    bzero(&ors_Addr_, sizeof(ors_Addr_));
    ors_Addr_.sin_family = AF_INET;
    ors_Addr_.sin_port = htons(_ors_port_);
    inet_pton(AF_INET, _ors_ip_.c_str(), &(ors_Addr_.sin_addr));

    /*  connect() to the remote echo server  */

    if (connect(socket_file_descriptor_, (struct sockaddr *)&ors_Addr_, sizeof(struct sockaddr_in)) < 0) {
      fprintf(stderr, "connect() failed on %s:%d\n", _ors_ip_.c_str(), _ors_port_);
      close(socket_file_descriptor_);
      socket_file_descriptor_ = -1;
    }
  }

  /// write _len_ bytes of data from _src_ to socket if connection_open_
  inline int WriteNTMO(const unsigned int _len_, const void *_src_) const {
    int n = write(socket_file_descriptor_, _src_, _len_);

    if ((n < 0) && (errno == EAGAIN)) {
      return -2;
    }

    return n;
  }

  /// write _len_ bytes of data from _src_ to socket if connection_open_
  inline int WriteN(const unsigned int len, const void *src) const {
    if (socket_file_descriptor_ != -1) {
      return write(socket_file_descriptor_, src, len);
    }
    return -1;
  }

  inline int ReadNTMO(const unsigned int len, void *dest) const {
    int n = read(socket_file_descriptor_, dest, len);

    if (n < 0 && errno == EAGAIN) {
      return -2;
    }

    return n;
  }

  /// read upto _len_ bytes of data from socket to _dest_ if connection_open_
  inline int ReadN(const unsigned int _len_, void *_dest_) const {
    if (socket_file_descriptor_ != -1) {
      return read(socket_file_descriptor_, _dest_, _len_);
    }
    return -1;
  }

  /// read exactly _len_ bytes of data from socket to _dest_ if connection_open_
  // Use with caution, as this is a blocking call (nd will wait for exact number of bytes)
  inline int ReadNFixed(const unsigned int _len_, void *_dest_) {
    if (socket_file_descriptor_ <= 0) {
      // Invalid fd
      return -1;
    }
    while (offset_ < _len_) {
      int read_len = read(socket_file_descriptor_, ((char *)_dest_) + offset_, _len_ - offset_);
      if (read_len < 0) {
        // Socket disconnected or network error
        return -1;
      }
      offset_ += read_len;
    }
    offset_ = 0;
    return _len_;
  }

  // /// read upto _len_ bytes of data from socket to _dest_ if connection_open_
  // // also return the error code
  // inline int ReadNWithENO ( const unsigned int _len_, void * _dest_ , int & error_no)
  // {
  //   error_no = 0;

  //   if ( socket_file_descriptor_ != -1 )
  // 	{
  // 	  int ret_val = read ( socket_file_descriptor_, _dest_, _len_ );
  // 	  if ( ret_val == -1 )  // On failure it returns -1 and will set errno
  // 	    {
  // 	      error_no = errno;
  // 	    }
  // 	  return ret_val;
  // 	}
  //   return -1;
  // }

  inline bool IsOpen() const { return (socket_file_descriptor_ != -1); }

  inline int socket_file_descriptor() const { return socket_file_descriptor_; }

  // The socket remains valid (only gets disconnected) =>
  // the object can be reused to connect to other servers
  void DisConnect() {
    /*  disconnect from the remote echo server  */
    shutdown(socket_file_descriptor_, SHUT_RDWR);  // stop any further read/write (but receive pending data)
    close(socket_file_descriptor_);                // close the connection
  }

  // Invalidates the file descriptor (the object can't be reused for connecting)
  void Close() {
    if (socket_file_descriptor_ != -1) {
      shutdown(socket_file_descriptor_, SHUT_RDWR);
      close(socket_file_descriptor_);
      socket_file_descriptor_ = -1;
    }
  }
};

class SocketSet {
 public:
  struct SocketContext {
    void *ref;
    int sock, padding;
  };
  typedef std::vector<SocketContext> Selected;

 private:
  Selected socks;
  int maxfd;
  fd_set fds;

 public:
  SocketSet() {
    FD_ZERO(&fds);
    maxfd = -1;
  }
  // ctx is a void* that is accociated with the file descriptor;
  // everytime we return a socket, we also return this 'context'
  // to reduce book keeping at the user's end;
  void add(const std::vector<int> &fs, void *ctx) {
    for (auto f : fs) {
      SocketContext sc;
      sc.ref = ctx;
      sc.sock = f;
      socks.push_back(sc);
      maxfd = std::max(f, maxfd);
      FD_SET(f, &fds);
    }
  }
  Selected AllocateSelectedContainer() { return Selected(socks.size()); }
  int waitForAny(Selected &selected) {
    selected.resize(socks.size());
    int count = select(maxfd + 1, &fds, 0, 0, 0);
    int i = 0;
    if (count > 0) {
      for (auto ctx : socks) {
        auto fd = ctx.sock;
        if (FD_ISSET(fd, &fds))
          selected[i++] = ctx;
        else
          FD_SET(fd, &fds);  // set it for the next call to select
      }
    }
    assert(count == i);
    return count;
  }
};
}

class TcpClientSocketWithLogging : public HFSAT::TCPClientSocket {
  AsyncWriter *pReader, *pWriter;
  ChannelId readChannel, writeChannel;

 public:
  TcpClientSocketWithLogging(bool use_ethnet5, AsyncWriter *pWriter_, AsyncWriter *pReader_, std::string logfile);
  int WriteN(const unsigned int _len_, const void *_src_);
  int ReadN(const unsigned int _len_, void *_dest_);
};
