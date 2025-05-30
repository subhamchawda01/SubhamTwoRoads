// =====================================================================================
//
//       Filename:  client_logger_shm_handler.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  07/31/2014 10:49:19 AM
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

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/shared_logging_defines.hpp"

namespace HFSAT {
namespace Utils {

class ClientLoggerShmHandler {
 private:
  HFSAT::DebugLogger &dbglogger_;
  key_t key_;
  int shmid_;

  volatile HFSAT::CDef::LogBuffer *logger_shm_queue_, *logger_shm_queue_pointer_;

  struct shmid_ds shm_ds_;
  int last_write_seq_num_;
  volatile int *shm_queue_index_;

 public:
  ClientLoggerShmHandler(HFSAT::DebugLogger &_dbglogger_, const key_t &_key_)
      : dbglogger_(_dbglogger_),
        key_(_key_),
        shmid_(-1),
        logger_shm_queue_(NULL),
        shm_ds_(),
        last_write_seq_num_(0),
        shm_queue_index_(NULL)

  {
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

    if ((volatile HFSAT::CDef::LogBuffer *)-1 ==
        (logger_shm_queue_ = (volatile HFSAT::CDef::LogBuffer *)shmat(shmid_, NULL, 0))) {
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

    memset((void *)logger_shm_queue_, 0,
           (LOGGING_SEGMENTS_QUEUE_SIZE * (sizeof(HFSAT::CDef::LogBuffer)) + sizeof(int32_t)));
    logger_shm_queue_pointer_ = logger_shm_queue_;
    shm_queue_index_ = (volatile int32_t *)(logger_shm_queue_ + LOGGING_SEGMENTS_QUEUE_SIZE);
  }

  void Log(const HFSAT::CDef::LogBuffer *_log_buffer_struct_) {
    memcpy((void *)logger_shm_queue_pointer_, (void *)_log_buffer_struct_, sizeof(HFSAT::CDef::LogBuffer));

    last_write_seq_num_ = *shm_queue_index_;
    last_write_seq_num_ = (last_write_seq_num_ + 1) & (LOGGING_SEGMENTS_QUEUE_SIZE - 1);
    *shm_queue_index_ = last_write_seq_num_;

    if (0 == last_write_seq_num_) {
      logger_shm_queue_pointer_ = logger_shm_queue_;

    } else {
      logger_shm_queue_pointer_++;
    }
  }

  void CleanUp() {
    shmdt((const void *)logger_shm_queue_);

    if (1 == shm_ds_.shm_nattch) {  // last attached consumer

      shmctl(shmid_, IPC_RMID, 0);
    }
  }
};
}
}
