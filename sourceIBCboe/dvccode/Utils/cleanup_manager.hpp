// =====================================================================================
//
//       Filename:  cleanup_manager.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  07/05/2016 07:02:39 AM
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
#include <vector>

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/Utils/cleanup_listener_interface.hpp"

namespace HFSAT {
namespace Utils {

class CleanUpManager {
 private:
  std::vector<CleanUpListener*> cleanup_listener_vec_;
  CleanUpManager() {}
  CleanUpManager(CleanUpManager const& disabled_copy_constructor) = delete;

 public:
  static CleanUpManager& GetUniqueInstance() {
    static CleanUpManager unique_instance;
    return unique_instance;
  }

  void AddCleanUpListener(CleanUpListener* cleanup_listener) {
    VectorUtils::UniqueVectorAdd(cleanup_listener_vec_, cleanup_listener);
  }

  void NotifyListenersOnShutdown() {
    for (auto& itr : cleanup_listener_vec_) {
      itr->OnShutdownCleanUp();
    }
  }
};
}
}
