/**
   \file Tests/dvctrade/MarketAdapter/ntp_mvm_tests.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include "baseinfra/Tools/common_smv_source.hpp"
#include "dvccode/Tests/TestUtils/run_test.hpp"

namespace HFTEST {

using namespace HFSAT;

/*
 * The Test class in which we write the test cases.
 */
class NtpMvmTests : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(NtpMvmTests);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition below.
  CPPUNIT_TEST(TestPriceLevelNew);
  CPPUNIT_TEST(TestPriceLevelModify);
  CPPUNIT_TEST(TestNTPMvm);
  CPPUNIT_TEST_SUITE_END();

 public:
  // These are like constructors and destructors of the Test suite
  // setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  // tearDown -> Destructor | Just free memory etc.
  void setUp(void);
  void tearDown(void);

 protected:
  // Test Cases
  void TestPriceLevelNew(void);
  void TestPriceLevelModify(void);
  void TestNTPMvm(void);

 private:
  std::string exchange_;
  std::vector<std::vector<std::string> > ntp_instruction_list_;
  CommonSMVSource* common_smv_source_;
  Watch* watch_;
};
}
