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
#include "dvccode/Utils/tcp_server_socket.hpp"
#include "dvccode/Utils/shared_mem_writer.hpp"
#include "dvccode/Utils/shared_mem_reader.hpp"
#include "dvccode/Utils/CPUAffinity.hpp"

#include "dvccode/CommonDataStructures/simple_security_symbol_indexer.hpp"
#include "infracore/BasicOrderRoutingServer/sequence_generator.hpp"
#include "infracore/BasicOrderRoutingServer/order_manager.hpp"
#include "infracore/BasicOrderRoutingServer/client_request_processor.hpp"
#include "infracore/BasicOrderRoutingServer/margin_checker.hpp"
#include "infracore/BasicOrderRoutingServer/account_thread.hpp"
#include "infracore/BasicOrderRoutingServer/account_manager.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "infracore/BasicOrderRoutingServer/multisession_engine.hpp"
#include "dvccode/Utils/runtime_profiler.hpp"

#ifndef BASE_BASICORDERROUTINGSERVER_SHMCLIENTRECEIVER_H
#define BASE_BASICORDERROUTINGSERVER_SHMCLIENTRECEIVER_H

namespace HFSAT {
namespace ORS {

/// Separate thread that interacts with one client, receives client messages and acts accordingly
class ShmClientReceiver : public Thread, HFSAT::ShmWriterAddRemoveListner {
 private:
  std::set<int32_t> list_of_live_pids_;

 public:
  bool cancel_live_order_;
  ShmClientReceiver(DebugLogger& dbglogger, Settings& settings, std::string exch);
  virtual ~ShmClientReceiver();
  void addShmWriterAddRemoveListner() { reader->addShmWriterAddRemoveListner(this); }

  /// inherited from Thread class.
  /// listen to client, typically read GenericORSRequestStruct messages from client

  void thread_main();
  void AddWriter(int writer_id, int32_t const& writer_pid);
  void RemoveWriter(int writer_id, int32_t const& writer_pid);
  void StopClientThread();

 private:
  DebugLogger& dbglogger_;
  Settings& settings_;
  bool use_affinity_;
  SharedMemReader<GenericORSRequestStruct>* reader;
  std::set<int> active_clients_;
  HFSAT::ORS::ClientRequestProcessor* client_request_processor_;
  std::string exch;
  HFSAT::RuntimeProfiler& runtime_profiler_;
  uint64_t idle_cycles_start_;
  uint64_t last_heartbeat_start_;
  uint64_t READ_TRIGGER_CYCLES;
  bool use_lockfree_ors_;
  bool client_disconnect_;
  std::vector<int32_t> client_saci_disconnected_;
  uint64_t disconnect_cycle_count_;
};
}
}

#endif  //  BASE_BASICORDERROUTINGSERVER_SHMCLIENTRECEIVER_H
