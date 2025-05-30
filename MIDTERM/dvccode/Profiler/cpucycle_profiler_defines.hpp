/**
    \file dvccode/Profiler/cpucycle_profiler_defines.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include <stdlib.h>
#include <string>

namespace HFSAT {

typedef unsigned long cyclecount_t;

struct CpucycleProfilerSummaryStruct {
  cyclecount_t min_;
  cyclecount_t max_;
  cyclecount_t mean_;
  cyclecount_t fifty_percentile_;
  cyclecount_t ninety_percentile_;
  cyclecount_t ninetyfive_percentile_;
  cyclecount_t ninetynine_percentile_;
  cyclecount_t ninetynine_nine_percentile_;
  unsigned long total_occurrence_;

  std::string tag_name_;

  CpucycleProfilerSummaryStruct* associatedDataSummary;
};
}
