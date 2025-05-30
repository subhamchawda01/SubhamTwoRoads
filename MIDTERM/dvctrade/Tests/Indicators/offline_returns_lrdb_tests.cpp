// =====================================================================================
//
//       Filename:  dvctrade/Tests/Indicators/offline_returns_lrdb_tests.cpp
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

#include "dvctrade/Tests/Indicators/offline_returns_lrdb_tests.hpp"

namespace HFTEST {
using namespace HFSAT;

void OfflineReturnsLRDBTests::setUp(void) {
  // using FGBM_0 as dependent for all lrdb tests
  // indeps will vary to accomodate the different test-cases
  shortcode_vec_.push_back("FGBM_0");

  // tests will start with the following date
  // if needed, tests will be done on few other dates
  // to test the lrdb values across different months
  tradingdate_ = 20170610;

  // initializing common_smv_source and fetching watch from it
  common_smv_source_ = new CommonSMVSource(shortcode_vec_, tradingdate_, false);
  common_smv_source_->InitializeVariables();
  watch_ = &common_smv_source_->getWatch();

  // generating the lrdb object for FGBM_0
  lrdb_ = &OfflineReturnsLRDB::GetUniqueInstance(common_smv_source_->getLogger(), *watch_, shortcode_vec_[0]);
}

void OfflineReturnsLRDBTests::TestLRCoeffPresent(void) {
  bool lr_present_ = false;
  std::string dep_ = shortcode_vec_[0];

  // test presence for FGBM_0^ZN_0
  std::string indep_ = "ZN_0";
  lr_present_ = lrdb_->LRCoeffPresent(dep_, indep_);
  CPPUNIT_ASSERT_MESSAGE("LRCoeffPresent for FGBM_0^ZN_0 must return true", lr_present_);

  // test presence for FGBM_0^Si_0
  indep_ = "Si_0";
  lr_present_ = lrdb_->LRCoeffPresent(dep_, indep_);
  CPPUNIT_ASSERT_MESSAGE("LRCoeffPresent for FGBM_0^Si_0 must return false", ~lr_present_);
}

void OfflineReturnsLRDBTests::TestLRCoeffPresentForSupervisedPort(void) {
  bool lr_present_ = false;
  std::string dep_ = shortcode_vec_[0];

  // test LRCoeffPresent for FGBM_0^EBFUT:FGBM_0
  // Since "EBFUT:FGBM_0" has a Supervised-Port format,
  // LRCoeffPresent should return true
  std::string indep_ = "EBFUT:FGBM_0";
  lr_present_ = lrdb_->LRCoeffPresent(dep_, indep_);
  CPPUNIT_ASSERT_MESSAGE("LRCoeffPresent for FGBM_0^EBFUT:FGBM_0 must return true", lr_present_);
}

void OfflineReturnsLRDBTests::TestGetLRCoeffForSupervisedPort(void) {
  LRInfo current_lrinfo_;
  std::string msg_;
  std::string dep_ = shortcode_vec_[0];

  // test presence for FGBM_0^EBFUT:FGBM_0
  // Since "EBFUT:FGBM_0" has a Supervised-Port format,
  // lrinfo should be (1,1)
  std::string indep_ = "EBFUT:FGBM_0";
  current_lrinfo_ = lrdb_->GetLRCoeff(dep_, indep_);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(1, current_lrinfo_.lr_coeff_, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(1, current_lrinfo_.lr_correlation_, DOUBLE_ASSERT_PRECISION);
}

void OfflineReturnsLRDBTests::TestGetLRCoeff(void) {
  LRInfo current_lrinfo_;
  std::string msg_;
  std::string dep_ = shortcode_vec_[0];
  std::string date_str = std::to_string(tradingdate_);

  // test presence for FGBM_0^Si_0
  // Since the beta for this pair doesn't exist,
  // beta, correlation must be returned as (0,0)
  std::string indep_ = "Si_0";
  current_lrinfo_ = lrdb_->GetLRCoeff(dep_, indep_);
  msg_ = "lrinfo for FGBM_0,Si_0 must be (0,0)";
  CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(msg_, 0, current_lrinfo_.lr_coeff_, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(msg_, 0, current_lrinfo_.lr_correlation_, DOUBLE_ASSERT_PRECISION);


  // FGBM_0^ZN_0
  // EU: CET_1000, US: EST_1000
  // Tests: beta must lie within (0.35, 0.8); corr within (0.5, 1.0)
  // eu beta > us_beta (since fgbm sd in eu is higher)
  // (it will validate the functionality of SetLRDBSessionNow)
  {
    indep_ = "ZN_0";

    // EU hours
    ttime_t current_time = ttime_t (DateTime::GetTimeFromTZHHMMStr(tradingdate_, "CET_1000"), 0);
    watch_->OnTimeReceived(current_time);
    LRInfo current_lrinfo_eu_ = lrdb_->GetLRCoeff(dep_, indep_);

    msg_ = "lr_coeff for FGBM_0,ZN_0 " + date_str + ", EU session must lie between (0.3,0.8)";
    CPPUNIT_ASSERT_MESSAGE(msg_, current_lrinfo_eu_.lr_coeff_ > 0.3);
    CPPUNIT_ASSERT_MESSAGE(msg_, current_lrinfo_eu_.lr_coeff_ < 0.8);

    msg_ = "lr_correlation for FGBM_0,ZN_0 " + date_str + ", EU session must lie between (0.5,1)";
    CPPUNIT_ASSERT_MESSAGE(msg_, current_lrinfo_eu_.lr_correlation_ > 0.5);
    CPPUNIT_ASSERT_MESSAGE(msg_, current_lrinfo_eu_.lr_correlation_ < 1.0);

    // US hours
    current_time = ttime_t (DateTime::GetTimeFromTZHHMMStr(tradingdate_, "EST_1000"), 0);
    watch_->OnTimeReceived(current_time);
    LRInfo current_lrinfo_us_ = lrdb_->GetLRCoeff(dep_, indep_);

    msg_ = "lr_coeff for FGBM_0,ZN_0 " + date_str + ", US session must lie between (0.25,0.6)";
    CPPUNIT_ASSERT_MESSAGE(msg_, current_lrinfo_us_.lr_coeff_ > 0.25);
    CPPUNIT_ASSERT_MESSAGE(msg_, current_lrinfo_us_.lr_coeff_ < 0.6);

    msg_ = "lr_correlation for FGBM_0,ZN_0 " + date_str + ", US session must lie between (0.5,1)";
    CPPUNIT_ASSERT_MESSAGE(msg_, current_lrinfo_us_.lr_correlation_ > 0.5);
    CPPUNIT_ASSERT_MESSAGE(msg_, current_lrinfo_us_.lr_correlation_ < 1.0);

    // beta (EU) < beta (US)
    msg_ = "lr_coeff(FGBM_0,ZN_0,EU-session) must exceed lr_coeff(FGBM_0,ZN_0,US-session)";
    CPPUNIT_ASSERT_MESSAGE(msg_, current_lrinfo_us_.lr_coeff_ < current_lrinfo_eu_.lr_coeff_);
  }

  // FGBM_0^EBFUT
  // Tests: corr within (0.9, 1.0)
  {
    indep_ = "EBFUT";

    // US hours
    ttime_t current_time = ttime_t (DateTime::GetTimeFromTZHHMMStr(tradingdate_, "EST_1000"), 0);
    watch_->OnTimeReceived(current_time);
    LRInfo current_lrinfo_us_ = lrdb_->GetLRCoeff(dep_, indep_);

    msg_ = "lr_correlation for FGBM_0,EBFUT " + date_str + " must lie between (0.9,1)";
    CPPUNIT_ASSERT_MESSAGE(msg_, current_lrinfo_us_.lr_correlation_ > 0.9);
    CPPUNIT_ASSERT_MESSAGE(msg_, current_lrinfo_us_.lr_correlation_ < 1.0);
  }
}

void OfflineReturnsLRDBTests::tearDown(void) {

  HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
  HFSAT::SecurityDefinitions::RemoveUniqueInstance();
  HFSAT::SecurityNameIndexer::GetUniqueInstance().Clear();
  HFSAT::CurrencyConvertor::RemoveInstance();

  OfflineReturnsLRDB::RemoveUniqueInstance();
  delete common_smv_source_;
  common_smv_source_ = nullptr;
}
}
