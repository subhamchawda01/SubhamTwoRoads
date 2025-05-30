// =====================================================================================
//
//       Filename:  cleanup_listener_interface.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  07/05/2016 06:59:11 AM
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

namespace HFSAT {
namespace Utils {

class CleanUpListener {
 public:
  virtual ~CleanUpListener() {}
  virtual void OnShutdownCleanUp() = 0;
};
}
}
