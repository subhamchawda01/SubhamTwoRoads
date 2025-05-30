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
#include "baseinfra/Tests/MarketAdapter/trade_time_manager_test.hpp"
#include "baseinfra/Tests/OrderRouting/order_modify_tests.hpp"
#include "baseinfra/Tests/OrderRouting/om_tests.hpp"
#include "baseinfra/Tests/MarketAdapter/market_adapter_tests.hpp"
#include "baseinfra/Tests/VolatileTradingInfo/commish_tests.hpp"
#include "baseinfra/Tests/LoggedSources/common_logged_message_filenamer_tests.hpp"

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
   *
   *Trade time manager test
   *
   */

  CPPUNIT_TEST_SUITE_REGISTRATION(TradeTimeManagerTest);

  /**
   *  Order Manager Tests
   *
   */

  CPPUNIT_TEST_SUITE_REGISTRATION(OrderModifyTests);
  CPPUNIT_TEST_SUITE_REGISTRATION(OmTests);
  /**
   * SMV test
   */
  CPPUNIT_TEST_SUITE_REGISTRATION(SecurityMarketViewTests);

  CPPUNIT_TEST_SUITE_REGISTRATION(CommishTests);

  /**
   * Book Manger tests
   */

  // Base MarketViewManagerTests
  CPPUNIT_TEST_SUITE_REGISTRATION(BaseMarketViewManagerTests);

  // CME price level book
  CPPUNIT_TEST_SUITE_REGISTRATION(CMEMvmTests);

  // EOBI book
  CPPUNIT_TEST_SUITE_REGISTRATION(EOBIMvmTests);

  // NSE book created directly from order feed
  CPPUNIT_TEST_SUITE_REGISTRATION(NSEMvmTests);
  // OSE price feed book
  CPPUNIT_TEST_SUITE_REGISTRATION(OSEPFMvmTests);

  // OSE order feed book
  CPPUNIT_TEST_SUITE_REGISTRATION(OSEOFMvmTests);

  // ICE price feed book
  CPPUNIT_TEST_SUITE_REGISTRATION(ICEMvmTests);

  // TMX order feed book
  CPPUNIT_TEST_SUITE_REGISTRATION(TMXMvmTests);

  // RTS price feed book
  CPPUNIT_TEST_SUITE_REGISTRATION(RTSMvmTests);

  // RTS order feed book
  CPPUNIT_TEST_SUITE_REGISTRATION(RTSOfMvmTests);

  // MICEX price feed book
  CPPUNIT_TEST_SUITE_REGISTRATION(MICEXMvmTests);

  // CFE Price feed book
  CPPUNIT_TEST_SUITE_REGISTRATION(CFEMvmTests);

  // HKEX Price feed book
  CPPUNIT_TEST_SUITE_REGISTRATION(HKEXMvmTests);

  CPPUNIT_TEST_SUITE_REGISTRATION(NtpMvmTests);

  // FPGA book
  CPPUNIT_TEST_SUITE_REGISTRATION(FPGAMvmTests);

  // Call RunTests, we give the file name where the TestResult file will be saved

  return RunTest(output_file);
}
