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
#include "infracore/lwfixfast/fields/DictionaryEntry.hpp"

template <class T>
class IncrementField {
 public:
  DictionaryEntry<T> previousValue;
  bool isMandatory;

  IncrementField(bool isMandatory_, bool hasValue, T value) : isMandatory(isMandatory_) {
    if (hasValue)
      previousValue = DictionaryEntry<T>(value);
    else
      previousValue = DictionaryEntry<T>();
  }

  void decode(FFUtils::ByteStreamReader& input, FFUtils::PMap& pmap) {
    if (pmap.nextBit()) {
      T val;
      bool isNull = input.interpret(val, isMandatory);

      if (!isNull) {
        // Here we have an increment field with the presence map bit set.
        // This means that the value appears in the stream and needs to be decoded
        // and this value should be the new dictionary value.
        previousValue.setValue(val);
      } else {
        if (false == isMandatory) {
          // Here the value in the stream was null (0x80) which means that this
          // optional field is absent and nothing should be put into the message
          // and the dictionary value should be set to "empty"
          previousValue.setAssigned(false);
        } else {
          // We should never actually get here unless there is a bug in the decoder
          // The reason for this is because if the field is set as mandatory
          // the NonNullableDecoder implementation is used which never returns null
          // for the value of a field.
        }
      }
    } else {
      if (true == previousValue.isAssigned()) {
        // Here we have an increment field with the presence map bit cleared,
        // and there is a value assigned in the dictionary..
        // This means we should use the value from the dictionary (incremented by 1).
        // And that new incremented value should be put back into the dictionary
        previousValue.setValue(1 + previousValue.getValue());
      } else {
        if (false == isMandatory) {
          // Here we have an optional increment field with the presence map bit cleared
          // and no value in the dictionary for this field. It is considered absent and nothing
          // should be put into the message and the original offset returned
          // (because nothing was decoded out of the stream)

        } else {
          // Here we have a mandatory increment field with the presence map bit cleared,
          // and there is no value in the dictionary for this field.
          // This is an error
        }
      }
    }
  }

  inline void reset() { previousValue.reset(); }

  inline bool requiresPresenceMapBit() {
    // Increment fields always require a presence map bit
    return true;
  }
};
