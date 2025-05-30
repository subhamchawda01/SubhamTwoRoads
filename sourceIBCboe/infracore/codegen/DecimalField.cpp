/**
    \file DecimalField.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

*/
#include "infracore/codegen/Field.hpp"

namespace FFCodeGen {

std::string DecimalField::getProperty() {
  std::stringstream ss;
  static Indentation& ind = Indentation::instance();
  if (scalar) {
    ss << ((Field*)this)->getProperty();
  } else {
    ss << "//decimal fields " << name_attr << "\n";
    ss << ind.indent();
    ss << exponent->getProperty();
    ss << ind.indent();
    ss << mantissa->getProperty();
  }
  return ss.str();
};

std::string DecimalField::getConstructorString() {
  std::stringstream ss;
  static Indentation& ind = Indentation::instance();
  if (scalar) {
    ss << ((Field*)this)->getConstructorString();
  } else {
    ss << "//decimal fields " << name_attr << "\n";
    ss << ind.indent();
    ss << exponent->getConstructorString();
    ss << ind.indent();
    ss << mantissa->getConstructorString();
  }
  return ss.str();
};

std::string DecimalField::getResetString() {
  std::stringstream ss;
  static Indentation& ind = Indentation::instance();
  if (scalar) {
    ss << ((Field*)this)->getResetString();
  } else {
    ss << "//decimal fields " << name_attr << "\n";
    ss << ind.indent();
    ss << exponent->getResetString();
    ss << ind.indent();
    ss << mantissa->getResetString();
  }
  return ss.str();
};

std::string DecimalField::getDestructorString() {
  std::stringstream ss;
  static Indentation& ind = Indentation::instance();
  if (scalar) {
    ss << ((Field*)this)->getDestructorString();
  } else {
    ss << "//decimal fields " << name_attr << "\n";
    ss << ind.indent();
    ss << exponent->getDestructorString();
    ss << ind.indent();
    ss << mantissa->getDestructorString();
  }
  return ss.str();
};

std::string DecimalField::getDecodeString() {
  std::stringstream ss;
  static Indentation& ind = Indentation::instance();
  if (scalar) {
    ss << ((Field*)this)->getDecodeString();
  } else {
    ss << "//decimal fields " << name_attr << "\n";
    ss << ind.indent() << exponent->getDecodeString();
    // if decimal is optional and exponent is null, do not decode mantissa
    if (exponent->field_operator != Default) {
      ss << ind.indent() << "if ( " << exponent->name_attr << "->isMandatory || " << exponent->name_attr
         << "->previousValue.isAssigned() ) \n";
      ind.increase();
    } else {
      ss << ind.indent() << "if ( " << exponent->name_attr << "->isMandatory || " << exponent->name_attr
         << "->isValueSet  ) \n";
      ind.increase();
    }
    ss << ind.indent() << mantissa->getDecodeString();
    ind.decrease();
  }
  return ss.str();
};
}
