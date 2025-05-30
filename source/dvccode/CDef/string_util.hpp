/**
    \file dvccode/CDef/string_util.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         India
         +91 80 4190 3551
*/
#ifndef BASE_CDEF_STRING_UTIL_H
#define BASE_CDEF_STRING_UTIL_H

#include <unistd.h>
#include <time.h>
#include <sstream>

namespace HFSAT {
namespace StringUtil {

template <class T>
inline std::string toString(const T& t) {
  std::stringstream ss;
  ss << t;
  return ss.str();
}
}
}

#endif  // BASE_CDEF_STRING_UTIL_H
