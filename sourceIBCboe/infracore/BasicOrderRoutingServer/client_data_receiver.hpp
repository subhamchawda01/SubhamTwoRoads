/**
   \file BasicOrderRoutingServer/shm_client_receiver.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
 */

#include <tr1/unordered_map>

#include "dvccode/CDef/debug_logger.hpp"

#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/CPUAffinity.hpp"
#include "infracore/BasicOrderRoutingServer/account_thread.hpp"
#include "infracore/BasicOrderRoutingServer/account_manager.hpp"
#include "infracore/BasicOrderRoutingServer/client_request_processor.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"

#ifndef BASE_BASICORDERROUTINGSERVER_SHMCLIENTRECEIVER_H
#define BASE_BASICORDERROUTINGSERVER_SHMCLIENTRECEIVER_H

namespace HFSAT {
namespace ORS {

/// Separate thread that interacts with one client, receives client messages and acts accordingly
class ClientDataReceiver : public Thread {
 private:
  std::set<int32_t> list_of_live_pids_;

 public:
  bool cancel_live_order_=false;
  ClientDataReceiver(DebugLogger& dbglogger, Settings& settings, std::string exch);
  virtual ~ClientDataReceiver();

  /// inherited from Thread class.
  void thread_main();

 private:
  DebugLogger& dbglogger_;
  Settings& settings_;
  bool use_affinity_;
  HFSAT::ORS::ClientRequestProcessor* client_request_processor_;
  std::string exch;
  uint64_t idle_cycles_start_;
  uint64_t last_heartbeat_start_;
  uint64_t READ_TRIGGER_CYCLES;
};
}
}

#endif  //  BASE_BASICORDERROUTINGSERVER_SHMCLIENTRECEIVER_H
