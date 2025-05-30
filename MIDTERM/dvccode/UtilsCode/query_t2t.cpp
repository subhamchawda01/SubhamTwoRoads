#include <fstream>
#include <iostream>
#include <set>
#include <stdlib.h>
#include <string>

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/Utils/query_t2t.hpp"

namespace HFSAT {

QueryT2T* QueryT2T::unique_instance_ = nullptr;

QueryT2T& QueryT2T::GetUniqueInstance() {
  if (unique_instance_ == nullptr) {
    unique_instance_ = new QueryT2T();
  }
  return *(unique_instance_);
}

void QueryT2T::ResetUniqueInstance() {
  if (unique_instance_ != nullptr) {
    delete unique_instance_;
    unique_instance_ = nullptr;
  }
}

QueryT2T::QueryT2T() : allow_stats_(true), allow_overall_stats_(true) {
  std::ostringstream t_temp_oss;
  t_temp_oss << "/spare/local/logs/profilelogs/runtime_profiler_" << HFSAT::DateTime::GetCurrentIsoDateLocal()
             << ".dat";

  outfile_.open(t_temp_oss.str().c_str(), std::ofstream::app);
  outfile_ << std::fixed;
  outfile_ << std::setprecision(2);
}

QueryT2T::~QueryT2T() {
  outfile_.close();

  for (auto iter : saci_to_profiler_) {
    if (iter.second != nullptr) {
      delete iter.second;
      iter.second = nullptr;
    }
  }
}

void QueryT2T::Start(const ProfilerTimeInfo& time_info, int saci) {
  if (saci_to_profiler_.find(saci) == saci_to_profiler_.end()) {
    saci_to_profiler_[saci] = new RuntimeProfiler(ProfilerType::kCMEILINKORS);
  }

  saci_to_profiler_[saci]->Start(time_info);
}

void QueryT2T::End(int saci) {
  if (saci_to_profiler_.find(saci) == saci_to_profiler_.end()) {
    saci_to_profiler_[saci] = new RuntimeProfiler(ProfilerType::kCMEILINKORS);
  }

  saci_to_profiler_[saci]->End();
}

const std::string QueryT2T::GetStats() {
  if (!allow_stats_) {
    // printing to cout is highly irregular!
    std::cout << "Again Quering T2T stats. No more stats available.\n";
    return "";
  }

  allow_stats_ = false;  // We want to print stats only once. We have seen cases where due to multiple signals at the
                         // end, this function is called multiple times and stats get overlapped.

  // This variable temp_oss is being created
  // to make the string that we are sending back
  // to the calling function.
  std::ostringstream temp_oss;

  // We are assuming that `outfile_` exists and
  // is writeable. We are writing the exact same thing
  // that we are writing to temp_oss.
  // Another implementation could have been to call
  // outfile_ << temp_oss.str() just before the return statement
  for (auto iter : saci_to_profiler_) {
    if (iter.second != nullptr) {
      const std::string stats = iter.second->GetStats();
      temp_oss << iter.first << " " << stats << "\n";
      outfile_ << iter.first << " " << stats << "\n";
    }
  }

  std::string overall_stats = GetOverallStats();
  temp_oss << "Overall: " << overall_stats << "\n";
  outfile_ << "Overall: " << overall_stats << "\n";
  outfile_.flush();

  return temp_oss.str();
}

const std::string QueryT2T::GetOverallStats() {
  if (!allow_overall_stats_) {
    std::cout << "Again Quering Overall T2T stats. No more stats available.\n";
    return "";
  }

  // We want to print stats only once. We have seen cases where due to multiple signals at the
  // end, this function is called multiple times and stats get overlapped.
  allow_overall_stats_ = false;

  std::vector<T2TStatsStruct> stats_per_saci;
  for (auto iter : saci_to_profiler_) {
    if (iter.second != nullptr) {
      stats_per_saci.push_back(iter.second->GetStatsStruct());
    }
  }

  T2TStatsStruct overall_stats;
  for (auto& t_stats_per_saci : stats_per_saci) {
    // for now we are using auto & t_stats_per_saci.
    // In future I suggest using const auto & t_stats_per_saci
    // We need tests for that.
    // I don't know c++ well enough to use it here - gchak
    overall_stats.last_time = std::max(overall_stats.last_time, t_stats_per_saci.last_time);
    overall_stats.sample_size += t_stats_per_saci.sample_size;

    for (size_t j = 0; j < overall_stats.list_stats.size(); j++) {
      overall_stats.list_stats[j].min = std::min(t_stats_per_saci.list_stats[j].min, overall_stats.list_stats[j].min);
      overall_stats.list_stats[j].max = std::max(t_stats_per_saci.list_stats[j].max, overall_stats.list_stats[j].max);

      overall_stats.list_stats[j].mean += (t_stats_per_saci.sample_size * t_stats_per_saci.list_stats[j].mean);
      overall_stats.list_stats[j].median += (t_stats_per_saci.sample_size * t_stats_per_saci.list_stats[j].median);
      overall_stats.list_stats[j].p_95 += (t_stats_per_saci.sample_size * t_stats_per_saci.list_stats[j].p_95);
      overall_stats.list_stats[j].p_99 += (t_stats_per_saci.sample_size * t_stats_per_saci.list_stats[j].p_99);
    }
  }

  for (auto& t_list_stats : overall_stats.list_stats) {
    if (overall_stats.sample_size > 0) {
      // we are checking for zero sample size
      // to avoid divide by zero
      t_list_stats.mean /= overall_stats.sample_size;
      t_list_stats.median /= overall_stats.sample_size;
      t_list_stats.p_95 /= overall_stats.sample_size;
      t_list_stats.p_99 /= overall_stats.sample_size;
    }
    // else missing
  }

  std::ostringstream temp_oss;
  temp_oss << std::fixed;
  temp_oss << std::setprecision(2);
  temp_oss << overall_stats.sample_size << " " << overall_stats.last_time
           << " CSHMW: " << overall_stats.list_stats[0].ToString()
           << " SHM1: " << overall_stats.list_stats[1].ToString()
           << " Query: " << overall_stats.list_stats[2].ToString()
           << " SHM2: " << overall_stats.list_stats[3].ToString() << " ORS: " << overall_stats.list_stats[4].ToString()
           << " T2T: " << overall_stats.list_stats[5].ToString() << "\n";

  return temp_oss.str();
}
}
