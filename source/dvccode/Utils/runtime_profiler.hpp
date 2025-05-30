#ifndef _RUNTIME_PROFILER_
#define _RUNTIME_PROFILER_

#include <stdint.h>
#include <fstream>
#include <sstream>
#include <vector>

namespace HFSAT {

typedef struct {
  uint64_t cshmw_start_time;
  uint64_t cshmw_end_time;
  uint64_t tradeinit_start_time;
  uint64_t tradeinit_end_time;
  uint64_t ors_start_time;
  uint64_t ors_end_time;
} ProfilerTimeInfo;

enum ProfilerType {
  kCMEILINKORS = 0,
  kTRADEINIT,
  kCONSOLETRADER,
  kCOMBINEDSHMWRITER,
  kSIZE  // This should be the last element
};

class RuntimeProfiler {
 private:
  inline uint64_t GetCpuCycleCount() {
    unsigned int lo, hi;
    asm volatile("rdtscp"
                 : "=a"(lo), "=d"(hi) /* outputs */
                 : "a"(0)             /* inputs */
                 : "%ebx", "%ecx");   /* clobbers*/
    return ((uint64_t)lo) | (((uint64_t)hi) << 32);
  }

  void ResetTimeInfo() {
    time_info_.cshmw_start_time = 0;
    time_info_.cshmw_end_time = 0;
    time_info_.tradeinit_start_time = 0;
    time_info_.tradeinit_end_time = 0;
    time_info_.ors_start_time = 0;
    time_info_.ors_end_time = 0;
  }

  ProfilerTimeInfo time_info_;

  RuntimeProfiler() : time_info_() {
    time_info_.cshmw_start_time = 0;
    time_info_.cshmw_end_time = 0;
    time_info_.tradeinit_start_time = 0;
    time_info_.tradeinit_end_time = 0;
    time_info_.ors_start_time = 0;
    time_info_.ors_end_time = 0;
  }

  RuntimeProfiler(RuntimeProfiler const& disabled_copy_constructor) = delete;

 public:
  static RuntimeProfiler& GetUniqueInstance() {
    static RuntimeProfiler unique_instance;
    return unique_instance;
  }

  inline void Start(ProfilerType type, uint64_t count = -1) {
    switch (type) {
      case kCOMBINEDSHMWRITER: {
        time_info_.cshmw_start_time = (count == (uint64_t)-1) ? GetCpuCycleCount() : count;
      } break;
      case kTRADEINIT:
      case kCONSOLETRADER: {
        time_info_.tradeinit_start_time = (count == (uint64_t)-1) ? GetCpuCycleCount() : count;
      } break;
      case kCMEILINKORS: {
        time_info_.ors_start_time = (count == (uint64_t)-1) ? GetCpuCycleCount() : count;
      } break;
      default: { ResetTimeInfo(); } break;
    }
  }

  inline void End(ProfilerType type, uint64_t count = -1) {
    switch (type) {
      case kCOMBINEDSHMWRITER: {
        time_info_.cshmw_end_time = (count == (uint64_t)-1) ? GetCpuCycleCount() : count;
      } break;
      case kTRADEINIT:
      case kCONSOLETRADER: {
        time_info_.tradeinit_end_time = (count == (uint64_t)-1) ? GetCpuCycleCount() : count;
      } break;
      case kCMEILINKORS: {
        time_info_.ors_end_time = (count == (uint64_t)-1) ? GetCpuCycleCount() : count;
      } break;
      default: { ResetTimeInfo(); } break;
    }
  }

  inline ProfilerTimeInfo& GetProfilerTimeInfo() { return time_info_; }
};
}

#endif
