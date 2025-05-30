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
#include "infracore/BasicOrderRoutingServer/client_data_receiver.hpp"

#define HEARTBEAT_CYCLES_DIFF 2500000000  // 1 sec
#define FAKEREQUEST_CYCLES_DIFF 7500000   // 3 mili

#define DISCONNECT_CHECK 7500000000  // 3sec

#define BSE_DEBUG_INFO 1

namespace HFSAT {
namespace ORS {

ClientDataReceiver::ClientDataReceiver(DebugLogger& dbglogger, Settings& settings, std::string exch)
    : dbglogger_(dbglogger),
      settings_(settings),
      use_affinity_((settings.has("UseAffinity") && settings.getValue("UseAffinity") == "Y")),
      exch(exch),
      idle_cycles_start_(0),
      last_heartbeat_start_(HFSAT::GetCpucycleCountForTimeTick() + (uint64_t)(112500000000)),
      READ_TRIGGER_CYCLES(settings.getIntValue("ReadTriggerCycles", 30000)){
    
     client_request_processor_ =
        new ClientRequestProcessor(dbglogger_, *(AccountManager::GetAccountThread()), settings_);

  DBGLOG_CLASS_FUNC_LINE_INFO << "READ TRIGGER CYCLES SET TO : " << READ_TRIGGER_CYCLES << DBGLOG_ENDL_DUMP;
}

ClientDataReceiver::~ClientDataReceiver() {}

void ClientDataReceiver::thread_main() {
  if (use_affinity_) {
    setName("client_data_receiver");
    AllocateCPUOrExit();
  }
  bool use_data_over_html=false;
  if (settings_.has("USE_DATA_OVER_HTML") && (settings_.getValue("USE_DATA_OVER_HTML")) == std::string("Y")) {
    use_data_over_html=true;
    DBGLOG_CLASS_FUNC_LINE_FATAL << "USE_DATA_OVER_HTML " <<use_data_over_html<< DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
  }

  if (use_data_over_html) {
    while (true) {

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
        }
    }

  }
}

}
}
