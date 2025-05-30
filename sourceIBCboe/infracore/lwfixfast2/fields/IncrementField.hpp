/**
    \file IncrementField.hpp

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
template <class T, bool mandatory, bool hasInitialValue>
class IncrementField {
  T initialVal;
  PreviousVal<T>& previousValue;

 public:
  IncrementField(T value, std::string uinqName)
      : initialVal(value), previousValue(DictionaryStore<T>::getUniqInstance().Add(uinqName)) {}

  void decode(FFUtils::ByteStreamReader& input, FFUtils::PMap& pmap) {
    if (pmap.nextBit()) {
      bool isNull = input.interpret(previousValue.val, mandatory);
      if (mandatory) {
        previousValue.status = ASSIGNED_PREV_VAL;
        // Here we have an increment field with the presence map bit set.
        // This means that the value appears in the stream and needs to be decoded
        // and this value should be the new dictionary value.
      } else {
        previousValue.status = isNull ? EMPTY_PREV_VAL : ASSIGNED_PREV_VAL;
        // case isNull:
        // Here we have an increment field with the presence map bit set.
        // This means that the value appears in the stream and needs to be decoded
        // and this value should be the new dictionary value.
        // case !isNull
        // Here the value in the stream was null (0x80) which means that this
        // optional field is absent and nothing should be put into the message
        // and the dictionary value should be set to "empty"
      }
    } else {
      switch (previousValue.status) {
        case UNDEFINED_PREV_VAL:
          if (hasInitialValue) {
            previousValue.val = initialVal;
            previousValue.status = ASSIGNED_PREV_VAL;
          } else {
            if (mandatory)
              std::cerr << "INCREMENTAL_FIELD_ERROR: dynamic error [ERR D5]. no initial value and undefined previous "
                           "value when no data in stream\n";
            else
              previousValue.status = EMPTY_PREV_VAL;
          }
          break;
        case EMPTY_PREV_VAL:
          if (mandatory) {
            std::cerr << "INCREMENTAL_FIELD_ERROR: empty previous value when no data in stream\n";
          }
          break;
        case ASSIGNED_PREV_VAL:
          previousValue.val = previousValue.val + 1;
          break;
      }
    }
  }

  inline void reset() { previousValue.reset(); }

  inline bool requiresPresenceMapBit() {
    // Increment fields always require a presence map bit
    return true;
  }
  bool hasValue() { return ASSIGNED_PREV_VAL == previousValue.status; }

  T getValue() { return previousValue.val; }
};
}
