// =====================================================================================
//
//       Filename:  client_logging_segment_initializer.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  07/31/2014 11:08:57 AM
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

#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <pthread.h>
#include <string>
#include <iostream>
#include <errno.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/shared_logging_defines.hpp"
#include "dvccode/Utils/client_logger_shm_handler.hpp"
#include "dvccode/Utils/sem_utils.hpp"

#define MAX_NUMBER_OF_SPIN_ATTEMPTS 50000
#define SLEEP_TIMEOUT 10

namespace HFSAT {
namespace Utils {

class ClientLoggingSegmentInitializer {
 private:
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::Utils::ClientLoggerShmHandler* client_logger_shm_handler_;

 protected:
  int32_t assigned_client_id_;
  int32_t logging_id_;
  int32_t semaphore_lock_id_;
  int32_t shm_segment_id_;
  int32_t assigned_segment_key_;
  char logging_directory_[512];
  char logfilename_[256];

 public:
  void Initialize() {
    if (-1 == shm_segment_id_ || -1 == semaphore_lock_id_) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO INITIALIZE, START THE LOGGING SERVER FIRST" << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;

      exit(-1);
    }

    bool is_id_requested_ = false;

    volatile HFSAT::CDef::ClientMetaData* client_metadata =
        (volatile HFSAT::CDef::ClientMetaData*)shmat(shm_segment_id_, 0, 0);

    for (uint32_t spin_counter = 0; spin_counter < MAX_NUMBER_OF_SPIN_ATTEMPTS; spin_counter++) {
      SemUtils::wait_and_lock(semaphore_lock_id_);

      if (is_id_requested_) {
        if (-1 != client_metadata->assigned_client_id_) {
          assigned_client_id_ = client_metadata->assigned_client_id_;
          assigned_segment_key_ = client_metadata->assigned_shm_key_;
          client_metadata->assigned_client_id_ = -2;
          SemUtils::signal_and_unlock(semaphore_lock_id_);
          shmdt((const void*)client_metadata);
          return;
        }

      } else {
        if (-2 == client_metadata->assigned_client_id_) {
          client_metadata->client_pid_ = getpid();
          client_metadata->assigned_client_id_ = -1;
          client_metadata->client_provided_logging_id_ = logging_id_;
          memcpy((void*)client_metadata->logging_directory_path_, (void*)logging_directory_, 512);
          memcpy((void*)client_metadata->logfilename_, (void*)logfilename_, 256);

          DBGLOG_CLASS_FUNC_LINE_INFO << " PATH : " << std::string((char*)client_metadata->logging_directory_path_)
                                      << " " << std::string((char*)client_metadata->logfilename_) << "\n";
          DBGLOG_DUMP;

          is_id_requested_ = true;
        }
      }

      SemUtils::signal_and_unlock(semaphore_lock_id_);
      usleep(SLEEP_TIMEOUT);
    }

    SemUtils::wait_and_lock(semaphore_lock_id_);
    client_metadata->client_pid_ = -1;
    client_metadata->assigned_client_id_ = -2;
    SemUtils::signal_and_unlock(semaphore_lock_id_);

    shmdt((const void*)client_metadata);
  }

  ClientLoggingSegmentInitializer(HFSAT::DebugLogger& _dbglogger_, const int32_t& _logging_id_,
                                  const std::string& _logging_directory_, const std::string& _logfilename_)
      : dbglogger_(_dbglogger_),
        client_logger_shm_handler_(NULL),
        assigned_client_id_(-1),
        logging_id_(_logging_id_),
        semaphore_lock_id_(-1),
        shm_segment_id_(-1),
        assigned_segment_key_(-1),
        logging_directory_(),
        logfilename_()

  {
    memset((void*)logging_directory_, 0, 512);
    memset((void*)logfilename_, 0, 256);

    memcpy((void*)logging_directory_, (void*)_logging_directory_.c_str(), _logging_directory_.length());
    memcpy((void*)logfilename_, (void*)_logfilename_.c_str(), _logfilename_.length());

    shm_segment_id_ = shmget(LOGGING_MANAGER_SHM_KEY, sizeof(HFSAT::CDef::ClientMetaData), 0);
    semaphore_lock_id_ = semget(LOGGING_MANAGER_SEMAPHORE_KEY, 1, 0);

    assigned_client_id_ = -1;

    Initialize();

    DBGLOG_CLASS_FUNC_LINE_INFO << "ASSIGNED CLIENT ID : " << assigned_client_id_
                                << " ASSIGNED SEGMENT : " << assigned_segment_key_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    if (-1 == assigned_client_id_) {
      DBGLOG_CLASS_FUNC_LINE_FATAL
          << "FAILED TO OBTAIN CLIENT ID FROM SERVER, IS SERVER RUNNING ? AND ON CORRECT SEGMENT ?"
          << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      exit(-1);

    } else {
      client_logger_shm_handler_ = new HFSAT::Utils::ClientLoggerShmHandler(dbglogger_, assigned_segment_key_);
    }
  }

  void Log(const HFSAT::CDef::LogBuffer* _data_) { client_logger_shm_handler_->Log(_data_); }

  void CleanUp() {
    if (NULL != client_logger_shm_handler_) {
      client_logger_shm_handler_->CleanUp();
      delete client_logger_shm_handler_;
      client_logger_shm_handler_ = NULL;
    }
  }

  ~ClientLoggingSegmentInitializer() { CleanUp(); }
};
}
}
