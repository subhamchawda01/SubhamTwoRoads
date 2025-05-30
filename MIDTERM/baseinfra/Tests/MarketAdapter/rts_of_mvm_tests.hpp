/**
   \file Tests/MarketAdapter/rts_of_mvm_tests.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include "baseinfra/Tools/common_smv_source.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "dvccode/Tests/TestUtils/run_test.hpp"

#define RTS_OF_MVM_DBG 0  // enable this to see book after all updates

namespace HFTEST {

using namespace HFSAT;

class RTSOfMvmTests : public CppUnit::TestFixture {
  /**
   * General testing framework,
   * Currently It reads inputs from a file and processes accordingly
   */
  CPPUNIT_TEST_SUITE(RTSOfMvmTests);
  CPPUNIT_TEST(TestRTSOfMvm);
  CPPUNIT_TEST_SUITE_END();

  /// List of the test case functions written.
  /// To add a test case, we just add a function name here and write its definition below.
  ///  public:

  void TestRTSOfMvm(void);

  std::string exchange_;
  std::vector<std::vector<std::string> > rts_of_instruction_list_;
  CommonSMVSource* common_smv_source_;
  Watch* watch_;
  RTS_MDS::RTSOFCommonStructv2* order;

 public:
  /// These are like constructors and destructors of the Test suite
  /// setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  void setUp(void);

  /// tearDown -> Destructor | Just free memory etc.
  void tearDown(void);
  ///
 private:
  void showBook(int sec_id);
};
}
