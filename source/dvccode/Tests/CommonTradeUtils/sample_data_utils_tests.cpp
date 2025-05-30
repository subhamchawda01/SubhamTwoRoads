// =====================================================================================
//
//       Filename:  dvccode/Tests/dvccode/CommonTradeUtils/sample_data_utils_tests.cpp
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

#include <cstdio>
#include <ctime>
#include <stdlib.h>
#include <iostream>

#include "dvccode/Tests/CommonTradeUtils/sample_data_utils_tests.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CDef/currency_convertor.hpp"

namespace HFTEST {
using namespace HFSAT;

/**
 * Checks Avg for Period for XT_0 for two cases, VOL and STDEV
 * In this case we are testing the values for a single day as well as multiple days
 **/
void SampleDataUtilsTests::TestGetAvgForPeriod() {
  std::stringstream message;

  int tradingdate = 20170627;
  std::map<int, double> feature_sum_net_;
  bool exit_on_error_ = true;

  // a ) Volume for one day one period
  auto this_volume = SampleDataUtil::GetAvgForPeriod("XT_0", tradingdate, 1, 7200000, 14400000, "VOL", feature_sum_net_,
                                                     exit_on_error_);
  message << "Assesrion Failed : Expected Volume:  159.44. But actual " << std::floor(this_volume);
  CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(), 159.44, this_volume, 1);

  tradingdate = 20170404;
  auto this_stdev = SampleDataUtil::GetAvgForPeriod("XT_0", tradingdate, 1, 43200000, 50400000, "STDEV",
                                                    feature_sum_net_, exit_on_error_);
  message << "Assesrion Failed : Expected Volume:  0.001689. But actual " << this_stdev;
  CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(), 0.001689, this_stdev, 0.00001);

  // b ) Avg Volume for n days for given period
  this_volume = SampleDataUtil::GetAvgForPeriod("XT_0", tradingdate, 2, 14400000, 28800000, "VOL", feature_sum_net_,
                                                exit_on_error_);
  message << "Assesrion Failed : Expected Volume:  403.99. But actual " << this_volume;
  CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(), 403.99, this_volume, 1);

  this_stdev = SampleDataUtil::GetAvgForPeriod("XT_0", tradingdate, 2, 14400000, 28800000, "STDEV", feature_sum_net_,
                                               exit_on_error_);
  message << "Assesrion Failed : Expected Volume:  0.000747157. But actual " << this_stdev;
  CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(), 0.000747157, this_stdev, 0.000001);
}

/**
 *  check Day total value for VOL and STDEV for XT_0
 **/
void SampleDataUtilsTests::TestGetAvgForDayTotal() {
  std::stringstream message;
  int tradingdate = 20170404;
  bool exit_on_error_ = true;

  auto this_volume =
      SampleDataUtil::GetAvgForDayTotal("XT_0", tradingdate, 2, 14400000, 28800000, "VOL", exit_on_error_);
  message << "Assesrion Failed : Expected Volume:  6805.2115. But actual " << this_volume;
  CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(), 6805.2115, this_volume, 1);

  auto this_stdev =
      SampleDataUtil::GetAvgForDayTotal("XT_0", tradingdate, 2, 14400000, 28800000, "STDEV", exit_on_error_);
  message << "Assesrion Failed : Expected Volume:  0.01195. But actual " << this_stdev;
  CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(), 0.01195, this_stdev, 0.0001);
}

/**
 ** check Percentile value for VOL and STDEV for XT_0
 **/
void SampleDataUtilsTests::TestGetPercentileForPeriod() {
  std::stringstream message;
  int tradingdate = 20170404;
  bool exit_on_error_ = true;

  double percentile_ = 0.8;
  auto this_volume = SampleDataUtil::GetPercentileForPeriod("XT_0", tradingdate, 1, 7200000, 43200000, "VOL",
                                                            percentile_, exit_on_error_);
  message << "Assesrtion Failed : Expected Volume:  594.41. But actual " << this_volume;
  CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(), 594.41, this_volume, 1);

  percentile_ = 0.8;
  auto this_stdev = SampleDataUtil::GetPercentileForPeriod("XT_0", tradingdate, 1, 7200000, 43200000, "STDEV",
                                                           percentile_, exit_on_error_);
  message << "Assesrion Failed : Expected Volume:  0.001798. But actual " << this_stdev;
  CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(), 0.001798, this_stdev, 0.00001);
}

