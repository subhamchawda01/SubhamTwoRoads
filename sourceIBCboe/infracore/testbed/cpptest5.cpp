/**
    \file testbed/cpptest5.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include <iostream>
#include "dvccode/CDef/ors_messages.hpp"
#include "infracore/MDSMessages/mds_messages.hpp"

bool operator<(const timeval& t1, const timeval& t2) {
  return (t1.tv_sec < t2.tv_sec) || ((t1.tv_sec == t2.tv_sec) && (t1.tv_usec < t2.tv_usec));
}

int main() {
  std::cout << " " << sizeof(HFSAT::GenericORSReplyStruct) << " " << sizeof(HFSAT::GenericORSRequestStruct) << " "
            << sizeof(HFSAT::GenericMDSPriceLevelStruct) << " " << sizeof(timeval) << " " << sizeof(HFSAT::ttime_t)
            << " " << sizeof(unsigned long) << " " << sizeof(unsigned long long) << " " << sizeof(int) << " "
            << std::endl;  // 72 48 72 16 8 8 8 4

  HFSAT::GenericORSRequestStruct x;
  HFSAT::GenericORSReplyStruct y;
  HFSAT::GenericMDSPriceLevelStruct z;

  HFSAT::ttime_t tv_ttt = HFSAT::GetTimeOfDay();
  timeval tv_;
  gettimeofday(&tv_, NULL);

  std::cout << tv_ttt.tv_sec << "." << tv_ttt.tv_usec << std::endl;
  std::cout << tv_.tv_sec << "." << tv_.tv_usec << std::endl;
  timeval tv2_;
  gettimeofday(&tv2_, NULL);
  if (tv_ < tv2_) {
    std::cout << tv_ttt.tv_sec << "." << tv_ttt.tv_usec << std::endl;
    std::cout << tv_.tv_sec << "." << tv_.tv_usec << std::endl;
  }

  for (int i = 0; i < 10000; i++) {
    HFSAT::ttime_t prev = tv_ttt;
    tv_ttt.tv_usec += 1000;
    if (tv_ttt.tv_usec > 1000000) {
      tv_ttt.tv_sec++;
      tv_ttt.tv_usec -= 1000000;
    }
    if (tv_ttt < prev) {
      std::cout << " wring " << std::endl;
    }
    if (tv_ttt.val < prev.val) {
      std::cout << " wrong " << std::endl;
    }
  }

  return 0;
}
