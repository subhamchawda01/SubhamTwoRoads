// =====================================================================================
//
//       Filename:  baseinfra/Tests/dvctrade/MarketAdapter/base_market_view_manager_tests.cpp
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

#include "baseinfra/Tests/MarketAdapter/mvm_tests_utils.hpp"

#include "baseinfra/Tools/common_smv_source.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"

#define LOW_ACCESS_INDEX 50

namespace HFTEST {
using namespace HFSAT;

class BaseMarketViewManagerTests : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(BaseMarketViewManagerTests);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition below.

  CPPUNIT_TEST(TestBidRebuildIndexHighAccess);
  CPPUNIT_TEST(TestAskRebuildIndexHighAccess);
  CPPUNIT_TEST(TestBidRebuildIndexLowAccess);
  CPPUNIT_TEST(TestAskRebuildIndexLowAccess);
  CPPUNIT_TEST(TestBidRescaleBook);
  CPPUNIT_TEST(TestAskRescaleBook);
  CPPUNIT_TEST(TestBidBuildIndex);
  CPPUNIT_TEST(TestAskBuildIndex);

  CPPUNIT_TEST(TestOrderExecuted);
  CPPUNIT_TEST(TestUpdateBestBidVariablesUsingOurOrders);
  CPPUNIT_TEST(TestUpdateBestAskVariablesUsingOurOrders);
  CPPUNIT_TEST(TestUpdateBestBidVariables);
  CPPUNIT_TEST(TestUpdateBestAskVariables);
  CPPUNIT_TEST(TestSanitizeBidSide);
  CPPUNIT_TEST(TestSanitizeAskSide);
  CPPUNIT_TEST(TestAdjustAskIndex);
  CPPUNIT_TEST(TestAdjustBidIndex);

  CPPUNIT_TEST_SUITE_END();

 public:
  // These are like constructors and destructors of the Test suite
  // setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  // tearDown -> Destructor | Just free memory etc.
  void setUp(void);
  void tearDown(void);

 protected:
  void TestBidRebuildIndexHighAccess(void);
  void TestAskRebuildIndexHighAccess(void);
  void TestBidRebuildIndexLowAccess(void);
  void TestAskRebuildIndexLowAccess(void);
  void TestBidRescaleBook(void);
  void TestAskRescaleBook(void);
  void TestBidBuildIndex(void);
  void TestAskBuildIndex(void);
  void TestOrderExecuted(void);
  void TestUpdateBestBidVariablesUsingOurOrders(void);
  void TestUpdateBestAskVariablesUsingOurOrders(void);
  void TestUpdateBestBidVariables(void);
  void TestUpdateBestAskVariables(void);
  void TestSanitizeBidSide(void);
  void TestSanitizeAskSide(void);
  void TestAdjustAskIndex(void);
  void TestAdjustBidIndex(void);

 private:
  int tradingdate_;
  std::vector<std::string> shortcode_vec_;

  CommonSMVSource* common_smv_source_;

  SecurityMarketView* smv_;
  Watch* watch_;
};
}
