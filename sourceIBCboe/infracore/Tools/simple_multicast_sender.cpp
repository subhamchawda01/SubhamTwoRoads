#include <iostream>
#include <string>
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "infracore/Tools/simple_multicast_test_data.hpp"

int main(int argc, char** argv) {
  if (argc < 6) {
    std::cerr << " Usage: " << argv[0] << " <mcast-ip> <mcast-port> <no_of_packets> <sleep_time_us> <interface>"
              << std::endl;
    exit(0);
  }

  std::string mcast_ip_ = argv[1];
  int mcast_port_ = atoi(argv[2]);
  int no_of_packets_ = atoi(argv[3]);
  int sleep_time_ = atoi(argv[4]);
  std::string interface_ = argv[5];

  int secs_ = sleep_time_ / 1000000;   // Seconds
  int usecs_ = sleep_time_ % 1000000;  // usecs

  HFSAT::MulticastSenderSocket mc_sender_sock(mcast_ip_, mcast_port_, interface_);

  TestOrderDatagram test_order_packet;

  while (no_of_packets_-- > 0) {
    test_order_packet.PrepareNextPacket();
    int retval_ = mc_sender_sock.WriteN(sizeof(TestOrderDatagram), &test_order_packet);

    if (retval_ < (int)sizeof(TestOrderDatagram)) {
      fprintf(stdout, "retval_ : %d\n", retval_);
      exit(0);
    } else {
      std::cout << " Sent::" << test_order_packet.ToString() << " on " << mcast_ip_ << ":" << mcast_port_ << std::endl;
    }

    for (int _secs_ = 0; _secs_ < secs_; ++_secs_) {
      HFSAT::usleep(999999);
    }
    HFSAT::usleep(usecs_);
  }
}
