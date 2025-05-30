/**
    \file dvccode/Utils/shm_queue.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#ifndef BASE_UTILS_LOCK_FREE_Q_H_
#define BASE_UTILS_LOCK_FREE_Q_H_

#include <cstring>

namespace HFSAT {

/**
 * An implementation of Lock Free Circular Queue of fixed length, where elements are copied (deep cloned) and not just a
 * storage of references
 *
 * Thread safe for one reader, and one writer
 *
 */
template <class T ,int BufferSize = 8192 * 8192>
class LockFreeQ {
 public:
  LockFreeQ() : tail(0), head(0) {}
  virtual ~LockFreeQ() {}

  bool push(T& item_) {
    unsigned int nextTail = increment(tail);
    if (nextTail != head) {
      memcpy((void*)(array + tail), (void*)&item_, sizeof(T));
      tail = nextTail;
      return true;
    }

    // queue was full
    return false;
  }

  bool pop(T& item_) {
    if (head == tail) return false;  // empty queue
    memcpy((void*)&item_, (void*)(array + head), sizeof(T));
    head = increment(head);
    return true;
  }
  int volatile Size() {
    if (tail >= head)
      return (tail - head);
    else
      return ((Capacity - head) + tail + 1);
  }

  bool isEmpty() const { return (head == tail); }
  bool isFull() const {
    unsigned int tailCheck = (tail + 1) % Capacity;
    return (tailCheck == head);
  }

 private:
  static constexpr int Capacity = BufferSize;
  volatile unsigned int tail;  // input index
  T array[Capacity];
  volatile unsigned int head;  // output index

  unsigned int increment(unsigned int idx_) const {
    idx_ = (idx_ + 1) % Capacity;
    return idx_;
  }
};
}
#endif /* BASE_UTILS_LOCK_FREE_Q_H_ */
