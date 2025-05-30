
#ifndef _CacheProfiler_hpp_
#define _CacheProfiler_hpp_

#include <iostream>
#include <cstring>
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include "dvccode/Papi/include/papi_test.hpp"
#define MAX_HW_COUNTERS 4

class ProfilerTypes {
 public:
  enum HwEvent {
    L1I_L1D_CACHE_PROFILE = 30,
    L2_L3_CACHE_PROFILE = 31,
  };
};

class CacheProfiler {
 public:
  static CacheProfiler& GetUniqueInstance(const ProfilerTypes::HwEvent& profiler_type) {
    static CacheProfiler instance(profiler_type);
    return instance;
  }
  ~CacheProfiler();

  void StartProfiler();
  void EndProfiler();
  void ReadProfilerStats();
  void ResetProfilerStats();
  std::string GetProfilerStats();
  std::string GetCacheMissRatio();

 protected:
  bool AddPapiEvent(int event);
  bool RemovePapiEvent(int event);
  int GetNumEvents();

  void AddL1InstructionAndDataEvents();
  void AddL2L3Events();

 private:
  CacheProfiler(const ProfilerTypes::HwEvent& profiler_type);
  int _event_set;
  long long _refvalue[MAX_HW_COUNTERS];
  const ProfilerTypes::HwEvent& _profiler_type;
  bool PapiInit();
};

#endif
