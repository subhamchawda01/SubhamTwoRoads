#include <iostream>
#include <sstream>

#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/CommonDataStructures/simple_security_symbol_indexer.hpp"

/// To compile :
/// g++ -o exec_test43 cpptest43.cpp -I"/home/gchak/infracore_install/" -L"/home/gchak/infracore_install/libdebug/"
/// -lProfiler
int main(int argc, char** argv) {
  HFSAT::SimpleSecuritySymbolIndexer& sssi_ = HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance();
  HFSAT::CpucycleProfiler cp_(1);

  for (unsigned int i = 0; i < 1000; i++) {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << i;
    const char* abc = t_temp_oss_.str().c_str();
    cp_.Start(0);
    sssi_.AddString(abc);
    cp_.End(0);
  }

  std::vector<HFSAT::CpucycleProfilerSummaryStruct> cpss_ = cp_.GetCpucycleSummary();
  for (unsigned int i = 0; i < cpss_.size(); i++) {
    std::cout << "Min: " << cpss_[i].min_ << " "
              << "Max: " << cpss_[i].max_ << " "
              << "Mean: " << cpss_[i].mean_ << " "
              << "50p: " << cpss_[i].fifty_percentile_ << " "
              << "60p: " << cpss_[i].sixty_percentile_ << " "
              << "70p: " << cpss_[i].seventy_percentile_ << " "
              << "80p: " << cpss_[i].eighty_percentile_ << " "
              << "90p: " << cpss_[i].ninety_percentile_ << " "
              << "95p: " << cpss_[i].ninetyfive_percentile_ << std::endl;
  }
  return 0;
}
