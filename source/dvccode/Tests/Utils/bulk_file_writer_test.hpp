/**
  \file dvccode/Tests/dvccode/Utils/bulk_file_writer_test.cpp

  \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
*/

#pragma once

#include <boost/filesystem.hpp>

#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/file_utils.hpp"

#include "dvccode/Tests/TestUtils/run_test.hpp"
#include "dvccode/Tests/TestUtils/cpptest_utils.hpp"

namespace HFTEST {

using namespace HFSAT;

/*
 * The Test class in which we write the test cases.
 */
class BulkFileWriterTest : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(BulkFileWriterTest);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition below.

  CPPUNIT_TEST(TestFileWrite);
  CPPUNIT_TEST(TestFileAppend);

  CPPUNIT_TEST_SUITE_END();

 public:
  // These are like constructors and destructors of the Test suite
  // setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  // tearDown -> Destructor | Just free memory etc.
  void setUp(void);
  void tearDown(void);

 protected:
  // Test Cases
  void TestFileWrite(void);
  void TestFileAppend(void);

 private:
  HFSAT::BulkFileWriter *bulk_file_writer_;
};
}
