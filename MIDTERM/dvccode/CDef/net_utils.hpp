/**
    \file dvccode/CDef/net_utils.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

     Created on: Aug 25, 2011
*/

#ifndef BASE_CDEF_NETUTILS_H_
#define BASE_CDEF_NETUTILS_H_

#include <stdio.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

namespace HFSAT {
class NetUtils {
  static std::vector<std::pair<std::string, std::string> > GetIP4List() {
    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *ifa = NULL;
    void *tmpAddrPtr = NULL;

    getifaddrs(&ifAddrStruct);

    std::vector<std::pair<std::string, std::string> > ipList;

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
      if (ifa->ifa_addr->sa_family == AF_INET) {  // check it is IP4
        // is a valid IP4 Address
        tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
        char addressBuffer[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
        std::pair<std::string, std::string> pair;
        pair.first = std::string(ifa->ifa_name);
        pair.second = std::string(addressBuffer);
        ipList.push_back(pair);
      }
    }
    if (ifAddrStruct != NULL) freeifaddrs(ifAddrStruct);
    return ipList;
  }
};
}

#endif /* BASE_CDEF_NETUTILS_H_ */
