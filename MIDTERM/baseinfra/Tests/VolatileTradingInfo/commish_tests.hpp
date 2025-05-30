/**
   \file baseinfra/Tests/VolatileTradingInfo/commish_testscpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include "dvccode/Tests/TestUtils/run_test.hpp"
#include "baseinfra/VolatileTradingInfo/base_commish.hpp"

namespace HFTEST {

using namespace HFSAT;

class CommishTests : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(CommishTests);

  CPPUNIT_TEST(TestFixedCommish);
  CPPUNIT_TEST(TestMICEXCommish);
  // CPPUNIT_TEST(TestNSECommish);
  CPPUNIT_TEST_SUITE_END();

 public:
  // These are like constructors and destructors of the Test suite
  // setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  // tearDown -> Destructor | Just free memory etc.
  void setUp(void);
  void tearDown(void);

 protected:
  /**
   * Test the commissions for a shortcode which is fixed per contract
   */
  void TestFixedCommish(void);
  /**
   * Tests the commissions for a shortcode for which commiss is dependant on price
   */
  void TestMICEXCommish(void);

  /**
   *
   */
  void TestNSECommish(void);
};
}
