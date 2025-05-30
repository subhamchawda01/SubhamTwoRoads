#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sstream>
#include <ctime>

#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/CDef/ttime.hpp"

int main(int argc, char** argv) {
  HFSAT::CpucycleProfiler& cpucycle_profiler_ = HFSAT::CpucycleProfiler::GetUniqueInstance(10);

#ifdef USING_TIMEVAL
  fprintf(stdout, "using timeval\n");
  timeval const_tv;
  gettimeofday(&const_tv, NULL);
#else
  fprintf(stdout, "using ttime_t\n");
  HFSAT::ttime_t const_ttime_t_ = HFSAT::GetTimeOfDay();
#endif

  int x = 0;
  for (int i = 0; i < 1000000; i++) {
    cpucycle_profiler_.Start(0);

#ifdef USING_TIMEVAL
    timeval tv;
    gettimeofday(&tv, NULL);
#else
    HFSAT::ttime_t ttime_t_ = HFSAT::GetTimeOfDay();
#endif

    cpucycle_profiler_.End(0);
    cpucycle_profiler_.Start(1);

#ifdef USING_TIMEVAL
    if ((tv.tv_sec < const_tv.tv_sec) || ((tv.tv_sec == const_tv.tv_sec) && (tv.tv_usec < const_tv.tv_usec))) x++;
#else
    if (ttime_t_ < const_ttime_t_) x++;
#endif
    cpucycle_profiler_.End(1);
  }

  std::vector<HFSAT::CpucycleProfilerSummaryStruct> cpucycle_summary_vec_ = cpucycle_profiler_.GetCpucycleSummary();
  for (unsigned int i = 0; i < cpucycle_summary_vec_.size(); i++) {
    fprintf(stderr, "%d:\t Min:%u\t Max:%u\t Mean:%u Median:%u\n", i, cpucycle_summary_vec_[i].min_,
            cpucycle_summary_vec_[i].max_, cpucycle_summary_vec_[i].mean_, cpucycle_summary_vec_[i].fifty_percentile_);
  }
  fprintf(stdout, "%d\n", x);
}
