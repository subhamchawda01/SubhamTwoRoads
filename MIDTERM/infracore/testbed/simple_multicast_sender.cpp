#include <iostream>
#include "dvccode/CDef/ttime.hpp"

#include "dvccode/Utils/multicast_sender_socket.hpp"

int main(int argc, char** argv) {
  if (argc < 5) {
    std::cerr << " Usage: " << argv[0] << "<ip> <port> <no_of_packets> <sleep_time_us> <interface>" << std::endl;
    exit(0);
  }

  std::string mcast_ip_ = argv[1];
  int mcast_port_ = atoi(argv[2]);
  int no_of_packets_ = atoi(argv[3]);
  unsigned long sleep_time_ = atoi(argv[4]);

  int secs_ = sleep_time_ / 1000000;   // Seconds
  int usecs_ = sleep_time_ % 1000000;  // usecs
  std::string interface_ = argv[5];
  HFSAT::MulticastSenderSocket abc_(mcast_ip_, mcast_port_, interface_);

  while (true) {
    HFSAT::ttime_t time_ = HFSAT::GetTimeOfDay();

    int retval_ = abc_.WriteN(sizeof(HFSAT::ttime_t), &time_);

    if (retval_ < sizeof(HFSAT::ttime_t)) {
      fprintf(stdout, "retval_ : %d\n", retval_);
      exit(0);
    } else {
      std::cout << " Sent at : " << time_ << " on " << mcast_ip_ << ":" << mcast_port_ << std::endl;
    }

    for (int _secs_ = 0; _secs_ < secs_; ++_secs_) {
      HFSAT::usleep(999999);
    }
    HFSAT::usleep(usecs_);
  }
}
