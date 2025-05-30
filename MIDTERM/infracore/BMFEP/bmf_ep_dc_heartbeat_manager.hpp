// =====================================================================================
//
//       Filename:  bmf_ep_heartbeat_manager.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  05/16/2014 12:42:52 PM
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

#include "dvccode/Utils/thread.hpp"
#include "infracore/BMFEP/BMFDropCopyEngineThread.hpp"

namespace HFSAT {
namespace Utils {

/// Separate thread that interacts with BMFEPEngine, triggers a heartbeat
class BMFEPDCHeartBeatManager : public Thread {
 public:
  BMFEPDCHeartBeatManager(Settings& r_settings_, DebugLogger& _dbglogger_, bool _use_affinity_,
                          HFSAT::Utils::BMFDropCopyEngine* _p_bmf_ep_engine_)
      : p_bmf_ep_engine_(_p_bmf_ep_engine_),
        heartbeat_interval_(0),
        heartbeat_ready_to_be_sent_(false),
        use_affinity_(_use_affinity_) {
    heartbeat_interval_ = r_settings_.getIntValue("HeartBtInt", 30) - 3;
    timeout_.tv_sec = heartbeat_interval_;
    timeout_.tv_usec = 0;
  }

  void thread_main() {
    time_t cptime = time(NULL);

    // This will send a heartbeat if no message has been sent for last n units from our side
    while (true) {
      int retval = 0;
      heartbeat_ready_to_be_sent_ = false;
      retval = select(0, 0, 0, 0, &timeout_);
      if (retval == 0) {
        cptime = time(NULL);
        int diff = cptime - p_bmf_ep_engine_->lastMsgSentAtThisTime();
        if (diff >= heartbeat_interval_ - 1) {
          // Timed out , time to send  a heartbeat
          heartbeat_ready_to_be_sent_ = true;
          // std::cout << " Will be sending hbt. 2" << "TIMEOUT: "<< timeout_.tv_sec << std::endl;
          p_bmf_ep_engine_->sendHeartbeat();
        } else {
          // Change the amount to sleep only this extra
          // Incase there is a garbage in the diff due to thread_contention
          timeout_.tv_sec = std::max(0, std::min(heartbeat_interval_, heartbeat_interval_ - diff));
          timeout_.tv_usec = 0;
          // std::cout << " Sleeping some more " << timeout_.tv_sec << std::endl;
        }
        if (!p_bmf_ep_engine_->is_engine_running()) {
          // Engine stopped, HBT should stop as well
          // std::cout << "Engine Stopped: "<< std::endl;
          break;
        }
      }
    }
  }

  void reset_timeout() {
    timeout_.tv_sec = heartbeat_interval_;
    timeout_.tv_usec = 0;
  }

  inline int get_timeout() { return timeout_.tv_sec; }

  inline bool heartBeatReady() { return heartbeat_ready_to_be_sent_; }

 private:
  // Need to invoke the engines HBT's, access its last_send_time, etc
  HFSAT::Utils::BMFDropCopyEngine* p_bmf_ep_engine_;
  int heartbeat_interval_;
  bool heartbeat_ready_to_be_sent_;
  struct timeval timeout_;

  bool use_affinity_;
};
}
}
