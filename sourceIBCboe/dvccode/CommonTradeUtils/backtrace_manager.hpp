/**
 *     \file dvccode/CommonTradeUtils/backtrace_manager.hpp
 *
 *         \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 *        Address:
 *      Suite No 353, Evoma, #14, Bhattarhalli,
 *      Old Madras Road, Near Garden City College,
        KR Puram, Bangalore 560049, India
       +91 80 4190 3551
1*/

#ifndef BASE_COMMONTRADEUTILS_BACKTRACEMANAGER_H
#define BASE_COMMONTRADEUTILS_BACKTRACEMANAGER_H

#include <execinfo.h>
#include <cxxabi.h>
#include <iostream>
#include <sstream>
namespace HFSAT {

class BackTraceManager {
 public:
  BackTraceManager(unsigned int max_frames);
  void collect_stacktrace(std::ostringstream& t_temp_oss);

 private:
  unsigned int max_frames_;
};
}
#endif  // BASE_COMMONTRADEUTI_BACKTRACEMANAGER_H
