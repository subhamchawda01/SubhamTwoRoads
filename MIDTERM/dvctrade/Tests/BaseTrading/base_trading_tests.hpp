// =====================================================================================
//
//       Filename:  dvctrade/Tests/BaseTrading/base_trading_tests.hpp
//
//    Description:  Tests for Basetrading code
//
//        Version:  1.0
//        Created:  08/02/2017 04:31:28 PM
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
#include "dvctrade/ExecLogic/base_trading.hpp"

#include "baseinfra/Tools/common_smv_source.hpp"
// #include "dvctrade/InitCommon/paramset.hpp"
#include "baseinfra/SimMarketMaker/sim_market_maker_list.hpp"
#include "baseinfra/SimMarketMaker/sim_market_maker_helper.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/SmartOrderRouting/smart_order_manager.hpp"
#include "baseinfra/BaseTrader/sim_trader_helper.hpp"
#include "baseinfra/SimPnls/sim_base_pnl.hpp"
#include "dvctrade/ExecLogic/price_based_aggressive_trading.hpp"
#include "baseinfra/Tests/MarketAdapter/mvm_tests_utils.hpp"
#include "dvctrade/ModelMath/model_creator.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "baseinfra/SimMarketMaker/sim_config.hpp"

namespace HFTEST {

using namespace HFSAT;

class BaseTradingTests : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(BaseTradingTests);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition below.

  CPPUNIT_TEST(TestCollectORSShortCodes);
  CPPUNIT_TEST(TestCollectShortCodes);
  CPPUNIT_TEST(TestsOnPositionChange);
  CPPUNIT_TEST(TestsGetFlatTradingLogic);
  CPPUNIT_TEST(TestsPlaceCancelNonBestLevels);
  CPPUNIT_TEST(TestsProcessAllowedEco);
  CPPUNIT_TEST_SUITE_END();

 public:
  // These are like constructors and destructors of the Test suite
  // setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  // tearDown -> Destructor | Just free memory etc.
  void setUp(void);
  void tearDown(void);

 protected:
   void TestCollectORSShortCodes(void);
   void TestCollectShortCodes(void);
   void TestsOnPositionChange(void);
   void TestsGetFlatTradingLogic(void);
   void TestsPlaceCancelNonBestLevels(void);
   void TestsProcessAllowedEco(void);

 private:
  CommonSMVSource* common_smv_source_;
  Watch* watch_;
  SmartOrderManager* om_;
  BaseSimMarketMaker* smm_;
  BaseTrader* sim_trader;
  BaseTrading* bt_;
  SecurityNameIndexer& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  SimTimeSeriesInfo* sim_time_series_info_ptr_  = new SimTimeSeriesInfo(sec_name_indexer.NumSecurityId());
  int secid_;
  HFSAT::BulkFileWriter* trades_writer_;
  BookInfoManager* book_info_manager_;

  HFSAT::EconomicEventsManager* economic_events_manager_;
  BaseModelMath* base_model_math_;
  int date_to_run;

};
}