/**
 ** Tests the time difference between two time zones
 ** In this we have tested scenario before and after day light saving changes
 **/
void SampleDataUtilsTests::TestGetTZDiff() {
  std::stringstream message;

  int tradingdate1 = 20170331;
  SampleDataUtil::set_global_start_tz(std::string("AST_0905"));
  SampleDataUtil::set_global_end_tz(std::string("AST_0935"));
  SampleDataUtil::GetTZDiff(tradingdate1, tradingdate1);
  auto this_start_time_diff = SampleDataUtil::GetTZDiffStartTime(tradingdate1);
  auto this_end_time_diff = SampleDataUtil::GetTZDiffEndTime(tradingdate1);
  message << "Assesrion Failed : Expected Difference in Time:  -39600. But actual " << this_start_time_diff;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), -39600, this_start_time_diff);
  message << "Assesrion Failed : Expected Difference in Time:  -39600. But actual " << this_end_time_diff;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), -39600, this_end_time_diff);

  tradingdate1 = 20170401;
  SampleDataUtil::GetTZDiff(tradingdate1, tradingdate1);
  this_start_time_diff = SampleDataUtil::GetTZDiffStartTime(tradingdate1);
  this_end_time_diff = SampleDataUtil::GetTZDiffEndTime(tradingdate1);
  message << "Assesrion Failed : Expected Difference in Time:  -39600. But actual " << this_start_time_diff;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), -39600, this_start_time_diff);
  message << "Assesrion Failed : Expected Difference in Time:  -39600. But actual " << this_end_time_diff;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), -39600, this_end_time_diff);

  tradingdate1 = 20170403;
  SampleDataUtil::GetTZDiff(tradingdate1, tradingdate1);
  this_start_time_diff = SampleDataUtil::GetTZDiffStartTime(tradingdate1);
  this_end_time_diff = SampleDataUtil::GetTZDiffEndTime(tradingdate1);
  message << "Assesrion Failed : Expected Difference in Time:  -36000. But actual " << this_start_time_diff;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), -36000, this_start_time_diff);
  message << "Assesrion Failed : Expected Difference in Time:  -36000. But actual " << this_end_time_diff;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), -36000, this_end_time_diff);
}

void SampleDataUtilsTests::TestGetLastSampleBeforeDate() {
  std::stringstream message;

  auto this_last_date = SampleDataUtil::GetLastSampleBeforeDate("XT_0", 20170531, "VOL", 5, 1);

  message << "Assesrion Failed : Expected Date: 137.064. But actual " << this_last_date;
  CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(), 137.064, this_last_date, 0.01);
}

/**
 *  * Tests the mapping of feature -> filebasename
 *   * Test for VOL, STDEV, L1SZ, L1EVPerSec, TREND, ORDSZ
 *    */
void SampleDataUtilsTests::TestGetSampleFileName() {
  std::stringstream message;

  auto this_filename = SampleDataUtil::GetSampleFileName("L1SZ");
  message << "Assesrion Failed : Expected File: RollingAvgL1Size300.txt. But actual " << this_filename;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), std::string("RollingAvgL1Size300.txt"), std::string(this_filename));

  this_filename = SampleDataUtil::GetSampleFileName("STDEV");
  message << "Assesrion Failed : Expected File: RollingStdev300.txt. But actual " << this_filename;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), std::string("RollingStdev300.txt"), std::string(this_filename));

  this_filename = SampleDataUtil::GetSampleFileName("L1EVPerSec");
  message << "Assesrion Failed : Expected File: L1EventsPerSecond.txt. But actual " << this_filename;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), std::string("L1EventsPerSecond.txt"), std::string(this_filename));

  this_filename = SampleDataUtil::GetSampleFileName("TREND");
  message << "Assesrion Failed : Expected File: SimpleTrend300.txt. But actual " << this_filename;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), std::string("SimpleTrend300.txt"), std::string(this_filename));

  this_filename = SampleDataUtil::GetSampleFileName("ORDSZ");
  message << "Assesrion Failed : Expected File: RollingAvgOrdSize300.txt. But actual " << this_filename;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), std::string("RollingAvgOrdSize300.txt"), std::string(this_filename));
}

void SampleDataUtilsTests::setUp(void) {}

void SampleDataUtilsTests::tearDown(void) {
  SampleDataUtil::ClearSampleDataUtilsVars();
  CurrencyConvertor::RemoveInstance();
}
}
