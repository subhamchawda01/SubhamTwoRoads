/**
    \file DictionaryEntry.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

     Created on: Nov 3, 2011

*/
#pragma once

template <class T>
class DictionaryEntry {
  bool isAssigned_;
  bool hasInitialValue_;
  T value;
  T initialValue;

 public:
  DictionaryEntry() : isAssigned_(false), hasInitialValue_(false) {}
  DictionaryEntry(T initialValue_)
      : isAssigned_(true), hasInitialValue_(true), value(initialValue_), initialValue(initialValue_) {}

  T getValue() { return value; }

  T& getValueRef() { return value; }

  void setValue(T value_) {
    value = value_;
    isAssigned_ = true;
  }

  bool isAssigned() { return isAssigned_; }

  void setAssigned(bool _isAssigned_) { isAssigned_ = _isAssigned_; }

  bool hasInitialValue() { return hasInitialValue_; }

  void reset() {
    if (true == hasInitialValue_) {
      value = initialValue;
      isAssigned_ = true;
    } else
      isAssigned_ = false;
  }
};
