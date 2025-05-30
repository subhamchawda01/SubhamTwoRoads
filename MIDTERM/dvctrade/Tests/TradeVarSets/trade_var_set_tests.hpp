// =====================================================================================
//
//       Filename:  dvctrade/Tests/Indiators/trade_var_set_tests.hpp
//
//    Description:  Tests for thresholds map building function
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
#include "baseinfra/Tools/common_smv_source.hpp"
#include "dvctrade/ExecLogic/ExecLogicHelper/tradevarset_builder.hpp"
#include "dvctrade/InitCommon/paramset.hpp"

namespace HFTEST {

using namespace HFSAT;

class TradeVarSetTests : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(TradeVarSetTests);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition below.
  CPPUNIT_TEST(TestTradeVarsValue);
  CPPUNIT_TEST(TestHighPosTradeVarsValue);

  CPPUNIT_TEST_SUITE_END();

 public:
  // These are like constructors and destructors of the Test suite
  // setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  // tearDown -> Destructor | Just free memory etc.
  void setUp(void);
  void tearDown(void);

 protected:
  void TestTradeVarsValue(void);
  void TestHighPosTradeVarsValue(void);

 private:
  std::vector<std::string> shortcode_vec_;
  int tradingdate_;

  CommonSMVSource* common_smv_source_;
  SecurityMarketView* smv;
  Watch* watch_;
  DebugLogger* dbglogger_;
  TradeVarSetBuilder* tradevarset_builder_;

  /// mapping position to tradevarset
  /// 0 position corresponds to index MAX_POS_MAP_SIZE,
  int map_pos_increment_;  ///< = (int)std::max ( 1, ( param_set_.max_position_ / MAX_POS_MAP_SIZE ) ) )
  /// idx=2*MAX_POS_MAP_SIZE corresponds to position = ( MAX_POS_MAP_SIZE * map_pos_increment_ )
  /// idx=0 corresponds to position = ( MAX_POS_MAP_SIZE * map_pos_increment_ )
  /// position P corresponds to idx= MAX_POS_MAP_SIZE + ( P / map_pos_increment_ )

  PositionTradeVarSetMap position_tradevarset_map_;
  unsigned int P2TV_zero_idx_;
  ParamSet* param_;
};
}
