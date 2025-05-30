#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include <iostream>
#include <chrono>
int main(){
    HFSAT::CpucycleProfiler::SetUniqueInstance(3);
    /*
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(1, "Get current time");
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(2, "Get time since epoch in micro");
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(0, "Get current time in micros");
    
    for(int i=0;i<1e5;i++){
        HFSAT::CpucycleProfiler::GetUniqueInstance().Start(0);
        HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1);
        // Get the current time point
        auto now = std::chrono::high_resolution_clock::now();
        HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);

        HFSAT::CpucycleProfiler::GetUniqueInstance().Start(2);
         // Convert to time since epoch in microseconds
        std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
        HFSAT::CpucycleProfiler::GetUniqueInstance().End(2);

        HFSAT::CpucycleProfiler::GetUniqueInstance().End(0);

    }
    */
    /*
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(0, "Number of cycles used in counting cycles");
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(1, "Number of cycles between start and end");

    for(int i=0;i<1e5;i++){
      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(0);
      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1);
      HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);
      HFSAT::CpucycleProfiler::GetUniqueInstance().End(0);
    }
    */
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(0, "Writing a string");
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(1, "Writing an integer");
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(2, "Reading a string");
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(3, "Reading an integer");
    int32_t num=0,num2=0;
    std::string name,name2;
    for(int i=0;i<1e5;i++){
      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(0);
      name="100002";
      HFSAT::CpucycleProfiler::GetUniqueInstance().End(0);
      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1);
      name2=name;
      HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);
      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(2);
      num=100002;
      HFSAT::CpucycleProfiler::GetUniqueInstance().End(2);
      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(3);
      num2=num;
      HFSAT::CpucycleProfiler::GetUniqueInstance().End(3);
    }
    num2*=num2;
    std::vector<HFSAT::CpucycleProfilerSummaryStruct> cpss_ = HFSAT::CpucycleProfiler::GetUniqueInstance().GetCpucycleSummary();
    
  for (unsigned int i = 0; i < cpss_.size(); i++) {
    std::cout << "Tag Name: " << cpss_[i].tag_name_ << " "
              << "Total Occurance: " << cpss_[i].total_occurrence_ << " "
              << "Min: " << cpss_[i].min_ << " "
              << "Max: " << cpss_[i].max_ << " "
              << "Mean: " << cpss_[i].mean_ << " "
              << "50p: " << cpss_[i].fifty_percentile_ << " "
              << "90p: " << cpss_[i].ninety_percentile_ << " "
              << "99p: " << cpss_[i].ninetynine_percentile_ << " "
              << "95p: " << cpss_[i].ninetyfive_percentile_ << std::endl;
  }

}