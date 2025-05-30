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

#define MAX_MSG 100

int main(int argc, char *argv[]) {
  int serv_port_(15111);

  int socket_file_descriptor_ = -1;
  struct sockaddr_in mcast_Addr;

  if (argc < 2) {
    printf("usage : %s <serv_port> \n", argv[0]);
    exit(1);
  } else {
    serv_port_ = atoi(argv[1]);
  }

  socket_file_descriptor_ = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_file_descriptor_ < 0) {
    printf("%s cannot open socket \n", "TCPServer");
    exit(1);
  }

  bzero(&mcast_Addr, sizeof(mcast_Addr));
  mcast_Addr.sin_family = AF_INET;
  mcast_Addr.sin_port = htons(serv_port_);
  mcast_Addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(socket_file_descriptor_, (struct sockaddr *)&mcast_Addr, sizeof(struct sockaddr_in)) < 0) {
    printf("%s cannot bind port number %d \n", "TCPServer", serv_port_);
    shutdown(socket_file_descriptor_, SHUT_RDWR);
    close(socket_file_descriptor_);
    exit(1);
  }

  if (listen(socket_file_descriptor_, 1024) < 0) {
    fprintf(stderr, "%s: Error calling listen()\n", "TCPServer");
    shutdown(socket_file_descriptor_, SHUT_RDWR);
    close(socket_file_descriptor_);
    exit(EXIT_FAILURE);
  }

  int connected_socket_file_descriptor_ = -1;
  if ((connected_socket_file_descriptor_ = accept(socket_file_descriptor_, NULL, NULL)) < 0) {
    fprintf(stderr, "%s Error calling accept()\n", "TCPServer");
    shutdown(socket_file_descriptor_, SHUT_RDWR);
    close(socket_file_descriptor_);
    exit(1);
  }

  char msg[MAX_MSG];
  bzero(msg, MAX_MSG);

  while (1) {
    int char_received_count_ = read(connected_socket_file_descriptor_, msg, MAX_MSG);
    if (char_received_count_ < 0) {
      printf("%s: cannot receive data \n", "TCPServer");
    } else {
      printf("%s\n", msg);
      write(connected_socket_file_descriptor_, msg, strlen(msg));
    }
  }

  shutdown(connected_socket_file_descriptor_, SHUT_RDWR);
  close(connected_socket_file_descriptor_);

  shutdown(socket_file_descriptor_, SHUT_RDWR);
  close(socket_file_descriptor_);
  return 0;
}
