/**
    \file dvccode/Profiler/cpucycle_profiler.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include <stdio.h>
#include <string.h>
#include <vector>
#include <map>
#include <iomanip>
#include <inttypes.h>

#include "dvccode/Profiler/cpucycle_profiler_defines.hpp"

namespace HFSAT {

/** \brief Simple Profiler class that stores the value for every sample profiled and
 * has support to compute percentiles at the end by just doing a quicksort
 */
class CpucycleProfiler {
 private:
 protected:
  /// \brief return the curent cpu cyclecount
  static __inline__ cyclecount_t GetCpucycleCount(void) {
    unsigned int lo, hi;
    asm volatile("rdtscp"
                 : "=a"(lo), "=d"(hi) /* outputs */
                 : "a"(0)             /* inputs */
                 : "%ebx", "%ecx");   /* clobbers*/
    return ((cyclecount_t)lo) | (((cyclecount_t)hi) << 32);
  }

  /// \brief returns the difference in cyclecount_t
  static __inline__ cyclecount_t Diff(cyclecount_t t1, cyclecount_t t0) { return (t1 - t0); }

  /// \brief returns the difference in cyclecount_t in Kilo ( or multiples of 2^10 = 1024 )
  static __inline__ cyclecount_t KDiff(cyclecount_t t1, cyclecount_t t0) { return (t1 - t0) >> 10; }

  /// \brief returns the difference in cyclecount_t in Mega ( or multiples of 2^20 = 1024*1024 )
  static __inline__ cyclecount_t MDiff(cyclecount_t t1, cyclecount_t t0) { return (t1 - t0) >> 20; }

  /// the number of independant counters we are using
  const unsigned int max_counters_;
  /// the amount of memory to keep per counter
  const unsigned int max_memory_per_counter_;

  cyclecount_t* last_start_vec_;

  /* used for simple percentile calculation .. storing everything */
  cyclecount_t** store_elapsed_vec_;
  size_t* store_elapsed_vec_front_marker_;

  /// used for storing associated data at the end of an event for simple percentile calculation ..
  /// storing everything
  std::vector<std::vector<int> > store_associated_data_vec_;

  std::vector<std::string> tag_names_;
  std::map<std::string, int> tag_names_map_;
  std::vector<bool> record_session_for_tag;
  std::vector<int> end_logtime_vec_;

 private:
  CpucycleProfiler(unsigned int _max_counters_, unsigned int _max_memory_per_counter_);

 public:
#define DEFAULT_MAX_SEV_SIZE 32768
  int current_tag_ = 0;

  /// Always make sure that SetUniqueInstance is called first
  static CpucycleProfiler& SetUniqueInstance(unsigned int _max_counters_,
                                             unsigned int _max_memory_per_counter_ = DEFAULT_MAX_SEV_SIZE) {
    return GetUniqueInstance(_max_counters_, _max_memory_per_counter_);
  }

  /// Note that once this method has been called,
  /// subsequent calls makes the usage of argument _max_counters redundant
  static CpucycleProfiler& GetUniqueInstance(unsigned int _max_counters_ = 0,
                                             unsigned int _max_memory_per_counter_ = DEFAULT_MAX_SEV_SIZE) {
    static CpucycleProfiler* p_unique_instance_ = NULL;
    if (p_unique_instance_ == NULL) {
      if (_max_counters_ == 0) {
        fprintf(stderr,
                "CpucycleProfiler::GetUniqueInstance : Please call SetUniqueInstance ( non-zero-max-counters ), before "
                "calling GetUniqueInstance (). p_unique_instance_ is still NULL \n");
        // exit(1);
      }
      p_unique_instance_ = new CpucycleProfiler(std::max(_max_counters_, 30u), _max_memory_per_counter_);
    }
    return *p_unique_instance_;
  }
#undef DEFAULT_MAX_SEV_SIZE

