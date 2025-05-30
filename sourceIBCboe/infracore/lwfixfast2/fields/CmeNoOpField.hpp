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

namespace FF_GLOB {

// based on template, we don't see any optional_no_op_field with initial value present, so
// the class NoOpField will not account for initial value
// Please note that this is not a true implementation of fixfast 1.1 spec, just the parts
// we need for CME templates
template <class T>
class CmeNoOpField {
 public:
  bool isNull;
  bool mandatory;
  T streamVal;
  CmeNoOpField(bool mandatory_) : mandatory(mandatory_) {}
  void decode(FFUtils::ByteStreamReader& input) { isNull = input.interpret(streamVal, mandatory); }

  bool hasValue() { return !isNull; }

  T getValue() { return streamVal; }
};
}
