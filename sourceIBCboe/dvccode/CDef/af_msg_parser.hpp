// =====================================================================================
//
//       Filename:  af_msg_parser.hpp
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

#ifndef AF_MSG_PARSER_H
#define AF_MSG_PARSER_H

#include <iostream>
#include <iomanip>
#include <string>

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/af_xmlspecs.hpp"

typedef std::uint8_t byte;
#define ntoh64 __builtin_bswap64
#define ntoh32 __builtin_bswap32
#define ntoh16 ntohs

namespace AF_MSGSPECS {
static const int HEADER_SIZE = 10;  // Size of header on the wire
static const int CRC_SIZE = 4;      // Size of trailing CRC on the wire

static const short FLOAT_FIELD_SIZE = 4;        // Size of float on the wire
static const short SHORT_FIELD_SIZE = 2;        // Size of short on the wire
static const short LONG_FIELD_SIZE = 8;         // Size of long on the wire
static const short DOUBLE_FIELD_SIZE = 8;       // Size of double on the wire
static const short BOOL_FIELD_SIZE = 1;         // Size of boolean on the wire
static const short YES_NO_NA_FIELD_SIZE = 1;    // Size of yes_no_na on the wire
static const short DIRECTIONAL_FIELD_SIZE = 1;  // Size of directional on the wire
static const short INT_FIELD_SIZE = 4;          // Size of int on the wire
static const short TIME_FIELD_SIZE = 4;         // Size of int on the wire

static const short FLOAT_INDICATOR_SIZE = FLOAT_FIELD_SIZE + 2;              // Size of type/id/float on the wire
static const short SHORT_INDICATOR_SIZE = SHORT_FIELD_SIZE + 2;              // Size of type/id/short on the wire
static const short LONG_INDICATOR_SIZE = LONG_FIELD_SIZE + 2;                // Size of type/id/long on the wire
static const short DOUBLE_INDICATOR_SIZE = DOUBLE_FIELD_SIZE + 2;            // Size of type/id/double on the wire
static const short BOOL_INDICATOR_SIZE = BOOL_FIELD_SIZE + 2;                // Size of type/id/boolean on the wire
static const short YES_NO_NA_INDICATOR_SIZE = YES_NO_NA_FIELD_SIZE + 2;      // Size of type/id/yes_no_na on the wire
static const short DIRECTIONAL_INDICATOR_SIZE = DIRECTIONAL_FIELD_SIZE + 2;  // Size of type/id/directional on the wire
static const short INT_INDICATOR_SIZE = INT_FIELD_SIZE + 2;                  // Size of type/id/int on the wire
static const short TIME_INDICATOR_SIZE = TIME_FIELD_SIZE + 2;                // Size of type/id/int on the wire

struct Time_st {
 public:
  Time_st() {}
  Time_st(std::string date_) {
    yyyy_ = (short)atoi(date_.substr(0, 4).c_str());
    mm_ = date_.at(5);
    dd_ = date_.at(6);
  }
  unsigned short yyyy_;
  char mm_;
  char dd_;
};

class AF_MsgParser {
 protected:
  std::vector<Category *> catg_list;
  HFSAT::DebugLogger *dbglogger_;
  char delim_;

 protected:
  AF_MsgParser(HFSAT::DebugLogger *dbglogger = NULL, char _delim_ = '|') : dbglogger_(dbglogger), delim_(_delim_) {
    GetCatgXMLSpecs(catg_list, dbglogger_);
  }

  ~AF_MsgParser() { freeAll(catg_list); }

 public:
  static AF_MsgParser &GetUniqueInstance(HFSAT::DebugLogger *dbglogger = NULL) {
    static AF_MsgParser af_msgparser_(dbglogger);
    return af_msgparser_;
  }

  Category *getCategoryforId(uint16_t catg_id_);
  Message *getMsgFromCatg(Category *t_catg_, short msg_type_);
  Field *getFieldFromMsg(Message *t_msg_, short field_id_, short field_type_);
  Field *getFieldFromMsg(Message *t_msg_, short field_id_);
  Datum *getDatumFromField(Category *t_catg_, short datum_id_);
  std::string getFieldTypeValue(Field *t_field_, char *msgdata, int &field_buffer_offset_, int msg_length_);

  double swapDouble(uint8_t *dblBuf);
  float swapFloat(uint8_t *dblBuf);
  std::vector<char> hex2char(std::string str);
  std::vector<char> hex2char_ws(std::string str);

  /*  Prints a formatted parsed msg, with input message contents as byte array taken directly from the wire */
  std::string msgParse(char *msgdata);
};
}

#endif
