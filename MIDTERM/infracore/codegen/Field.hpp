/**
    \file Template.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

*/

#pragma once
#include <string.h>
#include <cstring>
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <vector>
#include "infracore/codegen/Indentation.hpp"
#include "infracore/codegen/FFUtils.hpp"

namespace FFCodeGen {

struct Field {
  int field_id;
  FieldOp field_operator;
  FieldType field_type;
  bool optional;
  bool has_def_value;
  std::string default_value;
  std::string pmap_variable_name;
  bool requires_pmap_;
  std::string name_attr;

  Field(std::string fieldName, int fieldId, FieldOp op, FieldType type, bool isOpt, bool hasDefVal, std::string defVal,
        bool requiresPMap) {
    name_attr = fieldName;
    field_id = fieldId;
    field_operator = op;
    field_type = type;
    optional = isOpt;
    has_def_value = hasDefVal;
    default_value = defVal;
    requires_pmap_ = requiresPMap;
  }
  Field() {}

  std::string getProperty() {
    static std::stringstream ss;
    ss.str("");
    ss << Utils::getOperatorString(field_operator, optional);
    ss << " < " << Utils::getPrimitiveFromFieldType(field_type) << " > *";
    ss << name_attr << ";\n";
    return ss.str();
  }

  std::string getConstructorString() {
    static std::stringstream ss;
    ss.str("");
    ss << name_attr << " = new " << Utils::getOperatorString(field_operator, optional) << " < "
       << Utils::getPrimitiveFromFieldType(field_type) << " > "
       << "( " << ((field_operator == Const) ? "" : (optional ? "false, " : "true, "))
       << (has_def_value ? "true" : "false") << ", " << default_value << " ) ;\n";
    return ss.str();
  }

  std::string getDestructorString() {
    static std::stringstream ss;
    ss.str("");
    ss << "delete " << name_attr << ";"
       << "\n";
    return ss.str();
  }
  std::string getDecodeString() {
    static std::stringstream ss;
    ss.str("");
    if (field_operator == Delta && (field_type == ByteVec || field_type == Str))
      ss << name_attr << "->decodeString( input );\n";
    else if (!requires_pmap_)
      ss << name_attr << "->decode ( " << ((field_operator == Const) ? "" : "input") << ");\n";
    else
      ss << name_attr << "->decode( " << ((field_operator == Const) ? "" : "input, ")
         << PMapNameManager::instance().get_name() << ");\n";
    return ss.str();
  }

  std::string getResetString() {
    static std::stringstream ss;
    ss.str("");
    ss << name_attr << "->reset ( );"
       << "\n";
    return ss.str();
  }
};

struct Group : Field {
  std::vector<Field *> fieldArr;

  std::string getDecodeString();
  std::string getProperty();
  std::string getConstructorString();
  std::string getDestructorString();
  std::string getResetString();
  Group() {}
};

struct Sequence : Field {
  std::vector<Field *> fieldArr;

  std::string getDecodeString();
  std::string getProperty();
  std::string getConstructorString();
  std::string getDestructorString();
  std::string getResetString();

  Sequence(std::string fieldName, int fieldId, FieldOp op, bool isOpt);
};

struct DecimalField : Field {
  bool scalar;
  Field *exponent;
  Field *mantissa;

  std::string getDecodeString();
  std::string getProperty();
  std::string getConstructorString();
  std::string getDestructorString();
  std::string getResetString();
};
}
