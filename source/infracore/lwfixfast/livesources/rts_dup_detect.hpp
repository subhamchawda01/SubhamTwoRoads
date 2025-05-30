/**
    \file lwfixfast/livesources/rts_dup_detect.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#pragma once
#include <inttypes.h>

namespace HFSAT {
// singleton return by ref

class RtsDupDetect {
  unsigned int prev_seq_num_;

 public:
  RtsDupDetect(int initial_seq_num) { prev_seq_num_ = initial_seq_num; }
  static RtsDupDetect& GetUniqInstance() {
    static RtsDupDetect* p_uniq_instance_ = NULL;
    if (p_uniq_instance_ == NULL) {
      p_uniq_instance_ = new RtsDupDetect(0);
    }
    return *p_uniq_instance_;
  }

  bool set_curr_channel(uint32_t channel_id) { return false; }

  bool isDup(uint32_t seqNo) {
    if (prev_seq_num_ == 0) {
      prev_seq_num_ = seqNo;
      return false;
    }
    if (prev_seq_num_ + 1 == seqNo) {
      prev_seq_num_ = seqNo;
      return false;
    }
    return true;
  }
};
}
