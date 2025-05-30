/**
    \file CopyField.hpp

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
class CopyField {
  T currVal;

 public:
  const bool isMandatory;

 private:
  T initialValue;
  PreviousVal<T>& previousValue;

 public:
  CopyField(T value, std::string uinqName)
      : isMandatory(mandatory),
        initialValue(value),
        previousValue(DictionaryStore<T>::getUniqInstance().Add(uinqName)) {}

  void decode(FFUtils::ByteStreamReader& input, FFUtils::PMap& pmap) {
    if (pmap.nextBit()) {
      bool isNull = input.interpret(previousValue.val, mandatory);
      if (mandatory) {
        // never null since mandatory
        // Here an actual value was in the stream and should be put into the message.
        // and the dictionary value should be updated to reflect this value.
        previousValue.status = ASSIGNED_PREV_VAL;
      } else {
        previousValue.status = isNull ? EMPTY_PREV_VAL : ASSIGNED_PREV_VAL;
        // NotNull
        // Here an actual value was in the stream and should be put into the message.
        // and the dictionary value should be updated to reflect this value.
        // Null
        // Here the value in the stream was null (0x80) which means that this
        // optional field is absent and nothing should be put into the message,
        // and the dictionary value should be set to "empty"
      }
    } else {
      switch (previousValue.status) {
        case UNDEFINED_PREV_VAL:
          // the value of the field is the initial value that also becomes the new previous value.
          // Unless the field has optional presence, it is a dynamic error [ERR D5] if the instruction context
          // has no initial value.
          // If the field has optional presence and no initial value, the field is considered
          // absent and the state of the previous value is changed to empty.
          if (hasInitialValue) {
            previousValue.status = ASSIGNED_PREV_VAL;
            previousValue.val = initialValue;
          } else {
            if (!mandatory)
              previousValue.status = EMPTY_PREV_VAL;
            else
              std::cerr << "ERROR_DECODING_COPY_FILED: dynamic error [ERR D5]. no initial value and undefined previous "
                           "value for mandatory copy field that has presence bit off.";
          }
          break;
        case EMPTY_PREV_VAL:
          // the value of the field is empty. If the field is optional the value is considered absent. It is
          // a dynamic error [ERR D6] if the field is mandatory.

          if (mandatory) {
            std::cerr << "ERROR_DECODING_COPY_FILED: dynamic error [ERR D6]. empty previous value for mandatory copy "
                         "field that has presence bit off.";
          }
          break;
        case ASSIGNED_PREV_VAL:
          // the value of the field is the previous value.
          break;
      }
    }
  }

  inline void reset() { previousValue.reset(); }

  inline bool requiresPresenceMapBit() {
    // Copy fields always require a presence map bit
    return true;
  }

  bool hasValue() { return ASSIGNED_PREV_VAL == previousValue.status; }

  T getValue() { return previousValue.val; }
};
}
