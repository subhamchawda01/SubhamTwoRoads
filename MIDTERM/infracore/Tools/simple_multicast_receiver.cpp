
#include <vector>
#include <iostream>
#include <signal.h>
#include <math.h>
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "infracore/Tools/simple_multicast_test_data.hpp"

int no_packets_recv_ = 0;
std::vector<HFSAT::ttime_t> times_vec_;

void termination_handler(int signal_num_) {
  int sum_ = 0;
  int max_ = -1, min_ = 65553;
  no_packets_recv_ = 0;

  for (unsigned int i = 0; i < times_vec_.size(); ++i) {
    if (times_vec_[i].tv_sec == 0 && times_vec_[i].tv_usec > 0) {
      sum_ += times_vec_[i].tv_usec;
      ++no_packets_recv_;

      max_ = (max_ > times_vec_[i].tv_usec) ? max_ : times_vec_[i].tv_usec;
      min_ = (min_ < times_vec_[i].tv_usec) ? min_ : times_vec_[i].tv_usec;
    }
  }

  double avg_ = sum_ / (double)no_packets_recv_;

  double std_dev_ = 0.0;
  for (unsigned int i = 0; i < times_vec_.size(); ++i) {
    if (times_vec_[i].tv_sec == 0 && times_vec_[i].tv_usec > 0) {
      std_dev_ += ((avg_ - times_vec_[i].tv_usec) * (avg_ - times_vec_[i].tv_usec));
    }
  }

  std_dev_ /= no_packets_recv_;

  std_dev_ = sqrt(std_dev_);

  // std::cout << "TimeStamp  No_Of_Packets  Min  Max  Avg  StdDev" << std::endl;
  std::cout << HFSAT::GetTimeOfDay() << " " << no_packets_recv_ << " " << (min_ / 1000.0) << " " << (max_ / 1000.0)
            << " " << (avg_ / 1000.0) << " " << (std_dev_ / 1000.0) << std::endl;

  exit(0);
}

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << " Usage: " << argv[0] << "  <interface>  <mcast-ip>  <mport>" << std::endl;
    exit(0);
  }

  signal(SIGINT, termination_handler);

  std::string interface_ = argv[1];
  std::string mcast_ip_ = argv[2];
  int mcast_port_ = atoi(argv[3]);

  HFSAT::MulticastReceiverSocket abc_(mcast_ip_, mcast_port_, interface_);
  char* u_buf = new char[1550];
  while (1) {
    abc_.ReadN(sizeof(TestOrderDatagram), u_buf);

    TestOrderDatagram* test_order_dgram = reinterpret_cast<TestOrderDatagram*>(u_buf);

    HFSAT::ttime_t difftime = HFSAT::GetTimeOfDay() - test_order_dgram->time_;

    std::cout << " Received seq: " << test_order_dgram->seq_ << " after : " << difftime << " on " << interface_ << " "
              << mcast_ip_ << ":" << mcast_port_ << std::endl;

    ++no_packets_recv_;

    times_vec_.push_back(difftime);
  }
}
