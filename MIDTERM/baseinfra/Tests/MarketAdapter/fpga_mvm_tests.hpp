/*  \file Tests/dvctrade/MarketAdapter/fpga_mvm_tests.hpp

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

namespace HFTEST {

using namespace HFSAT;
class FPGAMvmTests : public CppUnit::TestFixture {
  /**
   * General testing framework,
   * Currently It reads inputs from a file and processes accordingly
   */
  CPPUNIT_TEST_SUITE(FPGAMvmTests);
  CPPUNIT_TEST(TestFpgaMvm);
  CPPUNIT_TEST_SUITE_END();

  /// List of the test case functions written.
  /// To add a test case, we just add a function name here and write its definition below.
  ///  public:

  void TestFpgaMvm(void);

  void InitFpgaStruct(FPGAHalfBook& fpga_halfbook);
  void ReadFpgaHalfBookUpdatesFile();
  bool IsSideUpdate(HFSAT::TradeType_t side_, FPGAHalfBook* halfbook_);
  void UpdateFpgaStruct(FPGAHalfBook* fpga_halfbook, HFSAT::TradeType_t buy_sell, int level, int price, int num_orders,
                        int order_size);

  std::string exchange_;
  std::string halfbook_update_cfg_filename_;
  std::vector<std::vector<std::string> > cme_instruction_list_;
  std::map<int, FPGAHalfBook*> halfbook_update_map_;
  CommonSMVSource* common_smv_source_;
  Watch* watch_;

 public:
  /// These are like constructors and destructors of the Test suite
  /// setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  FPGAMvmTests();
  void setUp(void);

  /// tearDown -> Destructor | Just free memory etc.
  void tearDown(void);
  ///
};
}
