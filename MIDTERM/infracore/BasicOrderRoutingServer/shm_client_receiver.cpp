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

namespace HFSAT {
namespace ORS {

ShmClientReceiver::ShmClientReceiver(DebugLogger& dbglogger, Settings& settings, std::string exch)
    : dbglogger_(dbglogger),
      settings_(settings),
      use_affinity_((settings.has("UseAffinity") && settings.getValue("UseAffinity") == "Y")),
      reader(new SharedMemReader<GenericORSRequestStruct>((atoi(settings.getValue("Client_Base").c_str())) << 16,
                                                          HFSAT::StringToExchSource(exch))),
      exch(exch) {
  if (exch == "BMFEP" || "BMFEQ" == exch || "NSE_FO" == exch || "NSE_CD" == exch ||
      "NSE_EQ" == exch || "MSSIM" == exch) {
    client_request_processor_ =
        new ClientRequestProcessor(dbglogger_, *(AccountManager::GetAccountThread()), settings_, true);

  } else {
    client_request_processor_ =
        new ClientRequestProcessor(dbglogger_, *(AccountManager::GetAccountThread()), settings_, false);
  }
}

ShmClientReceiver::~ShmClientReceiver() {}

void ShmClientReceiver::thread_main() {
  if (use_affinity_) {
    setName("shm_client_receiver");
    AllocateCPUOrExit();
  }

  DataWriterIdPair<GenericORSRequestStruct> ors_request;

  while (true) {
    // Get the write id, so we can know which client thread
    // this request is aimed towards.

    reader->readT(ors_request);
    client_request_processor_->ProcessClientRequest(ors_request.data, ors_request.writer_id);
  }
}

void ShmClientReceiver::RemoveWriter(int writer_id, int32_t const& writer_pid) {
  if (active_clients_.find(writer_id) == active_clients_.end()) {
    // error
    if (dbglogger_.CheckLoggingLevel(ORS_ERROR)) {
      std::cout << "writer id (" << writer_id << ") handler not found\n";
    }
  } else {
    // Some Base live trader wants to disconnect.
    // Continuing the previous semantics, we cancel all pending orders.
    client_request_processor_->CancelPendingOrders(writer_id);
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
