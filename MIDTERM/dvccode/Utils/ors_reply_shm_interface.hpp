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

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CDef/ors_reply_shm_interface_defines.hpp"
#include "dvccode/Utils/combined_source_generic_logger.hpp"
#include "dvccode/Utils/runtime_profiler.hpp"
#include "dvccode/Utils/shm1_queue.hpp"

namespace HFSAT {
namespace Utils {

class ORSReplyShmInterface {
 private:
  key_t key;
  int shmid;

  volatile HFSAT::MDS_MSG::GenericMDSMessage *mds_shm_queue_, *mds_shm_queue_pointer_;

  struct shmid_ds shm_ds;
  int count;
  int last_write_seq_num_;
  volatile int *shm_queue_index_;

  HFSAT::Shm1Queue<HFSAT::MDS_MSG::GenericMDSMessage> *shm_queue_;
  unsigned int queue_size_;

  // private constructor
  ORSReplyShmInterface()
      : key(ORS_REPLY_MDS_SHM_KEY),
        shmid(-1),
        shm_queue_(nullptr),
        queue_size_(sizeof(HFSAT::Shm1Queue<HFSAT::MDS_MSG::GenericMDSMessage>)) {}

  // disable copy construction
  ORSReplyShmInterface(const ORSReplyShmInterface &_disabled_copy_construction_);

 public:
  static ORSReplyShmInterface &GetUniqueInstance() {
    static ORSReplyShmInterface mds_shm_interface_;
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
             shmget(key, (size_t)(ORS_REPLY_MDS_QUEUE_SIZE * (sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) + sizeof(int)),
                    IPC_CREAT | 0666)) < 0) {
      std::cerr << "Sizeof Of the Segment "
                << (size_t)(ORS_REPLY_MDS_QUEUE_SIZE * (sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) + sizeof(int))
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
           (ORS_REPLY_MDS_QUEUE_SIZE * (sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) + sizeof(int)));
    mds_shm_queue_pointer_ = mds_shm_queue_;

    shm_queue_index_ = (volatile int *)(mds_shm_queue_ + ORS_REPLY_MDS_QUEUE_SIZE);
  }

  void InitializeThreadSafeSHM() {
    std::cout << "ORSReplyShmInterface::InitializeThreadSafeSHM: " << shmid << " KEY: " << key << " SIZE "
              << queue_size_ << std::endl;
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
    std::cout << "ORSReplyShmInterface::InitializeThreadSafeSHM: " << shmid << " KEY: " << key << std::endl;
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

  void Initialize() { InitializeThreadSafeSHM(); }

  void Push(HFSAT::MDS_MSG::GenericMDSMessage *_generic_mds_struct_) {
    memcpy((void *)mds_shm_queue_pointer_, (void *)(_generic_mds_struct_), sizeof(HFSAT::MDS_MSG::GenericMDSMessage));

    *shm_queue_index_ = last_write_seq_num_;

    last_write_seq_num_ = (last_write_seq_num_ + 1) & (ORS_REPLY_MDS_QUEUE_SIZE - 1);

    count++;

    if (last_write_seq_num_ == 0) {
      count = 0;

      mds_shm_queue_pointer_ = mds_shm_queue_;  // reset back to start of data

    } else {
      mds_shm_queue_pointer_++;
    }
  }

  void WriteGenericStructLockFree(HFSAT::MDS_MSG::GenericMDSMessage *_generic_mds_struct_) {
    shm_queue_->PushLockFree(*_generic_mds_struct_);
  }

  void WriteGenericStruct(HFSAT::MDS_MSG::GenericMDSMessage *_generic_mds_struct_) {
    shm_queue_->Push(*_generic_mds_struct_);
  }
};
}  // namespace Utils
}  // namespace HFSAT
