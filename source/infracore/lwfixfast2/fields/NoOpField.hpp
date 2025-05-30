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
#include "infracore/lwfixfast2/fields/DictionaryEntry.hpp"

namespace FF_GLOB {
template <class T, bool mandatory, bool hasInitialValue>
class NoOpField {
 public:
  bool isMandatory;
  T streamVal;
  T initialVal;
  bool isNull;

  NoOpField(T value, std::string uinqName) : isMandatory(mandatory), initialVal(value) {}

  void decode(FFUtils::ByteStreamReader& input) { isNull = input.interpret(streamVal, mandatory); }

  inline void reset() {
    // Since NoOp fields do not use a dictionary entry there is nothing to reset.
  }

  inline bool requiresPresenceMapBit() {
    // NoOp fields never require a presence map bit
    return false;
  }

  bool hasValue() {
    if (hasInitialValue || mandatory)
      return true;
    else
      return !isNull;
    // return !isNull || initialVal;
  }

  T getValue() {
    if (mandatory) {
      return streamVal;
    } else {
      return isNull ? initialVal : streamVal;
    }
  }
};
}
