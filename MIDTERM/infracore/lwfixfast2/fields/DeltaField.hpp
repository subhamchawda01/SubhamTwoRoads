/**
    \file DeltaField.hpp

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
#include "infracore/lwfixfast2/fields/DictionaryEntry.hpp"

namespace FF_GLOB {

template <class T, bool mandatory>
class DeltaField {
  void applyDelta(T& orig, T& delta, int subLen) {
    if (subLen >= 0) {
      char* buf = ByteArrayPool::get();

      // copy original + append at last
      int len2Copy = orig.len - subLen;
      if (len2Copy > 0 && !orig.isnull)
        strncpy(buf, orig.bytes, len2Copy);
      else if (len2Copy < 0)
        std::cerr << "len2Copy is negative in delta operator\n..";

      strncpy(buf + len2Copy, delta.bytes, delta.len);
      orig.bytes = buf;
      orig.len = len2Copy + delta.len;
      orig.isnull = false;
    } else {
      subLen = -subLen - 1;  //-1 is because we have to encode -ve 0 as well.
      char* buf = ByteArrayPool::get();

      // copy delta + append at last
      strncpy(buf, delta.bytes, delta.len);
      int len2Copy = orig.len - subLen;
      if (len2Copy > 0 && !orig.isnull)
        strncpy(buf + delta.len, orig.bytes + subLen, len2Copy);
      else if (len2Copy < 0)
        std::cerr << "len2Copy is negative in delta operator\n..";

      orig.bytes = buf;
      orig.len = len2Copy + delta.len;
      orig.isnull = false;
    }
  }

 public:
  T initVal;
  PreviousVal<T>& previousValue;
  bool isValueSet;

  inline void reset() { previousValue.reset(); }

  // ensure that constructor is called with type dependent initial base value in case template is absent.
  // typically it is 0for numerics and empty string/byte-vector as applicable to other primitives
  DeltaField(T value, std::string uinqName)
      : initVal(value), previousValue(DictionaryStore<T>::getUniqInstance().Add(uinqName)) {}

  T getBaseValue() {
    switch (previousValue.status) {
      case UNDEFINED_PREV_VAL:
        return initVal;
      case EMPTY_PREV_VAL:
        std::cerr << "DELTA_FIELD_ERROR: dynamic error [ERR D6]. EMPTY previousValue\n";
        return previousValue.val;
      case ASSIGNED_PREV_VAL:
      default:
        return previousValue.val;
    }
  }

  void decode(FFUtils::ByteStreamReader& input);

  // works for string and byte vector.. for others call simply decode
  // Manually edit this in the auto templates file
  void decodeString(FFUtils::ByteStreamReader& input) {
    int32_t subLen;
    bool isNull = input.interpret(subLen, mandatory);
    isValueSet = !isNull;
    if (isNull) return;
    // A NULL delta is represented as a NULL subtraction length
    // if not null, the subsequent field is mandatory
    T delta;
    isNull = input.interpret(delta, true);
    if (previousValue.status == ASSIGNED_PREV_VAL)
      applyDelta(previousValue.val, delta, subLen);
    else {
      previousValue.status = ASSIGNED_PREV_VAL;
      previousValue.val = delta;
    }
  }

  inline bool requiresPresenceMapBit() {
    // Delta fields never require a presence map bit
    return false;
  }

  bool hasValue() { return isValueSet; }

  // we should only call this when we are sure the field has a value
  T getValue() { return previousValue.val; }
};
}
