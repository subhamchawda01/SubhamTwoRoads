/**
    \file testbed/cpptest10.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
// compile with g++ cpptest10.cpp -I/apps/boost/include -L/apps/boost/lib -lboost_thread -lpthread -static

#include <iostream>
#include <vector>
#include <map>
#include <tr1/unordered_map>
#include <boost/unordered_map.hpp>
#include <algorithm>

#define LEN 10240

typedef unsigned long long ticks;

static __inline__ ticks GetTicks(void) {
  unsigned a, d;
  asm volatile("rdtsc" : "=a"(a), "=d"(d));
  return ((ticks)a) | (((ticks)d) << 32);
}

int main(int argc, char** argv) {
  int retval = -1;
  std::map<int, int, std::greater<int> > __A__map_;
  std::tr1::unordered_map<int, int> __B__map_;
  boost::unordered_map<int, int> __C__map_;

  sleep(1);

  ticks p1 = GetTicks();

  for (unsigned int i = 0; i < LEN; i++) {
    __A__map_[i] = i;
  }

  for (unsigned int i = 0; i < LEN; i += 2) {
    retval += __A__map_[i];
  }

  // 3130 cycles

  ticks p2 = GetTicks();

  for (unsigned int i = 0; i < LEN; i++) {
    __B__map_[i] = i;
  }

  for (unsigned int i = 0; i < LEN; i += 2) {
    retval += __B__map_[i];
  }

  // 794 cycles
  ticks p3 = GetTicks();

  for (unsigned int i = 0; i < LEN; i++) {
    __C__map_[i] = i;
  }

  for (unsigned int i = 0; i < LEN; i += 2) {
    retval += __C__map_[i];
  }

  // 1047 cycles
  ticks p4 = GetTicks();

  std::cout << (p2 - p1) / LEN << " " << (p3 - p2) / LEN << " " << (p4 - p3) / LEN << " "
            << ((double)(p2 - p1) / (double)(p3 - p2)) << " " << ((double)(p2 - p1) / (double)(p4 - p3)) << std::endl;
  return 0;
}
