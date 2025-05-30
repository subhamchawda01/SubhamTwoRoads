#ifndef _QUERY_T2T_HPP_
#define _QUERY_T2T_HPP_

#include <map>

#include "dvccode/Utils/runtime_profiler.hpp"

namespace HFSAT {

class QueryT2T {
 public:
  static QueryT2T& GetUniqueInstance();
  static void ResetUniqueInstance();

  QueryT2T();
  ~QueryT2T();

  void Start(const ProfilerTimeInfo& time_info, int saci);
  void End(int saci);

  const std::string GetStats();

 private:
  const std::string GetOverallStats();

  static QueryT2T* unique_instance_;
  std::map<int, HFSAT::RuntimeProfiler*> saci_to_profiler_;

  std::ofstream outfile_;
  bool allow_stats_;
  bool allow_overall_stats_;
};
}

#endif
