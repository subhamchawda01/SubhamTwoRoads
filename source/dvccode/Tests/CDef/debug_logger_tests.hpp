#pragma once

#include "dvccode/Tests/TestUtils/run_test.hpp"
#include "dvccode/CDef/debug_logger.hpp"

namespace HFTEST {
using namespace HFSAT;
class DebugLoggerTests : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(DebugLoggerTests);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition below.
  CPPUNIT_TEST(TestGetLogFileName);
  CPPUNIT_TEST(TestAddLogLevel);
  CPPUNIT_TEST(TestRemLogLevel);
  CPPUNIT_TEST(TestSetNoLogs);

  CPPUNIT_TEST_SUITE_END();

 public:
  // These are like constructors and destructors of the Test suite
  // setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  // tearDown -> Destructor | Just free memory etc.
  void setUp(void);
  void tearDown(void);

 protected:
  void TestGetLogFileName(void);
  void TestAddLogLevel(void);
  void TestRemLogLevel(void);
  void TestSetNoLogs(void);
};
}
