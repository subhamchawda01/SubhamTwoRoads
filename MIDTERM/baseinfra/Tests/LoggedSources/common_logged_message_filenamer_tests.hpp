// =====================================================================================
//
//       Filename:  basetrade/Tests/LoggedSources/common_logged_message_filenamer_tests.hpp
//
//    Description:  Utility Functions for fetching the Sample Data values from the last 30 days
//
//        Version:  1.0
//        Created:  01/15/2016 12:38:39 PM
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
#include "baseinfra/LoggedSources/common_logged_message_filenamer.hpp"

namespace HFTEST {

using namespace HFSAT;

class CommonLoggedMessageFileNamerTests : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(CommonLoggedMessageFileNamerTests);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition below.

  CPPUNIT_TEST(TestGetNameWithoutTodaysData);

  CPPUNIT_TEST_SUITE_END();

 public:
  // These are like constructors and destructors of the Test suite
  // setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  // tearDown -> Destructor | Just free memory etc.
  void setUp(void);
  void tearDown(void);

 protected:
  /**
   *
   */
  void TestGetNameWithoutTodaysData(void) {
    /// CME

    TradingLocation_t trading_location = kTLocCHI;
    std::string filepath = CommonLoggedMessageFileNamer::GetName(kExchSourceCME, "ZNH6", 20160128, trading_location);

    // Comparing index rather than exact match as we have prefix on cloud
    CPPUNIT_ASSERT(filepath.find("/NAS1/data/CMELoggedData/CHI/2016/01/28/ZNH6_20160128.gz") != std::string::npos);

    /// CFE
    trading_location = kTLocCFE;
    filepath = CommonLoggedMessageFileNamer::GetName(kExchSourceCFE, "VX201602", 20160128, trading_location);

    CPPUNIT_ASSERT(filepath.find("/NAS1/data/CSMLoggedData/CFE/2016/01/28/VX201602_20160128.gz") != std::string::npos);

    /// EUREX
    trading_location = kTLocFR2;
    filepath = CommonLoggedMessageFileNamer::GetName(kExchSourceEUREX, "FGBM201603", 20160128, trading_location);

    CPPUNIT_ASSERT(filepath.find("/NAS1/data/EOBIPriceFeedLoggedData/FR2/2016/01/28/FGBM201603_20160128.gz") !=
                   std::string::npos);

    filepath = CommonLoggedMessageFileNamer::GetName(kExchSourceEUREX, "FGBM201312", 20131021, trading_location);

    CPPUNIT_ASSERT(filepath.find("/NAS1/data/EUREXLoggedData/FR2/2013/10/21/FGBM201312_20131021.gz") !=
                   std::string::npos);

    /// ASX

    trading_location = kTLocSYD;
    filepath = CommonLoggedMessageFileNamer::GetName(kExchSourceASX, "XT201603", 20160128, trading_location);

    CPPUNIT_ASSERT(filepath.find("/NAS1/data/ASXPFLoggedData/SYD/2016/01/28/XT201603_20160128.gz") !=
                   std::string::npos);

    /// ICE

    trading_location = kTLocBSL;
    filepath = CommonLoggedMessageFileNamer::GetName(kExchSourceICE, "R   FMH0016!", 20160128, trading_location);
    std::replace(filepath.begin(), filepath.end(), ' ', '~');

    CPPUNIT_ASSERT(filepath.find("/NAS1/data/ICELoggedData/BSL/2016/01/28/R~~~FMH0016!_20160128.gz") !=
                   std::string::npos);

    /// JPY

    trading_location = kTLocJPY;

    filepath = CommonLoggedMessageFileNamer::GetName(kExchSourceJPY, "NK1612", 20161128, trading_location);
    CPPUNIT_ASSERT(filepath.find("/NAS1/data/OSEPFLoggedData/TOK/2016/11/28/NK1612_20161128.gz") != std::string::npos);

    /// TMX

    trading_location = kTLocTMX;

    filepath = CommonLoggedMessageFileNamer::GetName(kExchSourceTMX, "CGBH7", 20170102, trading_location);
    CPPUNIT_ASSERT(filepath.find("/NAS1/data/TMX_OBF_PFLoggedData/TOR/2017/01/02/CGBH7_20170102.gz") !=
                   std::string::npos);
  }

 private:
};
}
