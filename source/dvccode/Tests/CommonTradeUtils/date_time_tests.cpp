// =====================================================================================
//
//       Filename:  dvccode/Tests/dvccode/CommonTradeUtils/date_time_tests.cpp
//
//    Description:  Tests for Utility Functions for fetching the Sample Data values from the last 30 days
//
//        Version:  1.0
//        Created:  01/14/2016 04:31:28 PM
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

#include <cstdio>
#include <ctime>
#include <stdlib.h>

#include "dvccode/Tests/CommonTradeUtils/date_time_tests.hpp"

namespace HFTEST {
using namespace HFSAT;

/**
 * This tests following functions
 * GetTimeMidnight
 * GetTimeFromTZHHMM
 *
 * Following potential test-cases could be there
 * a ) One with Daylight Savings enabled other without
 * b ) 0 and not zero hour  values
 */
void DateTimeTests::TestGetTimeFromTZHHMMStrAEST(void) {
  std::stringstream message;

  /// daylight saving disabled
  int tradingdate = 20161121;

  /// 0 Time
  auto this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "AST_0000");

  message << "Assesrion Failed : Expected Time:  1479646800. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479646800l, this_time);

  /// non Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "AST_1100");

  message << "Assesrion Failed : Expected Time:  1479686400. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479686400l, this_time);

  /// daylight savings enabled
  tradingdate = 20170501;

  // Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "AST_0000");

  message << "Assesrion Failed : Expected Time: 1493560800 . But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493560800l, this_time);

  /// non zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "AST_1100");

  message << "Assesrion Failed : Expected Time:  1493600400. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493600400l, this_time);
}

/**
 * This tests following functions
 * GetTimeMidnight
 * GetTimeFromTZHHMM
 *
 * Following potential test-cases could be there
 * a ) One with Daylight Savings enabled other without
 * b ) 0 and not zero hour  values
 */
void DateTimeTests::TestGetTimeFromTZHHMMStrAMS(void) {
  std::stringstream message;

  /// daylight saving disabled
  int tradingdate = 20161121;

  /// 0 Time
  auto this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "AMS_0000");

  message << "Assesrion Failed : Expected Time:  1479682800. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479682800l, this_time);

  /// non Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "AMS_1100");

  message << "Assesrion Failed : Expected Time:  1479722400. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479722400l, this_time);

  /// daylight savings enabled
  tradingdate = 20170501;

  // Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "AMS_0000");

  message << "Assesrion Failed : Expected Time:  1493589600. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493589600l, this_time);

  /// non zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "AMS_1100");

  message << "Assesrion Failed : Expected Time:  1493629200. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493629200l, this_time);
}

/**
 * This tests following functions
 * GetTimeMidnight
 * GetTimeFromTZHHMM
 *
 * Following potential test-cases could be there
 * a ) One with Daylight Savings enabled other without
 * b ) 0 and not zero hour  values
 */
void DateTimeTests::TestGetTimeFromTZHHMMStrBRT(void) {
  std::stringstream message;

  /// daylight saving disabled
  int tradingdate = 20161121;

  /// 0 Time
  auto this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "BRT_0000");

  message << "Assesrion Failed : Expected Time:  1479693600. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479693600l, this_time);

  /// non Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "BRT_1100");

  message << "Assesrion Failed : Expected Time:  1479733200. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479733200l, this_time);

  /// daylight savings enabled
  tradingdate = 20170501;

  // Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "BRT_0000");

  message << "Assesrion Failed : Expected Time:  1493607600. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493607600l, this_time);

  /// non zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "BRT_1100");

  message << "Assesrion Failed : Expected Time:  1493647200. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493647200l, this_time);
}

/**
 * This tests following functions
 * GetTimeMidnight
 * GetTimeFromTZHHMM
 *
 * Following potential test-cases could be there
 * a ) One with Daylight Savings enabled other without
 * b ) 0 and not zero hour  values
 */
