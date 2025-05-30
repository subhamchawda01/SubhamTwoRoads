#include <iostream>
#include <cmath>
#include "dvccode/Profiler/cpucycle_profiler.hpp"
using namespace HFSAT;
int main() {
  HFSAT::CpucycleProfiler& cpucycle_profiler_ = HFSAT::CpucycleProfiler::SetUniqueInstance(10);
  cpucycle_profiler_.SetTag(0, "Processing 10000 sigmoid calculations");
  cpucycle_profiler_.SetTag(1, "Processing 10000 linear model calculations");

  double sigmoid = 0.0;
  double x = 0.5;
  double y = 0.2;
  double z = 0.0;
  int iters = 1000000;
  cpucycle_profiler_.Start(0);
  for (int i = 0; i < iters; i++) {
    sigmoid = 1 / (1 + exp(i));
  }
  cpucycle_profiler_.End(0);

  std::cout << exp(-x) << " " << sigmoid << "\n";

  cpucycle_profiler_.Start(1);
  for (int i = 0; i < iters; i++) {
    z = 0.1 * i;
  }
  cpucycle_profiler_.End(1);

  std::cout << z << "\n";
  std::cout << "Printing the results of CPU Profiler " << std::endl;
  const std::vector<HFSAT::CpucycleProfilerSummaryStruct> prof_summary = cpucycle_profiler_.GetCpucycleSummary();

  for (unsigned int ii = 0; ii < prof_summary.size(); ii++) {
    if (prof_summary[ii].total_occurrence_ > 0) {
      std::cout << prof_summary[ii].tag_name_ << " " << prof_summary[ii].fifty_percentile_ / iters << " "
                << ((double)prof_summary[ii].ninetyfive_percentile_ / (double)prof_summary[ii].fifty_percentile_) << ' '
                << prof_summary[ii].total_occurrence_ << std::endl;
    }
  }
}
