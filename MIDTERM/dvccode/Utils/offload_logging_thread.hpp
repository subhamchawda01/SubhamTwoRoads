// =====================================================================================
//
//       Filename:  offload_logging_thread.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  07/29/2014 02:31:02 PM
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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/offload_logging_file_manager.hpp"

#define MAX_NUMBER_OF_SPIN_ATTEMPTS 1024
#define SLEEP_INTERVAL_USEC 10000

namespace HFSAT {
namespace Utils {

class OffloadLoggerThread : public HFSAT::Thread {
 private:
  HFSAT::CDef::ClientMetaData client_metadata_;
  HFSAT::DebugLogger& dbglogger_;
  OffloadLoggingFileManager& offload_logging_file_manager_;
  HFSAT::CDef::LogBuffer log_buffer_struct_;
  std::string unique_logging_key_;

  key_t key_;
  int32_t shmid_;

  bool keep_running_;

  volatile HFSAT::CDef::LogBuffer* logger_shm_queue_;
  struct shmid_ds shm_ds_;
  int32_t index_;
  volatile int32_t* volatile_shm_position_ptr_;
  std::ofstream output_file;

 public:
  OffloadLoggerThread(const HFSAT::CDef::ClientMetaData& _client_metadata_, HFSAT::DebugLogger& _dbglogger_)
      : client_metadata_(_client_metadata_),
        dbglogger_(_dbglogger_),
        offload_logging_file_manager_(HFSAT::Utils::OffloadLoggingFileManager::GetUniqueInstance(dbglogger_)),
        log_buffer_struct_(),
        key_(_client_metadata_.assigned_shm_key_),
        shmid_(-1),
        keep_running_(false),
        logger_shm_queue_(NULL),
        shm_ds_(),
        index_(-1),
        volatile_shm_position_ptr_(NULL)

  {
    memset((void*)&log_buffer_struct_, 0, sizeof(HFSAT::CDef::LogBuffer));

    if ((shmid_ =
             shmget(key_, (size_t)(LOGGING_SEGMENTS_QUEUE_SIZE * (sizeof(HFSAT::CDef::LogBuffer)) + sizeof(int32_t)),
                    IPC_CREAT | 0666)) < 0) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO GET SHM SEGMENT, KEY : " << key_ << " SIZE OF THE SEGMENT : "
                                   << (size_t)(LOGGING_SEGMENTS_QUEUE_SIZE * (sizeof(HFSAT::CDef::LogBuffer)) +
                                               sizeof(int32_t))
                                   << " ERROR : " << strerror(errno) << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;

      exit(-1);
    }

    if ((volatile HFSAT::CDef::LogBuffer*)-1 ==
        (logger_shm_queue_ = (volatile HFSAT::CDef::LogBuffer*)shmat(shmid_, NULL, 0))) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO ATTACH TO SEGMENT, KEY : " << key_ << " SIZE OF THE SEGMENT : "
                                   << (size_t)(LOGGING_SEGMENTS_QUEUE_SIZE * (sizeof(HFSAT::CDef::LogBuffer)) +
                                               sizeof(int32_t))
                                   << " ERROR : " << strerror(errno) << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }

    if (-1 == shmctl(shmid_, IPC_STAT, &shm_ds_)) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO CONTROL SEGMENT, KEY : " << key_ << " SIZE OF THE SEGMENT : "
                                   << (size_t)(LOGGING_SEGMENTS_QUEUE_SIZE * (sizeof(HFSAT::CDef::LogBuffer)) +
                                               sizeof(int32_t))
                                   << " ERROR : " << strerror(errno) << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }

    if (1 == shm_ds_.shm_nattch) {
      memset((void*)logger_shm_queue_, 0,
             (LOGGING_SEGMENTS_QUEUE_SIZE * (sizeof(HFSAT::CDef::LogBuffer)) + sizeof(int32_t)));
    }

    volatile_shm_position_ptr_ = ((volatile int32_t*)(logger_shm_queue_ + LOGGING_SEGMENTS_QUEUE_SIZE));
    index_ = *volatile_shm_position_ptr_;

    std::ostringstream t_temp_oss;
    t_temp_oss << _client_metadata_.logging_directory_path_ << "/" << _client_metadata_.logfilename_;
    unique_logging_key_ = t_temp_oss.str();
  }

  ~OffloadLoggerThread() { CleanUp(); }

  void thread_main() {
    DBGLOG_CLASS_FUNC_LINE_INFO << "SEGMENT KEY : " << key_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    keep_running_ = true;

    while (keep_running_) {
      for (uint32_t number_of_idle_spin_counter = 0; number_of_idle_spin_counter < MAX_NUMBER_OF_SPIN_ATTEMPTS;
           number_of_idle_spin_counter++) {
        if (index_ == *volatile_shm_position_ptr_) {
          continue;
        }

        memcpy((void*)&log_buffer_struct_, (HFSAT::CDef::LogBuffer*)(logger_shm_queue_ + index_),
               sizeof(HFSAT::CDef::LogBuffer));

        HFSAT::CDef::BufferContentType buffer_type = log_buffer_struct_.content_type_;
        offload_logging_file_manager_.CommitDataToFile(unique_logging_key_, log_buffer_struct_.ToString().c_str(),
                                                       buffer_type);

        index_ = (index_ + 1) & (LOGGING_SEGMENTS_QUEUE_SIZE - 1);
      }

      HFSAT::usleep(SLEEP_INTERVAL_USEC);
    }

    shmdt((const void*)logger_shm_queue_);

    if (1 == shm_ds_.shm_nattch) {  // last attached consumer

      shmctl(shmid_, IPC_RMID, 0);
    }
  }

  void CleanUp() {
    keep_running_ = false;

    std::ostringstream t_temp_oss;
    t_temp_oss << client_metadata_.logging_directory_path_ << "/" << client_metadata_.logfilename_;
    unique_logging_key_ = t_temp_oss.str();

    // Close out the file
    offload_logging_file_manager_.CloseFile(unique_logging_key_);
  }
};
}
}
