/**
   \file Tests/dvctrade/MarketAdapter/security_market_view_tests.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "baseinfra/Tools/common_smv_source.hpp"

#include "dvccode/Tests/TestUtils/run_test.hpp"

namespace HFTEST {

using namespace HFSAT;

class SecurityMarketViewTests : public CppUnit::TestFixture {
  /**
   * General testing framework,
   * Currently It reads inputs from a file and processes accordingly
   */
  CPPUNIT_TEST_SUITE(SecurityMarketViewTests);

  // Add unit tests to testsuite
  CPPUNIT_TEST(TestMidPrice);
  CPPUNIT_TEST(TestMktPrice);
  CPPUNIT_TEST(TestOrderSizeWPrice);
  CPPUNIT_TEST(TestOrderWPrice);
  CPPUNIT_TEST(TestMktSinuSoidal);
  CPPUNIT_TEST(TestTradeWPrice);
  CPPUNIT_TEST(TestOfflineMixMMS);
  CPPUNIT_TEST(TestValidLevelPrice);
  CPPUNIT_TEST(TestTradeOrderWPrice);
  CPPUNIT_TEST(TestTradeMktSizeWPrice);
  CPPUNIT_TEST(TestTradeMktSinPrice);
  CPPUNIT_TEST(TestTradeTradeWPrice);
  CPPUNIT_TEST(TestTradeOmixPrice);
  CPPUNIT_TEST(TestOnlineMixPrice);
  CPPUNIT_TEST(TestStableBidPrice);
  CPPUNIT_TEST(TestStableAskPrice);

  CPPUNIT_TEST_SUITE_END();

  void TestMidPrice(void);
  void TestMktPrice(void);
  void TestOrderSizeWPrice(void);
  void TestOrderWPrice(void);
  void TestMktSinuSoidal(void);

  void TestTradeWPrice(void);
  void TestOfflineMixMMS(void);
  void TestValidLevelPrice(void);

  void TestTradeOrderWPrice(void);
  void TestTradeMktSizeWPrice(void);
  void TestTradeMktSinPrice(void);
  void TestTradeTradeWPrice(void);
  void TestTradeOmixPrice(void);
  void TestOnlineMixPrice(void);
  void TestStableBidPrice(void);
  void TestStableAskPrice(void);

 private:
  CommonSMVSource* common_smv_source_;
  Watch* watch_;

 public:
  void setUp(void);
  void tearDown(void);
};
}
