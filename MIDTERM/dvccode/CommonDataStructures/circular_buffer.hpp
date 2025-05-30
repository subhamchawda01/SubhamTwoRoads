/**
   \file dvccode/CommonDataStructures/circular_buffer.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_COMMONDATASTRUCTURES_CIRCULAR_BUFFER_H
#define BASE_COMMONDATASTRUCTURES_CIRCULAR_BUFFER_H

#include <stdlib.h>

/// Should get this sort of stuff, tokenizer etc from BOOST if possible.
namespace HFSAT {

template <class T>
class CircularBuffer {
 protected:
  T* store_data_;
  int capacity_;
  int size_;

  int front_marker_;  // old item --- also index = 0
  int back_marker_;   // new item

  inline int from_external_index_to_internal_index(int i) const {
    i = i + front_marker_;
    if (i >= capacity_) {
      i -= capacity_;
    }
    return i;
  }

 public:
  CircularBuffer() : store_data_(NULL), capacity_(1), size_(0), front_marker_(0), back_marker_(0) {
    store_data_ = (T*)calloc(capacity_, sizeof(T));
  }

  explicit CircularBuffer(int _new_capacity_)
      : store_data_(NULL), capacity_(_new_capacity_), size_(0), front_marker_(0), back_marker_(0) {
    store_data_ = (T*)calloc(capacity_, sizeof(T));
  }

  inline unsigned int size() const { return size_; }
  inline int capacity() const { return capacity_; }
  inline bool full() const { return (size_ == capacity_); }
  inline bool empty() const { return (size_ == 0); }

  void push_back(const T& _new_item_) {
    if (size_ != 0) {
      back_marker_++;
      if (back_marker_ == capacity_) {
        back_marker_ = 0;
      }                                        // roll forward back marker
      store_data_[back_marker_] = _new_item_;  // overwrites without checking for front_marker_

      if (back_marker_ != front_marker_) {
        size_++;
      } else {
        front_marker_++;
        if (front_marker_ == capacity_) {
          front_marker_ = 0;
        }  // roll forward front marker
      }
    } else {
      store_data_[back_marker_] = _new_item_;
      size_++;
    }
  }

  void push_front(const T& _new_item_) {
    if (size_ != 0) {
      if (front_marker_ == 0) {
        front_marker_ = (capacity_ - 1);
      } else {
        front_marker_--;
      }  // roll left front marker

      if (front_marker_ == back_marker_) {
        if (back_marker_ == 0) {
          back_marker_ = (capacity_ - 1);
        } else {
          back_marker_--;
        }  // roll left back marker
      } else {
        size_++;
      }
      store_data_[front_marker_] = _new_item_;
    } else {
      store_data_[front_marker_] = _new_item_;
      size_++;
    }
  }

  void pop_front() {
    if (size_ != 0) {
      front_marker_++;
      if (front_marker_ == capacity_) {
        front_marker_ = 0;
      }  // roll forward front marker
      size_--;
    }
  }

  void pop_back() {
    if (size_ != 0) {
      if (back_marker_ == 0) {
        back_marker_ = (capacity_ - 1);
      } else {
        back_marker_--;
      }  // roll left back marker
      size_--;
    }
  }

  const T& front() { return store_data_[front_marker_]; }
  const T& back() { return store_data_[back_marker_]; }

  // Rule of thumb: subscript operator often come in pair
  inline T const& operator[](int i) const { return store_data_[from_external_index_to_internal_index(i)]; }

  inline T& operator[](int i) { return store_data_[from_external_index_to_internal_index(i)]; }

  void set_capacity(int _new_capacity_) {
    if (_new_capacity_ < capacity_) {
      // reset
      capacity_ = _new_capacity_;
      size_ = 0;
      front_marker_ = 0;
      back_marker_ = 0;
    }

    if (_new_capacity_ > capacity_) {
      T* _new_store_data_ = (T*)calloc(_new_capacity_, sizeof(T));
      for (int i = 0; i < size_; i++) {
        _new_store_data_[i] = store_data_[from_external_index_to_internal_index(i)];
      }
      front_marker_ = 0;
      if (size_ == 0) {
        back_marker_ = 0;
      } else {
        back_marker_ = (size_ - 1);
      }
    }
  }

  int insert_level(const unsigned int pos, T& item) {
    // Almost all the time, it will be true
    if (size_ == capacity_) {
      if (pos > 0) {
        // All entries are valid here
        // We are not trying to rotate the buffer
        // Donot move Front/Back marker , just push the last data out as invalid
        for (int ii = size_ - 1; ii > (int)pos; ii--) {
          T old_item = store_data_[from_external_index_to_internal_index(ii - 1)];
          store_data_[from_external_index_to_internal_index(ii)] = old_item;
        }
        store_data_[from_external_index_to_internal_index(pos)] = item;
        return 0;

      } else {
        // O(1), push at the head
        // Since Size is full, FM and BM are adjacent anyways
        push_front(item);
      }

    } else  // Almost will never come , just for completion of code
    {
      // space available on the top and to insert at the begining
      if (front_marker_ >= 1 && pos == 0) {
        push_front(item);
        return 1;
        //  return;
      }

      // Untested
      if (back_marker_ != capacity_ - 1) {
        store_data_[back_marker_ + 1] = store_data_[back_marker_];
        back_marker_++;
        size_++;
        // Copy rest of them
        // We are not trying to rotate the buffer here i.e donot roll back
        for (int ii = size_ - 1; ii > (int)pos; ii--) {
          T old_item = store_data_[from_external_index_to_internal_index(ii - 1)];
          store_data_[from_external_index_to_internal_index(ii)] = old_item;
        }
        store_data_[from_external_index_to_internal_index(pos)] = item;
        return 2;
      }
    }
    return 3;
  }

  void invalidate_level_through(const unsigned int start_level, const unsigned int end_level) {
    if (end_level < start_level) return;

    unsigned int levels_to_replace = end_level - start_level + 1;
    if (size_ == capacity_) {
      for (unsigned int ii = 0; ii < levels_to_replace; ii++) {
        if ((int)(end_level + ii) < size_) {
          // We donot want to rotate
          T item = store_data_[start_level + ii];
          // store_data_ [start_level + ii] = store_data_[size_ -1 -ii];
          // store_data_ [size_ -1 - ii] = item;
          store_data_[start_level + ii] = store_data_[end_level + ii];
          store_data_[end_level + ii] = item;
        }
      }
    }
    // No change in the markers...
    else {
      // TODO {} not interested right now
    }
  }

  void percolate_down_level(const T item, const unsigned int level) {
    for (int ii = level; ii < size_ - 1; ii++) {
      store_data_[from_external_index_to_internal_index(ii)] =
          store_data_[from_external_index_to_internal_index(ii + 1)];
    }
    store_data_[from_external_index_to_internal_index(size_ - 1)] = item;
  }

  void pop_front_update_bmarker(const T& item) {
    // Keep size constant, just increment Front & back marker

    if (size_ != 0) {
      front_marker_++;
      if (front_marker_ == capacity_) {
        front_marker_ = 0;
      }

      back_marker_++;
      if (back_marker_ == capacity_) {
        back_marker_ = 0;
      }
      store_data_[back_marker_] = item;
    }
  }

  int get_front_marker() { return front_marker_; }
  int get_back_marker() { return back_marker_; }
  int get_size() { return size_; }
};
}

#endif  // BASE_COMMONDATASTRUCTURES_CIRCULAR_BUFFER_H
