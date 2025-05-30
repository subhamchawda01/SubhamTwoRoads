// =====================================================================================
//
//       Filename:  dvctrade/Tests/Indiators/book_info_manager_tests.hpp
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
#include "dvccode/Tests/TestUtils/cpptest_utils.hpp"
#include "baseinfra/Tests/MarketAdapter/mvm_tests_utils.hpp"

#include "baseinfra/Tools/common_smv_source.hpp"
#include "dvctrade/Indicators/book_info_manager.hpp"

namespace HFTEST {

using namespace HFSAT;

class BookInfoManagerTests : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(BookInfoManagerTests);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition below.
  CPPUNIT_TEST(TestComputeSumSize);
  CPPUNIT_TEST(TestComputeSumFactorSize);
  CPPUNIT_TEST(TestComputeSumPrice);
  CPPUNIT_TEST(TestComputeSumFactorPriceSize);
  CPPUNIT_TEST(TestComputeSumOrder);
  CPPUNIT_TEST(TestComputeSumFactorPriceOrder);
  CPPUNIT_TEST(TestComputeSumFactor);
  CPPUNIT_TEST(TestDynamicScaling);

  CPPUNIT_TEST_SUITE_END();

 public:
  // These are like constructors and destructors of the Test suite
  // setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  // tearDown -> Destructor | Just free memory etc.
  void setUp(void);
  void tearDown(void);

 protected:
  void TestComputeSumSize(void);
  void TestComputeSumFactorSize(void);
  void TestComputeSumPrice(void);
  void TestComputeSumFactorPriceSize(void);
  void TestComputeSumOrder(void);
  void TestComputeSumFactorPriceOrder(void);
  void TestComputeSumFactor(void);
  void TestDynamicScaling(void);

 private:
  std::vector<std::string> shortcode_vec_;
  int tradingdate_;

  Watch* watch_;
  SecurityMarketView* smv_;
  BookInfoManager* book_info_manager_;
  CommonSMVSource* common_smv_source_;
};
}
