// =====================================================================================
//
//       Filename:  dvccode/Tests/dvccode/CommonTradeUtils/sample_dat_utils_tests.hpp
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
#include "dvccode/CommonTradeUtils/sample_data_util.hpp"

namespace HFTEST {
using namespace HFSAT;

class SampleDataUtilsTests : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(SampleDataUtilsTests);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition below.

  CPPUNIT_TEST(TestGetTZDiff);
  CPPUNIT_TEST(TestGetLastSampleBeforeDate);
  CPPUNIT_TEST(TestGetSampleFileName);
  CPPUNIT_TEST(TestGetAvgForPeriod);
  CPPUNIT_TEST(TestGetPercentileForPeriod);
  CPPUNIT_TEST(TestGetAvgForDayTotal);

  CPPUNIT_TEST_SUITE_END();

 public:
  // These are like constructors and destructors of the Test suite
  // setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  // tearDown -> Destructor | Just free memory etc.
  void setUp(void);
  void tearDown(void);

 protected:
  /// We need to add extended list here
  /// For a given shortcode
  void TestGetTZDiff();
  void TestGetLastSampleBeforeDate();
  void TestGetSampleFileName();
  void TestGetAvgForPeriod();
  void TestGetPercentileForPeriod();
  void TestGetAvgForDayTotal();
};
}
