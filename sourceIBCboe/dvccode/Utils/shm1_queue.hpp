/**
    \file dvccode/Utils/shm1_queue.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#ifndef BASE_UTILS_SHM1_QUEUE_H_
#define BASE_UTILS_SHM1_QUEUE_H_

#include <algorithm>
#include <cstring>
#include <climits>
#include "dvccode/CDef/ors_reply_shm_interface_defines.hpp"
#include "dvccode/Utils/sem_utils.hpp"

#define Q_SIZE_1 131072

namespace HFSAT {

template <class T>
class Shm1Queue {
 private:
  static const unsigned long Q_MASK = Q_SIZE_1 - 1;

 protected:
  volatile T data_array_[Q_SIZE_1];
  volatile uint64_t writer_pos_;
  volatile int mutex_;

 public:
  Shm1Queue() : writer_pos_(0) {}

  void Push(T &value) {
    HFSAT::SemUtils::spin_lock(&mutex_);
    memcpy((void *)&data_array_[writer_pos_ & Q_MASK], (void *)&value, sizeof(T));
    writer_pos_++;
    HFSAT::SemUtils::spin_unlock(&mutex_);
  }

  void PushLockFree(T &value) {
    memcpy((void *)&data_array_[writer_pos_ & Q_MASK], (void *)&value, sizeof(T));
    writer_pos_++;
  }

  void Pop(T &value, int64_t index) { memcpy((void *)&value, (void *)&data_array_[index & Q_MASK], sizeof(T)); }

  void Reset() { writer_pos_ = 0; }

  inline uint64_t writer_pos() { return writer_pos_; }
};
}
#endif /* BASE_UTILS_SHM1_QUEUE_H_ */
