// =====================================================================================
//
//       Filename:  dvccode/Tests/dvccode/CommonTradeUtils/watch_tests.hpp
//
//    Description:  Tests for Utility Functions for fetching the Sample Data values from the last 30 days
//
//        Version:  1.0
//        Created:
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
#include "dvccode/Tests/CommonTradeUtils/watch_tests_helper.hpp"

#include "dvccode/CommonTradeUtils/watch.hpp"

namespace HFTEST {
using namespace HFSAT;

class WatchTests : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(WatchTests);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition below.

  CPPUNIT_TEST(TestResetWatch);
  CPPUNIT_TEST(TestMidnightChange);
  CPPUNIT_TEST(TestTimePeriodListener);
  CPPUNIT_TEST(TestBigTimePeriodListener);
  CPPUNIT_TEST(TestFifteenSecondsTimePeriodListener);
  CPPUNIT_TEST(TestOneMinutesTimePeriodListener);
  CPPUNIT_TEST(TestFifteenMinutesTimePeriodListener);

  CPPUNIT_TEST_SUITE_END();

 public:
  // These are like constructors and destructors of the Test suite
  // setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  // tearDown -> Destructor | Just free memory etc.
  void setUp(void);
  void tearDown(void);

 protected:
  void TestResetWatch(void);
  void TestMidnightChange(void);
  void TestTimePeriodListener(void);
  void TestBigTimePeriodListener(void);
  void TestFifteenSecondsTimePeriodListener(void);
  void TestOneMinutesTimePeriodListener(void);
  void TestFifteenMinutesTimePeriodListener(void);

 private:
  DebugLogger* dbglogger_;
  Watch* watch_;
  int sec_start_time_;
  WatchListener* dummy_listener_;
  WatchListener* time_period_lisener_;
  WatchListener* big_time_period_listener_;
  WatchListener* fifteen_seconds_time_period_listener_;
  WatchListener* one_minute_time_period_listener_;
  WatchListener* fifteen_minutes_time_period_listner_;
};
}
