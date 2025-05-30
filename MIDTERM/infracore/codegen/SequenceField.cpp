/**
    \file SequenceField.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

*/

#include "infracore/codegen/Field.hpp"

namespace FFCodeGen {

Sequence::Sequence(std::string fieldName, int fieldId, FieldOp op, bool isOpt) {
  name_attr = fieldName;
  field_id = fieldId;
  field_operator = op;
  field_type = Seq;
  optional = isOpt;
}

std::string Sequence::getProperty() {
  std::stringstream ss;
  static Indentation& ind = Indentation::instance();

  ss << "//fields inside sequence " << name_attr << "\n";

  for (unsigned int i = 0; i < fieldArr.size(); ++i) {
    ss << ind.indent();
    if (fieldArr[i]->field_type == Decimal)
      ss << ((DecimalField*)fieldArr[i])->getProperty();
    else if (fieldArr[i]->field_type == Grp)
      ss << ((Group*)fieldArr[i])->getProperty();
    else if (fieldArr[i]->field_type == Seq)
      ss << ((Sequence*)fieldArr[i])->getProperty();
    else
      ss << fieldArr[i]->getProperty();
  }

  ss << ind.indent() << "//fields inside sequence " << name_attr << " ends\n";
  return ss.str();
}

std::string Sequence::getConstructorString() {
  std::stringstream ss;
  static Indentation& ind = Indentation::instance();

  ss << "//constructor for sequence " << name_attr << "\n";

  for (unsigned int i = 0; i < fieldArr.size(); ++i) {
    ss << ind.indent();
    if (fieldArr[i]->field_type == Decimal)
      ss << ((DecimalField*)fieldArr[i])->getConstructorString();
    else if (fieldArr[i]->field_type == Grp)
      ss << ((Group*)fieldArr[i])->getConstructorString();
    else if (fieldArr[i]->field_type == Seq)
      ss << ((Sequence*)fieldArr[i])->getConstructorString();
    else
      ss << fieldArr[i]->getConstructorString();
  }

  ss << ind.indent() << "//constructor for sequence " << name_attr << " ends\n";
  return ss.str();
}

std::string Sequence::getDestructorString() {
  std::stringstream ss;
  static Indentation& ind = Indentation::instance();

  ss << "//destructor for sequence " << name_attr << "\n";

  for (unsigned int i = 0; i < fieldArr.size(); ++i) {
    ss << ind.indent();
    if (fieldArr[i]->field_type == Decimal)
      ss << ((DecimalField*)fieldArr[i])->getDestructorString();
    else if (fieldArr[i]->field_type == Grp)
      ss << ((Group*)fieldArr[i])->getDestructorString();
    else if (fieldArr[i]->field_type == Seq)
      ss << ((Sequence*)fieldArr[i])->getDestructorString();
    else
      ss << fieldArr[i]->getDestructorString();
  }

  ss << ind.indent() << "//destructor for sequence " << name_attr << " ends\n";
  return ss.str();
}

std::string Sequence::getResetString() {
  std::stringstream ss;
  static Indentation& ind = Indentation::instance();

  ss << "//reset for sequence " << name_attr << "\n";

  for (unsigned int i = 0; i < fieldArr.size(); ++i) {
    ss << ind.indent();
    if (fieldArr[i]->field_type == Decimal)
      ss << ((DecimalField*)fieldArr[i])->getResetString();
    else if (fieldArr[i]->field_type == Grp)
      ss << ((Group*)fieldArr[i])->getResetString();
    else if (fieldArr[i]->field_type == Seq)
      ss << ((Sequence*)fieldArr[i])->getResetString();
    else
      ss << fieldArr[i]->getResetString();
  }

  ss << ind.indent() << "//reset for sequence " << name_attr << " ends\n";
  return ss.str();
}

std::string Sequence::getDecodeString() {
  std::stringstream ss;
  static Indentation& ind = Indentation::instance();

  ss << "//decode for sequence " << name_attr << "\n";
  ss << ind.indent() << fieldArr[0]->getDecodeString() << "\n";
  if (fieldArr[0]->field_operator == Default)
    ss << ind.indent() << "int " << fieldArr[0]->name_attr << "_len  = " << fieldArr[0]->name_attr << "->currVal; \n";
  else
    ss << ind.indent() << "int " << fieldArr[0]->name_attr << "_len  = " << fieldArr[0]->name_attr
       << "->previousValue.getValue(); \n";

  if (optional) {
    ss << ind.indent() << "if ( " << fieldArr[0]->name_attr << "->previousValue.isAssigned()  ) { "
       << "\n";
    ind.increase();
  }

  ss << ind.indent() << "for(int i =0 ; i < " << fieldArr[0]->name_attr << "_len ; ++i){\n";
  ind.increase();  // begin of loop for iterating over sequence entries

  if (requires_pmap_) {
    PMapNameManager::instance().create_new();
    ss << ind.indent() << "//extract pmap\n";
    ss << ind.indent() << "FFUtils::PMap " << PMapNameManager::instance().get_name() << " = input.extractPmap();"
       << "\n\n";
  }

  for (unsigned int i = 1; i < fieldArr.size(); ++i) {
    ss << ind.indent();
    if (fieldArr[i]->field_type == Decimal)
      ss << ((DecimalField*)fieldArr[i])->getDecodeString();
    else if (fieldArr[i]->field_type == Grp)
      ss << ((Group*)fieldArr[i])->getDecodeString();
    else if (fieldArr[i]->field_type == Seq)
      ss << ((Sequence*)fieldArr[i])->getDecodeString();
    else
      ss << fieldArr[i]->getDecodeString();
  }

  ind.decrease();
  ss << ind.indent() << "}\n";  // end of loop for iterating over sequence entries

  if (requires_pmap_) PMapNameManager::instance().remove_old();

  if (optional) {
    ind.decrease();
    ss << ind.indent() << "}\n";
  }

  ss << ind.indent() << "//decode for sequence " << name_attr << " ends\n";
  return ss.str();
}
}
