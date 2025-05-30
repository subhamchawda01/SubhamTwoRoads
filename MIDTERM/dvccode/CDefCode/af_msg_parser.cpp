// =====================================================================================
//
//       Filename:  af_msg_parser.cpp
//
//    Description:  parser for binary messages from alphsflash
//
//        Version:  1.0
//        Created:  Friday 12 December 2014 11:20:20  IST
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include "dvccode/CDef/af_msg_parser.hpp"

namespace AF_MSGSPECS {
double AF_MsgParser::swapDouble(uint8_t *dblBuf) {
  double swappedDblResult;
  uint8_t *dst = (uint8_t *)&swappedDblResult;
  uint8_t *src = dblBuf;

  dst[0] = src[7];
  dst[1] = src[6];
  dst[2] = src[5];
  dst[3] = src[4];
  dst[4] = src[3];
  dst[5] = src[2];
  dst[6] = src[1];
  dst[7] = src[0];

  return swappedDblResult;
}

float AF_MsgParser::swapFloat(uint8_t *fltBuf) {
  union v {
    float f;
    unsigned int i;
  };
  v val;
  val.i = (unsigned int)ntoh32(*(reinterpret_cast<uint32_t *>(fltBuf)));
  return val.f;
}

Category *AF_MsgParser::getCategoryforId(uint16_t catg_id_) {
  for (std::vector<Category *>::iterator it = catg_list.begin(); it != catg_list.end(); it++) {
    Category *t_catg_ = *it;
    if (t_catg_->category_id_ == (int)catg_id_) {
      return t_catg_;
    }
  }
  if (dbglogger_) {
    (*dbglogger_) << "Category: " << catg_id_ << " is NOT defined\n";
  } else {
    std::cerr << "Category: " << catg_id_ << " is NOT defined\n";
  }
  return NULL;
}

Message *AF_MsgParser::getMsgFromCatg(Category *t_catg_, short msg_type_) {
  for (std::vector<Message *>::iterator it = t_catg_->msgs.begin(); it != t_catg_->msgs.end(); it++) {
    Message *t_msg_ = *it;
    if (t_msg_->msg_type_ == (int)msg_type_) {
      return t_msg_;
    }
  }
  if (dbglogger_) {
    (*dbglogger_) << "Msgtype " << msg_type_ << " does not match any message in Category " << t_catg_->category_id_
                  << "\n";
  } else {
    std::cerr << "Msgtype " << msg_type_ << " does not match any message in Category " << t_catg_->category_id_ << "\n";
  }
  return NULL;
}

Field *AF_MsgParser::getFieldFromMsg(Message *t_msg_, short field_id_, short field_type_) {
  for (std::vector<Field *>::iterator it = t_msg_->fields.begin(); it != t_msg_->fields.end(); it++) {
    Field *t_field_ = *it;
    if (t_field_->datum_id_ == (int)field_id_) {
      if ((short)t_field_->field_type_ == field_type_) {
        return t_field_;
      } else {
        if (dbglogger_) {
          (*dbglogger_) << "Field_id " << field_id_ << " matches.. but field_type does not match: " << field_type_
                        << " and " << t_field_->field_type_ << "\n";
        } else {
          std::cerr << "Field_id " << field_id_ << " matches.. but field_type does not match: " << field_type_
                    << " and " << t_field_->field_type_ << "\n";
        }
      }
    }
  }
  if (dbglogger_) {
    (*dbglogger_) << "No Field_id match: " << field_id_ << "\n";
  } else {
    std::cerr << "No Field_id match: " << field_id_ << "\n";
  }
  return NULL;
}

Field *AF_MsgParser::getFieldFromMsg(Message *t_msg_, short field_id_) {
  for (std::vector<Field *>::iterator it = t_msg_->fields.begin(); it != t_msg_->fields.end(); it++) {
    Field *t_field_ = *it;
    if (t_field_->datum_id_ == (int)field_id_) {
      return t_field_;
    }
  }
  if (dbglogger_) {
    (*dbglogger_) << "No Field_id match: " << field_id_ << "\n";
  } else {
    std::cerr << "No Field_id match: " << field_id_ << "\n";
  }

  return NULL;
}

Datum *AF_MsgParser::getDatumFromField(Category *t_catg_, short datum_id_) {
  for (std::vector<Datum *>::iterator it = t_catg_->data.begin(); it != t_catg_->data.end(); it++) {
    Datum *t_datum_ = *it;
    if (t_datum_->id_ == (int)datum_id_) {
      return t_datum_;
    }
  }
  if (dbglogger_) {
    (*dbglogger_) << "No datum_id match: " << datum_id_ << "\n";
  } else {
    std::cerr << "No datum_id match: " << datum_id_ << "\n";
  }
  return NULL;
}

/* returns  */
std::string AF_MsgParser::getFieldTypeValue(Field *t_field_, char *msgdata, int &field_buffer_offset_,
                                            int msg_length_) {
  FieldType field_type_ = t_field_->field_type_;
  std::stringstream rt_ss;
  int value_offset = field_buffer_offset_ + 2;

  switch (field_type_) {
    case kShort_value_enumeration:
    case kShort: {
      int16_t field_value = ntoh16(*(reinterpret_cast<uint16_t *>(&msgdata[value_offset])));
      field_buffer_offset_ += SHORT_INDICATOR_SIZE;
      rt_ss << field_type_ << delim_ << field_value;
    } break;

    case kInt: {
      int32_t field_value = ntoh32(*(reinterpret_cast<uint32_t *>(&msgdata[value_offset])));
      field_buffer_offset_ += INT_INDICATOR_SIZE;
      rt_ss << field_type_ << delim_ << field_value;
    } break;

    case kLong: {
      int64_t field_value = ntoh64(*(reinterpret_cast<uint64_t *>(&msgdata[value_offset])));
      field_buffer_offset_ += LONG_INDICATOR_SIZE;
      rt_ss << field_type_ << delim_ << field_value;
    } break;

    case kFloat: {
      float field_value = swapFloat((uint8_t *)&msgdata[value_offset]);
      field_buffer_offset_ += FLOAT_INDICATOR_SIZE;
      rt_ss << field_type_ << delim_ << field_value;
    } break;

    case kDouble: {
      double field_value = swapDouble((uint8_t *)&msgdata[value_offset]);
      field_buffer_offset_ += DOUBLE_INDICATOR_SIZE;
      rt_ss << field_type_ << delim_ << field_value;
    } break;

    case kBoolean: {
      bool field_value = msgdata[value_offset];
      field_buffer_offset_ += BOOL_INDICATOR_SIZE;
      rt_ss << field_type_ << delim_ << field_value;
    } break;

    case kEnumeration:
    case kYes_no_na:
    case kDirectional: {
      char field_value = msgdata[value_offset];
      field_buffer_offset_ += DIRECTIONAL_INDICATOR_SIZE;
      rt_ss << field_type_ << delim_ << (short)field_value;
    } break;

    case kTime: {
      Time_st time_st_;
      uint8_t *yyyy_ptr = reinterpret_cast<uint8_t *>(&msgdata[value_offset]);
      time_st_.yyyy_ = ((yyyy_ptr[0] << 8) | (yyyy_ptr[1]));
      time_st_.mm_ = msgdata[value_offset + 2];
      time_st_.dd_ = msgdata[value_offset + 3];
      field_buffer_offset_ += TIME_INDICATOR_SIZE;
      rt_ss << field_type_ << delim_ << time_st_.yyyy_ << "." << time_st_.mm_ << "." << time_st_.dd_;
    } break;

    default: {
      rt_ss << field_type_ << delim_ << "UNKNOWN_DATUM";
      field_buffer_offset_ = msg_length_;
    } break;
  }
  return rt_ss.str();
}

std::vector<char> AF_MsgParser::hex2char(std::string str) {
  // std::stringstream converter;
  std::istringstream ss(str);
  std::vector<char> bytes;

  std::string word;
  while (ss >> word) {
    byte t_byte_ = (byte)strtoul(word.substr(0, 2).c_str(), NULL, 16);
    bytes.push_back(static_cast<char>(t_byte_));
  }
  return bytes;
}

std::vector<char> AF_MsgParser::hex2char_ws(std::string str) {
  std::vector<char> bytes;

  unsigned int ind = 0;
  while (ind + 1 < str.length()) {
    byte t_byte_ = (byte)strtoul(str.substr(ind, 2).c_str(), NULL, 16);
    bytes.push_back((char)t_byte_);
    ind += 2;
  }
  return bytes;
}

std::string AF_MsgParser::msgParse(char *msgdata) {
  std::stringstream parsed_;

  uint16_t msg_length_ = ntoh16(*(reinterpret_cast<uint16_t *>(&msgdata[0])));

  // convert header values
  int32_t txmit_id = (int32_t)ntoh32(*(reinterpret_cast<uint32_t *>(&msgdata[2])));  // signed long int
  // uint32_t crc32 = ntohl(*(reinterpret_cast<uint32_t *> (&msgdata[msg_length_ - CRC_SIZE])) );

  short type_ = msgdata[6];
  uint16_t category_id_ = ntoh16(*(reinterpret_cast<uint16_t *>(&msgdata[8])));
  parsed_ << txmit_id << delim_ << category_id_ << delim_;

  Category *catg_ = getCategoryforId(category_id_);
  if (catg_ == NULL) {
    return parsed_.str();
  }

  Message *msg_ = getMsgFromCatg(catg_, type_);
  if (msg_ == NULL) {
    return parsed_.str();
  }
  parsed_ << category_id_ << delim_ << catg_->category_name_ << delim_ << catg_->description_ << delim_
          << catg_->country_ << delim_ << msg_->msg_type_ << delim_;

  int field_buffer_offset_ = HEADER_SIZE;
  do {
    short field_type_ = msgdata[field_buffer_offset_];
    short field_id_ = msgdata[field_buffer_offset_ + 1];

    Field *t_field_ = getFieldFromMsg(msg_, field_id_, field_type_);
    if (t_field_ == NULL) {
      return parsed_.str();
    }

    Datum *t_datum_ = getDatumFromField(catg_, t_field_->datum_id_);
    if (t_datum_ == NULL) {
      std::cerr << "No match: field type: " << field_type_ << " id: " << field_id_ << " for category: " << category_id_
                << " msg type: " << type_ << std::endl;
      return parsed_.str();
    }

    std::string field_val_ = getFieldTypeValue(t_field_, msgdata, field_buffer_offset_, msg_length_);

    parsed_ << t_field_->datum_id_ << delim_ << t_datum_->desc_ << delim_ << t_datum_->type_ << delim_ << field_val_
            << delim_;
  } while (field_buffer_offset_ < (msg_length_ - CRC_SIZE));

  return parsed_.str();
}
}
