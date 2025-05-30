/**
   \file BasicOrderRoutingServer/shm_client_receiver.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
 */

#include <string.h>  // for memcpy

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ors_messages.hpp"
#include "infracore/BasicOrderRoutingServer/shm_client_receiver.hpp"

#define HEARTBEAT_CYCLES_DIFF 2500000000  // 1 sec
#define FAKEREQUEST_CYCLES_DIFF 7500000   // 3 mili

#define DISCONNECT_CHECK 7500000000  // 3sec

#define BSE_DEBUG_INFO 1

namespace HFSAT {
namespace ORS {

ShmClientReceiver::ShmClientReceiver(DebugLogger& dbglogger, Settings& settings, std::string exch)
    : cancel_live_order_(false),
      dbglogger_(dbglogger),
      settings_(settings),
      use_affinity_((settings.has("UseAffinity") && settings.getValue("UseAffinity") == "Y")),
      reader(new SharedMemReader<GenericORSRequestStruct>((atoi(settings.getValue("Client_Base").c_str())) << 16,
                                                          HFSAT::StringToExchSource(exch))),
      exch(exch),
      runtime_profiler_(HFSAT::RuntimeProfiler::GetUniqueInstance()),
      idle_cycles_start_(0),
      last_heartbeat_start_(HFSAT::GetCpucycleCountForTimeTick() + (uint64_t)(112500000000)),
      READ_TRIGGER_CYCLES(settings.getIntValue("ReadTriggerCycles", 30000)),
      use_lockfree_ors_(((settings.has("UseLockFreeORS") && settings.getValue("UseLockFreeORS") == "Y"))),
      client_disconnect_(false),
      client_saci_disconnected_(),
      disconnect_cycle_count_(0) {
    
     client_request_processor_ =
        new ClientRequestProcessor(dbglogger_, *(AccountManager::GetAccountThread()), settings_);

  DBGLOG_CLASS_FUNC_LINE_INFO << "READ TRIGGER CYCLES SET TO : " << READ_TRIGGER_CYCLES
                              << " Is LOCKFREE ORS ENABLED ? : " << use_lockfree_ors_ << DBGLOG_ENDL_DUMP;
}

ShmClientReceiver::~ShmClientReceiver() {}

void ShmClientReceiver::thread_main() {
  if (use_affinity_) {
    setName("shm_client_receiver");
    AllocateCPUOrExit();
  }

  DataWriterIdPair<GenericORSRequestStruct> ors_request;

  if (use_lockfree_ors_) {
    while (true) {
      // Get the write id, so we can know which client thread
      // this request is aimed towards.

      bool success = reader->readTNonBlocking(ors_request);

      if (success == true) {
        runtime_profiler_.Start(HFSAT::ProfilerType::kCOMBINEDSHMWRITER, ors_request.data.t2t_cshmw_start_time_);
        //        runtime_profiler_.End(HFSAT::ProfilerType::kCOMBINEDSHMWRITER, ors_request.data.t2t_cshmw_end_time_);
        //        runtime_profiler_.Start(HFSAT::ProfilerType::kTRADEINIT, ors_request.data.t2t_tradeinit_start_time_);
        //        runtime_profiler_.End(HFSAT::ProfilerType::kTRADEINIT, ors_request.data.t2t_tradeinit_end_time_);
        //        runtime_profiler_.Start(HFSAT::ProfilerType::kCMEILINKORS);

        client_request_processor_->ProcessClientRequest(ors_request.data, ors_request.writer_id);

        idle_cycles_start_ = 0;
        idle_cycles_start_ = HFSAT::GetCpucycleCountForTimeTick();

      } else {
        if (0 == idle_cycles_start_) {
          idle_cycles_start_ = HFSAT::GetCpucycleCountForTimeTick();
          continue;
        }

        uint64_t current_cycles = HFSAT::GetCpucycleCountForTimeTick();
        uint64_t cycles_diff = (current_cycles - idle_cycles_start_);

        // for now just using 3 if, as we can ideally put any values in any
        // TODO - move to a better branch construct with some assumptions

        if (cycles_diff > READ_TRIGGER_CYCLES) {
          // initiate exchange read now since we seem to be idle
          client_request_processor_->ProcessExchangeTCPReadRequest();

          cycles_diff = (current_cycles - last_heartbeat_start_);
          // Time for heartbeat
          if (cycles_diff > HEARTBEAT_CYCLES_DIFF) {
            client_request_processor_->ProcessHeartBeatRequest();
            last_heartbeat_start_ = HFSAT::GetCpucycleCountForTimeTick();
          }

          idle_cycles_start_ = 0;

          if (true == client_disconnect_) {
            uint64_t disconnect_since = (current_cycles - disconnect_cycle_count_);

            if(disconnect_cycle_count_ < current_cycles && disconnect_since > DISCONNECT_CHECK){

              //cancel all pending orders for disconnected clients
              for(auto & itr : client_saci_disconnected_){
                DBGLOG_CLASS_FUNC_LINE_INFO << "CANCELLING ORDERS FOR SACI : " << itr << DBGLOG_ENDL_DUMP ; 
                client_request_processor_->CancelPendingOrders(itr);
              }

              client_saci_disconnected_.clear();
              client_disconnect_ = false;
            }
          }
          if (true == cancel_live_order_) {
            //cancel all pending orders for active clients
            for(auto & itr : active_clients_) {
              DBGLOG_CLASS_FUNC_LINE_INFO << "CANCELLING ORDERS FOR SACI : " << itr << DBGLOG_ENDL_DUMP ;
              client_request_processor_->CancelPendingOrders(itr);
            }
            cancel_live_order_ = false;
          } 
        }
      }
    }

  } else {
    while (true) {
      // Get the write id, so we can know which client thread
      // this request is aimed towards.

      reader->readT(ors_request);

      runtime_profiler_.Start(HFSAT::ProfilerType::kCOMBINEDSHMWRITER, ors_request.data.t2t_cshmw_start_time_);
      //      runtime_profiler_.End(HFSAT::ProfilerType::kCOMBINEDSHMWRITER, ors_request.data.t2t_cshmw_end_time_);
      //      runtime_profiler_.Start(HFSAT::ProfilerType::kTRADEINIT, ors_request.data.t2t_tradeinit_start_time_);
      //      runtime_profiler_.End(HFSAT::ProfilerType::kTRADEINIT, ors_request.data.t2t_tradeinit_end_time_);
      //      runtime_profiler_.Start(HFSAT::ProfilerType::kCMEILINKORS);

      client_request_processor_->ProcessClientRequest(ors_request.data, ors_request.writer_id);
    }
  }
}

void ShmClientReceiver::RemoveWriter(int writer_id, int32_t const& writer_pid) {
  if (active_clients_.find(writer_id) == active_clients_.end()) {
    // error
    if (dbglogger_.CheckLoggingLevel(ORS_ERROR)) {
      std::cout << "writer id (" << writer_id << ") handler not found\n";
    }
  } else {
    // indicates at least some client has gotten disconnected
    client_saci_disconnected_.push_back(writer_id);
    disconnect_cycle_count_ = HFSAT::GetCpucycleCountForTimeTick();
    client_disconnect_ = true;

    // Some Base live trader wants to disconnect.
    // Continuing the previous semantics, we cancel all pending orders.
    //    client_request_processor_->CancelPendingOrders(writer_id);
    active_clients_.erase(writer_id);

    client_request_processor_->WriterRemoved(writer_id);
  }

  // Unconditionally check whether PID is already present
  if (list_of_live_pids_.end() != list_of_live_pids_.find(writer_pid)) {
    list_of_live_pids_.erase(list_of_live_pids_.find(writer_pid));
  }
}

void ShmClientReceiver::StopClientThread() {
  // Let's try to kill all these clients as we are going down
  for (auto& itr : list_of_live_pids_) {
    if (itr == getpid()) continue;
    std::ostringstream cmd_stream;
    cmd_stream << "sudo kill -9 " << itr;

    // Let's execute this as system cmd since we want sudo
    system(cmd_stream.str().c_str());
    std::cout << " Active Client : " << itr << " WILL BE NOTIFIED : " << cmd_stream.str() << std::endl;
  }

  reader->interruptRead();
  this->stop();
  reader->cleanUp();
}

void ShmClientReceiver::AddWriter(int writer_id, int32_t const& writer_pid) {
  // Track PID
  list_of_live_pids_.insert(writer_pid);
  if (active_clients_.find(writer_id) == active_clients_.end()) {
    client_request_processor_->WriterAdded(writer_id);
  }
  active_clients_.insert(writer_id);
}
}
}
