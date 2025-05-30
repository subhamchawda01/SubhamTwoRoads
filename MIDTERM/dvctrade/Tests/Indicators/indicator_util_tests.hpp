// =====================================================================================
//
//       Filename:  dvctrade/Tests/Indicators/indicator_util_tests.hpp
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
#include "dvctrade/Indicators/indicator_util.hpp"

namespace HFTEST {

class IndicatorUtilsTests : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(IndicatorUtilsTests);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition below.

  CPPUNIT_TEST(TestGetLastTwoArgumentsFromIndicator);
  CPPUNIT_TEST(TestOfflineReturnsRatio);
  //  CPPUNIT_TEST(TestLoadDI1EigenVector);
  CPPUNIT_TEST(TestAddDI1PortfolioShortCodeVec);
  CPPUNIT_TEST(TestMinPriceIncrementForPricePortfolio);
  CPPUNIT_TEST(TestCollectShortcodeOrPortfolio);
  CPPUNIT_TEST(TestIndicatorHasExchange);

  CPPUNIT_TEST_SUITE_END();

 public:
  // These are like constructors and destructors of the Test suite
  // setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  // tearDown -> Destructor | Just free memory etc.
  void setUp(void);
  void tearDown(void);

 protected:
  void TestGetLastTwoArgumentsFromIndicator(void);
  void TestOfflineReturnsRatio(void);
  void TestLoadDI1EigenVector(void);
  void TestAddDI1PortfolioShortCodeVec(void);
  void TestMinPriceIncrementForPricePortfolio(void);
  void TestCollectShortcodeOrPortfolio(void);
  void TestIndicatorHasExchange(void);
};
}
