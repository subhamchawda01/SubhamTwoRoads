/**
    \file dvccode/Utils/udp_sender_socket.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_UTILS_UDP_SENDER_SOCKET_H
#define BASE_UTILS_UDP_SENDER_SOCKET_H

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

namespace HFSAT {

class UDPSenderSocket {
 protected:
  const std::string send_ip_;
  const int send_port_;

  struct sockaddr_in dest_Addr_;
  const socklen_t dest_Addr_len_;

  int socket_file_descriptor_;

 public:
  UDPSenderSocket(const std::string &_send_ip_, const int &_send_port_)
      : send_ip_(_send_ip_),
        send_port_(_send_port_),
        dest_Addr_(),
        dest_Addr_len_(sizeof(struct sockaddr_in)),
        socket_file_descriptor_(-1) {
    /* socket creation */
    socket_file_descriptor_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_file_descriptor_ < 0) {
      printf("%s: cannot open socket \n", "UDPSenderSocket");
      exit(1);
    }

    // setting destination address
    bzero(&dest_Addr_, sizeof(dest_Addr_));
    dest_Addr_.sin_family = AF_INET;
    dest_Addr_.sin_port = htons(send_port_);
    inet_pton(AF_INET, send_ip_.c_str(), &(dest_Addr_.sin_addr));
    bzero(&(dest_Addr_.sin_zero), 8);  // zero the rest of the struct
  }

  // inline const int socket_file_descriptor() const { return socket_file_descriptor_; }

  inline bool IsOpen() const { return (socket_file_descriptor_ != -1); }

  /// write _len_ bytes of data from _src_ to socket if connection is open
  inline int WriteN(unsigned int _len_, const void *_src_) const {
    if (socket_file_descriptor_ != -1) {
      return sendto(socket_file_descriptor_, _src_, _len_, 0, (const struct sockaddr *)&dest_Addr_, dest_Addr_len_);
    }
    return -1;
  }

  void McastSetTTL(unsigned char mc_ttl) {
    /* set the TTL (time to live/hop count) for the send */
    if ((setsockopt(socket_file_descriptor_, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&mc_ttl, sizeof(mc_ttl))) < 0) {
      perror("setsockopt() failed");
      Close();
      exit(1);
    }
  }

  void Close() {
    if (socket_file_descriptor_ != -1) {
      shutdown(socket_file_descriptor_, SHUT_RDWR);
      close(socket_file_descriptor_);
      socket_file_descriptor_ = -1;
    }
  }
};
}

#endif  // BASE_UTILS_UDP_SENDER_SOCKET_H
