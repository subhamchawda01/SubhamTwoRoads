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

#include "dvctrade/Tests/Indicators/common_indicator_tests.hpp"


namespace HFTEST {
using namespace HFSAT;

void CommonIndicatorTests::setUp(void) {
  std::vector<std::string> shortcode_vec_;
  int tradingdate_;
  Watch* watch_;
  CommonSMVSource* common_smv_source_;
  shortcode_vec_.push_back("FGBM_0");
  tradingdate_ = 20170610;
  common_smv_source_ = new CommonSMVSource(shortcode_vec_, tradingdate_, false);
  common_smv_source_->InitializeVariables();
  watch_ = &common_smv_source_->getWatch();
  DebugLogger* dbglogger_ = &common_smv_source_->getLogger();
  std::string _portfolio_descriptor_shortcode_ = "test_desc";
  dummy_common_indicator = new DummyCommonIndicator(*dbglogger_, *watch_, _portfolio_descriptor_shortcode_);
}

void CommonIndicatorTests::TestSetTimeDecayWeights(void) {
  dummy_common_indicator->SetTimeDecayWeights();

  // Assuming the default value of PageWidth =500  and DecayPageFactor = 0.95
  CPPUNIT_ASSERT_DOUBLES_EQUAL(10, dummy_common_indicator->GetPageWidth(), DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5, dummy_common_indicator->GetDecayPageFactor(), DOUBLE_ASSERT_PRECISION);
}

void CommonIndicatorTests::tearDown() {
  delete dummy_common_indicator;
  dummy_common_indicator = nullptr;
  HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
  HFSAT::SecurityDefinitions::RemoveUniqueInstance();
  HFSAT::CurrencyConvertor::RemoveInstance();

  delete common_smv_source_;
  common_smv_source_ = nullptr;
}
}