void DateTimeTests::TestGetTimeFromTZHHMMStrBST(void) {
  std::stringstream message;

  /// daylight saving disabled
  int tradingdate = 20161121;

  /// 0 Time
  auto this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "BST_0000");

  message << "Assesrion Failed : Expected Time:  1479686400. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479686400l, this_time);

  /// non Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "BST_1100");

  message << "Assesrion Failed : Expected Time:  1479726000. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479726000l, this_time);

  /// daylight savings enabled
  tradingdate = 20170501;

  // Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "BST_0000");

  message << "Assesrion Failed : Expected Time:  1493593200. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493593200l, this_time);

  /// non zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "BST_1100");

  message << "Assesrion Failed : Expected Time:  1493632800. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493632800l, this_time);
}

/**
 * This tests following functions
 * GetTimeMidnight
 * GetTimeFromTZHHMM
 *
 * Following potential test-cases could be there
 * a ) One with Daylight Savings enabled other without
 * b ) 0 and not zero hour  values
 */
void DateTimeTests::TestGetTimeFromTZHHMMStrCET(void) {
  std::stringstream message;

  /// daylight saving disabled
  int tradingdate = 20161121;

  /// 0 Time
  auto this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "CET_0000");

  message << "Assesrion Failed : Expected Time:  1479682800. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479682800l, this_time);

  /// non Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "CET_1100");

  message << "Assesrion Failed : Expected Time:  1479722400. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479722400l, this_time);

  /// daylight savings enabled
  tradingdate = 20170501;

  // Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "CET_0000");

  message << "Assesrion Failed : Expected Time:  1493589600. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493589600l, this_time);

  /// non zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "CET_1100");

  message << "Assesrion Failed : Expected Time:  1493629200. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493629200l, this_time);
}
/**
 * This tests following functions
 * GetTimeMidnight
 * GetTimeFromTZHHMM
 *
 * Following potential test-cases could be there
 * a ) One with Daylight Savings enabled other without
 * b ) 0 and not zero hour  values
 */
void DateTimeTests::TestGetTimeFromTZHHMMStrCST(void) {
  std::stringstream message;

  /// daylight saving disabled
  int tradingdate = 20161121;

  /// 0 Time
  auto this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "CST_0000");

  message << "Assesrion Failed : Expected Time:  1479708000. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479708000l, this_time);

  /// non Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "CST_1100");

  message << "Assesrion Failed : Expected Time:  1479747600. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479747600l, this_time);

  /// daylight savings enabled
  tradingdate = 20170501;

  // Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "CST_0000");

  message << "Assesrion Failed : Expected Time:  1493614800. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493614800l, this_time);

  /// non zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "CST_1100");

  message << "Assesrion Failed : Expected Time:  1493654400. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493654400l, this_time);
}

/**
 * This tests following functions
 * GetTimeMidnight
 * GetTimeFromTZHHMM
 *
 * Following potential test-cases could be there
 * a ) One with Daylight Savings enabled other without
 * b ) 0 and not zero hour  values
 */
void DateTimeTests::TestGetTimeFromTZHHMMStrEST(void) {
  std::stringstream message;

  /// daylight saving disabled
  int tradingdate = 20161121;

  /// 0 Time
  auto this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "EST_0000");

  message << "Assesrion Failed : Expected Time:  1479704400. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479704400l, this_time);

  /// non Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "EST_1100");

  message << "Assesrion Failed : Expected Time:  1479744000. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479744000l, this_time);

  /// daylight savings enabled
  tradingdate = 20170501;

  // Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "EST_0000");

  message << "Assesrion Failed : Expected Time:  1493611200 . But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493611200l, this_time);

  /// non zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "EST_1100");

  message << "Assesrion Failed : Expected Time:  1493650800 . But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493650800l, this_time);
}

/**
 * This tests following functions
 * GetTimeMidnight
 * GetTimeFromTZHHMM
 *
 * Following potential test-cases could be there
 * a ) One with Daylight Savings enabled other without
 * b ) 0 and not zero hour  values
 */
