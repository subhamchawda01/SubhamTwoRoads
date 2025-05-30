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
        allow_new_orders_(true),
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
  virtual void MirrorOrder(Order* ord, int mirror_factor) {
    std::cout << __func__ << ":" << __LINE__ << std::endl;
    std::cout << "Mirror Order base class." << std::endl;
  }
  // #ifdef ONLY_FOR_AUTOCERTPLUS
  //       virtual void SendTIFOrder ( const Order* ord, const std::string & tif_str_ ) = 0;
  // #endif
  virtual void CancelOrder(Order* ord) = 0;
  virtual void ModifyOrder(Order* ord, Order *orig_order) = 0;
  virtual void AdjustSymbol(const char* symbol) {
    std::cout << "Called base class \n";
    // do nothing
  }
  virtual void ChangeSpreadRatio(const char* symbol, const char* ratio) {
    std::cout << "Called base class for change spread \n";
    // do nothing
  }

  virtual bool SendCrossTrade(const char* _symbol_, const int32_t& _order_qty_, const double& _price_,
                              const char* _account_, const char* _broker_tag_, const char* _counter_party_) {
    std::cerr << "Wrong SendCrossTrade() called from base class\n";
    return false;
  }
  virtual int onInputAvailable() { return 0; }
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

  // Need to change sender_sub_id midday
  virtual void setSenderSubId(const char* sender_sub_id_) {}
  virtual void CommitFixSequences() {}

  // Need to change client_info
  virtual void SetClientInfo(const unsigned int t_security_id_, const char* t_client_info_) {}

  virtual void DumpEngineLogginInfo() {}

  bool peek_reject_for_time_throttle(int server_sequence, int no_of_orders_) {
    return p_throttler_->peek_reject_for_time_throttle(no_of_orders_);
  }

  bool reject_for_time_throttle(int server_sequence) { return p_throttler_->reject_for_time_throttle(); }

  bool reject_for_time_throttle_ioc(int server_sequence) { return p_ioc_throttler_->reject_for_time_throttle(); }

  /// listener for callback ..
  virtual void setListener(EngineListener* l) { p_engine_listener_ = l; }

  virtual void enableNewOrders() { allow_new_orders_ = true; }
  virtual void disableNewOrders() { allow_new_orders_ = false; }
  virtual void SetPlaybackMode(bool val) { is_playback_mode_ = false; }

  virtual bool is_engine_running() {
    return false;
  }  //  Make this function pure virtual when its implemented by all exchanges
  virtual time_t LastMsgSentAtThisTime() {
    return 0;
  }  // Make this function pure virtual when its implemented by all exchanges
  virtual void QueryClrInfo() {}
  virtual void QueryOrderBook(std::string symbol) {}
  virtual void QueryInactiveOrderBook(std::string symbol) {}
  virtual void QueryMissingTrade(int _start_seq_, int _last_seq_, std::string date_, std::string symbol_) {}
  virtual void QueryMissingBroadcast(int _start_seq_, int _last_seq_, std::string date_, std::string symbol_) {}
  virtual void QueryMarketStatus() {}
  virtual void QuerySeriesRealTime() {}
  virtual void QueryComboSeriesRealTime() {}
  virtual void QueryInstrumentClass() {}

  virtual void CancelOrderExplicit(uint64_t order_num_, char buysell_, std::string symbol_) {}
  virtual void CancelOrderExplicitNormal(uint64_t order_num_, char buysell_, std::string symbol_) {}
  virtual void ExplicitModifyOrder(uint64_t order_num_, char buysell_, std::string symbol_, double price_, int qty_) {}
  virtual void ActivateOrder(uint64_t order_num_, char buysell_, std::string symbol_) {}
  virtual void SetOSEUserPassword(const unsigned int _user_id_, std::string new_password_) {}

  virtual void ProcessGeneralControlCommand(const char* input_stream, int stream_length) {}

  virtual void SetExchOrderType(int this_exch_order_type_) {}
  virtual void SetOrderValidity(int this_validity_) {}
  virtual void SendOrderExplicit(std::string exchange_symbol_, const int size_remaining_, const int price_,
                                 const char bid_ask_, const int saos_) {}
  virtual void CancelExplicitOrder(uint64_t order_number_, std::string exchange_symbol_, const char bid_ask_,
                                   const int saos_) {}

  void SetEngineID(int engine_id) { engine_id_ = engine_id; }
  int GetEngineID() { return engine_id_; }

  int AllocateToSiblingCore() {
    int core = CPUManager::GetCoreForProc("shm_client_receiver");
    if (core > 0) {
      if (!allocateToSiblingCPU(core)) return -1;
    }
    return core;
  }

//  bool IsOnloadMsgWarmSuppoted(int fd) {
//    if (m_settings.has("SendFakeTCPPackets") &&
//        std::string("Y") == std::string(m_settings.getValue("SendFakeTCPPackets"))) {
//      if (onload_fd_check_feature(fd, ONLOAD_FD_FEAT_MSG_WARM) > 0) {
//        std::cout << "ONLOAD MSG WARM FEATURE IS SUPPORTED FOR FD : " << fd << std::endl;
//        is_msg_warm_supported_ = true;
//        return true;
//      }
//    }
//    return false;
//  }

 protected:
  virtual void thread_main() = 0;

 protected:
  Settings m_settings;
  DebugLogger& dbglogger_;
  EngineListener* p_engine_listener_;
  ThrottleManager* p_throttler_;
  ThrottleManager* p_ioc_throttler_;

  bool allow_new_orders_;
  bool is_playback_mode_;
  int engine_id_;
  bool is_msg_warm_supported_;
};
}
}
#endif  // BASE_BASICORDERROUTINGSERVER_BASEENGINE_H
