/**
 \file Tests/dvccode/CDef/ttime_tests.cpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 353, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551
 */

#include "dvccode/Tests/CDef/ttime_tests.hpp"

namespace HFTEST {
using namespace HFSAT;

/**
 *
 */
void TTimeTTests::setUp() {}

/**
 *
 */
void TTimeTTests::tearDown() {}
/**
 *
 */
void TTimeTTests::TestSum(void) {
  ttime_t time1(1490000000, 10);
  ttime_t time2(100, 100);
  ttime_t time3(0, 999999);
  ttime_t time4(-2, 1000);

  // Test normal sum
  auto sum1 = time1 + time2;
  CPPUNIT_ASSERT_MESSAGE(std::string("Assertion Failed between : 1490000100.000110 AND ") + sum1.ToString(),
                         sum1.tv_sec == 1490000100 && sum1.tv_usec == 110);

  // Test if carry from usecs is handled correclty
  auto sum2 = time1 + time2 + time3;
  CPPUNIT_ASSERT_MESSAGE(std::string("Assertion Failed between : 1490000001.000109 AND ") + sum2.ToString(),
                         sum2.tv_sec = 1490000101 && sum2.tv_usec == 109);

  // Not sure if -ve times are supposed to work this way
  auto sum3 = time1 + time2 + time3 + time4;
  CPPUNIT_ASSERT_MESSAGE(std::string("Assertion Failed between : 1489999999.1109 AND ") + sum3.ToString(),
                         sum3.tv_sec == 1490000099 && sum3.tv_usec == 1109);
}

/**
 *
 */
void TTimeTTests::TestSubtract(void) {
  ttime_t time1(100, 10);
  ttime_t time2(1, 1);
  ttime_t time3(0, 100);
  ttime_t time4(-1, 999999);

  // Normal subtract
  auto val = time1 - time2;
  CPPUNIT_ASSERT_MESSAGE(std::string("Assertion Failed between: 99.9 AND ") + val.ToString(),
                         val.tv_sec == 99 && val.tv_usec == 9);

  // usecs carry
  auto val2 = time1 - time2 - time3;
  CPPUNIT_ASSERT_MESSAGE(std::string("Assertion Failed between: 98.999909 AND ") + val2.ToString(),
                         val2.tv_sec == 98 && val2.tv_usec == 999909);

  // negative value handling
  auto val3 = time1 - time2 - time3 - time4;
  CPPUNIT_ASSERT_MESSAGE(std::string("Assertion Failed between 98.999910 AND ") + val3.ToString(),
                         val3.tv_sec == 98 && val3.tv_usec == 999910);
}

/**
 *
 */
void TTimeTTests::TestAddMsecs(void) {
  ttime_t time0(10, 100);
  ttime_t time1(100, 999999);
  ttime_t time2(-1, 999999);

  // normal
  time0.addmsecs(1);
  CPPUNIT_ASSERT_MESSAGE(std::string("Assertion Failed between 10.001100 AND ") + time0.ToString(),
                         time0.tv_sec == 10 && time0.tv_usec == 1100);

  // usecs carry
  time1.addmsecs(2);
  CPPUNIT_ASSERT_MESSAGE(std::string("Assertion Failed between 101.001999 AND ") + time1.ToString(),
                         time1.tv_sec == 101 && time1.tv_usec == 1999);

  // negative value handling
  time2.addmsecs(2);
  CPPUNIT_ASSERT_MESSAGE(std::string("Assertion Failed between 0.001999 AND ") + time2.ToString(),
                         time2.tv_sec == 0 && time2.tv_usec == 1999);
}

/**
 *
 */
void TTimeTTests::TestAddUsecs(void) {
  ttime_t time0(10, 100);
  ttime_t time1(100, 999999);
  ttime_t time2(-1, 999999);

  // normal
  time0.addusecs(1);
  CPPUNIT_ASSERT_MESSAGE(std::string("Assertion Failed between 10.001100 AND ") + time0.ToString(),
                         time0.tv_sec == 10 && time0.tv_usec == 101);

  // carry
  time1.addusecs(2);
  CPPUNIT_ASSERT_MESSAGE(std::string("Assertion Failed between 101.000001 AND ") + time1.ToString(),
                         time1.tv_sec == 101 && time1.tv_usec == 1);

  // negative value handling
  time2.addusecs(2);
  CPPUNIT_ASSERT_MESSAGE(std::string("Assertion Failed between 0.000001 AND ") + time2.ToString(),
                         time2.tv_sec == 0 && time2.tv_usec == 1);
}
}
