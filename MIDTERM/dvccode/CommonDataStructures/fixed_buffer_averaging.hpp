/**
    \file dvccode/CommonDataStructures/fixed_buffer_averaging.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_COMMONDATASTRUCTURES_FIXED_BUFFER_AVERAGING_H
#define BASE_COMMONDATASTRUCTURES_FIXED_BUFFER_AVERAGING_H

#include "dvccode/CommonDataStructures/circular_buffer.hpp"

namespace HFSAT {

template <class T>
class FixedBufferAveraging {
 protected:
  int capacity_;
  CircularBuffer<T> circular_buffer_data_;
  T sum_;
  T average_;

 public:
  FixedBufferAveraging() : capacity_(1), circular_buffer_data_(1), sum_(0), average_(0) {
    circular_buffer_data_.set_capacity(capacity_);
    //      average_ = 0 ;
  }

  FixedBufferAveraging(int _capacity_)
      : capacity_(_capacity_), circular_buffer_data_(_capacity_), sum_(0), average_(0) {
    // circular_buffer_data_.set_capacity ( capacity_ ) ;
  }

  inline void set_capacity(int _capacity_) {
    capacity_ = _capacity_;
    circular_buffer_data_.set_capacity(_capacity_);
    Recompute();
  }

  inline void push_replace(T _new_item_) {
    if (circular_buffer_data_.empty()) {
      circular_buffer_data_.push_back(_new_item_);
      sum_ += _new_item_;
      average_ += _new_item_;
    } else {
      if (circular_buffer_data_.full()) {
        sum_ -= circular_buffer_data_.front();
        sum_ += _new_item_;
        circular_buffer_data_.push_back(_new_item_);
        average_ = (sum_ / circular_buffer_data_.size());
      } else {
        sum_ += _new_item_;
        circular_buffer_data_.push_back(_new_item_);
        average_ = (sum_ / circular_buffer_data_.size());
      }
    }
  }

  inline T average() const { return average_; }
  inline T sum() const { return sum_; }

  inline void Recompute() {
    sum_ = 0;
    if (circular_buffer_data_.size() > 0) {
      for (int i = 0; i < circular_buffer_data_.size(); i++) {
        sum_ += circular_buffer_data_[i];
      }
      average_ = (sum_ / circular_buffer_data_.size());
    } else {
      average_ = 0;
    }
  }
};
}

#endif  // BASE_COMMONDATASTRUCTURES_FIXED_BUFFER_AVERAGING_H
