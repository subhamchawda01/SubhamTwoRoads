/**
   \file dvccode/CommonDataStructures/simple_mempool.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/
#ifndef BASE_COMMONDATASTRUCTURES_THREADSAFE_SIMPLE_MEMPOOL_H
#define BASE_COMMONDATASTRUCTURES_THREADSAFE_SIMPLE_MEMPOOL_H

#include <iostream>
#include <stdlib.h>

#include <stack>

namespace HFSAT {

#define MEMPOOL_INCR_LEN 16

/// \brief a class to reduce the number of system calls to allocate memory and reuse previously allocated memory
template <class T>
class ThreadSafeSimpleMempool {
 protected:
  unsigned int capacity_;

  T** memstack_;
  unsigned int size_;
  T parent;
  bool to_replicate_;
  std::vector<T*> calloc_markers_;
  std::vector<T*> mem_pool_;

 public:
  /** Constructor default maxsize = MEMPOOL_INCR_LEN */
  ThreadSafeSimpleMempool(unsigned int _capacity_ = MEMPOOL_INCR_LEN)
      : capacity_(std::max((unsigned int)MEMPOOL_INCR_LEN, _capacity_)), size_(0), to_replicate_(false) {
    T* new_block = (T*)calloc(capacity_, sizeof(T));
    calloc_markers_.push_back(new_block);
    memstack_ = (T**)calloc(capacity_, sizeof(T*));

    for (auto i = 0u; i < capacity_; i++) {
      memstack_[i] = new_block + i;
      mem_pool_.push_back(new_block + i);
    }
  }

  ThreadSafeSimpleMempool(unsigned int _capacity_, T& source)
      : capacity_(std::max((unsigned int)MEMPOOL_INCR_LEN, _capacity_)), size_(0), parent(source), to_replicate_(true) {
    T* new_block = (T*)calloc(capacity_, sizeof(T));
    calloc_markers_.push_back(new_block);
    memstack_ = (T**)calloc(capacity_, sizeof(T*));

    for (auto i = 0u; i < capacity_; i++) {
      memstack_[i] = new_block + i;
      memcpy(memstack_[i], &parent, sizeof(T));
      mem_pool_.push_back(new_block + i);
    }
  }
  ~ThreadSafeSimpleMempool() {
    for (size_t i = 0; i < calloc_markers_.size(); i++) {
      free(calloc_markers_[i]);
    }
    free(memstack_);
  }

  /** \brief Allocate new memory */
  T* Alloc() {
    if (size_ == capacity_) {
      free(memstack_);
      memstack_ = (T**)calloc(2 * capacity_, sizeof(T*));

      T* new_block = (T*)calloc(capacity_, sizeof(T));
      calloc_markers_.push_back(new_block);  // this is only for destructor
      for (auto i = 0u; i < capacity_; i++) {
        memstack_[capacity_ + i] = new_block + i;
        mem_pool_.push_back(new_block + i);
      }

      if (to_replicate_)
        for (auto i = 0u; i < capacity_; i++) memcpy(memstack_[capacity_ + i], &parent, sizeof(T));

      capacity_ = 2 * capacity_;
    }

    T* next_ = memstack_[size_++];
    return next_;
  }

  void DeAlloc(T* _old_data_) {
    // making function thread safe
    int one_ = 1;
    __sync_sub_and_fetch(&size_, one_);
    memstack_[size_] = _old_data_;
  }
};
}
#endif  // BASE_COMMONDATASTRUCTURES_SIMPLE_MEMPOOL_H
