/**
    \file dvccode/CommonDataStructures/fixed_length_circular_vector.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_COMMONDATASTRUCTURES_FIXED_LENGTH_CIRCULAR_VECTOR_H
#define BASE_COMMONDATASTRUCTURES_FIXED_LENGTH_CIRCULAR_VECTOR_H

#include <stdlib.h>

namespace HFSAT {

/// \brief class to manage marketbooks...
/// top heavy .. insertion is allowed at the top only if push_front is called ...
/// changing back is not allowed to loop around.
/// Should get this sort of stuff, tokenizer etc from BOOST if possible.
template <class T>
class FixedLengthCircularVector {
 protected:
  T* const store_data_;
  int capacity_;

  int front_marker_;  // front item --- also index = 0 in random access operator
  int back_marker_;   // back item -- also index = ( capacity_ - 1 ) in random access operator

 public:
  /**
   * @param _store_data_ assumed to be an array with at least _new_capacity_ number of elements accessible
   * @param _new_capacity_ the fixed capacity
   *
   * Initialize to size even though values are not valid
   */
  explicit FixedLengthCircularVector(T* const _store_data_, int _new_capacity_)
      : store_data_(_store_data_),
        capacity_(std::max(2, _new_capacity_)),
        front_marker_(0),
        back_marker_((std::max(2, _new_capacity_) - 1)) {}

  /// returns the number of inserted elements
  inline int Size() const { return capacity_; }
  inline unsigned int size() const { return (unsigned int)capacity_; }

  inline void RollLeft() {
    // front_marker_ = back_marker_; // roll left front marker
    if (front_marker_ == 0) {
      front_marker_ = (capacity_ - 1);
    } else {
      front_marker_--;
    }  // roll left front marker
    if (back_marker_ == 0) {
      back_marker_ = (capacity_ - 1);
    } else {
      back_marker_--;
    }  // roll left back marker
  }

  inline void RollRight() {
    // back_marker_ = front_marker_; // roll forward back marker
    back_marker_++;
    if (back_marker_ == capacity_) {
      back_marker_ = 0;
    }  // roll forward back marker
    front_marker_++;
    if (front_marker_ == capacity_) {
      front_marker_ = 0;
    }  // roll forward front marker
  }

  /// adds the given element at the new front of queue
  inline void PushFront(const T& _new_item_) {
    RollLeft();
    store_data_[front_marker_] = _new_item_;
  }

  /// removes current front of queue
  inline void PopFront() { RollRight(); }

  /// shortens queue and overwrites the space at _index_to_remove_, by shifting elements to the back of it by one unit
  inline void PopAt(int _index_to_remove_) {
    for (int i = _index_to_remove_; i < (capacity_ - 1); i++) {
      store_data_[from_external_index_to_internal_index(i)] = store_data_[from_external_index_to_internal_index(i + 1)];
    }
  }

  inline void PushAt(int _index_to_add_, const T& _new_item_) {
    for (int i = capacity_ - 1; i > (int)_index_to_add_; i--) {
      store_data_[from_external_index_to_internal_index(i)] = store_data_[from_external_index_to_internal_index(i - 1)];
    }
    store_data_[from_external_index_to_internal_index(_index_to_add_)] = _new_item_;
  }

  /// without checking for size simply return the front of queue
  inline T& front() { return store_data_[front_marker_]; }

  /// without checking for size simply return the back of queue
  inline T& back() { return store_data_[back_marker_]; }

  /// constant time access operator, valid inputs : 0 to ( capacity_ - 1 )
  inline T& operator[](int i) const { return store_data_[from_external_index_to_internal_index(i)]; }

  inline int from_external_index_to_internal_index(int i) const {
    i = (i + front_marker_);
    if (i >= capacity_) {
      i -= capacity_;
    }
    return i;
  }
};
}

#endif  // BASE_COMMONDATASTRUCTURES_FIXED_LENGTH_CIRCULAR_VECTOR_H
