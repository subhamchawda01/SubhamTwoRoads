// =====================================================================================
//
//       Filename:  centralized_logging_manager.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  07/31/2014 09:36:45 AM
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

#include <iostream>
#include <string>
#include <sstream>
#include <signal.h>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/Utils/offload_shared_logging_manager.hpp"

HFSAT::Utils::OffloadLoggingManager* offload_shared_logging_manager_ptr = NULL;

void termination_handler(int signum) {
  if (NULL != offload_shared_logging_manager_ptr) {
    offload_shared_logging_manager_ptr->CleanUp();
  }

  exit(-1);
}

int main() {
  // signal handling, Interrupts and seg faults
  signal(SIGINT, termination_handler);
  signal(SIGKILL, termination_handler);
  signal(SIGSEGV, termination_handler);
  signal(SIGABRT, termination_handler);

  HFSAT::DebugLogger dbglogger_(1024000, 1);
  dbglogger_.OpenLogFile("/spare/local/logs/alllogs/logging_server.log", std::ofstream::out);

  HFSAT::Utils::OffloadLoggingManager& offload_shared_logging_manager =
      HFSAT::Utils::OffloadLoggingManager::GetUniqueInstance(dbglogger_);
  offload_shared_logging_manager_ptr = &offload_shared_logging_manager;

  offload_shared_logging_manager.WaitForLoggersAndCleanUpOnExit();

  return 0;
}
