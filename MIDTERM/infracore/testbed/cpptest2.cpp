/**
    \file testbed/cpptest2.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include <iostream>
#include <vector>
#include <algorithm>

#define LEN 10000

typedef unsigned long long ticks;

static __inline__ ticks GetTicks(void) {
  unsigned a, d;
  asm volatile("rdtsc" : "=a"(a), "=d"(d));
  return ((ticks)a) | (((ticks)d) << 32);
}

int main(int argc, char** argv) {
  // int i;
  // ticks t1 = getticks();
  // i=i+2;
  // ticks t2 = getticks();
  // std::cout << t1 << std::endl;
  // std::cout << t2 << std::endl;
  // std::cout << (t2-t1) << std::endl;

  int x[LEN];
  std::vector<int> y(LEN);

  unsigned a1, a2;
  asm volatile("rdtsc" : "=a"(a1));
  for (unsigned int i = 0; i < LEN; i++) {
    x[i] = i;
  }
  asm volatile("rdtsc" : "=a"(a2));

  unsigned b1, b2;
  asm volatile("rdtsc" : "=a"(b1));
  for (unsigned int i = 0; i < LEN; i++) {
    y[i] = i;
  }
  asm volatile("rdtsc" : "=a"(b2));

  std::cout << " diff " << (a2 - a1) << " " << (b2 - b1) << std::endl;
}
