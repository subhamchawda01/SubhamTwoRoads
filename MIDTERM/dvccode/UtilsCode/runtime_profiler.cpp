#include "dvccode/Utils/runtime_profiler.hpp"

#include <iostream>
#include <iomanip>
#include <sys/time.h>

#include <algorithm>
#include <fstream>
#include <set>
#include <stdlib.h>
#include <string>

#include "dvccode/cpp11-range/range.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

namespace HFSAT {

std::vector<RuntimeProfiler*> RuntimeProfiler::runtime_profilers;

RuntimeProfiler& RuntimeProfiler::GetUniqueInstance(const ProfilerType type) {
  if (runtime_profilers.empty()) {
    for (int i = 0; i < ProfilerType::kSIZE; i++) {
      runtime_profilers.push_back(nullptr);
    }
  }
  if (runtime_profilers[type] == nullptr) {
    runtime_profilers[type] = new RuntimeProfiler(type);
  }
  return (*runtime_profilers[type]);
}

void RuntimeProfiler::ResetUniqueInstance(const ProfilerType type) {
  if (runtime_profilers.size() > type && runtime_profilers[type] != nullptr) {
    delete runtime_profilers[type];
    runtime_profilers[type] = nullptr;
  }
}

RuntimeProfiler::RuntimeProfiler(ProfilerType type)
    : index_(-1),
      time_info_(MAX_SIZE + 15),
      use_cpu_cycles_(true),
      is_disabled_(false),
      is_tradeinit_profiling_disbaled_(false),
      type_(type),
      cpu_freq_(0) {
  cached_time_info_.ors_start_time = 0;
  cached_time_info_.cshmw_end_time = 0;
  cached_time_info_.tradeinit_start_time = 0;
  cached_time_info_.tradeinit_end_time = 0;
  cached_time_info_.ors_start_time = 0;
  cached_time_info_.ors_end_time = 0;

  cpu_freq_ = GetCPUFreq();
}

RuntimeProfiler::~RuntimeProfiler() {}

void RuntimeProfiler::Start(const ProfilerTimeInfo& time_info) {
  if (is_disabled_ || (is_tradeinit_profiling_disbaled_ && type_ == kTRADEINIT)) {
    return;
  }

  cached_time_info_ = time_info;
  switch (type_) {
    case kCMEILINKORS:
      cached_time_info_.ors_start_time = GetCurrentTime();
      cached_time_info_.ors_end_time = 0;
      break;
    case kCONSOLETRADER:
      cached_time_info_.tradeinit_start_time = GetCurrentTime();
      cached_time_info_.tradeinit_end_time = 0;
      cached_time_info_.ors_start_time = 0;
      cached_time_info_.ors_end_time = 0;
      break;
    case kTRADEINIT:
      cached_time_info_.tradeinit_start_time = GetCurrentTime();
      cached_time_info_.tradeinit_end_time = 0;
      cached_time_info_.ors_start_time = 0;
      cached_time_info_.ors_end_time = 0;
      break;
    case kCOMBINEDSHMWRITER:
      cached_time_info_.cshmw_start_time = GetCurrentTime();
      cached_time_info_.cshmw_end_time = 0;
      cached_time_info_.tradeinit_start_time = 0;
      cached_time_info_.tradeinit_end_time = 0;
      cached_time_info_.ors_start_time = 0;
      cached_time_info_.ors_end_time = 0;
      break;
    default:
      std::cerr << "Invalid RuntimeProfilerType\n";
  }
}

void RuntimeProfiler::End() {
  if (is_disabled_) {
    return;
  }

  switch (type_) {
    case kCMEILINKORS:
      cached_time_info_.ors_end_time = GetCurrentTime();
      break;
    case kCONSOLETRADER:
      cached_time_info_.tradeinit_end_time = GetCurrentTime();
      break;
    case kTRADEINIT:
      if (is_tradeinit_profiling_disbaled_) {
        HFSAT::RuntimeProfiler::GetUniqueInstance(HFSAT::ProfilerType::kCONSOLETRADER).End();
      } else {
        cached_time_info_.tradeinit_end_time = GetCurrentTime();
      }
      break;
    case kCOMBINEDSHMWRITER:
      cached_time_info_.cshmw_end_time = GetCurrentTime();
      break;
    default:
      std::cerr << "Invalid RuntimeProfilerType\n";
  }
  if (type_ == kCMEILINKORS) {
    if (index_ >= 0 && index_ < MAX_SIZE - 1) {
      if ((time_info_[index_].cshmw_start_time < cached_time_info_.cshmw_start_time) &&
          (cached_time_info_.cshmw_start_time < cached_time_info_.cshmw_end_time) &&
          (time_info_[index_].tradeinit_start_time < cached_time_info_.tradeinit_start_time) &&
          (cached_time_info_.tradeinit_start_time < cached_time_info_.tradeinit_end_time) &&
          (time_info_[index_].ors_start_time < cached_time_info_.ors_start_time) &&
          (cached_time_info_.ors_start_time < cached_time_info_.ors_end_time)) {
        index_++;
        time_info_[index_] = cached_time_info_;
      }
    } else if (index_ == -1) {
      if ((cached_time_info_.cshmw_start_time < cached_time_info_.cshmw_end_time) &&
          (cached_time_info_.tradeinit_start_time < cached_time_info_.tradeinit_end_time) &&
          (cached_time_info_.ors_start_time < cached_time_info_.ors_end_time)) {
        index_ = 0;
        time_info_[index_] = cached_time_info_;
      }
    }

    if (index_ == MAX_SIZE - 1) {
      is_disabled_ = true;
      timeval tv;
      gettimeofday(&tv, NULL);
      std::cout << "T2T Last Packet Time : " << tv.tv_sec << std::endl;
    }
  }
}

const ProfilerTimeInfo& RuntimeProfiler::GetTimeInfo() const {
  if (is_tradeinit_profiling_disbaled_ && type_ == kTRADEINIT) {
    return HFSAT::RuntimeProfiler::GetUniqueInstance(HFSAT::ProfilerType::kCONSOLETRADER).GetTimeInfo();
  }
  return cached_time_info_;
}

const std::string RuntimeProfiler::GetStats() {
  if (type_ != kCMEILINKORS) {
    return "No Stats Available\n";
  }
  std::ostringstream temp_oss;
  try {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    std::set<int> ignore_index;

    // Try
    // for ( int i : util::lang::range ( 0, index_ ) ) {
    for (int i = 0; i <= index_; i++) {
      if (time_info_[i].cshmw_start_time == 0 || time_info_[i].cshmw_end_time == 0 ||
          time_info_[i].tradeinit_start_time == 0 || time_info_[i].tradeinit_end_time == 0 ||
          time_info_[i].ors_start_time == 0 || time_info_[i].ors_end_time == 0) {
        ignore_index.insert(i);
      }
    }

    std::vector<uint64_t> time_taken(index_ + 1 - ignore_index.size());
    for (int i = 0; i <= index_; i++) {
      if (ignore_index.find(i) != ignore_index.end()) {
        continue;
      }
      time_taken[i] = time_info_[i].cshmw_end_time - time_info_[i].cshmw_start_time;
    }

    temp_oss << index_ + 1 - ignore_index.size() << " " << tv.tv_sec
             << " CSHMW: " << ComputeStats(cpu_freq_, time_taken) << " ";

    for (int i = 0; i <= index_; i++) {
      if (ignore_index.find(i) != ignore_index.end()) {
        continue;
      }
      time_taken[i] = time_info_[i].tradeinit_start_time - time_info_[i].cshmw_end_time;
    }
    temp_oss << "SHM1: " << ComputeStats(cpu_freq_, time_taken) << " ";

    for (int i = 0; i <= index_; i++) {
      if (ignore_index.find(i) != ignore_index.end()) {
        continue;
      }
      time_taken[i] = time_info_[i].tradeinit_end_time - time_info_[i].tradeinit_start_time;
    }

    temp_oss << "Query: " << ComputeStats(cpu_freq_, time_taken) << " ";

    for (int i = 0; i <= index_; i++) {
      if (ignore_index.find(i) != ignore_index.end()) {
        continue;
      }
      time_taken[i] = time_info_[i].ors_start_time - time_info_[i].tradeinit_end_time;
    }
    temp_oss << "SHM2: " << ComputeStats(cpu_freq_, time_taken) << " ";

    for (int i = 0; i <= index_; i++) {
      if (ignore_index.find(i) != ignore_index.end()) {
        continue;
      }
      time_taken[i] = time_info_[i].ors_end_time - time_info_[i].ors_start_time;
    }

    temp_oss << "ORS: " << ComputeStats(cpu_freq_, time_taken) << " ";

    for (int i = 0; i <= index_; i++) {
      if (ignore_index.find(i) != ignore_index.end()) {
        continue;
      }
      time_taken[i] = time_info_[i].ors_end_time - time_info_[i].cshmw_start_time;
    }
    temp_oss << "T2T: " << ComputeStats(cpu_freq_, time_taken) << "\n";

  } catch (...) {
    temp_oss << "Exception while Stats Computation\n";
  }

  return temp_oss.str();
}

const T2TStatsStruct RuntimeProfiler::GetStatsStruct() {
  T2TStatsStruct stats;
  if (type_ != kCMEILINKORS) {
    return stats;
  }
  try {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    std::set<int> ignore_index;
    for (int i = 0; i <= index_; i++) {
      if (time_info_[i].cshmw_start_time == 0 || time_info_[i].cshmw_end_time == 0 ||
          time_info_[i].tradeinit_start_time == 0 || time_info_[i].tradeinit_end_time == 0 ||
          time_info_[i].ors_start_time == 0 || time_info_[i].ors_end_time == 0) {
        ignore_index.insert(i);
      }
    }

    stats.sample_size = index_ + 1 - ignore_index.size();

    std::vector<uint64_t> time_taken(index_ + 1 - ignore_index.size());
    for (int i = 0; i <= index_; i++) {
      if (ignore_index.find(i) != ignore_index.end()) {
        continue;
      }
      time_taken[i] = time_info_[i].cshmw_end_time - time_info_[i].cshmw_start_time;
    }

    stats.last_time = tv.tv_sec;

    ComputeStats(cpu_freq_, time_taken, stats.list_stats[0]);

    for (int i = 0; i <= index_; i++) {
      if (ignore_index.find(i) != ignore_index.end()) {
        continue;
      }
      time_taken[i] = time_info_[i].tradeinit_start_time - time_info_[i].cshmw_end_time;
    }

    ComputeStats(cpu_freq_, time_taken, stats.list_stats[1]);

    for (int i = 0; i <= index_; i++) {
      if (ignore_index.find(i) != ignore_index.end()) {
        continue;
      }
      time_taken[i] = time_info_[i].tradeinit_end_time - time_info_[i].tradeinit_start_time;
    }

    ComputeStats(cpu_freq_, time_taken, stats.list_stats[2]);

    for (int i = 0; i <= index_; i++) {
      if (ignore_index.find(i) != ignore_index.end()) {
        continue;
      }
      time_taken[i] = time_info_[i].ors_start_time - time_info_[i].tradeinit_end_time;
    }

    ComputeStats(cpu_freq_, time_taken, stats.list_stats[3]);

    for (int i = 0; i <= index_; i++) {
      if (ignore_index.find(i) != ignore_index.end()) {
        continue;
      }
      time_taken[i] = time_info_[i].ors_end_time - time_info_[i].ors_start_time;
    }

    ComputeStats(cpu_freq_, time_taken, stats.list_stats[4]);

    for (int i = 0; i <= index_; i++) {
      if (ignore_index.find(i) != ignore_index.end()) {
        continue;
      }
      time_taken[i] = time_info_[i].ors_end_time - time_info_[i].cshmw_start_time;
    }

    ComputeStats(cpu_freq_, time_taken, stats.list_stats[5]);

  } catch (...) {
  }

  return stats;
}

std::string RuntimeProfiler::ComputeStats(double cpu_feq, std::vector<uint64_t>& cpu_cycles) {
  if (cpu_cycles.empty()) {
    return "";
  }
  std::sort(cpu_cycles.begin(), cpu_cycles.end());
  uint32_t size = cpu_cycles.size();

  uint64_t sum = 0;
  uint64_t min = 99999999999999;
  uint64_t max = 0;
  for (uint64_t cycles : cpu_cycles) {
    if (cycles < min) {
      min = cycles;
    }
    if (cycles > max) {
      max = cycles;
    }
  }
  for (uint64_t cycles : cpu_cycles) {
    sum += (cycles - min);
  }

  double mean_cycles = min + (sum / ((double)size));
  double median_cycles = cpu_cycles[size >> 1];

  double percentile_95_cycles = cpu_cycles[uint32_t(0.95 * size)];
  double percentile_99_cycles = cpu_cycles[uint32_t(0.99 * size)];

  std::ostringstream temp_oss;
  temp_oss << std::fixed;
  temp_oss << std::setprecision(2);
  if (cpu_feq != 0.0) {
    temp_oss << min / cpu_feq << " ";
    temp_oss << max / cpu_feq << " ";
    temp_oss << mean_cycles / cpu_feq << " ";
    temp_oss << median_cycles / cpu_feq << " ";
    temp_oss << percentile_95_cycles / cpu_feq << " ";
    temp_oss << percentile_99_cycles / cpu_feq;
  } else {
    temp_oss << "0.0 ";
  }

  return temp_oss.str();
}

void RuntimeProfiler::ComputeStats(double cpu_feq, std::vector<uint64_t>& cpu_cycles, IndividualStats& stats) {
  if (cpu_cycles.empty()) {
    return;
  }
  std::sort(cpu_cycles.begin(), cpu_cycles.end());
  uint32_t size = cpu_cycles.size();

  uint64_t sum = 0;
  uint64_t min = 99999999999999;
  uint64_t max = 0;
  for (uint64_t cycles : cpu_cycles) {
    if (cycles < min) {
      min = cycles;
    }
    if (cycles > max) {
      max = cycles;
    }
  }
  for (uint64_t cycles : cpu_cycles) {
    sum += (cycles - min);
  }

  double mean_cycles = min + (sum / ((double)size));
  double median_cycles = cpu_cycles[size >> 1];

  double percentile_95_cycles = cpu_cycles[uint32_t(0.95 * size)];
  double percentile_99_cycles = cpu_cycles[uint32_t(0.99 * size)];

  if (cpu_feq != 0.0) {
    stats.min = min / cpu_feq;
    stats.max = max / cpu_feq;
    stats.mean = mean_cycles / cpu_feq;
    stats.median = median_cycles / cpu_feq;
    stats.p_95 = percentile_95_cycles / cpu_feq;
    stats.p_99 = percentile_99_cycles / cpu_feq;
  }
}

double RuntimeProfiler::GetCPUFreq() {
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);

