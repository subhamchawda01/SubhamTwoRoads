/**
   \file Tests/dvctrade/MarketAdapter/ose_pf_mvm_tests.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "baseinfra/Tools/common_smv_source.hpp"
#include "dvccode/Tests/TestUtils/run_test.hpp"

namespace HFTEST {

using namespace HFSAT;
class OSEPFMvmTests : public CppUnit::TestFixture {
  /**
   * General testing framework,
   * Currently It reads inputs from a file and processes accordingly
   */
  CPPUNIT_TEST_SUITE(OSEPFMvmTests);
  CPPUNIT_TEST(TestOSEPFMvm);
  CPPUNIT_TEST_SUITE_END();

  /// List of the test case functions written.
  /// To add a test case, we just add a function name here and write its definition below.
  ///  public:

  void TestOSEPFMvm(void);

  std::string exchange_;
  std::vector<std::vector<std::string> > cme_instruction_list_;
  CommonSMVSource* common_smv_source_;
  Watch* watch_;

 public:
  /// These are like constructors and destructors of the Test suite
  /// setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  void setUp(void);

  /// tearDown -> Destructor | Just free memory etc.
  void tearDown(void);
  ///
};
}
