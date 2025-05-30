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
#include "infracore/codegen/Field.hpp"

namespace FFCodeGen {
struct Template {
  std::string name;
  int id;
  std::vector<Field*> fieldArr;

  std::string getAutoCode() {
    std::stringstream ss;
    Indentation& ind = Indentation::instance();

    ind.increase();
    ss << ind.indent() << "class " << name << " : public FastDecoder"
       << "{\n";
    ss << ind.indent() << "public:\n";  // all members and functions are public

    ind.increase();

    ss << ind.indent() << "//All member functions\n";
    for (unsigned int i = 0; i < fieldArr.size(); ++i) {
      if (fieldArr[i]->field_type == Seq)
        ss << ind.indent() << ((Sequence*)fieldArr[i])->getProperty();
      else if (fieldArr[i]->field_type == Grp)
        ss << ind.indent() << ((Group*)fieldArr[i])->getProperty();
      else if (fieldArr[i]->field_type == Decimal)
        ss << ind.indent() << ((DecimalField*)fieldArr[i])->getProperty();
      else
        ss << ind.indent() << fieldArr[i]->getProperty();
    }

    // Constructor
    ss << "\n\n";
    ss << ind.indent() << "//Constructor\n";
    ss << ind.indent() << name << "( ) {\n";
    ind.increase();
    for (unsigned int i = 0; i < fieldArr.size(); ++i) {
      if (fieldArr[i]->field_type == Seq)
        ss << ind.indent() << ((Sequence*)fieldArr[i])->getConstructorString();
      else if (fieldArr[i]->field_type == Grp)
        ss << ind.indent() << ((Group*)fieldArr[i])->getConstructorString();
      else if (fieldArr[i]->field_type == Decimal)
        ss << ind.indent() << ((DecimalField*)fieldArr[i])->getConstructorString();
      else
        ss << ind.indent() << fieldArr[i]->getConstructorString();
    }

    ind.decrease();
    ss << ind.indent() << "}\n";

    // Decode logic

    ss << "\n\n";
    ss << ind.indent() << "//Decode\n";
    ss << ind.indent() << "void decode(  FFUtils::ByteStreamReader& input, FFUtils::PMap& pmap0 ) {\n";
    ind.increase();
    for (unsigned int i = 0; i < fieldArr.size(); ++i) {
      if (fieldArr[i]->field_type == Seq)
        ss << ind.indent() << ((Sequence*)fieldArr[i])->getDecodeString();
      else if (fieldArr[i]->field_type == Grp)
        ss << ind.indent() << ((Group*)fieldArr[i])->getDecodeString();
      else if (fieldArr[i]->field_type == Decimal)
        ss << ind.indent() << ((DecimalField*)fieldArr[i])->getDecodeString();
      else
        ss << ind.indent() << fieldArr[i]->getDecodeString();
    }

    ind.decrease();
    ss << ind.indent() << "}\n";

    // Decode logic
    ss << "\n";
    ss << ind.indent() << "//process : TO be done in corresponding cpp file manually\n";
    ss << ind.indent() << "void process( ) ;";

    // reset
    ss << "\n\n";
    ss << ind.indent() << "//Reset\n";
    ss << ind.indent() << "void reset( ) {\n";
    ind.increase();

    for (unsigned int i = 0; i < fieldArr.size(); ++i) {
      if (fieldArr[i]->field_type == Seq)
        ss << ind.indent() << ((Sequence*)fieldArr[i])->getResetString();
      else if (fieldArr[i]->field_type == Grp)
        ss << ind.indent() << ((Group*)fieldArr[i])->getResetString();
      else if (fieldArr[i]->field_type == Decimal)
        ss << ind.indent() << ((DecimalField*)fieldArr[i])->getResetString();
      else
        ss << ind.indent() << fieldArr[i]->getResetString();
    }

    ind.decrease();
    ss << ind.indent() << "}\n";

    // cleanup
    ss << "\n\n";
    ss << ind.indent() << "//Destructor\n";
    ss << ind.indent() << "~" << name << "( ) {\n";
    ind.increase();

    for (unsigned int i = 0; i < fieldArr.size(); ++i) {
      if (fieldArr[i]->field_type == Seq)
        ss << ind.indent() << ((Sequence*)fieldArr[i])->getDestructorString();
      else if (fieldArr[i]->field_type == Grp)
        ss << ind.indent() << ((Group*)fieldArr[i])->getDestructorString();
      else if (fieldArr[i]->field_type == Decimal)
        ss << ind.indent() << ((DecimalField*)fieldArr[i])->getDestructorString();
      else
        ss << ind.indent() << fieldArr[i]->getDestructorString();
    }

    ind.decrease();
    ss << ind.indent() << "}\n";

    ind.decrease();
    ss << ind.indent() << "};\n";
    ind.decrease();

    return ss.str();
  }
};
}
