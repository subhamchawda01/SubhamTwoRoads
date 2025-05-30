/**
    \file testbed/cpptest7.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include <iostream>
#include <vector>
#include <algorithm>

typedef unsigned long long cyclecount_t;

struct CpucycleProfilerSummaryStruct {
  cyclecount_t min_;
  cyclecount_t max_;
  cyclecount_t mean_;
  cyclecount_t fifty_percentile_;
  cyclecount_t sixty_percentile_;
  cyclecount_t seventy_percentile_;
  cyclecount_t eighty_percentile_;
  cyclecount_t ninety_percentile_;
  cyclecount_t ninetyfive_percentile_;
};

template <class T>
void FillInValue(std::vector<T>& _class_vec_, const T& _new_item_) {
  for (unsigned int i = 0; i < _class_vec_.size(); i++) {
    _class_vec_[i] = _new_item_;
  }
}

template <class T>
T GetMean(const std::vector<T>& src_vec_) {
  T retval_ = 0;
  for (unsigned int i = 0; i < src_vec_.size(); i++) {
    retval_ += src_vec_[i];
  }
  return (retval_ / (T)(src_vec_.size()));
}

class CpucycleProfiler {
 protected:
  /** @brief return the curent cpu cyclecount */
  static __inline__ cyclecount_t GetCpucycleCount(void) {
    unsigned a, d;
    asm volatile("rdtsc" : "=a"(a), "=d"(d));
    return ((cyclecount_t)a) | (((cyclecount_t)d) << 32);
  }

  static __inline__ cyclecount_t Diff(cyclecount_t t1, cyclecount_t t0) { return (t1 - t0); }

  static __inline__ cyclecount_t KDiff(cyclecount_t t1, cyclecount_t t0) { return (t1 - t0) >> 5; }

  static __inline__ cyclecount_t MDiff(cyclecount_t t1, cyclecount_t t0) { return (t1 - t0) >> 10; }

  unsigned int max_counters_;
  std::vector<cyclecount_t> last_start_vec_;

  /* simple percetile calculation .. storing everything */
  std::vector<std::vector<cyclecount_t> > store_elapsed_vec_;

 public:
  CpucycleProfiler(unsigned int _max_counters_)
      : max_counters_(_max_counters_), last_start_vec_(), store_elapsed_vec_() {
    last_start_vec_.resize(max_counters_);
    FillInValue(last_start_vec_, (cyclecount_t)0);
    store_elapsed_vec_.resize(max_counters_);
  }

  void Start(unsigned int _index_) { last_start_vec_[_index_] = GetCpucycleCount(); }

  void End(unsigned int _index_) {
    if (last_start_vec_[_index_] != 0) {
      store_elapsed_vec_[_index_].push_back(Diff(GetCpucycleCount(), last_start_vec_[_index_]));
      last_start_vec_[_index_] = 0;
    }
  }

  const std::vector<CpucycleProfilerSummaryStruct> GetCpucycleSummary() {
    std::vector<CpucycleProfilerSummaryStruct> summarystruct_vec_;
    for (unsigned int i = 0; i < max_counters_; i++) {
      CpucycleProfilerSummaryStruct _this_summarystruct_;
      if (store_elapsed_vec_[i].size() > 0) {
        std::sort(store_elapsed_vec_[i].begin(), store_elapsed_vec_[i].end());

        _this_summarystruct_.min_ = store_elapsed_vec_[i].front();
        _this_summarystruct_.max_ = store_elapsed_vec_[i].back();
        _this_summarystruct_.mean_ = GetMean(store_elapsed_vec_[i]);

        _this_summarystruct_.fifty_percentile_ = store_elapsed_vec_[i][(store_elapsed_vec_[i].size() * 50 / 100)];
        ;
        _this_summarystruct_.sixty_percentile_ = store_elapsed_vec_[i][(store_elapsed_vec_[i].size() * 60 / 100)];
        _this_summarystruct_.seventy_percentile_ = store_elapsed_vec_[i][(store_elapsed_vec_[i].size() * 70 / 100)];
        _this_summarystruct_.eighty_percentile_ = store_elapsed_vec_[i][(store_elapsed_vec_[i].size() * 80 / 100)];
        _this_summarystruct_.ninety_percentile_ = store_elapsed_vec_[i][(store_elapsed_vec_[i].size() * 90 / 100)];
        _this_summarystruct_.ninetyfive_percentile_ = store_elapsed_vec_[i][(store_elapsed_vec_[i].size() * 95 / 100)];

        summarystruct_vec_.push_back(_this_summarystruct_);
      }
    }

    return summarystruct_vec_;
  }

  const std::vector<std::vector<cyclecount_t> >& GetStoredElapsedVales() const { return store_elapsed_vec_; }
};

int main() {
  CpucycleProfiler cpucycle_profiler_(3);

  int abc_int = 0;
  long abc_long = 0;
  double abc_double = 0;

  unsigned int localitercount = 1000;
  unsigned int maxitercount = 40;

  for (unsigned int _itercount_ = 0; _itercount_ < maxitercount; _itercount_++) {
    for (unsigned int i = 0; i < 3; i++) {
      switch (i) {
        case 0: {
          cpucycle_profiler_.Start(i);
          for (unsigned int j = 0; j < localitercount; j++) {
            //	  abc_int += (int)1;
          }
          cpucycle_profiler_.End(i);
        } break;
        case 1: {
          cpucycle_profiler_.Start(i);
          for (unsigned int j = 0; j < localitercount; j++) {
            abc_int += (int)1;
          }
          cpucycle_profiler_.End(i);
        } break;
        case 2: {
          cpucycle_profiler_.Start(i);
          for (unsigned int j = 0; j < localitercount; j++) {
            abc_long += (long)1;
          }
          cpucycle_profiler_.End(i);
        } break;
      }
    }
  }

  const std::vector<CpucycleProfilerSummaryStruct> sumstr = cpucycle_profiler_.GetCpucycleSummary();
  for (unsigned int cnum_ = 0; cnum_ < sumstr.size(); cnum_++) {
    std::cout << " Values of counter : " << cnum_ << " : ";
    std::cout << sumstr[cnum_].min_ << " " << sumstr[cnum_].max_ << " " << sumstr[cnum_].mean_ << " "
              << sumstr[cnum_].fifty_percentile_ << " " << sumstr[cnum_].sixty_percentile_ << " "
              << sumstr[cnum_].seventy_percentile_ << " " << sumstr[cnum_].eighty_percentile_ << " "
              << sumstr[cnum_].ninety_percentile_ << " " << sumstr[cnum_].ninetyfive_percentile_ << " " << std::endl;
  }

  // const std::vector < std::vector < cyclecount_t > > & store_elapsed_vec_ = cpucycle_profiler_.GetStoredElapsedVales
  // ( );
  // for ( unsigned int cnum_ = 0 ; cnum_ < store_elapsed_vec_.size ( ) ; cnum_ ++ ) {
  //   std::cout << " Values of counter : " << cnum_ << " : " ;
  //   for ( unsigned int i = 0 ; i < store_elapsed_vec_[cnum_].size ( ) ; i ++ )
  //     {
  // 	std::cout << store_elapsed_vec_[cnum_][i] << " " ;
  //     }
  //   std::cout << std::endl;
  // }

  return 0;
}
