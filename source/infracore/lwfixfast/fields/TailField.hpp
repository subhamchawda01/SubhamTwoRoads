/**
    \file TailField.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

     Created on: Nov 3, 2011

*/
#pragma once

#include "infracore/lwfixfast/FFUtils.hpp"
#include "infracore/lwfixfast/fields/DictionaryEntry.hpp"

template <class T>
class TailField {
  // Only applicable for strings

  bool isMandatory;
  void applyTail(T& orig, T tail) {
    char* c = ByteArrayPool::get();
    int len = 0;
    if (!orig.isnull && orig.len > 0) {
      strncpy(c, orig.bytes, orig.len);
      len = orig.len;
    }
    if (!tail.isnull && tail.len > 0) {
      strncpy(c, tail.bytes, orig.tail);
      len += orig.len;
    }
    orig.bytes = c;
    orig.len = len;
    orig.isnull = false;
  }

 public:
  DictionaryEntry<T> previousValue;

  TailField(bool isMandatory_, bool hasValue, T value) : isMandatory(isMandatory_) {
    if (hasValue)
      previousValue = DictionaryEntry<T>(value);
    else
      previousValue = DictionaryEntry<T>();
  }

  void decode(FFUtils::ByteStreamReader& input, FFUtils::PMap& pmap) {
    if (pmap.nextBit()) {
      T tail;
      bool isNull = input.interpret(tail, isMandatory);
      if (isNull)
        // output value is not set
        previousValue.setAssigned(false);
      else {
        if (previousValue.isAssigned()) {
          applyTail(previousValue.getValueRef(), tail);
        } else if (previousValue.hasInitialValue()) {
          previousValue.reset();  // current value will be set to initial value, which we can utilize to apply delta
          applyTail(previousValue.getValueRef(), tail);
          // combine with intial value
        } else {
          // combine with type specific empty value
          previousValue.setValue(tail);  // set tail to current value
        }
      }
    } else {
      if (previousValue.isAssigned) {
        // do nothing.
      } else
        previousValue.reset();
    }
  }

  inline bool requiresPresenceMapBit() {
    // Tail Fields always require a presence map bit
    return true;
  }

  inline void reset() { previousValue.reset(); }
};
