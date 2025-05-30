#include "dvccode/Utils/source_specific_multicast_receiver_socket.hpp"
#include <iostream>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
  if (argv < 5) {
    std::cout << "usage: " << argv[0] << " source-ip listen-ip listen-port iface\n";
    return 0;
  }

  std::string srcip = argv[1];
  std::string listenip = argv[2];
  int port = atoi(argv[3]);
  std::string iface = argv[4];

  HFSAT::SourceSpecificMulticastReceiverSocket* s =
      new HFSAT::SourceSpecificMulticastReceiverSocket(srcip, listenip, port, iface);
  char buf[2048];
  for (int i = 0; i < 1000; ++i) {
    int len = s->ReadN(2048, buf);
    std::cout << len << "\n";
  }

  return 0;
}
