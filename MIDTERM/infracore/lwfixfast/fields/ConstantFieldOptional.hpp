/**
    \file ConstantField.hpp

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
class ConstantFieldOptional {
  bool isMandatory;
  bool hasInitialValue;
  T constValue;
  bool valuePresent;  // makes sense only when mandatory == false, and presence bit was off for this field

 public:
  ConstantFieldOptional(bool hasInitialValue_, T value)
      : isMandatory(true), hasInitialValue(hasInitialValue_), constValue(value) {}

  void decode(FFUtils::PMap& pmap) { valuePresent = pmap.nextBit(); }

  inline void reset() {
    // not applicable
  }

  // return true if value is set
  T getCurrentValue(bool& isSet) {
    isSet = hasInitialValue && valuePresent;
    return constValue;
  }

  inline bool requiresPresenceMapBit() { return true; }
};
