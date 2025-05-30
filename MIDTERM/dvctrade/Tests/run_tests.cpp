/**
   \file Tests/run_tests.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include <netinet/in.h>
#include <iostream>

#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include "dvctrade/Tests/Indicators/common_indicator_tests.hpp"

#include "dvctrade/Tests/Indicators/indicator_util_tests.hpp"
#include "dvctrade/Tests/Indicators/book_info_manager_tests.hpp"
#include "dvctrade/Tests/Indicators/pca_weights_manager_tests.hpp"
#include "dvctrade/Tests/Indicators/offline_returns_lrdb_tests.hpp"
#include "dvctrade/Tests/Indicators/offline_returns_rlrdb_tests.hpp"

#include "dvctrade/Tests/ParamSet/ParamSetTests.hpp"
#include "dvctrade/Tests/TradeVarSets/trade_var_set_tests.hpp"

#include "dvctrade/Tests/BaseTrading/base_trading_tests.hpp"

using namespace HFTEST;

int main(int argc, char* argv[]) {
  std::string output_file = "ALL";

  if (argc > 0) {
    bool logging = (atoi(argv[argc - 1]) != 0);
    if (logging) {
      setenv("CPPUNIT_LOGGING", "ENABLED", 1);
    }
  }

  /**
   * Indicators Test
   */
  CPPUNIT_TEST_SUITE_REGISTRATION(CommonIndicatorTests);
  CPPUNIT_TEST_SUITE_REGISTRATION(IndicatorUtilsTests);
  CPPUNIT_TEST_SUITE_REGISTRATION(BookInfoManagerTests);
  CPPUNIT_TEST_SUITE_REGISTRATION(PcaWeightsManagerTests);

  // ParamSet Tests
  CPPUNIT_TEST_SUITE_REGISTRATION(ParamSetTests);

  // TradeVarSet Tests
  CPPUNIT_TEST_SUITE_REGISTRATION(TradeVarSetTests);

  // LRDB Tests
  CPPUNIT_TEST_SUITE_REGISTRATION(OfflineReturnsLRDBTests);

  // BaseTrading Tests
  CPPUNIT_TEST_SUITE_REGISTRATION(BaseTradingTests);

  // Call RunTests, we give the file name where the TestResult file will be saved
  return RunTest(output_file);
}
