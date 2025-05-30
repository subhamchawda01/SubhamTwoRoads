/**
    \file dvccode/Utils/multicast_sender_socket.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_UTILS_MULTICAST_SENDER_SOCKET_H
#define BASE_UTILS_MULTICAST_SENDER_SOCKET_H

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
#include <ifaddrs.h>
#include <string>
#include "dvccode/Utils/strict_join.hpp"

namespace HFSAT {

class MulticastSenderSocket {
 protected:
  const std::string send_ip_;
  const int send_port_;

  struct sockaddr_in dest_addr_;
  const socklen_t dest_addr_len_;

  int socket_file_descriptor_;

 public:
  MulticastSenderSocket(const std::string &_send_ip_, const int &_send_port_,
                        const std::string &interface_)  /// force compulsory interface
      : send_ip_(_send_ip_),
        send_port_(_send_port_),
        dest_addr_(),
        dest_addr_len_(sizeof(struct sockaddr_in)),
        socket_file_descriptor_(-1) {
    /* socket creation */
    socket_file_descriptor_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_file_descriptor_ < 0) {
      printf("%s: cannot open socket \n", "MulticastSenderSocket");
      exit(1);
    }

    McastSetTTL(62);  // GT network needs higher TTL
    McastSetLoop();

    // set specified network interface if it exists
    struct ifaddrs *if_addr_list = NULL;
    struct ifaddrs *ifa = NULL;
    getifaddrs(&if_addr_list);

    for (ifa = if_addr_list; ifa != NULL; ifa = ifa->ifa_next) {
      if (ifa->ifa_addr == NULL || ifa->ifa_name == NULL) {
        continue;
      }

      if (ifa->ifa_addr->sa_family == AF_INET && strcmp(ifa->ifa_name, interface_.c_str()) == 0) {
        if ((setsockopt(socket_file_descriptor_, IPPROTO_IP, IP_MULTICAST_IF,
                        &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr, sizeof(struct in_addr))) < 0) {
          fprintf(stderr, "setsockopt() failed in setting outgoing mcast network interface to eth5 \n");
          exit(1);
        }
      }
    }
    freeifaddrs(if_addr_list);

    // setting destination address
    bzero(&dest_addr_, sizeof(dest_addr_));
    dest_addr_.sin_family = AF_INET;
    dest_addr_.sin_port = htons(send_port_);
    inet_pton(AF_INET, send_ip_.c_str(), &(dest_addr_.sin_addr));
    bzero(&(dest_addr_.sin_zero), 8);  // zero the rest of the struct

    /* set reuse port to on to allow multiple binds per host */
    {
      int flag_on = 1;
      if ((setsockopt(socket_file_descriptor_, SOL_SOCKET, SO_REUSEADDR, &flag_on, sizeof(flag_on))) < 0) {
        // fprintf ( stderr, "MulticastReceiverSocket setsockopt() SOL_SOCKET SO_REUSEADDR failed\n");
        exit(1);
      }
    }

    {
      /* construct a multicast address structure */
      struct sockaddr_in mcast_addr;
      bzero(&mcast_addr, sizeof(mcast_addr));
      mcast_addr.sin_family = AF_INET;
      if (SocketJoinType::IsStrictJoin()) {
        mcast_addr.sin_addr.s_addr = inet_addr(send_ip_.c_str());
      } else {
        mcast_addr.sin_addr.s_addr = htonl(INADDR_ANY);
      }
      mcast_addr.sin_port = htons(send_port_);
      /* bind to specified port on any interface */
      if (bind(socket_file_descriptor_, (struct sockaddr *)&mcast_addr, sizeof(struct sockaddr_in)) < 0) {
        fprintf(stderr, "%s cannot bind %s:%d \n", "MulticastReceiverSocket", send_ip_.c_str(), send_port_);
        exit(1);
      }
    }
  }

  // inline const int socket_file_descriptor() const { return socket_file_descriptor_; }

  inline bool IsOpen() const { return (socket_file_descriptor_ != -1); }

  /// write _len_ bytes of data from _src_ to socket if connection is open
  inline int WriteN(unsigned int _len_, const void *_src_) const {
    if (socket_file_descriptor_ != -1) {
      int retval =
          sendto(socket_file_descriptor_, _src_, _len_, 0, (const struct sockaddr *)&dest_addr_, dest_addr_len_);
      if (retval < 0) {
        fprintf(stdout, "Error: %s", strerror(retval));
      }
      return retval;
    }
    return -1;
  }

  void McastSetTTL(unsigned char mc_ttl) {
    /* set the TTL (time to live/hop count) for the send */
    if ((setsockopt(socket_file_descriptor_, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&mc_ttl, sizeof(mc_ttl))) < 0) {
      perror("setsockopt() McastSetTTL IP_MULTICAST_TTL failed");
      Close();
      exit(1);
    }
  }

  void McastSetLoop() {
    unsigned char one = 1;
    // send multicast traffic to oneself too
    if (setsockopt(socket_file_descriptor_, IPPROTO_IP, IP_MULTICAST_LOOP, &one, sizeof(unsigned char)) < 0) {
      perror("setsockopt() set IP_MULTICAST_LOOP = 1 failed ");
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

#endif  // BASE_UTILS_MULTICAST_SENDER_SOCKET_H
