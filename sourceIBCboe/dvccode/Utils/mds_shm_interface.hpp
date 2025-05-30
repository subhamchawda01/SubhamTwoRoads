// =====================================================================================
//
//       Filename:  mds_shm_interface.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  01/05/2014 08:38:48 AM
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

#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CDef/mds_shm_interface_defines.hpp"
#include "dvccode/Utils/combined_source_generic_logger.hpp"
#include "dvccode/Utils/runtime_profiler.hpp"
#include "dvccode/Utils/shm1_queue.hpp"

namespace HFSAT {
namespace Utils {

class MDSShmInterface {
 private:
  key_t key;
  int shmid;

  volatile HFSAT::MDS_MSG::GenericMDSMessage *mds_shm_queue_, *mds_shm_queue_pointer_;

  struct shmid_ds shm_ds;
  int count;
  int last_write_seq_num_;
  volatile int *shm_queue_index_;

  HFSAT::Utils::CombinedSourceGenericLogger &logger_;
  HFSAT::Shm1Queue<HFSAT::MDS_MSG::GenericMDSMessage> *shm_queue_;
  unsigned int queue_size_;
  HFSAT::RuntimeProfiler &runtime_profiler_;
  bool using_thread_safe_shm_;

  // private constructor
  MDSShmInterface(bool run_logger_thread = true)
      : key(GENERIC_MDS_SHM_KEY),
        shmid(-1),
        logger_(HFSAT::Utils::CombinedSourceGenericLogger::GetUniqueInstance()),
        shm_queue_(nullptr),
        queue_size_(sizeof(HFSAT::Shm1Queue<HFSAT::MDS_MSG::GenericMDSMessage>)),
        runtime_profiler_(HFSAT::RuntimeProfiler::GetUniqueInstance()),
        using_thread_safe_shm_(HFSAT::UseShmforORSReply()) {
    if (run_logger_thread) logger_.RunLoggerThread();
  }

  // disable copy construction
  MDSShmInterface(const MDSShmInterface &_disabled_copy_construction_);

 public:
  static MDSShmInterface &GetUniqueInstance(bool run_logger_thread = true) {
    static MDSShmInterface mds_shm_interface_(run_logger_thread);
    return mds_shm_interface_;
  }

  int32_t ConsumersAttached() {
    int32_t shmctl_ret_val = shmctl(shmid, IPC_STAT, &shm_ds);
    return (shmctl_ret_val < 0 ? 0 : shm_ds.shm_nattch);
  }

  void InitializeOldSHM() {
    // data member initialization
    last_write_seq_num_ = 0;
    count = 0;
    mds_shm_queue_ = mds_shm_queue_pointer_ = NULL;

    if ((shmid =
             shmget(key, (size_t)(GENERIC_MDS_QUEUE_SIZE * (sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) + sizeof(int)),
                    IPC_CREAT | 0666)) < 0) {
      std::cerr << "Sizeof Of the Segment "
                << (size_t)(GENERIC_MDS_QUEUE_SIZE * (sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) + sizeof(int))
                << " Key Value : " << key << "\n";
      std::cerr << "Failed to get segment : " << strerror(errno) << "\n";

      if (errno == EINVAL)
        printf("Invalid segment size specified\n");
      else if (errno == EEXIST)
        printf("Segment exists, cannot create it\n");
      else if (errno == EIDRM)
        printf("Segment is marked for deletion or was removed\n");
      else if (errno == ENOENT)
        printf("Segment does not exist\n");
      else if (errno == EACCES)
        printf("Permission denied\n");
      else if (errno == ENOMEM)
        printf("Not enough memory to create segment\n");

      exit(1);
    }

    if ((mds_shm_queue_ = (volatile HFSAT::MDS_MSG::GenericMDSMessage *)shmat(shmid, NULL, 0)) ==
        (volatile HFSAT::MDS_MSG::GenericMDSMessage *)-1) {
      perror("shmat");
      exit(0);
    }

    if (shmctl(shmid, IPC_STAT, &shm_ds) == -1) {
      perror("shmctl");
      exit(1);
    }

    memset((void *)mds_shm_queue_, 0,
           (GENERIC_MDS_QUEUE_SIZE * (sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) + sizeof(int)));
    mds_shm_queue_pointer_ = mds_shm_queue_;

    shm_queue_index_ = (volatile int *)(mds_shm_queue_ + GENERIC_MDS_QUEUE_SIZE);
  }

