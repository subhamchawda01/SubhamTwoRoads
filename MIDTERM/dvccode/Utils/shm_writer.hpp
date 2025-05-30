/**
    \file dvccode/Utils/shm_writer.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

*/

#pragma once

#include <iostream>

#include <errno.h>
#include <string.h>
#include <sys/shm.h>

namespace SHM {

template <class T>
class ShmWriter {
 private:
  key_t shm_key_;
  int shm_segment_size_;
  int shmid_;
  struct shmid_ds shmid_ds_;
  volatile int *seq_no_ptr_;
  volatile T *shm_queue_;
  volatile T *shm_queue_ptr_;

 public:
  ShmWriter(int t_key_, int t_segment_size_)
      : shm_key_(t_key_),
        shm_segment_size_(t_segment_size_),
        shmid_(-1),
        seq_no_ptr_(NULL),
        shm_queue_(NULL),
        shm_queue_ptr_(NULL) {
    shmid_ = shmget(shm_key_, (size_t)(shm_segment_size_ * sizeof(T) + sizeof(int)), IPC_CREAT | 0666);

    if (shmid_ < 0) {
      switch (errno) {
        case EINVAL:
          std::cerr << "Invalid segment size specified \n";
          break;
        case EEXIST:
          std::cerr << "Segment already exists \n";
          break;
        case EIDRM:
          std::cerr << "Segment is marked for deletion \n";
          break;
        case ENOENT:
          std::cerr << "Segment Doesn't Exist \n";
          break;
        case EACCES:
          std::cerr << "Permission Denied \n";
          break;
        case ENOMEM:
          std::cerr << "Not Enough Memory To Create Shm Segment \n";
          break;
        default:
          std::cerr << "Unknown error\n";
          break;
      }

      exit(1);
    }

    shm_queue_ = (volatile T *)shmat(shmid_, NULL, 0);

    if (shm_queue_ == (volatile T *)-1) {
      std::cerr << "shmat failed.\n";
      exit(1);
    }

    if (shmctl(shmid_, IPC_STAT, &shmid_ds_) == -1) {
      std::cerr << "shmctl failed.\n";
      exit(1);
    }

    std::cout << "Shmwriter: segment_size: " << shm_segment_size_ * sizeof(T) << " shm_key: " << shm_key_
              << " shmid: " << shmid_ << "\n";

    if (shmid_ds_.shm_nattch == 1) {
      memset((void *)((T *)shm_queue_), 0, (shm_segment_size_ * sizeof(T) + sizeof(int)));
    }

    shm_queue_ptr_ = shm_queue_;
    seq_no_ptr_ = (int *)(shm_queue_ + shm_segment_size_);

    int current_seq_number_ = *seq_no_ptr_;
    if (current_seq_number_ < 0 || current_seq_number_ >= shm_segment_size_) {
      std::cerr << "SHM: current_sequence_number: " << current_seq_number_ << std::endl;
    }

    shm_queue_ptr_ = shm_queue_ + current_seq_number_;
  }

  inline void Clear() {
    shmdt((void *)shm_queue_);

    if (shmctl(shmid_, IPC_STAT, &shmid_ds_) == -1) {
      std::cerr << "shmctl failed.\n";
      exit(1);
    }

    if (shmid_ds_.shm_nattch == 0) {
      shmctl(shmid_, IPC_RMID, 0);  // remove shm segment if no users are attached
    }
  }

  inline int GetAttachedProcessCount() { return shmid_ds_.shm_nattch; }

  inline void Write(T *t_struct_) {
    memcpy((void *)shm_queue_ptr_, (void *)t_struct_, sizeof(T));

    *seq_no_ptr_ = (*seq_no_ptr_ + 1) & (shm_segment_size_ - 1);

    if (*seq_no_ptr_ == 0) {
      shm_queue_ptr_ = shm_queue_;
    } else {
      shm_queue_ptr_++;
    }
  }
};
}
