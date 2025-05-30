#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <iostream>
#include <string>
#include <sstream>

struct CommandStruct {
  double xyz1_;
  double xyz2_;
  double xyz3_;
  double xyz4_;
  double xyz5_;
  double xyz6_;
  double xyz7_;
  double xyz8_;
  double xyz9_;
  double xyz10_;
};

int main(int argc, char **argv) {
  std::string bcast_ip_("invalid");
  int bcast_port_(15111);

  int socket_file_descriptor_(-1);
  struct sockaddr_in dest_Addr_;

  if (argc < 4) {
    printf("usage : %s <bcast_ip> <bcast_port> <data1> ... <dataN> \n", argv[0]);
    exit(1);
  } else {
    bcast_ip_ = std::string(argv[1]);
    bcast_port_ = atoi(argv[2]);
  }

  /* socket creation */
  socket_file_descriptor_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (socket_file_descriptor_ < 0) {
    printf("%s: cannot open socket \n", "UDPSenderSocket");
    exit(1);
  }

  {
    struct in_addr iaddr;
    iaddr.s_addr = INADDR_ANY;  // use DEFAULT interface
    // Set the outgoing interface to DEFAULT
    if (setsockopt(socket_file_descriptor_, IPPROTO_IP, IP_MULTICAST_IF, &iaddr, sizeof(struct in_addr)) < 0) {
      perror("setsockopt() set outgoing interface to DEFAULT failed");
      exit(1);
    }

    unsigned char mc_ttl = 6;
    // set the TTL (time to live/hop count) for the send
    if ((setsockopt(socket_file_descriptor_, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&mc_ttl, sizeof(mc_ttl))) < 0) {
      perror("setsockopt() set TTL to 6 failed");
      exit(1);
    }

    unsigned char one = 1;
    // send multicast traffic to oneself too
    if (setsockopt(socket_file_descriptor_, IPPROTO_IP, IP_MULTICAST_LOOP, &one, sizeof(unsigned char)) < 0) {
      perror("setsockopt() set IP_MULTICAST_LOOP = 1 failed ");
      exit(1);
    }
  }

  bzero(&dest_Addr_, sizeof(dest_Addr_));
  dest_Addr_.sin_family = AF_INET;
  inet_pton(AF_INET, bcast_ip_.c_str(), &(dest_Addr_.sin_addr));
  dest_Addr_.sin_port = htons(bcast_port_);
  bzero(&(dest_Addr_.sin_zero), 8);  // zero the rest of the struct

  CommandStruct cmd_struct_array_[2000];
  for (auto i = 0u; i < 2000; i++) {
    cmd_struct_array_[i].xyz10_ = i;
  }

  int char_sent_count_ = sendto(socket_file_descriptor_, cmd_struct_array_, (2000) * sizeof(CommandStruct), 0,
                                (const struct sockaddr *)&dest_Addr_, (socklen_t)sizeof(dest_Addr_));
  std::cout << " char_sent_count_ = " << char_sent_count_ << std::endl;

  shutdown(socket_file_descriptor_, 2);
  close(socket_file_descriptor_);

  return 1;
}
