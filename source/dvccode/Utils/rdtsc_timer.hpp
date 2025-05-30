
#pragma once
#include <cstdlib>
#include <ctime>
#include <unistd.h>

namespace HFSAT {
#define NANOSEC_PER_SEC 1000000000L

//! Return the RDTSC (Read TimeStamp Counter) value
static inline uint64_t GetReadTimeStampCounter() {
  uint32_t lo, hi;
  __asm __volatile__("rdtsc" : "=a"(lo), "=d"(hi) : :);
  return lo | (uint64_t)hi << 32;
}

static uint64_t TicksPerSec = 0;
//! Returns an approximate number of cpu cycles in a second
static inline uint64_t GetNoOfCpuCyclesPerSecond() {
  if (TicksPerSec != 0) {
    return TicksPerSec;
  }

  struct timespec begints, endts;
  uint64_t begin = GetReadTimeStampCounter();
  clock_gettime(CLOCK_MONOTONIC, &begints);
  sleep(1);
  clock_gettime(CLOCK_MONOTONIC, &endts);
  uint64_t end = GetReadTimeStampCounter();

  struct timespec diffts;
  diffts.tv_sec = endts.tv_sec - begints.tv_sec;
  diffts.tv_nsec = endts.tv_nsec - begints.tv_nsec;
  if (diffts.tv_nsec < 0) {
    diffts.tv_sec--;
    diffts.tv_nsec += NANOSEC_PER_SEC;
  }

  uint64_t nsec_elapsed = diffts.tv_sec * NANOSEC_PER_SEC + diffts.tv_nsec;
  uint64_t ticks_per_sec = ((double)(end - begin) / (double)nsec_elapsed) * NANOSEC_PER_SEC;
  TicksPerSec = ticks_per_sec;
  return ticks_per_sec;
}

}  // namespace HFSAT
