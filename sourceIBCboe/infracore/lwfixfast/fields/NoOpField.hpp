/**
    \file NoOpField.hpp

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
class NoOpField {
 public:
  bool isMandatory;  // opposite of optional
  DictionaryEntry<T> previousValue;

  NoOpField(bool isMandatory_, bool hasValue, T value) : isMandatory(isMandatory_) {
    if (hasValue)
      previousValue = DictionaryEntry<T>(value);
    else
      previousValue = DictionaryEntry<T>();
  }

  void decode(FFUtils::ByteStreamReader& input) {
    T value;  // decode from input
    bool isNull = input.interpret(value, isMandatory);
    previousValue.setValue(value);
    previousValue.setAssigned(!isNull);

    if (isNull && isMandatory) {
      // a mandatory field and null -- error
    }
  }

  inline void reset() {
    // Since NoOp fields do not use a dictionary entry there is nothing to reset.
  }

  inline bool requiresPresenceMapBit() {
    // NoOp fields never require a presence map bit
    return false;
  }
};
