// =====================================================================================
//
//       Filename:  dvccode/Tests/dvccode/CommonTradeUtils/watch_tests_helper.hpp
//
//    Description:  Tests for Utility Functions for fetching the Sample Data values from the last 30 days
//
//        Version:  1.0
//        Created:
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

#include "dvccode/ExternalData/external_time_listener.hpp"

namespace HFTEST {

using namespace HFSAT;

class WatchListener : public TimePeriodListener {
  /// Dummy class used to listen to watch updates
  int num_calls_;
  int last_num_pages_to_add_;

 public:
  WatchListener() : num_calls_(0), last_num_pages_to_add_(0) {}

  void OnTimePeriodUpdate(const int num_pages_to_add) override {
    num_calls_++;
    last_num_pages_to_add_ = num_pages_to_add;
  }
  void Reset() { num_calls_ = 0; }
  int NumCalls() { return num_calls_; }
  int LastPageWidth() { return last_num_pages_to_add_; }
};
}
