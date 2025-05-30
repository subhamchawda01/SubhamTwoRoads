// =====================================================================================
//
//       Filename:  dvctrade/Tests/Indiators/indicator_util_tests.cpp
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

#include "dvctrade/Tests/Indicators/indicator_util_tests.hpp"

namespace HFTEST {
using namespace HFSAT;

void IndicatorUtilsTests::setUp() {
  // Setting up the pca weights as it's required in HasExchange function
  PcaWeightsManager::SetUniqueInstance(DateTime::GetCurrentIsoDateLocal());
}

void IndicatorUtilsTests::TestGetLastTwoArgumentsFromIndicator(void) {
  // case 1 start is at 5
  std::vector<const char *> indicator_1 = {"INDICATOR", "1.00", "MultMktPrice", "FESX_0", "5", "0.8", "Midprice"};
  PriceType_t t_price_type_1;
  double first_argument_1 = 0;
  IndicatorUtil::GetLastTwoArgsFromIndicatorTokens(6, indicator_1, first_argument_1, t_price_type_1);
  CPPUNIT_ASSERT_EQUAL(t_price_type_1, StringToPriceType_t(std::string("Midprice")));

  // case 2 start is at 6
  std::vector<const char *> indicator_2 = {"INDICATOR", "1.00", "SimpleTrend", "FESX_0", "300", "Midprice"};
  PriceType_t t_price_type_2;
  double first_argument_2 = 0;
  IndicatorUtil::GetLastTwoArgsFromIndicatorTokens(4, indicator_2, first_argument_2, t_price_type_2);
  CPPUNIT_ASSERT_EQUAL(t_price_type_2, StringToPriceType_t(std::string("Midprice")));
  CPPUNIT_ASSERT_DOUBLES_EQUAL(300, first_argument_2, DOUBLE_ASSERT_PRECISION);
}

void IndicatorUtilsTests::TestOfflineReturnsRatio(void) {
  // as of 20170509 these are some values
  // NKMF_0 NKM_0 1.00
  // NKD_0 NIY_0 1.00
  // TOPIX_0 NK_0 0.88
  // JFFCE_0 FESX_0 0.92
  // FDAX_0 FESX_0 0.87
  // YM_0 ES_0 0.86
  // NQ_0 ES_0 1.20
  // JP400_0 TOPIX_0 0.87

  // first test
  auto dep_shortcode_ = std::string("NKMF_0");
  auto indep_shortcode_ = std::string("NKM_0");
  double offline_returns_ratio_;
  IndicatorUtil::GetOfflineReturnsRatio(dep_shortcode_, indep_shortcode_, offline_returns_ratio_);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(1.00, offline_returns_ratio_, DOUBLE_ASSERT_PRECISION);

  // second test
  dep_shortcode_ = std::string("TOPIX_0");
  indep_shortcode_ = std::string("NK_0");
  IndicatorUtil::GetOfflineReturnsRatio(dep_shortcode_, indep_shortcode_, offline_returns_ratio_);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(0.88, offline_returns_ratio_, DOUBLE_ASSERT_PRECISION);

  // third test
  dep_shortcode_ = std::string("FDAX_0");
  indep_shortcode_ = std::string("FESX_0");
  IndicatorUtil::GetOfflineReturnsRatio(dep_shortcode_, indep_shortcode_, offline_returns_ratio_);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(0.87, offline_returns_ratio_, DOUBLE_ASSERT_PRECISION);

  // what happens if i pass unknown shortcodes
  dep_shortcode_ = std::string("CGB_0");
  indep_shortcode_ = std::string("FESX_0");
  IndicatorUtil::GetOfflineReturnsRatio(dep_shortcode_, indep_shortcode_, offline_returns_ratio_);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(1.00, offline_returns_ratio_, DOUBLE_ASSERT_PRECISION);
}

void IndicatorUtilsTests::TestLoadDI1EigenVector(void) {
  // not writing tests as per ASirohiya 20170510 // PSarthy
}

void IndicatorUtilsTests::TestAddDI1PortfolioShortCodeVec(void) {
  const std::string portfolio_descriptor_shortcode_ = "DI1ALL";
  auto _date_ = 20161128;
  std::vector<std::string> _shortcode_vec_;
  std::vector<std::string> expected_shortcodes_ = {"DI1F15", "DI1J15", "DI1N15", "DI1F16", "DI1J16", "DI1N16",
                                                   "DI1F17", "DI1J17", "DI1N17", "DI1F18", "DI1F19", "DI1F20",
                                                   "DI1F21", "DI1F22", "DI1F23", "DI1F24", "DI1F25"};
  IndicatorUtil::AddDIPortfolioShortCodeVec(portfolio_descriptor_shortcode_, _shortcode_vec_, _date_);
  for (auto i = 0u; i < expected_shortcodes_.size(); i++) {
    CPPUNIT_ASSERT_EQUAL(expected_shortcodes_[i], _shortcode_vec_[i]);
  }
}

void IndicatorUtilsTests::TestMinPriceIncrementForPricePortfolio(void) {
  // EURBOND1 FGBL_0 FOAT_0 0.5 0.5
  // CMEEQ NIY_0 NKD_0 0.5 0.5
  // ERXEQMINI FESX_0 FDXM_0  0.5 0.5
  // ERXEQ FESX_0 FDAX_0  0.5 0.5
  // ERXEQOP FDAX_0 FESX_0 0.5 0.5
  // EXFOATFGBL FOAT_0 FGBL_0 0.5 0.5
  // EXFBTPFOAT FBTP_0 FOAT_0 0.5 0.5
  // EXFGBLX FGBL_0 FGBX_0 0.5 0.5
  // EXFGBML FGBM_0 FGBL_0 0.5 0.5
  // RUSCUR Si_0 USD000UTSTOM 0.5 0.5
  // CADAUD 6C_0 6A_0 0.5 0.5
  auto tradingdate = 20161128;
  auto portfolio_shortcode = std::string("EURBOND1");
  // FGBL minprice 0.01, FGBM also 0.01
  auto min_price_increment_ = IndicatorUtil::GetMinPriceIncrementForPricePortfolio(portfolio_shortcode, tradingdate);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(0.01, min_price_increment_, DOUBLE_ASSERT_PRECISION);
}

void IndicatorUtilsTests::TestCollectShortcodeOrPortfolio(void) {
  std::vector<std::string> shortcodes_affecting_this_indicator;
  std::vector<std::string> expected_shortcodes_ = {"FGBL_0", "FOAT_0"};
  std::vector<std::string> ors_source_needed_vec;
  std::vector<const char *> r_tokens = {"INDICATOR", "1.00", "MultMktPrice", "EURBOND1", "5", "0.8", "Midprice"};

  IndicatorUtil::CollectShortcodeOrPortfolio(shortcodes_affecting_this_indicator, ors_source_needed_vec, r_tokens);
  for (auto i = 0u; i < expected_shortcodes_.size(); i++) {
    CPPUNIT_ASSERT_EQUAL(expected_shortcodes_[i], shortcodes_affecting_this_indicator[i]);
  }

  // case 2 similar to 1
  expected_shortcodes_ = {"6C_0", "6A_0"};
  r_tokens = {"INDICATOR", "1.00", "MultMktPrice", "CADAUD", "5", "0.8", "Midprice"};
  std::vector<std::string> shortcodes_affecting_this_indicator_2;

  IndicatorUtil::CollectShortcodeOrPortfolio(shortcodes_affecting_this_indicator_2, ors_source_needed_vec, r_tokens);
  for (auto i = 0u; i < expected_shortcodes_.size(); i++) {
    CPPUNIT_ASSERT_EQUAL(expected_shortcodes_[i], shortcodes_affecting_this_indicator_2[i]);
  }
}

void IndicatorUtilsTests::TestIndicatorHasExchange(void) {
  ExchSource_t _exch_ = kExchSourceTMX;
  const std::string indicator_ = "INDICATOR 1.00 SimpleTrend CGB_0 300 Midprice";
  bool true_or_false_;
  true_or_false_ = IndicatorUtil::IndicatorHasExch(indicator_, _exch_);
  CPPUNIT_ASSERT(true_or_false_);
}

void IndicatorUtilsTests::tearDown(void) {
  HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
  HFSAT::SecurityDefinitions::RemoveUniqueInstance();
  HFSAT::SecurityNameIndexer::GetUniqueInstance().Clear();
  HFSAT::CurrencyConvertor::RemoveInstance();
  CurrencyConvertor::RemoveInstance();
}
}