  if (!strncmp(hostname, "sdv-bsl-srv", 11)) {
    return 2900;
  } else if (!strncmp(hostname, "sdv-fr2-srv13", 13)) {
    return 3332.338;
  } else if (!strncmp(hostname, "sdv-fr2-srv14", 13)) {
    return 3332.640;
  } else if (!strncmp(hostname, "sdv-fr2-srv15", 13)) {
    return 3100;
  } else if (!strncmp(hostname, "sdv-fr2-srv16", 13)) {
    return 3099.878;
  } else if (!strncmp(hostname, "sdv-ind-srv11", 13)) {
    return 3100.236;
  } else if (!strncmp(hostname, "sdv-ind-srv12", 13)) {
    return 3101;
  } else if (!strncmp(hostname, "sdv-ind-srv13", 13)) {
    return 3200;
  } else if (!strncmp(hostname, "sdv-ind-srv14", 13)) {
    return 2600;
  } else if (!strncmp(hostname, "sdv-ind-srv15", 13)) {
    return 3301.000;
  } else if (!strncmp(hostname, "sdv-ind-srv16", 13)) {
    return 3312.000;
  } else if (!strncmp(hostname, "sdv-cfe-srv11", 13)) {
    return 3299.960;
  } else if (!strncmp(hostname, "sdv-cfe-srv12", 13)) {
    return 3332.776;
  } else if (!strncmp(hostname, "sdv-cfe-srv13", 13)) {
    return 3332.316;
  } else if (!strncmp(hostname, "sdv-crt-srv11", 13)) {
    return 2533.423;
  } else if (!strncmp(hostname, "sdv-chi-srv13", 13)) {
    return 3332.767;
  } else if (!strncmp(hostname, "sdv-chi-srv14", 13)) {
    return 3333.043;
  } else if (!strncmp(hostname, "sdv-chi-srv15", 13)) {
    return 3099.732;
  } else if (!strncmp(hostname, "sdv-chi-srv16", 13)) {
    return 3099.659;
  } else if (!strncmp(hostname, "sdv-tor-srv11", 13)) {
    return 3332.783;
  } else if (!strncmp(hostname, "sdv-tor-srv12", 13)) {
    return 3333.109;
  } else if (!strncmp(hostname, "sdv-tor-srv13", 13)) {
    return 3000.000;
  } else if (!strncmp(hostname, "sdv-bmf-srv11", 13)) {
    return 2600.034;
  } else if (!strncmp(hostname, "sdv-bmf-srv12", 13)) {
    return 2599.954;
  } else if (!strncmp(hostname, "sdv-bmf-srv13", 13)) {
    return 3332.959;
  } else if (!strncmp(hostname, "sdv-bmf-srv14", 13)) {
    return 2899.479;
  } else if (!strncmp(hostname, "sdv-bmf-srv15", 13)) {
    return 2599.929;
  } else if (!strncmp(hostname, "SDV-HK-SRV11", 13)) {
    return 2899.769;
  } else if (!strncmp(hostname, "SDV-HK-SRV12", 13)) {
    return 2899.827;
  } else if (!strncmp(hostname, "sdv-mos-srv11", 13)) {
    return 2900.267;
  } else if (!strncmp(hostname, "sdv-mos-srv12", 13)) {
    return 3099.878;
  } else if (!strncmp(hostname, "sdv-mos-srv13", 13)) {
    return 3196.172;
  } else if (!strncmp(hostname, "SDV-ASX-SRV11", 13)) {
    return 3099.973;
  } else if (!strncmp(hostname, "SDV-ASX-SRV12", 13)) {
    return 2900.057;
  } else if (!strncmp(hostname, "sdv-ose-srv11", 13)) {
    return 3332.376;
  } else if (!strncmp(hostname, "sdv-ose-srv12", 13)) {
    return 3332.053;
  } else if (!strncmp(hostname, "sdv-ose-srv13", 13)) {
    return 3100.149;
  } else if (!strncmp(hostname, "sdv-ose-srv14", 13)) {
    return 3300.240;
  } else if (!strncmp(hostname, "sdv-sgx-srv11", 13)) {
    return 2999.777;
  } else if (!strncmp(hostname, "sdv-sgx-srv12", 13)) {
    return 3000.135;
  }