void DateTimeTests::TestGetTimeFromTZHHMMStrHKT(void) {
  std::stringstream message;

  /// daylight saving disabled
  int tradingdate = 20161121;

  /// 0 Time
  auto this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "HKT_0000");

  message << "Assesrion Failed : Expected Time:  1479657600. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479657600l, this_time);

  /// non Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "HKT_1100");

  message << "Assesrion Failed : Expected Time:  1479697200. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479697200l, this_time);

  /// daylight savings enabled
  tradingdate = 20170501;

  // Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "HKT_0000");

  message << "Assesrion Failed : Expected Time:  1493568000. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493568000l, this_time);

  /// non zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "HKT_1100");

  message << "Assesrion Failed : Expected Time:  1493607600. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493607600l, this_time);
}

/**
 * This tests following functions
 * GetTimeMidnight
 * GetTimeFromTZHHMM
 *
 * Following potential test-cases could be there
 * a ) One with Daylight Savings enabled other without
 * b ) 0 and not zero hour  values
 */
void DateTimeTests::TestGetTimeFromTZHHMMStrIST(void) {
  std::stringstream message;

  /// daylight saving disabled
  int tradingdate = 20161121;

  /// 0 Time
  auto this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "IST_0000");

  message << "Assesrion Failed : Expected Time:  1479666600. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479666600l, this_time);

  /// non Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "IST_1100");

  message << "Assesrion Failed : Expected Time:  1479706200. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479706200l, this_time);

  /// daylight savings enabled
  tradingdate = 20170501;

  // Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "IST_0000");

  message << "Assesrion Failed : Expected Time:  1493577000. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493577000l, this_time);

  /// non zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "IST_1100");

  message << "Assesrion Failed : Expected Time:  1493616600. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493616600l, this_time);
}

/**
 * This tests following functions
 * GetTimeMidnight
 * GetTimeFromTZHHMM
 *
 * Following potential test-cases could be there
 * a ) One with Daylight Savings enabled other without
 * b ) 0 and not zero hour  values
 */
void DateTimeTests::TestGetTimeFromTZHHMMStrJST(void) {
  std::stringstream message;

  /// daylight saving disabled
  int tradingdate = 20161121;

  /// 0 Time
  auto this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "JST_0000");

  message << "Assesrion Failed : Expected Time:  1479654000. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479654000l, this_time);

  /// non Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "JST_1100");

  message << "Assesrion Failed : Expected Time:  1479693600. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479693600l, this_time);

  /// daylight savings enabled
  tradingdate = 20170501;

  // Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "JST_0000");

  message << "Assesrion Failed : Expected Time:  1493564400. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493564400l, this_time);

  /// non zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "JST_1100");

  message << "Assesrion Failed : Expected Time:  1493604000. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493604000l, this_time);
}

/**
 * This tests following functions
 * GetTimeMidnight
 * GetTimeFromTZHHMM
 *
 * Following potential test-cases could be there
 * a ) One with Daylight Savings enabled other without
 * b ) 0 and not zero hour  values
 */
void DateTimeTests::TestGetTimeFromTZHHMMStrKST(void) {
  std::stringstream message;

  /// daylight saving disabled
  int tradingdate = 20161121;

  /// 0 Time
  auto this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "KST_0000");

  message << "Assesrion Failed : Expected Time:  1479654000. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479654000l, this_time);

  /// non Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "KST_1100");

  message << "Assesrion Failed : Expected Time:  1479693600. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479693600l, this_time);

  /// daylight savings enabled
  tradingdate = 20170501;

  // Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "KST_0000");

  message << "Assesrion Failed : Expected Time:  1493564400. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493564400l, this_time);

  /// non zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "KST_1100");

  message << "Assesrion Failed : Expected Time:  1493604000. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493604000l, this_time);
}

/**
 * This tests following functions
 * GetTimeMidnight
 * GetTimeFromTZHHMM
 *
 * Following potential test-cases could be there
 * a ) One with Daylight Savings enabled other without
 * b ) 0 and not zero hour  values
 */
