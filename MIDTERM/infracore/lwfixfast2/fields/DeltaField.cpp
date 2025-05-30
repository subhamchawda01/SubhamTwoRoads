#include "infracore/lwfixfast2/fields/DeltaField.hpp"

namespace FF_GLOB {

// mandatory field from stream is never null
template <>
void DeltaField<FFUtils::Decimal, true>::decode(FFUtils::ByteStreamReader& input) {
  FFUtils::Decimal val;
  isValueSet = !input.interpret(val, true);  // isValueset should always be true
  if (previousValue.status == ASSIGNED_PREV_VAL) {
    previousValue.val += val;
  } else {
    previousValue.status = ASSIGNED_PREV_VAL;
    previousValue.val.exponent = val.exponent;
    previousValue.val.mantissa = val.mantissa;
  }
}

template <>
void DeltaField<uint32_t, true>::decode(FFUtils::ByteStreamReader& input) {
  int64_t intVal;
  isValueSet = !input.interpret(intVal, true);
  intVal += getBaseValue();
  previousValue.status = ASSIGNED_PREV_VAL;
  previousValue.val = (uint32_t)intVal;
}

template <>
void DeltaField<int32_t, true>::decode(FFUtils::ByteStreamReader& input) {
  int64_t intVal;
  bool isNull = input.interpret(intVal, true);
  isValueSet = !isNull;
  if (isNull) return;
  intVal += getBaseValue();
  previousValue.status = ASSIGNED_PREV_VAL;
  previousValue.val = (int32_t)intVal;
}

template <>
void DeltaField<uint64_t, true>::decode(FFUtils::ByteStreamReader& input) {
  int64_t intVal;
  bool isNull = input.interpret(intVal, true);
  isValueSet = !isNull;
  if (isNull) return;
  intVal += getBaseValue();
  previousValue.status = ASSIGNED_PREV_VAL;
  previousValue.val = (uint64_t)intVal;
}

template <>
void DeltaField<int64_t, true>::decode(FFUtils::ByteStreamReader& input) {
  int64_t intVal;
  bool isNull = input.interpret(intVal, true);
  isValueSet = !isNull;
  if (isNull) return;
  intVal += getBaseValue();
  previousValue.status = ASSIGNED_PREV_VAL;
  previousValue.val = intVal;
}

template <>
void DeltaField<FFUtils::ByteArr, true>::decode(FFUtils::ByteStreamReader& input) {
  // dummy should never be called
}

template <>
void DeltaField<FFUtils::ByteVec, true>::decode(FFUtils::ByteStreamReader& input) {
  // dummy should never be called
}

////////////////////
// ALL false by now

template <>
void DeltaField<FFUtils::Decimal, false>::decode(FFUtils::ByteStreamReader& input) {
  FFUtils::Decimal val;
  bool isNull = input.interpret(val, false);
  isValueSet = !isNull;
  if (isNull) return;
  if (previousValue.status == ASSIGNED_PREV_VAL) {
    previousValue.val += val;
  } else {
    previousValue.status = ASSIGNED_PREV_VAL;
    previousValue.val.exponent = val.exponent;
    previousValue.val.mantissa = val.mantissa;
  }
}

template <>
void DeltaField<uint32_t, false>::decode(FFUtils::ByteStreamReader& input) {
  int64_t intVal;
  bool isNull = input.interpret(intVal, false);
  isValueSet = !isNull;
  if (isNull) return;
  intVal += getBaseValue();
  previousValue.status = ASSIGNED_PREV_VAL;
  previousValue.val = (uint32_t)intVal;
}

template <>
void DeltaField<int32_t, false>::decode(FFUtils::ByteStreamReader& input) {
  int64_t intVal;
  bool isNull = input.interpret(intVal, false);
  isValueSet = !isNull;
  if (isNull) return;
  intVal += getBaseValue();
  previousValue.status = ASSIGNED_PREV_VAL;
  previousValue.val = (int32_t)intVal;
}

template <>
void DeltaField<uint64_t, false>::decode(FFUtils::ByteStreamReader& input) {
  int64_t intVal;
  bool isNull = input.interpret(intVal, false);
  isValueSet = !isNull;
  if (isNull) return;
  intVal += getBaseValue();
  previousValue.status = ASSIGNED_PREV_VAL;
  previousValue.val = (uint64_t)intVal;
}

template <>
void DeltaField<int64_t, false>::decode(FFUtils::ByteStreamReader& input) {
  int64_t intVal;
  bool isNull = input.interpret(intVal, false);
  isValueSet = !isNull;
  if (isNull) return;
  intVal += getBaseValue();
  previousValue.status = ASSIGNED_PREV_VAL;
  previousValue.val = intVal;
}

template <>
void DeltaField<FFUtils::ByteArr, false>::decode(FFUtils::ByteStreamReader& input) {
  // dummy should never be called
}

template <>
void DeltaField<FFUtils::ByteVec, false>::decode(FFUtils::ByteStreamReader& input) {
  // dummy should never be called
}
}
