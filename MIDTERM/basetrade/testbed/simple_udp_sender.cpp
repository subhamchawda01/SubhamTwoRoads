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

#define MAX_MSG 124

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

  bzero(&dest_Addr_, sizeof(dest_Addr_));
  dest_Addr_.sin_family = AF_INET;
  dest_Addr_.sin_port = htons(bcast_port_);
  inet_pton(AF_INET, bcast_ip_.c_str(), &(dest_Addr_.sin_addr));
  bzero(&(dest_Addr_.sin_zero), 8);  // zero the rest of the struct

  char msg[MAX_MSG];
  for (unsigned int i = 3; i < argc; i++) {
    bzero(msg, MAX_MSG);
    strncpy(msg, argv[i], MAX_MSG);

    int char_sent_count_ = sendto(socket_file_descriptor_, msg, (strlen(msg) + 1), 0,
                                  (const struct sockaddr *)&dest_Addr_, (socklen_t)sizeof(dest_Addr_));
  }

  shutdown(socket_file_descriptor_, 2);
  close(socket_file_descriptor_);

  return 1;
}
