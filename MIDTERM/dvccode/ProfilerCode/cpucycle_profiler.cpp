/**
    \file ProfilerCode/cpucycle_profiler.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include <algorithm>
#include <sstream>

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/Utils/runtime_profiler.hpp"

namespace HFSAT {

CpucycleProfiler::CpucycleProfiler(unsigned int _max_counters_, unsigned int _max_memory_per_counter_)
    : max_counters_(_max_counters_),
      max_memory_per_counter_(_max_memory_per_counter_),
      last_start_vec_(NULL),
      store_elapsed_vec_(NULL),
      store_associated_data_vec_(),
      tag_names_(_max_counters_, ""),
      tag_names_map_(), 
      record_session_for_tag(_max_counters_, false),
      end_logtime_vec_(_max_counters_, -1){
  last_start_vec_ = (cyclecount_t*)calloc(max_counters_, sizeof(cyclecount_t));

  store_elapsed_vec_ = (cyclecount_t**)calloc(max_counters_, sizeof(cyclecount_t*));
  store_elapsed_vec_front_marker_ = (size_t*)calloc(max_counters_, sizeof(size_t));
  for (auto i = 0u; i < max_counters_; i++) {
    store_elapsed_vec_[i] = (cyclecount_t*)calloc(max_memory_per_counter_, sizeof(cyclecount_t));
    store_elapsed_vec_front_marker_[i] = 0;
  }

  store_associated_data_vec_.resize(max_counters_);
}

std::vector<CpucycleProfilerSummaryStruct> CpucycleProfiler::GetCpucycleSummary() {
  std::vector<CpucycleProfilerSummaryStruct> summarystruct_vec_;
  for (auto i = 0u; i < max_counters_; i++) {
    if (store_elapsed_vec_front_marker_[i] == 0) continue;

    CpucycleProfilerSummaryStruct _this_summarystruct_;
    _this_summarystruct_.tag_name_ = tag_names_[i];

    _this_summarystruct_.total_occurrence_ = store_elapsed_vec_front_marker_[i];
    uint64_t totalSum = 0;

    _this_summarystruct_.mean_ = totalSum / _this_summarystruct_.total_occurrence_;

    unsigned long currCount = 0;
    unsigned long percentile_50 = _this_summarystruct_.total_occurrence_ * 50 / 100;
    unsigned long percentile_90 = _this_summarystruct_.total_occurrence_ * 90 / 100;
    unsigned long percentile_95 = _this_summarystruct_.total_occurrence_ * 95 / 100;
    unsigned long percentile_99 = _this_summarystruct_.total_occurrence_ * 99 / 100;
    unsigned long percentile_99_9 = ((double)_this_summarystruct_.total_occurrence_ * 99.9) / 100;

    unsigned int currVal = 0;
    unsigned int nextCount = 0;

    for (unsigned int j = 0; j < max_memory_per_counter_; j++) {
      if (store_elapsed_vec_[i][j] == 0) continue;
      currVal = logInv(j);
      nextCount = currCount + store_elapsed_vec_[i][j];

      totalSum += currVal * store_elapsed_vec_[i][j];

      if (currCount == 0) {
        _this_summarystruct_.min_ = currVal;
      }
      if (currCount <= percentile_50 && nextCount >= percentile_50) {
        _this_summarystruct_.fifty_percentile_ = currVal;
      }
      if (currCount <= percentile_90 && nextCount >= percentile_90) {
        _this_summarystruct_.ninety_percentile_ = currVal;
      }
      if (currCount <= percentile_95 && nextCount >= percentile_95) {
        _this_summarystruct_.ninetyfive_percentile_ = currVal;
      }
      if (currCount <= percentile_99 && nextCount >= percentile_99) {
        _this_summarystruct_.ninetynine_percentile_ = currVal;
      }
      if (currCount <= percentile_99_9 && nextCount >= percentile_99_9) {
        _this_summarystruct_.ninetynine_nine_percentile_ = currVal;
      }
      if (nextCount >= _this_summarystruct_.total_occurrence_) {
        _this_summarystruct_.max_ = currVal;
        break;
      }
      currCount = nextCount;
    }
    _this_summarystruct_.mean_ = totalSum / _this_summarystruct_.total_occurrence_;

    // populate associated data summary if it exists
    if (store_associated_data_vec_[i].size() > 0) {
      std::sort(store_associated_data_vec_[i].begin(), store_associated_data_vec_[i].end());

      _this_summarystruct_.associatedDataSummary = new CpucycleProfilerSummaryStruct();

      _this_summarystruct_.associatedDataSummary->min_ = store_associated_data_vec_[i].front();
      _this_summarystruct_.associatedDataSummary->max_ = store_associated_data_vec_[i].back();
      _this_summarystruct_.associatedDataSummary->mean_ = VectorUtils::GetMean(store_associated_data_vec_[i]);
      _this_summarystruct_.associatedDataSummary->fifty_percentile_ =
          store_associated_data_vec_[i][(store_associated_data_vec_[i].size() * 50 / 100)];
      ;
      _this_summarystruct_.associatedDataSummary->ninety_percentile_ =
          store_associated_data_vec_[i][(store_associated_data_vec_[i].size() * 90 / 100)];
      _this_summarystruct_.associatedDataSummary->ninetyfive_percentile_ =
          store_associated_data_vec_[i][(store_associated_data_vec_[i].size() * 95 / 100)];
      _this_summarystruct_.associatedDataSummary->ninetynine_percentile_ =
          store_associated_data_vec_[i][(store_associated_data_vec_[i].size() * 99 / 100)];
      _this_summarystruct_.associatedDataSummary->ninetynine_nine_percentile_ =
          store_associated_data_vec_[i][(store_associated_data_vec_[i].size() * 999 / 1000)];
      _this_summarystruct_.associatedDataSummary->total_occurrence_ = store_associated_data_vec_[i].size();

    } else {
      _this_summarystruct_.associatedDataSummary = NULL;
    }

    summarystruct_vec_.push_back(_this_summarystruct_);
  }

  return summarystruct_vec_;
}

std::string CpucycleProfiler::GetCpucycleSummaryString() {
  std::stringstream ss;
  std::vector<HFSAT::CpucycleProfilerSummaryStruct> summary = GetCpucycleSummary();
  for (auto i = 0u; i < summary.size(); i++) {
    ss << "Summary for tag " << summary[i].tag_name_ << " :: ";
    ss << "Mean:" << summary[i].mean_ << "\tMedian: " << summary[i].fifty_percentile_
       << "\t90th: " << summary[i].ninety_percentile_ << "\t95th: " << summary[i].ninetyfive_percentile_
       << "\t99th: " << summary[i].ninetynine_percentile_ << "\t99.9th: " << summary[i].ninetynine_nine_percentile_
       << "\tMin: " << summary[i].min_ << "\tMax: " << summary[i].max_ << "\tcount: " << summary[i].total_occurrence_
       << "\n";
  }
  return ss.str();
}

std::string CpucycleProfiler::GetCpucycleSummaryStringUsecs() {
  std::stringstream ss;
  std::vector<HFSAT::CpucycleProfilerSummaryStruct> summary = GetCpucycleSummary();
  double cpu_freq = RuntimeProfiler::GetCPUFreq();

  if (fabs(cpu_freq - 0.0) < 0.001) {
    std::cout << "CPU Freq = 0 in CpuCycleProfiler";
    return "";
  }

  for (auto i = 0u; i < summary.size(); i++) {
    ss << "Summary for tag " << summary[i].tag_name_ << " :: ";
    ss << std::fixed << std::setprecision(2) << "Mean:" << summary[i].mean_ / cpu_freq
       << "\tMedian: " << summary[i].fifty_percentile_ / cpu_freq
       << "\t90th: " << summary[i].ninety_percentile_ / cpu_freq
       << "\t95th: " << summary[i].ninetyfive_percentile_ / cpu_freq
       << "\t99th: " << summary[i].ninetynine_percentile_ / cpu_freq
       << "\t99.9th: " << summary[i].ninetynine_nine_percentile_ / cpu_freq << "\tMin: " << summary[i].min_ / cpu_freq
       << "\tMax: " << summary[i].max_ / cpu_freq << "\tcount: " << summary[i].total_occurrence_ << "\n";
  }
  return ss.str();
}

void CpucycleProfiler::FreeMem() {
  std::vector<HFSAT::CpucycleProfilerSummaryStruct> summarystruct_vec_ = GetCpucycleSummary();

  for (auto i = 0u; i < summarystruct_vec_.size(); i++) {
    if (summarystruct_vec_[i].associatedDataSummary) {
      delete summarystruct_vec_[i].associatedDataSummary;
      summarystruct_vec_[i].associatedDataSummary = NULL;
    }
  }

  for (auto i = 0u; i < max_counters_; i++) {
    if (store_elapsed_vec_[i]) {
      free(store_elapsed_vec_[i]);
      store_elapsed_vec_[i] = NULL;
    }
  }

  if (store_elapsed_vec_) {
    free(store_elapsed_vec_);
    store_elapsed_vec_ = NULL;
  }

  if (store_elapsed_vec_front_marker_) {
    free(store_elapsed_vec_front_marker_);
    store_elapsed_vec_front_marker_ = NULL;
  }

  if (last_start_vec_) {
    free(last_start_vec_);
    last_start_vec_ = NULL;
  }
}
}
