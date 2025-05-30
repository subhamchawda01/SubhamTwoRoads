/**
    \file ConstantFieldMandatory.hpp

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
template <class T>
class ConstantFieldMandatory {
  bool hasInitialValue;
  T constValue;

 public:
  ConstantFieldMandatory(bool hasInitialValue_, T value, std::string uniqName)
      : hasInitialValue(hasInitialValue_), constValue(value) {}

  void decode() {}  // nothing to do

  inline void reset() {}

  bool hasValue() { return hasInitialValue; }

  T getValue() { return constValue; }

  inline bool requiresPresenceMapBit() { return false; }
};
}
