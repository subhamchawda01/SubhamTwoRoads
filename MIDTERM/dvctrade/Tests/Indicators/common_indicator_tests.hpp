// =====================================================================================
//
//       Filename:  dvctrade/Tests/Indiators/book_info_manager_tests.cpp
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
#include "dvctrade/ExecLogic/ExecLogicHelper/indicator_helper.hpp"
#include "dvctrade/ModelMath/base_model_math.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/SmartOrderRouting/smart_order_manager.hpp"
#include "baseinfra/Tools/common_smv_source.hpp"
#include "dvctrade/Tests/Indicators/dummy_common_indicator.hpp"
namespace HFTEST {
using namespace HFSAT;

class CommonIndicatorTests : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(CommonIndicatorTests);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition belo
  CPPUNIT_TEST(TestSetTimeDecayWeights);

  CPPUNIT_TEST_SUITE_END();

 public:
  // These are like constructors and destructors of the Test suite
  // setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  // tearDown -> Destructor | Just free memory etc.
  void setUp(void);
  void tearDown(void);

 protected:
  void TestSetTimeDecayWeights();

 private:
  DummyCommonIndicator* dummy_common_indicator;
  CommonSMVSource* common_smv_source_;
};
}
