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
#include "infracore/lwfixfast2/fields/DictionaryEntry.hpp"

namespace FF_GLOB {
template <class T, bool mandatory, bool hasInitialValue>
class DefaultField {
  T initialVal;
  T streamValue;
  bool isNull;

 public:
  bool isMandatory;

  inline void reset() {
    // Default fields never change from their initial value so there is no need to reset them
  }

  DefaultField(T value, std::string uniqName) : initialVal(value), isMandatory(mandatory) {}

  void decode(FFUtils::ByteStreamReader& input, FFUtils::PMap& pmap) {
    if (pmap.nextBit()) {
      isNull = input.interpret(streamValue, mandatory);
    } else {
      isNull = true;
      if (mandatory && !hasInitialValue) {
        std::cerr << "error decoding default field, null=true, mandatory=true, has_def_val=true\n";
      }
    }
  }

  inline bool requiresPresenceMapBit() {
    // Default fields always require a presence map bit
    return true;
  }

  bool hasValue() { return !isNull || (mandatory && hasInitialValue); }

  // we should only call this when we are sure the field has a value
  T getValue() {
    if (!isNull) return streamValue;
    return initialVal;  // no error checking here. is is already done in decoding
  }
};
}
