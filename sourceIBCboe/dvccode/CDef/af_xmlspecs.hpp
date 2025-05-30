// =====================================================================================
//
//       Filename:  af_xmlspecs.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Thursday 11 December 2014 08:14:26  IST
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

#ifndef AF_XMLSPECS_H
#define AF_XMLSPECS_H

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "dvccode/CDef/debug_logger.hpp"
#define MAX_AF_LINE_SIZE 32768

/*  @brief: AF_MSGSPECS namespace contains the specifications for all alphaflash messages
 *  for all categories, it has the messages that that category has, the data specifications that each message has */

namespace AF_MSGSPECS {
static const std::string XML_DIR =
    std::string(getenv("HOME")) + "/" + std::string("infracore_install/SysInfo/Alphaflash/xml_specs");
static const std::string XML_LIST_FNAME(XML_DIR + "/list.xml");

/*  FieldType defines the type of a field of a message */
typedef enum {
  kFloat = 0,
  kFloat_range,              // 1
  kShort,                    // 2
  kLong,                     // 3
  kDouble,                   // 4
  kDate,                     // 5
  kBoolean,                  // 6
  kYes_no_na,                // 7
  kDirectional,              // 8
  kInt,                      // 9
  kTime,                     // 10
  kTimeframe,                // 11
  kEnumeration,              // 12
  kIso_country_code,         // 13
  kShort_value_enumeration,  // 14
  kInvalidFieldType          // 15
} FieldType;

/*  MsgType:
 *  kEstimate: has the estimate values for the event, ( sent a day prior to the event )
 *  kRelease: Actual msg corresponding to the event
 *  kUndefined: Other messages, such as heartbeat and other system msgs */
typedef enum { kRelease = 0, kEstimate, kUndefined } MsgType;

/* Datum: One piece of information for a message */
struct Datum {
  Datum() : scale_("") {}
  std::string short_desc_;
  std::string desc_;
  std::string scale_;  // default: no_scaling
  int id_;
  std::string type_;
};

/*  Field: Encapsulates a datum */
struct Field {
  int datum_id_;
  FieldType field_type_;
  void* eg_value_;
};

struct Message {
  MsgType msg_type_;           // kRelease, kEstimate, kUndefined
  int msg_version_;            // define which xml specification version, it corresponds to
  std::string mult_group_;     // multicast_group, on which it can be received
  std::vector<Field*> fields;  // its data
};

struct Category {
  std::string category_name_;
  int category_id_;
  std::string country_;
  std::string description_;
  std::vector<Datum*> data;
  std::vector<Message*> msgs;
};

std::string& trim_right_inplace(std::string& s, const std::string& delimiters = " \f\n\r\t\v");
std::string& trim_left_inplace(std::string& s, const std::string& delimiters = " \f\n\r\t\v");
std::string& trim_inplace(std::string& s, const std::string& delimiters = " \f\n\r\t\v");

/*  String to MsgType converter */
MsgType parseMsgType(std::string name_str_);

/*  String to FieldType converter */
FieldType parseFieldType(std::string name_str_);

/*  Parses the xml_file and Loads up the Category struct vector  */
void parseXML(std::string xml_fname_, std::vector<Category*>& catg_list, HFSAT::DebugLogger* dbglogger = NULL);

/*  calls parseXML for each xml_file mentioned in list.xml and Loads the Category struct vector */
void GetCatgXMLSpecs(std::vector<Category*>& catg_list, HFSAT::DebugLogger* dbglogger = NULL);

std::string printCategory(Category* catg);

void freeAll(std::vector<Category*>& catg_list);
}

#endif