void DateTimeTests::TestGetTimeFromTZHHMMStrGMT(void) {
  std::stringstream message;

  /// daylight saving disabled
  int tradingdate = 20161121;

  /// 0 Time
  auto this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "GMT_0000");

  message << "Assesrion Failed : Expected Time:  1479686400. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479686400l, this_time);

  /// non Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "GMT_1100");

  message << "Assesrion Failed : Expected Time:  1479726000. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479726000l, this_time);

  /// daylight savings enabled
  tradingdate = 20170501;

  // Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "GMT_0000");

  message << "Assesrion Failed : Expected Time:  1493596800. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493596800l, this_time);

  /// non zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "GMT_1100");

  message << "Assesrion Failed : Expected Time:  1493636400. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493636400l, this_time);
}

/**
 * This tests following functions
 * GetTimeMidnight
 * GetTimeFromTZHHMM
 *
 * Following potential test-cases could be there
 * a ) One with Daylight Savings enabled other without
 * b ) 0 and not zero hour  values
 */
void DateTimeTests::TestGetTimeFromTZHHMMStrLON(void) {
  std::stringstream message;

  /// daylight saving disabled
  int tradingdate = 20161121;

  /// 0 Time
  auto this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "LON_0000");

  message << "Assesrion Failed : Expected Time:  1479686400. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479686400l, this_time);

  /// non Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "LON_1100");

  message << "Assesrion Failed : Expected Time:  1479726000. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479726000l, this_time);

  /// daylight savings enabled
  tradingdate = 20170501;

  // Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "LON_0000");

  message << "Assesrion Failed : Expected Time:  1493593200. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493593200l, this_time);

  /// non zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "LON_1100");

  message << "Assesrion Failed : Expected Time:  1493632800. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493632800l, this_time);
}

/**
 * This tests following functions
 * GetTimeMidnight
 * GetTimeFromTZHHMM
 *
 * Following potential test-cases could be there
 * a ) One with Daylight Savings enabled other without
 * b ) 0 and not zero hour  values
 */
void DateTimeTests::TestGetTimeFromTZHHMMStrMSK(void) {
  std::stringstream message;

  /// daylight saving disabled
  int tradingdate = 20161121;

  /// 0 Time
  auto this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "MSK_0000");

  message << "Assesrion Failed : Expected Time:  1479675600. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479675600l, this_time);

  /// non Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "MSK_1100");

  message << "Assesrion Failed : Expected Time:  1479715200. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479715200l, this_time);

  /// daylight savings enabled
  tradingdate = 20170501;

  // Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "MSK_0000");

  message << "Assesrion Failed : Expected Time:  1493586000. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493586000l, this_time);

  /// non zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "MSK_1100");

  message << "Assesrion Failed : Expected Time:  1493625600. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493625600l, this_time);
}

/**
 * This tests following functions
 * GetTimeMidnight
 * GetTimeFromTZHHMM
 *
 * Following potential test-cases could be there
 * a ) One with Daylight Savings enabled other without
 * b ) 0 and not zero hour  values
 */
void DateTimeTests::TestGetTimeFromTZHHMMStrPAR(void) {
  std::stringstream message;

  /// daylight saving disabled
  int tradingdate = 20161121;

  /// 0 Time
  auto this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "PAR_0000");

  message << "Assesrion Failed : Expected Time:  1479682800. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479682800l, this_time);

  /// non Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "PAR_1100");

  message << "Assesrion Failed : Expected Time:  1479722400. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479722400l, this_time);

  /// daylight savings enabled
  tradingdate = 20170501;

  // Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "PAR_0000");

  message << "Assesrion Failed : Expected Time:  1493589600. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493589600l, this_time);

  /// non zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "PAR_1100");

  message << "Assesrion Failed : Expected Time:  1493629200. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493629200l, this_time);
}

/**
 * This tests following functions
 * GetTimeMidnight
 * GetTimeFromTZHHMM
 *
 * Following potential test-cases could be there
 * a ) One with Daylight Savings enabled other without
 * b ) 0 and not zero hour  values
 */
