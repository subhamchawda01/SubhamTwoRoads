#include "infracore/lwfixfast/fields/DeltaField.hpp"

template <>
void DeltaField<FFUtils::Decimal>::decode(FFUtils::ByteStreamReader& input) {
  FFUtils::Decimal val;
  bool isNull = input.interpret(val, this->isMandatory);
  isValueSet = !isNull;
  if (isNull) return;
  if (true == this->previousValue.isAssigned()) val += this->previousValue.getValue();
  this->previousValue.setValue(val);
}

template <>
void DeltaField<uint32_t>::decode(FFUtils::ByteStreamReader& input) {
  int64_t intVal;
  bool isNull = input.interpret(intVal, this->isMandatory);
  isValueSet = !isNull;
  if (isNull) return;
  if (true == this->previousValue.isAssigned()) intVal += this->previousValue.getValue();
  this->previousValue.setValue((uint32_t)intVal);
}

template <>
void DeltaField<int32_t>::decode(FFUtils::ByteStreamReader& input) {
  int64_t intVal;
  bool isNull = input.interpret(intVal, this->isMandatory);
  isValueSet = !isNull;
  if (isNull) return;
  if (true == this->previousValue.isAssigned()) intVal += this->previousValue.getValue();
  this->previousValue.setValue((int32_t)intVal);
}

template <>
void DeltaField<uint64_t>::decode(FFUtils::ByteStreamReader& input) {
  int64_t intVal;
  bool isNull = input.interpret(intVal, this->isMandatory);
  isValueSet = !isNull;
  if (isNull) return;
  if (true == this->previousValue.isAssigned()) intVal += this->previousValue.getValue();
  this->previousValue.setValue((uint64_t)intVal);
}

template <>
void DeltaField<int64_t>::decode(FFUtils::ByteStreamReader& input) {
  int64_t intVal;
  bool isNull = input.interpret(intVal, this->isMandatory);
  isValueSet = !isNull;
  if (isNull) return;
  if (true == this->previousValue.isAssigned()) intVal += this->previousValue.getValue();
  this->previousValue.setValue(intVal);
}

template <>
void DeltaField<FFUtils::ByteArr>::decode(FFUtils::ByteStreamReader& input) {
  // dummy should never be called
}

template <>
void DeltaField<FFUtils::ByteVec>::decode(FFUtils::ByteStreamReader& input) {
  // dummy should never be called
}
