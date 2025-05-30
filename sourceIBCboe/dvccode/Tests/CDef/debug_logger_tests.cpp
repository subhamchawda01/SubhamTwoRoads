
#include "dvccode/Tests/CDef/debug_logger_tests.hpp"

namespace HFTEST {
using namespace HFSAT;

void DebugLoggerTests::setUp() {}
void DebugLoggerTests::tearDown() {}

/*
 * Test to check the GetLogFileName() function
 */
void DebugLoggerTests::TestGetLogFileName() {
  HFSAT::DebugLogger dbglogger(1024000, 1);
  std::string logfilename = "/tmp/log";
  dbglogger.OpenLogFile(logfilename.c_str(), std::ofstream::out);

  CPPUNIT_ASSERT(strcmp(logfilename.c_str(), dbglogger.GetLogFileName().c_str()) == 0);
}

/*
 * Test to check the AddLogLevel() function
 */
void DebugLoggerTests::TestAddLogLevel() {
  HFSAT::DebugLogger dbglogger(1024000, 1);
  dbglogger.AddLogLevel(TRADING_INFO);
  CPPUNIT_ASSERT(dbglogger.CheckLoggingLevel(TRADING_INFO));

  dbglogger.AddLogLevel(645);
  CPPUNIT_ASSERT(dbglogger.CheckLoggingLevel(645));

  dbglogger.SetNoLogs();
  CPPUNIT_ASSERT(!dbglogger.CheckLoggingLevel(TRADING_INFO));
  CPPUNIT_ASSERT(!dbglogger.CheckLoggingLevel(645));
}

/*
 * Test to check the RemLogLevel() function
 */
void DebugLoggerTests::TestRemLogLevel() {
  HFSAT::DebugLogger dbglogger(1024000, 1);
  dbglogger.AddLogLevel(TRADING_INFO);
  dbglogger.AddLogLevel(645);
  CPPUNIT_ASSERT(dbglogger.CheckLoggingLevel(645));
  CPPUNIT_ASSERT(dbglogger.CheckLoggingLevel(TRADING_INFO));

  dbglogger.RemLogLevel(TRADING_INFO);
  CPPUNIT_ASSERT(dbglogger.CheckLoggingLevel(645));
  CPPUNIT_ASSERT(!dbglogger.CheckLoggingLevel(TRADING_INFO));

  dbglogger.RemLogLevel(645);
  CPPUNIT_ASSERT(!dbglogger.CheckLoggingLevel(645));
  CPPUNIT_ASSERT(!dbglogger.CheckLoggingLevel(TRADING_INFO));
}

/*
 * Test to check the SetNoLogs() function
 */
void DebugLoggerTests::TestSetNoLogs() {
  HFSAT::DebugLogger dbglogger(1024000, 1);
  dbglogger.SetNoLogs();
  CPPUNIT_ASSERT(!dbglogger.CheckLoggingLevel(TRADING_INFO));
  CPPUNIT_ASSERT(!dbglogger.CheckLoggingLevel(645));
}
}