void DateTimeTests::TestGetTimeFromTZHHMMStrSGT(void) {
  std::stringstream message;

  /// daylight saving disabled
  int tradingdate = 20161121;

  /// 0 Time
  auto this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "SGT_0000");

  message << "Assesrion Failed : Expected Time:  1479657600. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479657600l, this_time);

  /// non Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "SGT_1100");

  message << "Assesrion Failed : Expected Time:  1479697200. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479697200l, this_time);

  /// daylight savings enabled
  tradingdate = 20170501;

  // Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "SGT_0000");

  message << "Assesrion Failed : Expected Time:  1493568000. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493568000l, this_time);

  /// non zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "SGT_1100");

  message << "Assesrion Failed : Expected Time:  1493607600. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493607600l, this_time);
}

/**
 * This tests following functions
 * GetTimeMidnightUTC
 * GetBoostDateFromIsoDate
 *
 * Following potential test-cases could be there
 * a ) One with Daylight Savings enabled other without
 * b ) 0 and not zero hour  values
 */
void DateTimeTests::TestGetTimeFromTZHHMMStrUTC(void) {
  std::stringstream message;

  /// daylight saving disabled
  int tradingdate = 20161121;

  /// 0 Time
  auto this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "UTC_0000");

  message << "Assesrion Failed : Expected Time:  1479686400. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479686400l, this_time);

  /// non Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "UTC_1100");

  message << "Assesrion Failed : Expected Time:  1479726000. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1479726000l, this_time);

  /// daylight savings enabled
  tradingdate = 20170501;

  // Zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "UTC_0000");

  message << "Assesrion Failed : Expected Time:  1493596800. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493596800l, this_time);

  /// non zero time
  this_time = DateTime::GetTimeFromTZHHMMStr(tradingdate, "UTC_1100");

  message << "Assesrion Failed : Expected Time:  1493636400. But actual " << this_time;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1493636400l, this_time);
}

/**
 * This tests following functions
 * GetCurrentIsoDateLocal
 * CalcPrevWeekDay
 *
 * Following potential test-cases could be there
 * a ) One for TODAY
 */

void DateTimeTests::TestGetIsoDateFromString(void) {
  std::stringstream message;

  int this_date = DateTime::GetIsoDateFromString("TODAY");

  // Get current date in buffer
  std::time_t rawtime;
  std::tm* timeinfo;
  char buffer[80];

  std::time(&rawtime);
  timeinfo = std::localtime(&rawtime);

  std::strftime(buffer, 80, "%Y%m%d", timeinfo);

  message << "Assesrion Failed : Expected Time: " << atoi(buffer) << ". But actual " << this_date;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), atoi(buffer), this_date);
}

/**
 * This tests following functions
 * GetTimeMidnightUTC
 * Get_UTC_YYYYMMDD_from_ttime
 *
 * Following potential test-cases could be there
 * a ) One with Daylight Savings enabled other without
 * b ) 0 and not zero hour  values
 */

void DateTimeTests::TestGetUTCHHMMFromTime(void) {
  std::stringstream message;

  /// daylight saving disabled - zero hour
  int this_utc = DateTime::GetUTCHHMMFromTime(1479686400);

  message << "Assesrion Failed : Expected Time:  0000. But actual " << this_utc;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 0000, this_utc);

  /// daylight saving disabled - non zero hour
  this_utc = DateTime::GetUTCHHMMFromTime(1479726000);

  message << "Assesrion Failed : Expected Time:  1100. But actual " << this_utc;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1100, this_utc);

  /// daylight saving enabled - zero hour
  this_utc = DateTime::GetUTCHHMMFromTime(1493596800);

  message << "Assesrion Failed : Expected Time:  0000. But actual " << this_utc;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 0000, this_utc);

  /// daylight saving enabled - non zero hour
  this_utc = DateTime::GetUTCHHMMFromTime(1493636400);

  message << "Assesrion Failed : Expected Time:  1100. But actual " << this_utc;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), 1100, this_utc);
}
}
