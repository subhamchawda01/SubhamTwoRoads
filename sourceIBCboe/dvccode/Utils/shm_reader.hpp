/**
    \file dvccode/Utils/shm_reader.hpp

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
class ShmReader {
 private:
  key_t shm_key_;
  int shm_segment_size_;
  int shmid_;
  struct shmid_ds shmid_ds_;
  volatile T *shm_struct_;
  volatile int *queue_position_;
  int index_;

  T next_event_;

 public:
  ShmReader(int t_key_, int t_segment_size_)
      : shm_key_(t_key_),
        shm_segment_size_(t_segment_size_),
        shmid_(-1),
        shm_struct_(NULL),
        queue_position_(NULL),
        index_(-1),
        next_event_() {
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

    shm_struct_ = (volatile T *)shmat(shmid_, NULL, 0);

    if (shm_struct_ == (volatile T *)-1) {
      std::cerr << "shmat failed.\n";
      exit(1);
    }

    if (shmctl(shmid_, IPC_STAT, &shmid_ds_) == -1) {
      std::cerr << "shmctl failed.\n";
      exit(1);
    }

    std::cout << "Shmreader: segment_size: " << shm_segment_size_ * sizeof(T) << " shm_key: " << shm_key_
              << " shmid: " << shmid_ << "\n";

    if (shmid_ds_.shm_nattch == 1) {
      memset((void *)((T *)shm_struct_), 0, (shm_segment_size_ * sizeof(T) + sizeof(int)));
    }

    queue_position_ = (volatile int *)(shm_struct_ + shm_segment_size_);
    index_ = *queue_position_;
  }

  void Clear() {
    shmdt((void *)shm_struct_);

    if (shmctl(shmid_, IPC_STAT, &shmid_ds_) == -1) {
      std::cerr << "shmctl failed.\n";
      exit(1);
    }

    if (shmid_ds_.shm_nattch == 0) {
      shmctl(shmid_, IPC_RMID, 0);  // remove shm segment if no users are attached
    }
  }

  T *ReadNextEvent() {
    if (index_ == *queue_position_) {
      return NULL;
    }

    memcpy(&next_event_, (T *)(shm_struct_ + index_), sizeof(T));

    index_ = (index_ + 1) & (shm_segment_size_ - 1);

    return &next_event_;
  }
};
}
