/**
    \file testbed/cpptest53.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include <math.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <vector>

typedef unsigned long cyclecount_t;

class CpucycleProfiler {
 public:
  /** @brief return the curent cpu cyclecount */
  static __inline__ cyclecount_t GetCpucycleCount(void) {
    uint32_t lo, hi;
    __asm__ __volatile__(  // serialize
        "xorl %%eax,%%eax \n        cpuid" ::
            : "%rax", "%rbx", "%rcx", "%rdx");
    /* We cannot use "=A", since this would use %rax on x86_64 and return only the lower 32bits of the TSC */
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return (cyclecount_t)hi << 32 | lo;
  }

  /// \brief returns the difference in cyclecount_t
  static __inline__ cyclecount_t Diff(cyclecount_t t1, cyclecount_t t0) { return (t1 - t0); }

  /// \brief returns the difference in cyclecount_t in Kilo ( or multiples of 2^10 = 1024 )
  static __inline__ cyclecount_t KDiff(cyclecount_t t1, cyclecount_t t0) { return (t1 - t0) >> 10; }

  /// \brief returns the difference in cyclecount_t in Mega ( or multiples of 2^20 = 1024*1024 )
  static __inline__ cyclecount_t MDiff(cyclecount_t t1, cyclecount_t t0) { return (t1 - t0) >> 20; }

  static __inline__ cyclecount_t ConvertCycleCountToUsec(cyclecount_t t) { return (t >> 10) / 3; }
};

int main(int argc, char** argv) {
  double arbit_retval_ = 0.0;
  size_t num_trips_ = 100000000;
  struct timeval start_loop_time_;
  gettimeofday(&start_loop_time_, NULL);

  int counted_msecs_ = 0;

  cyclecount_t start_cycle_count_ = CpucycleProfiler::GetCpucycleCount();
  cyclecount_t last_cycle_count_ = start_cycle_count_;
  for (size_t i = 0; i < num_trips_; i++) {
    if (i % 117 == 0) {
      arbit_retval_ += pow((double)i, 0.1);
    }

    cyclecount_t current_cycle_count_ = CpucycleProfiler::GetCpucycleCount();
    if (current_cycle_count_ - last_cycle_count_ >= 9000000) {
      counted_msecs_++;
      last_cycle_count_ = current_cycle_count_;
    }
  }

  cyclecount_t end_cycle_count_ = CpucycleProfiler::GetCpucycleCount();
  struct timeval end_loop_time_;
  gettimeofday(&end_loop_time_, NULL);
  printf("Arbit Retval %f timediff_usec %ld cyclediff %ld cycletimediff %ld microseconds counted_msecs_ = %d\n",
         arbit_retval_, ((end_loop_time_.tv_sec - start_loop_time_.tv_sec) * 1000000) +
                            (end_loop_time_.tv_usec - start_loop_time_.tv_usec),
         CpucycleProfiler::Diff(end_cycle_count_, start_cycle_count_),
         CpucycleProfiler::ConvertCycleCountToUsec(end_cycle_count_ - start_cycle_count_), counted_msecs_);
}
