// =====================================================================================
//
//       Filename:  timetest.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  08/02/2018 06:36:57 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include <iostream>
#include <cstdlib>
#include <inttypes.h>
#include <sys/time.h>
#include "dvccode/CDef/defines.hpp"
#include "dvccode/Utils/CPUAffinity.hpp"
#include "dvccode/Utils/rdtscp_timer.hpp"

static __inline__ uint64_t GetCpucycleCountForTimeTick(void) {
  unsigned int lo, hi;
  asm volatile("rdtscp"
               : "=a"(lo), "=d"(hi) /* outputs */
               : "a"(0)             /* inputs */
               : "%ebx", "%ecx");   /* clobbers*/
  return ((uint64_t)lo) | (((uint64_t)hi) << 32);
}

int main(int argc, char *argv[]) {
  int32_t pid = getpid();
  int32_t value = CPUManager::setAffinity(9, pid);

  std::cout << " affin : " << value << std::endl;

  // HFSAT::ClockSource &clocksource = HFSAT::ClockSource::GetUniqueInstance();

  uint64_t total_value = 0;
  struct timeval temp;
  uint64_t cpu_cycle_count = GetCpucycleCountForTimeTick();

  for (int32_t count = 0; count < 1000000; count++) {
    //    temp = clocksource.GetTimeOfDay();

    gettimeofday(&temp, NULL);
    total_value += temp.tv_sec;
  }

  uint64_t cpu_cycle_count_end = GetCpucycleCountForTimeTick();

  std::cout << " CYCLES: " << cpu_cycle_count_end - cpu_cycle_count << " VAL : " << total_value << std::endl;

  return EXIT_SUCCESS;
}  // ----------  end of function main  ----------