  void InitializeThreadSafeSHM() {
    if ((shmid = shmget(key, queue_size_, IPC_CREAT | 0666)) < 0) {
      std::cerr << "Sizeof Of the Segment " << queue_size_ << " Key Value : " << key << "\n";
      std::cerr << "Failed to get segment : " << strerror(errno) << "\n";

      if (errno == EINVAL)
        printf("Invalid segment size specified\n");
      else if (errno == EEXIST)
        printf("Segment exists, cannot create it\n");
      else if (errno == EIDRM)
        printf("Segment is marked for deletion or was removed\n");
      else if (errno == ENOENT)
        printf("Segment does not exist\n");
      else if (errno == EACCES)
        printf("Permission denied\n");
      else if (errno == ENOMEM)
        printf("Not enough memory to create segment\n");

      exit(1);
    }

    if ((shm_queue_ = (HFSAT::Shm1Queue<HFSAT::MDS_MSG::GenericMDSMessage> *)shmat(shmid, NULL, 0)) ==
        (HFSAT::Shm1Queue<HFSAT::MDS_MSG::GenericMDSMessage> *)-1) {
      perror("shmat");
      exit(0);
    }

    if (shmctl(shmid, IPC_STAT, &shm_ds) == -1) {
      perror("shmctl");
      exit(1);
    }

    if (ConsumersAttached() == 1) {
      memset((void *)shm_queue_, 0, queue_size_);
      shm_queue_->Reset();
    }
  }

  void Initialize() {
    if (using_thread_safe_shm_)
      InitializeThreadSafeSHM();
    else
      InitializeOldSHM();
  }

  void Push(HFSAT::MDS_MSG::GenericMDSMessage *_generic_mds_struct_) {
    memcpy((void *)mds_shm_queue_pointer_, (void *)(_generic_mds_struct_), sizeof(HFSAT::MDS_MSG::GenericMDSMessage));

    *shm_queue_index_ = last_write_seq_num_;

    last_write_seq_num_ = (last_write_seq_num_ + 1) & (GENERIC_MDS_QUEUE_SIZE - 1);

    count++;

    if (last_write_seq_num_ == 0) {
      count = 0;

      mds_shm_queue_pointer_ = mds_shm_queue_;  // reset back to start of data

    } else {
      mds_shm_queue_pointer_++;
    }
  }

  void PushThreadSafe(HFSAT::MDS_MSG::GenericMDSMessage *_generic_mds_struct_) {
    shm_queue_->Push(*_generic_mds_struct_);
  }

  void WriteGenericStruct(HFSAT::MDS_MSG::GenericMDSMessage *_generic_mds_struct_, bool log_to_generic_enabled = true,
                          bool is_ors = false) {
    runtime_profiler_.End(HFSAT::ProfilerType::kCOMBINEDSHMWRITER);
    const ProfilerTimeInfo &time_info = runtime_profiler_.GetProfilerTimeInfo();

    if (is_ors) {
      _generic_mds_struct_->t2t_cshmw_start_time_ = 0;
      _generic_mds_struct_->t2t_cshmw_time_ = 0;
    } else {
      _generic_mds_struct_->t2t_cshmw_start_time_ = time_info.cshmw_start_time;
      _generic_mds_struct_->t2t_cshmw_time_ = time_info.cshmw_end_time - time_info.cshmw_start_time;
    }

    if (!using_thread_safe_shm_)
      Push(_generic_mds_struct_);
    else
      PushThreadSafe(_generic_mds_struct_);

    if (log_to_generic_enabled) {
      // This is a temporary hack to deploy Request Based Combined Writer incrementally across all exchanges. This would
      // be removed after some time. For details contact pranjal/vedant.
      logger_.Log(*_generic_mds_struct_);
    }
  }
};
}
}
