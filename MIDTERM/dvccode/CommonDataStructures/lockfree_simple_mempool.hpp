
#pragma once
#include <boost/lockfree/stack.hpp>
#define MEMPOOL_INCR_LEN 16

template <class ObjectType>
class LockFreeSimpleMempool {
 private:
  unsigned int capacity_;
  boost::lockfree::stack<ObjectType*> object_pool_;
  ObjectType** memstack_;

 public:
  LockFreeSimpleMempool(unsigned int capacity = MEMPOOL_INCR_LEN)
      : capacity_(std::max((unsigned int)MEMPOOL_INCR_LEN, capacity)), object_pool_(capacity_) {
    ObjectType* new_object_block = (ObjectType*)calloc(capacity_, sizeof(ObjectType));
    memstack_ = (ObjectType**)calloc(capacity_, sizeof(ObjectType*));

    for (auto i = 0u; i < capacity_; i++) {
      DeAlloc(new_object_block + i);
      memstack_[i] = new_object_block + i;
    }
  }

  ~LockFreeSimpleMempool() { free(memstack_); }

  //! Alloc: Gets a free object pointer from the pool, and returns the same.
  //! if, pool is empty, alocates the capacity no. of objects into the pool and
  //! then returns an object.
  ObjectType* Alloc() {
    ObjectType* object;
    while (!object_pool_.pop(object)) {
      if (object_pool_.empty()) {
        // double checking, in case more than one thread is locked at this mutex.
        if (object_pool_.empty()) {
          unsigned int new_capacity = (capacity_ << 1);
          ObjectType* new_object_block = (ObjectType*)calloc(capacity_, sizeof(ObjectType));
          object_pool_.reserve(new_capacity);
          for (auto i = 0u; i < capacity_; i++) {
            DeAlloc(new_object_block + i);
          }

          ObjectType** new_memstack = (ObjectType**)calloc(new_capacity, sizeof(ObjectType*));
          for (auto i = 0u; i < capacity_; i++) {
            new_memstack[i] = memstack_[i];
            new_memstack[capacity_ + i] = new_object_block + i;
          }
          free(memstack_);
          memstack_ = new_memstack;
          capacity_ = new_capacity;
        }
      }
    }
    return object;
  }

  //! Clone: Allocates memory and gets a zeroed object.
  //! It then deepcopies the template object to the new object.
  ObjectType* Clone(ObjectType* template_object) {
    ObjectType* cloned_object = Alloc();
    memcpy(cloned_object, template_object, sizeof(ObjectType));
    return cloned_object;
  }

  inline void DeAlloc(ObjectType* old_object) { object_pool_.push(old_object); }
};
