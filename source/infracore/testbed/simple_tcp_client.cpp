#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctime>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <string>

#define MAX_MSG 12400

int main(int argc, char **argv) {
  std::string serv_ip_("invalid");
  int serv_port_(11111);

  int socket_file_descriptor_(-1);
  struct sockaddr_in dest_Addr_;

  if (argc < 4) {
    printf("usage : %s <serv_ip> <serv_port> <data1> ... <dataN> \n", argv[0]);
    exit(1);
  } else {
    serv_ip_ = std::string(argv[1]);
    serv_port_ = atoi(argv[2]);
  }

  /* socket creation */
  socket_file_descriptor_ = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_file_descriptor_ < 0) {
    printf("%s: cannot open socket \n", "TCPClient");
    exit(1);
  }

  bzero(&dest_Addr_, sizeof(dest_Addr_));
  dest_Addr_.sin_family = AF_INET;
  dest_Addr_.sin_port = htons(serv_port_);
  inet_pton(AF_INET, serv_ip_.c_str(), &(dest_Addr_.sin_addr));

  /*  connect() to the remote echo server  */

  if (connect(socket_file_descriptor_, (struct sockaddr *)&dest_Addr_, sizeof(struct sockaddr_in)) < 0) {
    printf("connect() failed\n");
    close(socket_file_descriptor_);
    exit(1);
  }

  char msg[MAX_MSG] = {0};
  while (1) {
    bzero(msg, MAX_MSG);
    int char_received_count_ = read(socket_file_descriptor_, msg, MAX_MSG);
    if (char_received_count_ < 0) {
      printf("%s: cannot receive data \n", "TCPClient");
    } else {
      printf("%s\n", msg);
    }
  }

  shutdown(socket_file_descriptor_, SHUT_RDWR);
  close(socket_file_descriptor_);

  return 1;
}
