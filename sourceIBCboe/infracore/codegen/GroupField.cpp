/**
    \file GroupField.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

*/
#include "infracore/codegen/Field.hpp"

namespace FFCodeGen {

std::string Group::getProperty() {
  std::stringstream ss;
  static Indentation& ind = Indentation::instance();

  ss << "//fields inside group " << name_attr << "\n";
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
  ss << ind.indent() << "//fields inside group " << name_attr << " ends\n";
  return ss.str();
}

std::string Group::getConstructorString() {
  std::stringstream ss;
  static Indentation& ind = Indentation::instance();

  ss << ind.indent() << "//constructor for group " << name_attr << "\n";

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

  ss << ind.indent() << "//constructor for group " << name_attr << " ends\n";
  return ss.str();
}

std::string Group::getDestructorString() {
  std::stringstream ss;
  static Indentation& ind = Indentation::instance();

  ss << "//destructor for group " << name_attr << "\n";

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

  ss << ind.indent() << "//destructor for group " << name_attr << " ends\n";
  return ss.str();
}

std::string Group::getDecodeString() {
  std::stringstream ss;
  static Indentation& ind = Indentation::instance();

  ss << "//decode for group " << name_attr << "\n";

  if (optional) {
    ss << ind.indent() << "if ( " << PMapNameManager::instance().get_name() << ".nextBit( ) ) {"
       << "\n";
    ind.increase();
  }

  if (requires_pmap_) {
    PMapNameManager::instance().create_new();
    ss << ind.indent() << "//extract pmap\n";
    ss << ind.indent() << "FFUtils::PMap " << PMapNameManager::instance().get_name() << " = input.extractPmap();"
       << "\n\n";
  }

  for (unsigned int i = 0; i < fieldArr.size(); ++i) {
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

  if (requires_pmap_) PMapNameManager::instance().remove_old();

  if (optional) {
    ind.decrease();
    ss << ind.indent() << "}\n";
  }

  ss << ind.indent() << "//decode for group " << name_attr << " ends\n";
  return ss.str();
}

std::string Group::getResetString() {
  std::stringstream ss;
  static Indentation& ind = Indentation::instance();

  ss << "//reset for group " << name_attr << "\n";

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

  ss << ind.indent() << "//reset for group " << name_attr << " ends\n";
  return ss.str();
}
}
