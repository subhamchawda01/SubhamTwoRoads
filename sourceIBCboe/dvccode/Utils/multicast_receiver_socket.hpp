/**
    \file dvccode/Utils/multicast_receiver_socket.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_UTILS_MULTICAST_RECEIVER_SOCKET_H
#define BASE_UTILS_MULTICAST_RECEIVER_SOCKET_H

#include <fcntl.h>   // Include this for fcntl
#include <iostream>  // Inlucde this for std::cerr
#include <stdio.h>
#include <stdlib.h>
#include <string>  // Include this for string
#include <arpa/inet.h>
#include <ifaddrs.h>     // Include this for getifaddrs and comparison with "bond0", "eth5"
#include <netdb.h>       // Include this for getprotobyname()
#include <netinet/in.h>  // Include this for htonl(), htons(), etc.
#include <sys/select.h>
#include <sys/socket.h>  // Include these for socket(), connect(), bind(), etc.
#include <sys/time.h>
#include <sys/types.h>
#include "dvccode/CDef/defines.hpp"
#include "dvccode/Utils/strict_join.hpp"

namespace HFSAT {

/// \brief class to receive multicast udp packets on a channel
class MulticastReceiverSocket {
 protected:
  const std::string listen_ip_;
  const short listen_port_;
  int socket_file_descriptor_;
  std::string preferred_ifa_name_;
  bool addr_join_;

 private:
  //    struct ip_mreq local_mc_req; // should we record the mc_req that worked
  // in future we need a better handl;e on what will be able on which interface ?
 public:
  MulticastReceiverSocket(const std::string& listen_ip, const int listen_port,
                          const std::string given_preferred_ifa_name)  /// force compulsory interface
      : listen_ip_(listen_ip),
        listen_port_(listen_port),
        socket_file_descriptor_(-1),
        preferred_ifa_name_(given_preferred_ifa_name),
        addr_join_(SocketJoinType::IsStrictJoin()) {
    /* create socket to join multicast group on */
    socket_file_descriptor_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_file_descriptor_ < 0) {
      fprintf(stderr, "cannot open socket \n");
      //exit(1);
    }

    /* set reuse port to on to allow multiple binds per host */
    {
      int flag_on = 1;
      if ((setsockopt(socket_file_descriptor_, SOL_SOCKET, SO_REUSEADDR, &flag_on, sizeof(flag_on))) < 0) {
        fprintf(stderr, "MulticastReceiverSocket setsockopt() SOL_SOCKET SO_REUSEADDR failed\n");
        exit(1);
      }
    }

    McastJoin();

    {
      /* construct a multicast address structure */
      struct sockaddr_in mcast_addr;
      bzero(&mcast_addr, sizeof(mcast_addr));
      mcast_addr.sin_family = AF_INET;

      if (true == addr_join_) {
        mcast_addr.sin_addr.s_addr = inet_addr(listen_ip_.c_str());
      } else {
        mcast_addr.sin_addr.s_addr = htonl(INADDR_ANY);
      }
      mcast_addr.sin_port = htons(listen_port_);

      /* bind to specified port onany interface */
      if (bind(socket_file_descriptor_, (struct sockaddr*)&mcast_addr, sizeof(struct sockaddr_in)) < 0) {
        fprintf(stderr, "%s cannot bind %s:%d \n", "MulticastReceiverSocket", listen_ip_.c_str(), listen_port_);
        exit(1);
      }
    }
  }

  ~MulticastReceiverSocket() { Close(); }

  inline int socket_file_descriptor() const { return socket_file_descriptor_; }

  inline char const* socket_ip_addr() const { return listen_ip_.c_str(); }
  inline int32_t const socket_port() const { return listen_port_; }

  void McastJoin() {
    /* construct an IGMP join request structure */
    struct ip_mreq mc_req;
    inet_pton(AF_INET, listen_ip_.c_str(), &(mc_req.imr_multiaddr.s_addr));

    /// if eth5 present then join on it else join on INADDR_ANY
    struct ifaddrs* ifAddrStruct = NULL;
    struct ifaddrs* ifa = NULL;
    getifaddrs(&ifAddrStruct);

    bool found = false;

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {

      if(ifa->ifa_addr == NULL)continue;

      if (ifa->ifa_addr->sa_family == AF_INET && strcmp(ifa->ifa_name, preferred_ifa_name_.c_str()) == 0) {
        mc_req.imr_interface = ((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;

        found = true;

        /* send an ADD MEMBERSHIP message via setsockopt */
        if ((setsockopt(socket_file_descriptor_, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&mc_req, sizeof(mc_req))) < 0) {
          fprintf(stderr, "setsockopt() failed in IP_ADD_MEMBERSHIP %s %s %d\n", listen_ip_.c_str(), ifa->ifa_name,
                  socket_file_descriptor_);
          //exit(1);
        }
      }
    }

    if (!found) {
      if (true == addr_join_) {
        mc_req.imr_interface.s_addr = inet_addr(listen_ip_.c_str());
      } else {
        mc_req.imr_interface.s_addr = htonl(INADDR_ANY);
      }

      /* send an ADD MEMBERSHIP message via setsockopt */
      if ((setsockopt(socket_file_descriptor_, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&mc_req, sizeof(mc_req))) < 0) {
        std::cerr << "setsockopt() failed in IP_ADD_MEMBERSHIP " << listen_ip_ << ": " << listen_port_ << std::endl;
        exit(1);
      }
    }

    if (ifAddrStruct != NULL) freeifaddrs(ifAddrStruct);
  }

  void McastJoin(const std::string maddr, std::string given_preferred_ifa_name = "") {
    /* construct an IGMP join request structure */
    struct ip_mreq mc_req;
    inet_pton(AF_INET, maddr.c_str(), &(mc_req.imr_multiaddr.s_addr));
    /// if eth5 present then join on it else join on INADDR_ANY
    struct ifaddrs* ifAddrStruct = NULL;
    struct ifaddrs* ifa = NULL;
    getifaddrs(&ifAddrStruct);

    bool found = false;

    if (given_preferred_ifa_name == "") given_preferred_ifa_name = preferred_ifa_name_;

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {

      if(ifa->ifa_addr == NULL)continue;
      
      if (ifa->ifa_addr->sa_family == AF_INET && strcmp(ifa->ifa_name, given_preferred_ifa_name.c_str()) == 0) {
        mc_req.imr_interface = ((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
        found = true;

        /* send an ADD MEMBERSHIP message via setsockopt */
        if ((setsockopt(socket_file_descriptor_, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&mc_req, sizeof(mc_req))) < 0) {
          fprintf(stderr, "setsockopt() failed in IP_ADD_MEMBERSHIP %s %s %d \n", maddr.c_str(), ifa->ifa_name,
                  socket_file_descriptor_);
          exit(1);
        }
      }
    }

    if (!found) {
      if (true == addr_join_) {
        mc_req.imr_interface.s_addr = inet_addr(listen_ip_.c_str());
      } else {
        mc_req.imr_interface.s_addr = htonl(INADDR_ANY);
      }

      /* send an ADD MEMBERSHIP message via setsockopt */
      if ((setsockopt(socket_file_descriptor_, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&mc_req, sizeof(mc_req))) < 0) {
        fprintf(stderr, "setsockopt() failed in IP_ADD_MEMBERSHIP %s\n", maddr.c_str());
        exit(1);
      }
    }

    if (ifAddrStruct != NULL) freeifaddrs(ifAddrStruct);
  }

  void McastDrop() { McastDrop(listen_ip_); }

  void McastDrop(
      const std::string maddr) /* technically needs another argument - works because of way in which we call Drops */
  {
    /* construct an IGMP join request structure */
    struct ip_mreq mc_req;
    inet_pton(AF_INET, maddr.c_str(), &(mc_req.imr_multiaddr.s_addr));
    /// if eth5 present then join on it else join on INADDR_ANY
    struct ifaddrs* ifAddrStruct = NULL;
    struct ifaddrs* ifa = NULL;
    getifaddrs(&ifAddrStruct);

    bool found = false;

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
      if (ifa->ifa_addr->sa_family == AF_INET &&
          strncmp(ifa->ifa_name, preferred_ifa_name_.c_str(), preferred_ifa_name_.length()) == 0) {
        mc_req.imr_interface = ((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
        found = true;

        /* send an DROP MEMBERSHIP message via setsockopt */
        if ((setsockopt(socket_file_descriptor_, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void*)&mc_req, sizeof(mc_req))) < 0) {
          fprintf(stderr, "setsockopt() failed in IP_DROP_MEMBERSHIP %s\n", maddr.c_str());
          // exit(1); //Should not exit in this case
        }
      }
    }

    if (!found) {
      if (true == addr_join_) {
        mc_req.imr_interface.s_addr = inet_addr(listen_ip_.c_str());
      } else {
        mc_req.imr_interface.s_addr = htonl(INADDR_ANY);
      }

      /* send an DROP MEMBERSHIP message via setsockopt */
      if ((setsockopt(socket_file_descriptor_, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void*)&mc_req, sizeof(mc_req))) < 0) {
        fprintf(stderr, "setsockopt() failed in IP_DROP_MEMBERSHIP %s\n", maddr.c_str());
        // exit(1); //Should not exit in this case
      }
    }

    if (ifAddrStruct != NULL) freeifaddrs(ifAddrStruct);
  }

  void setBufferSize(int size) {
    socklen_t optlen = sizeof(size);
    setsockopt(socket_file_descriptor_, SOL_SOCKET, SO_RCVBUF, &size, optlen);
    int set_size;
    getsockopt(socket_file_descriptor_, SOL_SOCKET, SO_RCVBUF, &set_size, &optlen);
    // fprintf( stderr, "Socket Buffer Size set to %d \n", set_size );
  }

  void SetNonBlocking() {
    int flags = fcntl(socket_file_descriptor_, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(socket_file_descriptor_, F_SETFL, flags);
  }

  /// TODO_OPT Add functionality in MulticastReceiverSocket to fetch more data than normal
  /// and not leave it up to the system buffer to lose data in case we are slow in processing.
  inline int ReadN(const unsigned int _len_, void* _dest_) {
    if (socket_file_descriptor_ != -1) {
      return recvfrom(socket_file_descriptor_, _dest_, _len_, 0, NULL, NULL);
    }
    return -1;
  }

  // ReadN with timeout, return -2 on timeout
  inline int ReadN(const unsigned int _len_, void* _dest_, timeval* t) {
    fd_set socks;
    FD_ZERO(&socks);
    FD_SET(socket_file_descriptor_, &socks);
    if (select(socket_file_descriptor_ + 1, &socks, NULL, NULL, t) == 1) {
      return ReadN(_len_, _dest_);
    } else
      return -2;  // might be due to time out or due to error.. error not handled specifically
  }

  inline bool IsOpen() const { return (socket_file_descriptor_ != -1); }

  inline int GetPort() { return listen_port_; }

  void Close() {
    if (socket_file_descriptor_ != -1) {
      shutdown(socket_file_descriptor_, SHUT_RDWR);
      close(socket_file_descriptor_);
      socket_file_descriptor_ = -1;
    }
  }
};
}

#endif  // BASE_UTILS_MULTICAST_RECEIVER_SOCKET_H
