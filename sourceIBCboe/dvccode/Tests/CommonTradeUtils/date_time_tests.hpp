// =====================================================================================
//
//       Filename:  dvccode/Tests/dvccode/CommonTradeUtils/date_time_tests.hpp
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

#pragma once

#include "dvccode/Tests/TestUtils/run_test.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

namespace HFTEST {
using namespace HFSAT;

class DateTimeTests : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(DateTimeTests);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition below.

  CPPUNIT_TEST(TestGetTimeFromTZHHMMStrAEST);
  CPPUNIT_TEST(TestGetTimeFromTZHHMMStrAMS);
  CPPUNIT_TEST(TestGetTimeFromTZHHMMStrBRT);
  CPPUNIT_TEST(TestGetTimeFromTZHHMMStrBST);
  CPPUNIT_TEST(TestGetTimeFromTZHHMMStrCET);
  CPPUNIT_TEST(TestGetTimeFromTZHHMMStrCST);
  CPPUNIT_TEST(TestGetTimeFromTZHHMMStrEST);
  CPPUNIT_TEST(TestGetTimeFromTZHHMMStrHKT);
  CPPUNIT_TEST(TestGetTimeFromTZHHMMStrIST);
  CPPUNIT_TEST(TestGetTimeFromTZHHMMStrJST);
  CPPUNIT_TEST(TestGetTimeFromTZHHMMStrKST);
  CPPUNIT_TEST(TestGetTimeFromTZHHMMStrGMT);
  CPPUNIT_TEST(TestGetTimeFromTZHHMMStrLON);
  CPPUNIT_TEST(TestGetTimeFromTZHHMMStrMSK);
  CPPUNIT_TEST(TestGetTimeFromTZHHMMStrPAR);
  CPPUNIT_TEST(TestGetTimeFromTZHHMMStrSGT);
  CPPUNIT_TEST(TestGetTimeFromTZHHMMStrUTC);
  CPPUNIT_TEST(TestGetIsoDateFromString);
  CPPUNIT_TEST(TestGetUTCHHMMFromTime);

  CPPUNIT_TEST_SUITE_END();

 public:
  // These are like constructors and destructors of the Test suite
  // setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  // tearDown -> Destructor | Just free memory etc.
  void setUp(void) {}
  void tearDown(void) {}

 protected:
  void TestGetTimeFromTZHHMMStrAEST(void);
  void TestGetTimeFromTZHHMMStrAMS(void);
  void TestGetTimeFromTZHHMMStrBRT(void);
  void TestGetTimeFromTZHHMMStrBST(void);
  void TestGetTimeFromTZHHMMStrCET(void);
  void TestGetTimeFromTZHHMMStrCST(void);
  void TestGetTimeFromTZHHMMStrEST(void);
  void TestGetTimeFromTZHHMMStrHKT(void);
  void TestGetTimeFromTZHHMMStrIST(void);
  void TestGetTimeFromTZHHMMStrJST(void);
  void TestGetTimeFromTZHHMMStrKST(void);
  void TestGetTimeFromTZHHMMStrGMT(void);
  void TestGetTimeFromTZHHMMStrLON(void);
  void TestGetTimeFromTZHHMMStrMSK(void);
  void TestGetTimeFromTZHHMMStrPAR(void);
  void TestGetTimeFromTZHHMMStrSGT(void);
  void TestGetTimeFromTZHHMMStrUTC(void);

  void TestGetIsoDateFromString(void);
  void TestGetUTCHHMMFromTime(void);
  /// We need to add extended list here
  /// For a given shortcode
};
}
