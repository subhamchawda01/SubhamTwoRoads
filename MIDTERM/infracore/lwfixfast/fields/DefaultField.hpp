/**
    \file DefaultField.hpp

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
class DefaultField {
  bool hasDefVal;
  T defVal;

 public:
  bool isMandatory;
  T currVal;
  bool isValueSet;

  inline void reset() {
    // Default fields never change from their initial value so there is no need to reset them
  }

  DefaultField(bool isMandatory_, bool hasValue, T value) : isMandatory(isMandatory_) {
    if (hasValue) {
      hasDefVal = hasValue;
      defVal = value;
    } else
      hasDefVal = false;
  }

  void decode(FFUtils::ByteStreamReader& input, FFUtils::PMap& pmap) {
    if (pmap.nextBit()) {
      bool isNull = input.interpret(currVal, isMandatory);
      isValueSet = !isNull;
    } else {
      if (isMandatory && !hasDefVal) {
        std::cerr << "error decoding default field, null=true, mandatory=true, has_def_val=true\n";
        isValueSet = false;
      } else if (hasDefVal) {
        currVal = defVal;
        isValueSet = true;
      } else {
        isValueSet = false;
      }
      // if(isMandatory && !hasDefVal) => ERROR
    }
  }

  inline bool requiresPresenceMapBit() {
    // Default fields always require a presence map bit
    return true;
  }
};
