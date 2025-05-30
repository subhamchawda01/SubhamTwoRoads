// =====================================================================================
//
//       Filename:  exch_sim_shm_interface.hpp
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

#include "dvccode/CDef/order.hpp"

#define EXCH_SIM_MDS_QUEUE_SIZE 1024
#define EXCH_SIM_MDS_SHM_KEY 8101

namespace HFSAT {
namespace Utils {

class ExchSimShmInterface {
 private:
  key_t key;
  int shmid;

  volatile HFSAT::ORS::Order *shm_queue_, *shm_queue_pointer_;

  struct shmid_ds shm_ds;
  int last_write_seq_num_;
  volatile int *shm_queue_index_;
  bool is_initialized_;

  // private constructor
  ExchSimShmInterface() : is_initialized_(false) {}

  // disable copy construction
  ExchSimShmInterface(const ExchSimShmInterface &_disabled_copy_construction_);

 public:
  static ExchSimShmInterface &GetUniqueInstance() {
    static ExchSimShmInterface shm_interface_;
    return shm_interface_;
  }

  void Initialize() {
    // data member initialization

    if (is_initialized_) return;

    key = EXCH_SIM_MDS_SHM_KEY;
    last_write_seq_num_ = 0;
    shmid = -1;
    shm_queue_ = shm_queue_pointer_ = NULL;

    if ((shmid = shmget(key, (size_t)(EXCH_SIM_MDS_QUEUE_SIZE * (sizeof(HFSAT::ORS::Order)) + sizeof(int)),
                        IPC_CREAT | 0666)) < 0) {
      std::cerr << "Sizeof Of the Segment "
                << (size_t)(EXCH_SIM_MDS_QUEUE_SIZE * (sizeof(HFSAT::ORS::Order)) + sizeof(int))
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

    if ((shm_queue_ = (volatile HFSAT::ORS::Order *)shmat(shmid, NULL, 0)) == (volatile HFSAT::ORS::Order *)-1) {
      perror("shmat");
      exit(0);
    }

    if (shmctl(shmid, IPC_STAT, &shm_ds) == -1) {
      perror("shmctl");
      exit(1);
    }

    memset((void *)shm_queue_, 0, (EXCH_SIM_MDS_QUEUE_SIZE * (sizeof(HFSAT::ORS::Order)) + sizeof(int)));
    shm_queue_pointer_ = shm_queue_;

    shm_queue_index_ = (volatile int *)(shm_queue_ + EXCH_SIM_MDS_QUEUE_SIZE);

    is_initialized_ = true;
  }

  void WriteOrderToExchSim(HFSAT::ORS::Order *order) {
    memcpy((void *)shm_queue_pointer_, (void *)(order), sizeof(HFSAT::ORS::Order));

    last_write_seq_num_++;

    *shm_queue_index_ = last_write_seq_num_;

    if ((last_write_seq_num_ % EXCH_SIM_MDS_QUEUE_SIZE) == 0) {
      shm_queue_pointer_ = shm_queue_;  // reset back to start of data
    } else {
      shm_queue_pointer_++;
    }
  }
};
}
}
