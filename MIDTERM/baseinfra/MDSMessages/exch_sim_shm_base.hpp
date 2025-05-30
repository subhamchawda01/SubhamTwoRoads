// =====================================================================================
//
//       Filename:  exch_sim_shm_base.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  01/30/2014 12:39:20 PM
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
#include "dvccode/CDef/order.hpp"
#include "dvccode/Utils/exch_sim_shm_interface.hpp"

namespace HFSAT {

class ExchSimShmBase {
 protected:
  key_t shm_key_;
  int shmid_;
  volatile HFSAT::ORS::Order *order_shm_struct_;
  struct shmid_ds shmid_ds_;
  volatile int shm_index_;

 public:
  ExchSimShmBase() : shm_key_(EXCH_SIM_MDS_SHM_KEY), shmid_(-1), order_shm_struct_(NULL), shm_index_(0) {
    if ((shmid_ = shmget(shm_key_, (size_t)(EXCH_SIM_MDS_QUEUE_SIZE * (sizeof(HFSAT::ORS::Order)) + sizeof(int)),
                         IPC_CREAT | 0666)) < 0) {
      if (errno == EINVAL)
        std::cerr << "Invalid segment size specified \n";

      else if (errno == EEXIST)
        std::cerr << "Segment already exists \n";

      else if (errno == EIDRM)
        std::cerr << " Segment is marked for deletion \n";

      else if (errno == ENOENT)
        std::cerr << " Segment Doesn't Exist \n";

      else if (errno == EACCES)
        std::cerr << " Permission Denied \n";

      else if (errno == ENOMEM)
        std::cerr << " Not Enough Memory To Create Shm Segment \n";

      exit(1);
    }

    if ((order_shm_struct_ = (volatile HFSAT::ORS::Order *)shmat(shmid_, NULL, 0)) ==
        (volatile HFSAT::ORS::Order *)-1) {
      perror("shmat failed");
      exit(0);
    }

    if (-1 == (shmctl(shmid_, IPC_STAT, &shmid_ds_))) {
      perror("shmctl");
      exit(1);
    }

    if (1 == shmid_ds_.shm_nattch) {
      memset((void *)((HFSAT::ORS::Order *)order_shm_struct_), 0,
             (EXCH_SIM_MDS_QUEUE_SIZE * sizeof(HFSAT::ORS::Order) + sizeof(int)));
    }
  }

  virtual ~ExchSimShmBase() {}

  inline virtual void RunLiveShmSource() = 0;
};
}
