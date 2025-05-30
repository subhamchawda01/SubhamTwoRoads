#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_MSG 1024

int main(int argc, char *argv[]) {
  std::string bcast_ip_1_("127.0.0.1");
  int bcast_port_1_(17107);

  int socket_file_descriptor_ = -1;
  struct sockaddr_in mcast_Addr;

  socket_file_descriptor_ = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_file_descriptor_ < 0) {
    printf("cannot open socket \n");
    exit(1);
  }

  bzero(&mcast_Addr, sizeof(mcast_Addr));
  mcast_Addr.sin_family = AF_INET;
  mcast_Addr.sin_port = htons(bcast_port_1_);
  inet_pton(AF_INET, bcast_ip_1_.c_str(), &(mcast_Addr.sin_addr));
  bzero(&(mcast_Addr.sin_zero), 8);  // zero the rest of the struct

  if (bind(socket_file_descriptor_, (struct sockaddr *)&mcast_Addr, sizeof(mcast_Addr)) < 0) {
    printf("cannot bind port number %d \n", bcast_port_1_);
    exit(1);
  }

  char msg[MAX_MSG];

  while (1) {
    bzero(msg, MAX_MSG);

    int received_char_count_ = recvfrom(socket_file_descriptor_, msg, MAX_MSG, 0, NULL, NULL);
    if (received_char_count_ < 0) {
      printf("%s: cannot receive data \n", "Receiver");
      break;
    } else {
      //      printf ( "%s\n", msg );
      printf("%d\n", received_char_count_);
    }
  }
  close(socket_file_descriptor_);
  return 0;
}
