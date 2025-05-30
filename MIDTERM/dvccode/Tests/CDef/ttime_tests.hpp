/**
 \file Tests/dvccode/CDef/ttime_tests.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 353, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551
 */

#pragma once

#include "dvccode/Tests/TestUtils/run_test.hpp"
#include "dvccode/CDef/ttime.hpp"

namespace HFTEST {
using namespace HFSAT;
class TTimeTTests : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(TTimeTTests);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition below.
  CPPUNIT_TEST(TestSum);
  CPPUNIT_TEST(TestSubtract);
  CPPUNIT_TEST(TestAddMsecs);
  CPPUNIT_TEST(TestAddUsecs);

  CPPUNIT_TEST_SUITE_END();

 public:
  // These are like constructors and destructors of the Test suite
  // setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  // tearDown -> Destructor | Just free memory etc.
  void setUp(void);
  void tearDown(void);

 protected:
  void TestSum(void);
  void TestSubtract(void);
  void TestAddMsecs(void);
  void TestAddUsecs(void);
};
}
