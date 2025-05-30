#ifndef _RUNTIME_PROFILER_
#define _RUNTIME_PROFILER_

#include <stdint.h>
#include <fstream>
#include <sstream>
#include <vector>

namespace HFSAT {

#define MAX_SIZE 16000

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

class IndividualStats {
 public:
  IndividualStats() : min(10000000), max(0), mean(0), median(0), p_95(0), p_99(0) {}
  std::string ToString() const;
  double min, max, mean, median, p_95, p_99;
};

class T2TStatsStruct {
 public:
  T2TStatsStruct() : sample_size(0), last_time(0), list_stats(6) {}

  int sample_size;
  time_t last_time;
  std::vector<IndividualStats> list_stats;
};

class RuntimeProfiler {
 public:
  static RuntimeProfiler& GetUniqueInstance(const ProfilerType type);
  static void ResetUniqueInstance(const ProfilerType type);

  RuntimeProfiler(ProfilerType type);
  ~RuntimeProfiler();

  void Start(const ProfilerTimeInfo& time_info);
  void End();

  const ProfilerTimeInfo& GetTimeInfo() const;
  const std::string GetStats();
  const T2TStatsStruct GetStatsStruct();

  void SetUseCPUCycles(bool use_cpu_cycles) { use_cpu_cycles_ = use_cpu_cycles; }
  void DisableTradeinitProfiling();

  static double GetCPUFreq();

 private:
  static uint64_t GetCpuCycleCount();
  uint64_t GetCurrentTime();
  std::string ComputeStats(double cpu_feq, std::vector<uint64_t>& cpu_cycles);
  void ComputeStats(double cpu_feq, std::vector<uint64_t>& cpu_cycles, IndividualStats& stats);

  int index_;
  std::vector<ProfilerTimeInfo> time_info_;
  bool use_cpu_cycles_;
  bool is_disabled_;
  bool is_tradeinit_profiling_disbaled_;
  ProfilerTimeInfo cached_time_info_;
  ProfilerType type_;
  double cpu_freq_;

  static std::vector<RuntimeProfiler*> runtime_profilers;
};
}

#endif
