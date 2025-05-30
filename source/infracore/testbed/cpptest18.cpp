/**
    \file testbed/cpptest18.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include <stdio.h>
#include <iostream>
#include <ctime>
#include <time.h>

#include "dvccode/CommonDataStructures/simple_mempool.hpp"
#include "infracore/OrderRouting/base_order.hpp"

#define LEN 10240

typedef unsigned long long ticks;
static __inline__ ticks GetTicks(void) {
  unsigned a, d;
  // asm volatile("cpuid":"=a"(a),"=d"(d):"0"(0):"ecx","ebx");
  asm volatile("rdtsc" : "=a"(a), "=d"(d));
  return ((ticks)a) | (((ticks)d) << 32);
}

int main() {
  HFSAT::SimpleMempool<HFSAT::BaseOrder> baseorder_mempool_(512);

  std::vector<HFSAT::BaseOrder*> temp_vec_;

  std::vector<HFSAT::BaseOrder*> samplevec_;

// for ( unsigned int i = 0; i < 100 ; i ++ ) {
//   temp_vec_.push_back ( baseorder_mempool_.Alloc ( ) );
// }

// for ( unsigned int i = 3; i < 100 ; i ++ ) {
//   baseorder_mempool_.DeAlloc ( temp_vec_ [ i ] ) ;
// }

#define SAMPLEVECLEN 10
  for (unsigned int i = 0; i < SAMPLEVECLEN; i++) {
    samplevec_.push_back(baseorder_mempool_.Alloc());
  }

#define BIGNUM 1024000
  ticks p1 = GetTicks();
  for (unsigned int j = 0; j < BIGNUM; j++) {
#ifdef BIGNUM
    for (size_t i = 0; i < samplevec_.size(); i++) {
      samplevec_[i]->size_remaining_ = samplevec_[i]->size_requested_ + 1;
      samplevec_[i]->size_requested_ = samplevec_[i]->queue_size_behind_ - 1;
    }
#else
    for (std::vector<HFSAT::BaseOrder*>::iterator oviter = samplevec_.begin(); oviter != samplevec_.end(); oviter++) {
      (*oviter)->size_remaining_ = (*oviter)->size_requested_ + 1;
      (*oviter)->size_requested_ = (*oviter)->queue_size_behind_ - 1;
    }
#endif
  }
  ticks p2 = GetTicks();
#undef BIGNUM

  std::cout << (p2 - p1) << std::endl;

  return 0;
}