  const char* cmd = " lscpu | grep 'CPU MHz' | cut -d':' -f2 | tr -d ' ' ";
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
  if (!pipe) return 0;
  char buffer[128];
  std::string result = "";
  while (!feof(pipe.get())) {
    if (fgets(buffer, 128, pipe.get()) != NULL) result += buffer;
  }

  return atof(result.c_str());
}

void RuntimeProfiler::DisableTradeinitProfiling() { is_tradeinit_profiling_disbaled_ = true; }

uint64_t RuntimeProfiler::GetCpuCycleCount(void) {
  unsigned int lo, hi;
  asm volatile("rdtscp"
               : "=a"(lo), "=d"(hi) /* outputs */
               : "a"(0)             /* inputs */
               : "%ebx", "%ecx");   /* clobbers*/
  return ((uint64_t)lo) | (((uint64_t)hi) << 32);
}

uint64_t RuntimeProfiler::GetCurrentTime() {
  if (use_cpu_cycles_) {
    return RuntimeProfiler::GetCpuCycleCount();
  } else {
    timeval tv;
    gettimeofday(&tv, NULL);
    return (((uint64_t)tv.tv_sec) * 1000000 + (uint64_t)tv.tv_usec);
  }
}

std::string IndividualStats::ToString() const {
  std::ostringstream temp_oss;
  temp_oss << std::fixed;
  temp_oss << std::setprecision(2);

  temp_oss << min << " " << max << " " << mean << " " << median << " " << p_95 << " " << p_99 << " ";

  return temp_oss.str();
}
}
