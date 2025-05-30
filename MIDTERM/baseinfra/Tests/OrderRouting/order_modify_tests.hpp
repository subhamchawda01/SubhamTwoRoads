// =====================================================================================
//
//       Filename:  order_modify_tests.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/16/2015 05:04:07 PM
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
#include "baseinfra/BaseTrader/sim_trader_helper.hpp"
#include "baseinfra/Tools/common_smv_source.hpp"
#include "baseinfra/SimMarketMaker/sim_market_maker_list.hpp"
#include "baseinfra/SimMarketMaker/sim_market_maker_helper.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/SmartOrderRouting/smart_order_manager.hpp"

namespace HFTEST {

using namespace HFSAT;

class OrderModifyTests : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(OrderModifyTests);
  CPPUNIT_TEST(TestSizeModify);
  CPPUNIT_TEST(TestPriceModify);
  CPPUNIT_TEST_SUITE_END();

 public:
  // These are like constructors and destructors of the Test suite
  // setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  // tearDown -> Destructor | Just free memory etc.
  void setUp(void);
  void tearDown(void);

  void TestSizeModify(void);
  void TestPriceModify(void);

 private:
  CommonSMVSource* common_smv_source_;
  Watch* watch_;
  BaseOrderManager* om_;
  BaseSimMarketMaker* smm_;
  BaseTrader* sim_trader;
  int secid_;
};
}
