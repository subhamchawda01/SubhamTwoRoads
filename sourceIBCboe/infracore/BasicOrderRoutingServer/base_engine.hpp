/**
    \file BasicOrderRoutingServer/base_engine.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */

#ifndef BASE_BASICORDERROUTINGSERVER_BASEENGINE_H
#define BASE_BASICORDERROUTINGSERVER_BASEENGINE_H

#include <string>
#include <iostream>
#include <time.h>

#include "dvccode/Utils/thread.hpp"
#include "dvccode/CDef/debug_logger.hpp"

#include "infracore/BasicOrderRoutingServer/defines.hpp"
#include "infracore/BasicOrderRoutingServer/engine_listener.hpp"
#include "dvccode/Utils/settings.hpp"
#include "infracore/BasicOrderRoutingServer/throttle_manager.hpp"
#include "dvccode/Utils/tcp_client_socket.hpp"
#include "dvccode/Utils/control_hub_listener.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "onload/extensions.h"

namespace HFSAT {
namespace ORS {
const int kSessionTerminated = -1;
const int kSessionNotRunning = -2;
const int kSessionClosed = -3;  // graceful termination
class BaseEngine : public Thread, public HFSAT::Utils::ControlHubListener {
 public:
  BaseEngine(Settings& ses, DebugLogger& dbg)
      : m_settings(ses),
        dbglogger_(dbg),
        p_engine_listener_(NULL),
        p_throttler_(NULL),
        is_playback_mode_(false),
        engine_id_(-1),
        is_msg_warm_supported_(false) {
    int limit = m_settings.getIntValue("ORSThrottleLimit", THROTTLE_MSG_LIMIT);
    p_throttler_ = new ThrottleManager(dbg, limit, true);
    int ioc_throttle_limit = m_settings.getIntValue("ORSThrottleLimit_IOC", THROTTLE_MSG_LIMIT_IOC);
    p_ioc_throttler_ = new ThrottleManager(dbg, ioc_throttle_limit, true);
  }

  virtual ~BaseEngine() {}
  /// order management calls
  virtual void SendOrder(Order* ord) = 0;
  virtual void SendOrder(std::vector<HFSAT::ORS::Order*> multi_leg_order_ptr_vec_) = 0;
  virtual void SendSpreadOrder(HFSAT::ORS::Order *order1_, HFSAT::ORS::Order *order2_) = 0;
  virtual void SendThreeLegOrder(HFSAT::ORS::Order *order1_, HFSAT::ORS::Order *order2_, HFSAT::ORS::Order *order3_) = 0;
  virtual void SendTwoLegOrder(HFSAT::ORS::Order *order1_, HFSAT::ORS::Order *order2_) = 0;
  virtual void MirrorOrder(HFSAT::ORS::Order* ord, int mirror_factor) {
    std::cout << __func__ << ":" << __LINE__ << std::endl;
    std::cout << "Mirror Order base class." << std::endl;
  }
  // #ifdef ONLY_FOR_AUTOCERTPLUS
  //       virtual void SendTIFOrder ( const Order* ord, const std::string & tif_str_ ) = 0;
  // #endif
  virtual void KillSwitch(int sec_id) {};
  virtual void KillSwitchNewForSecId( int32_t sec_id) {};
  virtual void CancelOrder(Order* ord) = 0;
  virtual void ModifyOrder(Order* ord, Order* orig_order) = 0;

  virtual int onInputAvailable(int32_t sock = 0, char* buffer = NULL, int32_t length = 0) { return 0; }
  virtual void ProcessLockFreeTCPDirectRead() { std::exit(-1); }
  virtual std::vector<int> init() { return std::vector<int>(); }

  /// technical session control calls
  virtual void Connect() = 0;
  virtual void DisConnect() = 0;
  virtual void Login() = 0;
  virtual void Logout() = 0;
  virtual void SendHeartbeat(){};         // Make this function pure virtual when its implemented by all exchanges
  virtual void CheckToSendHeartbeat(){};  // Make this function pure virtual when its implemented by all exchanges
  virtual void ProcessMessageQueue(){};
  virtual void ProcessFakeSend(Order* ord, ORQType_t type){};
  virtual void RequestORSPNL() {};
  virtual void FetchMarginUsage() {};
  virtual void RequestORSOpenPositions() {};
  virtual void RequestExecutedOrders() {};

  // Need to change sender_sub_id midday
  virtual void CommitFixSequences() {}

  virtual void DumpSettingsToFile() {}

  bool peek_reject_for_time_throttle(int server_sequence, int no_of_orders_) {
    return p_throttler_->peek_reject_for_time_throttle(no_of_orders_);
  }

  bool reject_for_time_throttle(int server_sequence) { return p_throttler_->reject_for_time_throttle(); }

  bool reject_for_time_throttle_ioc(int server_sequence) { return p_ioc_throttler_->reject_for_time_throttle(); }

  void throttle_update(int new_throttle_val) { p_throttler_->throttle_value_update(new_throttle_val); } 
  void throttle_update_ioc(int new_throttle_val) { p_ioc_throttler_->throttle_value_update(new_throttle_val); }

  uint64_t min_cycle_throttle_wait() { return p_throttler_->min_cycle_throttle_wait(); }

  uint64_t min_cycle_throttle_wait_ioc() { return p_ioc_throttler_->min_cycle_throttle_wait(); } 

  /// listener for callback ..
  virtual void setListener(EngineListener* l) { p_engine_listener_ = l; }

  virtual void SetPlaybackMode(bool val) { is_playback_mode_ = false; }

  virtual bool is_engine_running() {
    return false;
  }  //  Make this function pure virtual when its implemented by all exchanges
  virtual time_t LastMsgSentAtThisTime() {
    return 0;
  }  // Make this function pure virtual when its implemented by all exchanges

  virtual void ExplicitModifyOrder(uint64_t order_num_, char buysell_, std::string symbol_, double price_, int qty_) {}
  virtual void ActivateOrder(uint64_t order_num_, char buysell_, std::string symbol_) {}
  virtual void ProcessThrottleIncreaseReq(int new_throttle_val,int new_throttle_val_ioc) {}
  virtual void ProcessGeneralControlCommand(const char* input_stream, int stream_length) {}

  void SetEngineID(int engine_id) { engine_id_ = engine_id; }
  int GetEngineID() { return engine_id_; }

  int AllocateToSiblingCore() {
    int core = CPUManager::GetCoreForProc("shm_client_receiver");
    if (core > 0) {
      if (!allocateToSiblingCPU(core)) return -1;
    }
    return core;
  }

  bool IsOnloadMsgWarmSuppoted(int fd) {
    if (m_settings.has("SendFakeTCPPackets") &&
        std::string("Y") == std::string(m_settings.getValue("SendFakeTCPPackets"))) {
      if (onload_fd_check_feature(fd, ONLOAD_FD_FEAT_MSG_WARM) > 0) {
        std::cout << "ONLOAD MSG WARM FEATURE IS SUPPORTED FOR FD : " << fd << std::endl;
        is_msg_warm_supported_ = true;
        return true;
      }
    }
    return false;
  }

 protected:
  virtual void thread_main() = 0;

 protected:
  Settings m_settings;
  DebugLogger& dbglogger_;
  EngineListener* p_engine_listener_;
  ThrottleManager* p_throttler_;
  ThrottleManager* p_ioc_throttler_;

  bool is_playback_mode_;
  int engine_id_;
  bool is_msg_warm_supported_;
};
}
}
#endif  // BASE_BASICORDERROUTINGSERVER_BASEENGINE_H
