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
#include "dvccode/CDef/bse_shm_interface_defines.hpp"
#include "dvccode/Utils/combined_source_generic_logger.hpp"
#include "dvccode/Utils/runtime_profiler.hpp"
#include "dvccode/Utils/shm1_queue.hpp"

namespace HFSAT {
namespace Utils {

class BSEMDSShmInterface {
 private:
  key_t key;
  int shmid;

  volatile EOBI_MDS::EOBICommonStruct *mds_shm_queue_, *mds_shm_queue_pointer_;

  struct shmid_ds shm_ds;
  int count;
  int last_write_seq_num_;
  volatile int *shm_queue_index_;

  HFSAT::Shm1Queue<EOBI_MDS::EOBICommonStruct> *shm_queue_;
  HFSAT::RuntimeProfiler &runtime_profiler_;
  unsigned int queue_size_;

  // private constructor
  BSEMDSShmInterface()
      : key(BSE_MDS_SHM_KEY),
        shmid(-1),
        shm_queue_(nullptr),
        runtime_profiler_(HFSAT::RuntimeProfiler::GetUniqueInstance()),
        queue_size_(sizeof(HFSAT::Shm1Queue<EOBI_MDS::EOBICommonStruct>)) {}

  // disable copy construction
  BSEMDSShmInterface(const BSEMDSShmInterface &_disabled_copy_construction_);

 public:
  static BSEMDSShmInterface &GetUniqueInstance() {
    static BSEMDSShmInterface mds_shm_interface_;
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
             shmget(key, (size_t)(BSE_MDS_QUEUE_SIZE * (sizeof(EOBI_MDS::EOBICommonStruct)) + sizeof(int)),
                    IPC_CREAT | 0666)) < 0) {
      std::cerr << "Sizeof Of the Segment "
                << (size_t)(BSE_MDS_QUEUE_SIZE * (sizeof(EOBI_MDS::EOBICommonStruct)) + sizeof(int))
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

    if ((mds_shm_queue_ = (volatile EOBI_MDS::EOBICommonStruct *)shmat(shmid, NULL, 0)) ==
        (volatile EOBI_MDS::EOBICommonStruct *)-1) {
      perror("shmat");
      exit(0);
    }

    if (shmctl(shmid, IPC_STAT, &shm_ds) == -1) {
      perror("shmctl");
      exit(1);
    }

    memset((void *)mds_shm_queue_, 0,
           (BSE_MDS_QUEUE_SIZE * (sizeof(EOBI_MDS::EOBICommonStruct)) + sizeof(int)));
    mds_shm_queue_pointer_ = mds_shm_queue_;

    shm_queue_index_ = (volatile int *)(mds_shm_queue_ + BSE_MDS_QUEUE_SIZE);
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

    if ((shm_queue_ = (HFSAT::Shm1Queue<EOBI_MDS::EOBICommonStruct> *)shmat(shmid, NULL, 0)) ==
        (HFSAT::Shm1Queue<EOBI_MDS::EOBICommonStruct> *)-1) {
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

  void Initialize() { InitializeThreadSafeSHM(); }

  void Push(EOBI_MDS::EOBICommonStruct *_generic_mds_struct_) {
    memcpy((void *)mds_shm_queue_pointer_, (void *)(_generic_mds_struct_),
           sizeof(EOBI_MDS::EOBICommonStruct));

    *shm_queue_index_ = last_write_seq_num_;

    last_write_seq_num_ = (last_write_seq_num_ + 1) & (BSE_MDS_QUEUE_SIZE - 1);

    count++;

    if (last_write_seq_num_ == 0) {
      count = 0;

      mds_shm_queue_pointer_ = mds_shm_queue_;  // reset back to start of data

    } else {
      mds_shm_queue_pointer_++;
    }
  }

  void WriteGenericStruct(EOBI_MDS::EOBICommonStruct *_generic_mds_struct_) {
    //    runtime_profiler_.End(HFSAT::ProfilerType::kCOMBINEDSHMWRITER);
    ////const ProfilerTimeInfo &time_info = runtime_profiler_.GetProfilerTimeInfo();

    //_generic_mds_struct_->t2t_cshmw_start_time = time_info.cshmw_start_time;
    //_generic_mds_struct_->t2t_cshw_end_time = 0;

    shm_queue_->PushLockFree(*_generic_mds_struct_);
  }
};
}
}
