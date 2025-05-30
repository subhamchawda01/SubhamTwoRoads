// =====================================================================================
//
//       Filename:  minute_bar_data_filenamer.hpp
//
//    Description:  A class to provide abstation to load a data file based on given underlying name
//
//        Version:  1.0
//        Created:  04/05/2016 03:35:54 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include "baseinfra/EventDispatcher/assumption.hpp"

namespace hftrap {
namespace loggedsources {

class MinuteBarDataFileNamer {
 public:
  // Currently files are distinguished only based on underlying name
  // later on support can be extended to support multiple params
  static std::string GetFileName(std::string const& underlying, char const& segment) {
    std::string local_filename = std::string(NSE_MEDIUMTERM_MBAR_DATA_LOCATION);

    switch (segment) {
      case NSE_FUTOPT_SEGMENT: {
        local_filename += std::string(NSE_FUTOPT_DIR);
      } break;
      case NSE_FUTOPT_ADJUSTED_SEGMENT: {
        local_filename += std::string(NSE_FUTOPT_ADJUSTED_DIR);
      } break;

      // Currently not implemented
      case NSE_CASH_SEGMENT:
      case NSE_CURRENCY_SEGMENT: {
      } break;

      default: {
        std::cerr << "SHOULD NOT HAVE REACHED HERE ( hftrap/loggedsources/MinuteBarDataFileNamer/GetFileName ) WITH "
                     "SEGMENT : "
                  << segment << std::endl;
        std::exit(-1);
      } break;
    }

    // Now we have a complete name of the file
    local_filename += underlying;
    return local_filename;
  }
};
}
}
