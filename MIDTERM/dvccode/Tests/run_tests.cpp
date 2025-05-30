/**
   \file Tests/run_tests.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include <iostream>
#include <netinet/in.h>

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

#include "dvccode/Tests/CDef/ttime_tests.hpp"
#include "dvccode/Tests/CDef/debug_logger_tests.hpp"
#include "dvccode/Tests/CommonDataStructures/perishable_string_tokenizer_tests.hpp"
#include "dvccode/Tests/CommonTradeUtils/date_time_tests.hpp"
#include "dvccode/Tests/CommonTradeUtils/watch_tests.hpp"
#include "dvccode/Tests/CommonDataStructures/perishable_string_tokenizer_tests.hpp"
#include "dvccode/Tests/CommonTradeUtils/sample_data_utils_tests.hpp"
#include "dvccode/Tests/ExchangeSymbolManager/exchange_symbol_manager_tests.hpp"
#include "dvccode/Tests/SecurityDefinitions/security_definitions_tests.hpp"

#include "dvccode/Tests/HolidayManager/holiday_manager_tests.hpp"
#include "dvccode/Tests/Utils/bulk_file_reader_test.hpp"
#include "dvccode/Tests/Utils/bulk_file_writer_test.hpp"

using namespace HFTEST;

int main(int argc, char *argv[]) {
  std::string output_file = "ALL";

  if (argc > 0) {
    bool logging = (atoi(argv[argc - 1]) != 0);
    if (logging) {
      setenv("CPPUNIT_LOGGING", "ENABLED", 1);
    }
  }

  /// Holiday Manager test
  CPPUNIT_TEST_SUITE_REGISTRATION(HolidayManagerTests);
  // Exchange Symbol Manager Tests
  CPPUNIT_TEST_SUITE_REGISTRATION(ExchangeSymbolManagerTests);

  // Security Definitions Test
  // CPPUNIT_TEST_SUITE_REGISTRATION(SecurityDefinitionsTests);

  /// DataStructures test

  // Bulk File Reader Test
  CPPUNIT_TEST_SUITE_REGISTRATION(BulkFileReaderTest);

  // Bulk File Writer Test
  CPPUNIT_TEST_SUITE_REGISTRATION(BulkFileWriterTest);

  // dateTime TEsts
  CPPUNIT_TEST_SUITE_REGISTRATION(DateTimeTests);

  // Sample Data Utils Tests
  CPPUNIT_TEST_SUITE_REGISTRATION(SampleDataUtilsTests);

  // TtimeTests
  CPPUNIT_TEST_SUITE_REGISTRATION(TTimeTTests);

  // Watch Tests

  CPPUNIT_TEST_SUITE_REGISTRATION(WatchTests);

  // String Tokenizer Tests
  CPPUNIT_TEST_SUITE_REGISTRATION(PerishableStringTokenizerTests);

  // DebugLogger Tests
  CPPUNIT_TEST_SUITE_REGISTRATION(DebugLoggerTests);

  // Call RunTests, we give the file name where the TestResult file will be
  // saved
  return RunTest(output_file);
}
