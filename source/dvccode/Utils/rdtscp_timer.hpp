// =====================================================================================
//
//       Filename:  rdtsc_timer.hpp
//
//    Description:  The class IS NOT SAFE FOR MULTI-THREAD USAGE
//
//        Version:  1.0
//        Created:  10/03/2017 06:30:49 PM
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

#pragma once

#include <cstdlib>
#include <sys/time.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sys/syscall.h>
#include <sys/types.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <limits>
#include <iomanip>

#define CLOCK_FREQ 3312.058858592
#define CLOCK_SYNC_COUNT 1500000

namespace HFSAT {

static __inline__ uint64_t GetCpucycleCountForTimeTick(void) {
  unsigned int lo, hi;
  asm volatile("rdtscp"
               : "=a"(lo), "=d"(hi) /* outputs */
               : "a"(0)             /* inputs */
               : "%ebx", "%ecx");   /* clobbers*/
  return ((uint64_t)lo) | (((uint64_t)hi) << 32);
}

class ClockSource {
 private:
  struct timeval current_time;
  struct timeval last_system_time;
  struct timeval last_simulated_time;
  uint64_t last_cpu_cycle_count;
  uint64_t current_cpu_cycle_count;
  double const_value;
  double time_value;
  double last_time_value;
  double clock_freq;
  uint32_t sample_size;
  bool using_simulated_clocksource;
  uint32_t clock_sync_count;

  ClockSource()
      : current_time(),
        last_system_time(),
        last_simulated_time(),
        last_cpu_cycle_count(0),
        current_cpu_cycle_count(0),
        const_value(0),
        time_value(0),
        clock_freq(CLOCK_FREQ),
        sample_size(0),
        using_simulated_clocksource(false),
        clock_sync_count(CLOCK_SYNC_COUNT)

  {
    std::string simulated_clock_source_filename =
        PROD_CONFIGS_DIR + HFSAT::GetCurrentHostName() + "_overclocked_tsc_freq.txt";

    if (HFSAT::FileUtils::ExistsAndReadable(simulated_clock_source_filename)) {
      std::ifstream simulated_clock_source_file;
      simulated_clock_source_file.open(simulated_clock_source_filename.c_str());

      char buffer[256];
      while (simulated_clock_source_file.good()) {
        simulated_clock_source_file.getline(buffer, 256);
        HFSAT::PerishableStringTokenizer pst(buffer, 256);
        std::vector<const char*> const& tokens = pst.GetTokens();

        if (tokens.size() < 2) continue;

        if (std::string(tokens[0]) == std::string("SIMULATED")) {
          using_simulated_clocksource = true;
          clock_freq = atof(tokens[1]);
          std::cout << "USING SIMULATED CLOCK SOURCE WITH FREQ : " << std::fixed << std::setprecision(6) << clock_freq
                    << std::endl;

          if (tokens.size() >= 3) {
            clock_sync_count = atoi(tokens[2]);
            std::cout << "Using ClockSyncCount from Config : " << clock_sync_count << std::endl;
          }
        }
      }

      simulated_clock_source_file.close();
    }
  }

  ClockSource(ClockSource const& disabled_copy_constructor) = delete;

 public:
  static ClockSource& GetUniqueInstance() {
    static ClockSource unique_instance;
    return unique_instance;
  }

  bool AreWeUsingSimulatedClockSource() { return using_simulated_clocksource; }

  inline struct timeval GetTimeOfDay() {
    if (true == using_simulated_clocksource) {
      if (0 == last_cpu_cycle_count) {
        last_cpu_cycle_count = GetCpucycleCountForTimeTick();
        gettimeofday(&current_time, NULL);
        last_system_time = last_simulated_time = current_time;
        const_value = ((current_time.tv_sec * 1000000) + (current_time.tv_usec)) * 1.0;

      } else {
        current_cpu_cycle_count = GetCpucycleCountForTimeTick();
        time_value += (double)((current_cpu_cycle_count - last_cpu_cycle_count) / (clock_freq));
        last_cpu_cycle_count = current_cpu_cycle_count;
        current_time.tv_sec = (uint64_t)((const_value + time_value) / (double)1000000) + 0.5;
        current_time.tv_usec = ((uint64_t)(const_value + time_value + 0.5) % 1000000);

        sample_size++;

        if (sample_size == clock_sync_count) {
          struct timeval now_time;
          gettimeofday(&now_time, NULL);

          uint64_t system_diff = (now_time.tv_sec * 1000000 + now_time.tv_usec) -
                                 (last_system_time.tv_sec * 1000000 + last_system_time.tv_usec);
          uint64_t simulated_diff = (current_time.tv_sec * 1000000 + current_time.tv_usec) -
                                    (last_simulated_time.tv_sec * 1000000 + last_simulated_time.tv_usec);

          std::cout << " RESET CLOCK FREQ : " << system_diff << " " << simulated_diff << " " << std::fixed
                    << std::setprecision(6) << ((simulated_diff * clock_freq) / system_diff) << std::endl;
          clock_freq = (simulated_diff * clock_freq) / system_diff;
          last_system_time = now_time;
          last_simulated_time = current_time;

          sample_size = 0;
        }
      }

    } else {
      gettimeofday(&current_time, NULL);
    }

    return current_time;
  }
};
}
