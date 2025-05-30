/**
    \file FFUtils.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

namespace FFCodeGen {

enum FieldOp { Const, Copy, Default, Delta, Increment, NoOp, Tail };

enum FieldType { U32, I32, U64, I64, Decimal, Str, ByteVec, Seq, Grp, TypeRef, TemplateRef, Undefined };

class Utils {
 public:
  static FieldType getFieldType(std::string s) {
    if (s == "uInt32") return U32;
    if (s == "uInt64") return U64;
    if (s == "length") return U32;
    if (s == "int32") return I32;
    if (s == "int64") return I64;
    if (s == "decimal") return Decimal;
    if (s == "exponent") return I32;
    if (s == "mantissa") return I64;
    if (s == "string") return Str;
    if (s == "byteVector") return ByteVec;
    if (s == "sequence") return Seq;
    if (s == "group") return Grp;
    if (s == "typeRef") return TypeRef;
    if (s == "templateRef") return TemplateRef;

    // field type not handled
    return Undefined;
  }

  static bool isPmapRequired(FieldOp _field_operator_, bool _is_field_optional_ = false) {
    // delta never requires a presence map bit
    if (_field_operator_ == Delta) return false;
    // noop never requires a presence map bit
    if (_field_operator_ == NoOp) return false;

    // copy always requires a presence map bit
    if (_field_operator_ == Copy) return true;
    // increment always requires a presence map bit
    if (_field_operator_ == Increment) return true;
    // tail always requires a presence map bit
    if (_field_operator_ == Tail) return true;

    // for constant operator, if field is
    // optional then it requires pmap bit
    // otherwise it doesn't
    if (_field_operator_ == Const) return _is_field_optional_;

    // operator not handled
    return true;
  }

  static FieldOp getFieldOperatorType(std::string s) {
    if (s == "constant") return Const;
    if (s == "copy") return Copy;
    if (s == "default") return Default;
    if (s == "delta") return Delta;
    if (s == "increment") return Increment;
    if (s == "tail") return Tail;

    // defaults to

    return NoOp;
  }

  static std::string getPrimitiveFromFieldType(FieldType t) {
    switch (t) {
      case U32:
        return "uint32_t";
      case U64:
        return "uint64_t";
      case I32:
        return "int32_t";
      case I64:
        return "int64_t";

      case Decimal:
        return "FFUtils::Decimal";

      case Str:
        return "FFUtils::ByteArr";
      case ByteVec:
        return "FFUtils::ByteVec";

      case Seq:
      case Grp:
        return "";  // not a primitive type

      default:
        return "";  // Or perhaps throw an error
    }
  }

  static std::string getOperatorString(FieldOp op, bool isOptional) {
    switch (op) {
      case Const:
        if (isOptional)
          return "ConstantFieldOptional";
        else
          return "ConstantFieldMandatory";
      case Copy:
        return "CopyField";
      case Default:
        return "DefaultField";
      case Delta:
        return "DeltaField";
      case Increment:
        return "IncrementField";
      case NoOp:
        return "NoOpField";
      case Tail:
        return "TailField";
      default:
        std::cerr << "wrong field operator\n";
        exit(1);
    }
  }

  static std::string getDefaultValue(FieldType type, std::string defVal = "") {
    switch (type) {
      case U32:
      case U64:
      case I32:
      case I64:
        if (defVal.length() == 0) return "0";
        return defVal;

      case Decimal: {
        // Works only for scalar decimal
        if (defVal == "") defVal = "0";
        return "FFUtils::Decimal( " + defVal + " )";
      }

      case Str:
      case ByteVec:
        return "FFUtils::ByteArr( \"" + defVal + "\" )";

      case Seq:
      case Grp:
        return "";  // not a primitive type

      default:
        return "";  // Or perhaps throw an error
    }
  }
};
}
