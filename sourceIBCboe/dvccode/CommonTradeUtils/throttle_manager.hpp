/**
    \file dvccode/CommonTradeUtils/throttle_manager.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#ifndef BASE_COMMONTRADEUTILS_THROTTLEMANAGER_H
#define BASE_COMMONTRADEUTILS_THROTTLEMANAGER_H

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/Utils/lock.hpp"
#include "dvccode/CommonDataStructures/circular_buffer.hpp"

#define MINIMUM_LAG 1000  // 1 sec

// Limits the no of orders sent by an exec logic in one sec.
// Implementation slightly diff from Infracore.

namespace HFSAT {

class ThrottleManager {
 public:
  ThrottleManager(int t_default_size = THROTTLE_MSG_LIMIT)
      : size_(t_default_size),
        curr_indx_(0),
        size_minus_1_(t_default_size - 1),
        throttle_timestamp_buffer_((int*)calloc(t_default_size, sizeof(int))),
        use_throttle_manager_(false) {}

  inline bool allowed_through_throttle(int this_timestamp) {  // used by base_trading class
    if (!use_throttle_manager_) return true;

    int oldest_time_stamp = (curr_indx_ == size_minus_1_) ? 0 : curr_indx_ + 1;
    if (this_timestamp - throttle_timestamp_buffer_[oldest_time_stamp] > MINIMUM_LAG) {
      // curr_indx_ = oldest_time_stamp;
      // throttle_timestamp_buffer_[curr_indx_] = this_timestamp;
      return true;
    }
    return false;
  }

  inline void update_throttle_manager(int this_timestamp) {  // used by BOM / OM
    // called AFTER any msg is sent to exchange / ors
    if (!use_throttle_manager_) return;
    curr_indx_ = (curr_indx_ == size_minus_1_) ? 0 : curr_indx_ + 1;
    throttle_timestamp_buffer_[curr_indx_] = this_timestamp;
  }

  inline void start_throttle_manager(bool t_use_throttle_manager_) { use_throttle_manager_ = t_use_throttle_manager_; }

  inline void set_throttle_msg_limit(
      int t_new_size) {  // not used yet... if we ever define a user msg to reset this limit
    free(throttle_timestamp_buffer_);
    throttle_timestamp_buffer_ = (int*)calloc(t_new_size, sizeof(int));
    size_ = t_new_size;
    curr_indx_ = 0;
    size_minus_1_ = size_ - 1;
  }

 private:
  int size_;
  int curr_indx_;
  int size_minus_1_;
  int* throttle_timestamp_buffer_;
  bool use_throttle_manager_;
};
}

#endif  // BASE_COMMONTRADEUTILS_THROTTLEMANAGER_H
