#include <time.h>
#include <iostream>
#include "dvccode/Utils/tcp_server_socket.hpp"

int main(int argc, char** argv) {
  int tcp_port_;

  if (argc < 2) {
    std::cerr << " Usage: " << argv[0] << " <port> " << std::endl;
    exit(0);
  }

  tcp_port_ = atoi(argv[1]);

  HFSAT::TCPServerSocket abc_(tcp_port_);

  int nfd_ = abc_.Accept();
  char buf[124] = {0};
  int i = 0;

  int read_size = abc_.ReadN(124, buf);
  std::cout << buf << std::endl;
  sleep(60);
  buf[0] = 'M';
  abc_.WriteN(124, buf);
}
