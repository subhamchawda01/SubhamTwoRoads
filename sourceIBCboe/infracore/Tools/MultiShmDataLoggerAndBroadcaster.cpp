// =====================================================================================
//
//       Filename:  MultiShmDataLogger.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/26/2018 07:51:37 AM
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

#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <set>

#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CDef/nse_shm_interface_defines.hpp"
#include "dvccode/Utils/combined_mds_messages_multi_shm_base.hpp"

class CombinedMDSMessagesMultiShmProcessor : public HFSAT::MDSMessages::CombinedMDSMessagesNSEProShmBase,
                                             public HFSAT::MDSMessages::CombinedMDSMessagesORSProShmBase,
                                             public HFSAT::SimpleExternalDataLiveListener {
 private:
  int64_t index_;
  int64_t ors_reply_index_;
  bool keep_me_running_;

  NSE_MDS::NSETBTDataCommonStructProShm generic_mds_message_;
  HFSAT::GenericORSReplyStructLiveProShm ors_reply_mds_message_;
  uint32_t generic_mds_message_pkt_size_;
  uint32_t ors_reply_mds_shm_struct_pkt_size_;

  HFSAT::RuntimeProfiler &runtime_profiler_;

 public:
  CombinedMDSMessagesMultiShmProcessor()
      : CombinedMDSMessagesNSEProShmBase(),
        CombinedMDSMessagesORSProShmBase(),
        index_(-1),
        ors_reply_index_(-1),
        keep_me_running_(false),
        generic_mds_message_(),
        generic_mds_message_pkt_size_(sizeof(NSE_MDS::NSETBTDataCommonStructProShm)),
        ors_reply_mds_shm_struct_pkt_size_(sizeof(HFSAT::GenericORSReplyStructLiveProShm)) {}

  ~CombinedMDSMessagesMultiShmProcessor() {}

  void CleanUp() { keep_me_running_ = false; }

  int main(int argc, char *argv[]) { return EXIT_SUCCESS; }  // ----------  end of function main  ----------
