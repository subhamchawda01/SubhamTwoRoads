/**
    \file dvccode/Utils/shm_queue.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#ifndef BASE_UTILS_SHM_QUEUE_H_
#define BASE_UTILS_SHM_QUEUE_H_

#define DCACHE1_LINESIZE 64
#define ____cacheline_aligned __attribute__((aligned(DCACHE1_LINESIZE)))

#include <algorithm>
#include <cstring>
#include <climits>
#include "dvccode/CDef/assumptions.hpp"

#define Q_SIZE 64

namespace HFSAT {

static size_t __thread __thr_id;

inline size_t thr_id() { return __thr_id; }

inline void set_thr_id(size_t id) { __thr_id = id; }

// Reference : http://www.linuxjournal.com/content/lock-free-multi-producer-multi-consumer-queue-ring-buffer

template <class T>
class ShmQueue {
 private:
  static const unsigned long Q_MASK = Q_SIZE - 1;

  struct ThrPos {
    unsigned long head;
  };

 protected:
  volatile unsigned int max_clientd_id_;
  volatile unsigned long head_ ____cacheline_aligned;
  // current tail, next to pop
  volatile unsigned long tail_ ____cacheline_aligned;
  // last not-processed producer's pointer
  volatile unsigned long last_head_ ____cacheline_aligned;
  volatile ThrPos thr_pos_[ORS_MAX_NUM_OF_CLIENTS];
  volatile T data_array_[Q_SIZE];

 public:
  ShmQueue() : head_(0), tail_(0), last_head_(0), max_clientd_id_(4) {
    // Set per thread tail and head to ULONG_MAX ;
    for (auto thread_id = 0; thread_id < ORS_MAX_NUM_OF_CLIENTS; thread_id++) {
      thr_pos_[thread_id].head = ULONG_MAX;
    }
  }

  void update_last_head() {
    auto min = head_;

    // Update the last_head_.
    for (size_t i = 0; i <= max_clientd_id_; ++i) {
      auto tmp_h = thr_pos_[i].head;

      // Force compiler to use tmp_h exactly once.
      asm volatile("" ::: "memory");

      if (tmp_h < min) min = tmp_h;
    }
    unsigned int snapped_last_head = last_head_;

    while (snapped_last_head < min && !__sync_bool_compare_and_swap(&last_head_, snapped_last_head, min)) {
      snapped_last_head = last_head_;
    }
  }

  void push(const T &value) {
    thr_pos_[__thr_id].head = head_;

    thr_pos_[__thr_id].head = __sync_fetch_and_add(&head_, 1);

    while (__builtin_expect(thr_pos_[__thr_id].head >= (tail_ + Q_SIZE), 0)) {
      update_last_head();
    }

    memcpy((void *)&data_array_[(thr_pos_[__thr_id].head & Q_MASK)], (void *)&value, sizeof(T));

    unsigned long tmp_head = thr_pos_[__thr_id].head;
    // Allow consumers eat the item.
    thr_pos_[__thr_id].head = ULONG_MAX;

    // Required for :
    // (a) The most common case, that producer is the only one and it can be consumed by just incrementing head by 1
    // (b) Act as a full memory barrier for the above head value to be reflected to other threads.
    __sync_bool_compare_and_swap(&last_head_, tmp_head, tmp_head + 1);
    update_last_head();
  }

  void pop(T &dest) {
    /*
     * tid'th place in ptr_array_ is reserved by the thread -
     * this place shall never be rewritten by push() and
     * last_tail_ at push() is a guarantee.
     * last_head_ guaraties that no any consumer eats the item
     * before producer reserved the position writes to it.
     */
    while (tail_ >= last_head_)
      ;

    memcpy((void *)&dest, (void *)&data_array_[tail_ & Q_MASK], sizeof(T));
    tail_++;
  }

  inline void SetNumClients(unsigned int client_id) {
    if (client_id > max_clientd_id_) {
      max_clientd_id_ = client_id;
    }
  }

  inline void reset() {
    head_ = 0;
    tail_ = 0;
    last_head_ = 0;

    for (auto thread_id = 0; thread_id < ORS_MAX_NUM_OF_CLIENTS; thread_id++) {
      thr_pos_[thread_id].head = ULONG_MAX;
    }
  }
};
}
#endif /* BASE_UTILS_SHM_QUEUE_H_ */
