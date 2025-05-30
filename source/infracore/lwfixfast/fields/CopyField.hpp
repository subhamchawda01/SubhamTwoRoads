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
#include "infracore/lwfixfast/fields/DictionaryEntry.hpp"

template <class T>

class CopyField {
  DictionaryEntry<T> createPreviousValue(T initialValue);

 public:
  const bool isMandatory;
  DictionaryEntry<T> previousValue;

  CopyField(bool isMandatory_, bool hasValue, T value) : isMandatory(isMandatory_) {
    if (hasValue)
      previousValue = DictionaryEntry<T>(value);
    else
      previousValue = DictionaryEntry<T>();
  }

  void decode(FFUtils::ByteStreamReader& input, FFUtils::PMap& pmap) {
    if (pmap.nextBit()) {
      T value;
      bool isNull = input.interpret(value, isMandatory);

      if (!isNull) {
        // Here an actual value was in the stream and should be put into the message.
        // and the dictionary value should be updated to reflect this value.
        previousValue.setValue(value);
      } else {
        if (false == isMandatory) {
          // Here the value in the stream was null (0x80) which means that this
          // optional field is absent and nothing should be put into the message,
          // and the dictionary value should be set to "empty"
          previousValue.setAssigned(false);
        } else {
          // We should never actually get here unless there is a bug in the decoder
        }
      }
    } else {
      if (true == previousValue.isAssigned()) {
        // Here we have a copy field with the presence map bit cleared,
        // and there is a value assigned in the dictionary.
        // This means we should use the value from the dictionary.
        return;
      } else {
        if (false == isMandatory) {
          // Here we have an optional copy field with the presence map bit cleared
          // and no value in the dictionary for this field. It is considered absent and nothing
          // should be put into the message and the original offset returned
          // (because nothing was decoded out of the stream)
        } else {
          // Here we have a mandatory copy field with the presence map bit cleared,
          // and there is no value in the dictionary for this field.
          // This is an error
          std::cerr << " mandatory copy field with the presence map bit cleared, and "
                    << "there is no value in the dictionary for this field\n";
          if (previousValue.hasInitialValue()) previousValue.reset();  // not sure if this is the correct thing to do
        }
      }
    }
  }

  inline void reset() { previousValue.reset(); }

  inline bool requiresPresenceMapBit() {
    // Copy fields always require a presence map bit
    return true;
  }
};
