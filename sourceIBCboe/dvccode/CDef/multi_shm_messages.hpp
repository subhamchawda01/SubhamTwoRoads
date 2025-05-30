// =====================================================================================
//
//       Filename:  multi_shm_messages.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/08/2018 05:37:27 AM
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

#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/CDef/control_messages.hpp"

namespace HFSAT {

enum class MultiShmDataType { INVALID = 0, ORS, CONTROL };

struct MultiShmMessage {
  MultiShmDataType data_type_;
  union {
    GenericORSReplyStructLiveProShm ors_reply_data_;
    GenericControlRequestStruct control_data_;
  } data_;
};
}