  __inline__ cyclecount_t Start(unsigned int _index_) {
    last_start_vec_[_index_] = GetCpucycleCount();
    current_tag_ = _index_;
    return last_start_vec_[_index_];
  }
  __inline__ void Reset(unsigned int _index_, cyclecount_t cycle_count_) { last_start_vec_[_index_] = cycle_count_; }
  inline unsigned int myLog(unsigned int val) {
    if (val < 1024) return val;
    int msb;
    asm("bsrl %1,%0" : "=r"(msb) : "r"(val));
    msb -= 9;
    return (val >> msb) + (msb << 9);
  }

  inline unsigned int logInv(unsigned int val) {
    if (val < 1024) return val;
    int quotient = (val >> 9) - 1;
    int rem = val - (quotient << 9);
    return rem << quotient;
  }

  inline cyclecount_t End(unsigned int _index_) {
    cyclecount_t count = GetCpucycleCount();
    if (last_start_vec_[_index_] == 0) return count;
    int logTime = myLog(Diff(GetCpucycleCount(), last_start_vec_[_index_]));

    last_start_vec_[_index_] = 0;

    // Logs cycles only through the RecordLastSessionForTag function
    if (record_session_for_tag[_index_]){
      end_logtime_vec_[_index_] = logTime;
      return count;
    }

    store_elapsed_vec_[_index_][logTime]++;
    store_elapsed_vec_front_marker_[_index_]++;
    return count;
  }

  // An overloaded Start call if we have the cycle count from previous End call
  __inline__ void Start(unsigned int _index_, cyclecount_t count) { last_start_vec_[_index_] = count; }

  inline cyclecount_t End(unsigned int _index_, int data) {
    cyclecount_t count = GetCpucycleCount();
    if (last_start_vec_[_index_] == 0) return count;
    int logTime = myLog(Diff(count, last_start_vec_[_index_]));

    last_start_vec_[_index_] = 0;
    // Logs cycles only through the RecordLastSessionForTag function
    if (record_session_for_tag[_index_]){
      end_logtime_vec_[_index_] = logTime;
      return count;
    }

    store_elapsed_vec_[_index_][logTime]++;

    store_elapsed_vec_front_marker_[_index_]++;
    store_associated_data_vec_[_index_].push_back(data);
    return count;
  }

  // Deprecated, use: int SetTag (std::string name )
  void SetTag(unsigned int _counter_id_, std::string name) {
    tag_names_[_counter_id_] = name;
    tag_names_map_[name] = _counter_id_;
  }

  // use this to assign the value to a static variable and then use the tagId for every start and stop of the
  // corresponding profiler
  int SetTag(std::string name) {
    if (tag_names_map_.find(name) == tag_names_map_.end()) {
      int sz = tag_names_map_.size();
      tag_names_map_[name] = sz;
      tag_names_[sz] = name;
      return sz;
    }
    return tag_names_map_[name];
  }

  void StartNewSession(unsigned int _index_){
    last_start_vec_[_index_] = 0;
    end_logtime_vec_[_index_] = -1;
  }

  void RecordLastSessionForTag(unsigned int _index_){
    if (last_start_vec_[_index_] == 0){
      //End time has been recorded
      int logTime = end_logtime_vec_[_index_];
      if (logTime == -1) return;

      store_elapsed_vec_[_index_][logTime]++;
      store_elapsed_vec_front_marker_[_index_]++;
      end_logtime_vec_[_index_] = -1;
    } else {
      //Dangling tag. Just close the start tag.
      last_start_vec_[_index_] = 0;
    }
  }

  unsigned int GetTagCount(){
    return max_counters_;
  }

  void EnableSessionRecordingForTag(unsigned int _index_){
    record_session_for_tag[_index_] = true;
  }

  std::vector<CpucycleProfilerSummaryStruct> GetCpucycleSummary();
  std::string GetCpucycleSummaryString();
  std::string GetCpucycleSummaryStringUsecs();

  void FreeMem();
};
}
