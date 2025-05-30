/**
    \file TailField.hpp

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
template <class T, bool mandatory>
class TailField {
  // Only applicable for strings

  // bool isMandatory;
  void applyTail(T& orig, T tail) {
    char* c = ByteArrayPool::get();
    int len = 0;
    if (!orig.isnull && orig.len > 0) {
      strncpy(c, orig.bytes, orig.len);
      len = orig.len;
    }
    if (!tail.isnull && tail.len > 0) {
      strncpy(c, tail.bytes, orig.tail);
      len += orig.len;
    }
    orig.bytes = c;
    orig.len = len;
    orig.isnull = false;
  }

 public:
  T initialVal;
  PreviousVal<T>& previousValue;
  bool isNull;

  // Assume if hasValue is false then value = `default base value` for the type
  // hence always assume initial value is set (Same applies to DeltaField)
  TailField(T value, std::string uinqName)
      :  // isMandatory ( mandatory ),
        initialVal(value),
        previousValue(DictionaryStore<T>::getUniqInstance().Add(uinqName)) {}

  T getBaseVal() {
    switch (previousValue.status) {
      case UNDEFINED_PREV_VAL:
      case EMPTY_PREV_VAL:
        return initialVal.val;
      case ASSIGNED_PREV_VAL:
        return previousValue.val;
    }
  }

  void decode(FFUtils::ByteStreamReader& input, FFUtils::PMap& pmap) {
    if (pmap.nextBit()) {
      T tail;
      isNull = input.interpret(tail, mandatory);
      if (!mandatory)
        if (isNull) return;

      T baseval = getBaseVal();
      applyTail(baseval, tail);
      previousValue.val = baseval;
      previousValue.status = ASSIGNED_PREV_VAL;
    } else {
      isNull = true;
      if (previousValue.status == UNDEFINED_PREV_VAL) {
        //            if (initialVal.isSet){  //Always true for TailField is the assumption
        previousValue.val = initialVal.val;
        previousValue.status = ASSIGNED_PREV_VAL;
        //            }
        //            else if (mandatory){
        //                std::cerr << "TAIL_FIELD_ERROR: dynamic error [ERR D6]. no data on stream, undefined previous
        //                value, no initial value and mandatory field\n";
        //            }
      } else if (previousValue.status == EMPTY_PREV_VAL) {
        if (mandatory) {
          std::cerr << "TAIL_FIELD_ERROR: dynamic error [ERR D7]. no data on stream, empty previous value and "
                       "mandatory field\n";
        }
      }
    }
  }

  inline bool requiresPresenceMapBit() {
    // Tail Fields always require a presence map bit
    return true;
  }

  inline void reset() { previousValue.reset(); }
  bool hasValue() { return !isNull || ASSIGNED_PREV_VAL == previousValue.status; }

  T getValue() { return previousValue.val; }
};
}
