/**
    \file dvccode/Utils/signals.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_UTILS_SIGNALS_H
#define BASE_UTILS_SIGNALS_H

#include <signal.h>

inline const char* SimpleSignalString(int signum) {
  switch (signum) {
    case SIGINT:
      return "SIGINT";
    case SIGSEGV:
      return "SIGSEGV";
    case SIGILL:
      return "SIGILL";
    case SIGABRT:
      return "SIGABRT";
    case SIGFPE:
      return "SIGFPE";
    default:
      return "other_signal";
  }
  return "other_signal";
}

#endif  // BASE_UTILS_SIGNALS_H
