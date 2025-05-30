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

#include <iostream>
#include <string>
#include <sstream>

#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/Utils/multicast_receiver_socket.hpp"

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

int main(int argc, char** argv) {
  int mode = 0;
  std::string bcast_ip_;
  int bcast_port_;
  int num_iter;

  if (argc < 5) {
    printf("usage : %s <mode 1=sender. any=receiver> <bcast_ip> <bcast_port> <count> <interface?> <sleep_time_usec?>\n",
           argv[0]);
    exit(1);
  } else {
    mode = atoi(argv[1]);
    bcast_ip_ = std::string(argv[2]);
    bcast_port_ = atoi(argv[3]);
    num_iter = atoi(argv[4]);
  }

  std::string interface = "eth5";
  if (argc >= 6) interface = std::string(argv[5]);
  unsigned int sleep_time = 0;
  if (argc >= 7) sleep_time = atoi(argv[6]);

  int cnt = 0;
  if (mode == 1) {
    HFSAT::MulticastSenderSocket* sock = new HFSAT::MulticastSenderSocket(bcast_ip_, bcast_port_, interface);
    CommandStruct cmd_struct_;
    for (unsigned int i = 0; i < num_iter; i++) {
      cmd_struct_.xyz10_ = i;
      sock->WriteN(sizeof(CommandStruct), &cmd_struct_);
      cnt++;
      std::cout << " sent count " << cnt << "\n";
      if (sleep_time > 0) usleep(sleep_time);
    }
    sock->Close();
  } else {
    HFSAT::MulticastReceiverSocket* sock = new HFSAT::MulticastReceiverSocket(bcast_ip_, bcast_port_, interface);
    CommandStruct cmd_struct_;
    while (true) {
      sock->ReadN(sizeof(CommandStruct), &cmd_struct_);
      cnt++;
      std::cout << " received count " << cnt << "\n";
      //          if(cmd_struct_.xyz10_ == num_iter )
      //            break;
    }
    sock->Close();
  }
  return 0;
}
// g++ -o mcast_drop_count -O2 mcast_drop_count.cpp  -I../../infracore_install/ -L../../infracore_install/lib/
