/**
   \file Tests/dvctrade/OrderRouting/om_tests.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include "dvccode/Tests/TestUtils/run_test.hpp"
#include "baseinfra/Tools/common_smv_source.hpp"
#include "baseinfra/BaseTrader/sim_trader_helper.hpp"
#include "baseinfra/SimMarketMaker/sim_market_maker_list.hpp"
#include "baseinfra/SimMarketMaker/sim_market_maker_helper.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/SmartOrderRouting/smart_order_manager.hpp"

namespace HFTEST {

using namespace HFSAT;

/*
 * The Test class in which we write the test cases.
 */
class OmTests : public CppUnit::TestFixture {
  //  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(OmTests);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition below.
  CPPUNIT_TEST(TestBidIndexHighLow);
  CPPUNIT_TEST(TestAskIndexHighLow);

  CPPUNIT_TEST(TestReject);

  /// Test cases related to operations on order
  CPPUNIT_TEST(TestIntExecSeqdRejc);
  CPPUNIT_TEST(TestReject);
  CPPUNIT_TEST(TestBidSequenced);
  CPPUNIT_TEST(TestAskSequenced);
  CPPUNIT_TEST(TestBidConfirmed);
  CPPUNIT_TEST(TestAskConfirmed);
  CPPUNIT_TEST(TestBidCancel);
  CPPUNIT_TEST(TestAskCancel);
  CPPUNIT_TEST(TestBidCancelReject);
  CPPUNIT_TEST(TestAskCancelReject);
  CPPUNIT_TEST(TestBidExec);
  CPPUNIT_TEST(TestAskExec);
  CPPUNIT_TEST(TestFreezeDurToOrsNoReply);
  CPPUNIT_TEST(TestCancelBeforeConfirm);
  CPPUNIT_TEST_SUITE_END();

 public:
  // These are like constructors and destructors of the Test suite
  // setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  // tearDown -> Destructor | Just free memory etc.
  void setUp(void);
  void tearDown(void);

 protected:
  // Test Cases

  /// Test cases for bid/ask-index handling should be there
  //
  void TestBidIndexHighLow(void);
  void TestAskIndexHighLow(void);

  /// Test cases related to operations on order
  void TestIntExecSeqdRejc(void);
  void TestReject(void);
  // Check for bid and ask sequenced separately
  void TestBidSequenced(void);
  void TestAskSequenced(void);

  // Check for bid and ask confirmed separately
  void TestBidConfirmed(void);
  void TestAskConfirmed(void);

  void TestBidCancel(void);
  void TestAskCancel(void);

  void TestBidCancelReject(void);
  void TestAskCancelReject(void);

  void TestBidExec(void);
  void TestAskExec(void);

  void TestFreezeDurToOrsNoReply(void);
  void TestCancelBeforeConfirm(void);
  // private:

  std::vector<std::string> shortcode_list_;
  SimTimeSeriesInfo* sim_time_series_info_;
  CommonSMVSource* common_smv_source_;
  std::vector<HFSAT::MarketOrdersView*> sid_to_mov_ptr_map_;

  SecurityMarketView* smv_;
  Watch* watch_;
  BaseSimMarketMaker* smm_;
  BaseTrader* sim_trader_;
  unsigned int sec_id_;
  int time_at_midnight_;	
};
}
