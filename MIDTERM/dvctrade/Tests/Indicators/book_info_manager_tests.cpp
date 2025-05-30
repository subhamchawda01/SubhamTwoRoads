// =====================================================================================
//
//       Filename:  dvctrade/Tests/Indiators/book_info_manager_tests.cpp
//
//    Description:  Tests for Utility Functions for fetching the Sample Data values from the last 30 days
//
//        Version:  1.0
//        Created:  01/14/2016 04:31:28 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include "dvctrade/Tests/Indicators/book_info_manager_tests.hpp"

namespace HFTEST {
using namespace HFSAT;

void BookInfoManagerTests::setUp(void) {
  shortcode_vec_.push_back("FGBM_0");
  tradingdate_ = 20160310;

  common_smv_source_ = new CommonSMVSource(shortcode_vec_, tradingdate_, false);
  common_smv_source_->InitializeVariables();
  watch_ = &common_smv_source_->getWatch();

  auto& sid_to_smvmap = common_smv_source_->getSMVMap();
  smv_ = sid_to_smvmap[SecurityNameIndexer::GetUniqueInstance().GetIdFromString(shortcode_vec_[0].c_str())];
  book_info_manager_ = BookInfoManager::GetUniqueInstance(common_smv_source_->getLogger(), *watch_, *smv_);

  /**
   * Not creating SMV here as it doesn't get deleted
   */
}

/**
 * Test if SumSize is computed Correctly
 */
void BookInfoManagerTests::TestComputeSumSize(void) {
  int num_levels = 5;
  double decay_factor = 0.5;
  double stdev_duration = 00;  // not using stdev

  book_info_manager_->ComputeSumSize(num_levels, decay_factor, stdev_duration);
  auto book_info_struct_ = book_info_manager_->GetBookInfoStruct(num_levels, decay_factor, stdev_duration);

  MvmTestsUtils::ConstructDummyBook(smv_);

  // This updates BestAsk/BidLevelInfo structs
  smv_->UpdateL1Prices();

  smv_->NotifyL2Listeners();

  // Assert for both bid and ask sides
  CPPUNIT_ASSERT_DOUBLES_EQUAL(13500, book_info_struct_->sum_ask_size_, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(1500, book_info_struct_->sum_bid_size_, DOUBLE_ASSERT_PRECISION);

  MvmTestsUtils::ClearDummyBook(smv_);

  //  BookInfoManager::RemoveUniqueInstance(common_smv_source_->getLogger(), *watch_, *smv_);
}

/**
 * Test if SumSize is computed Correctly
 */

/**
 * Checking with non zero stdev duration i.e. varying values for levels and thresholds
 */
void BookInfoManagerTests::TestComputeSumFactorSize(void) {
  int num_levels = 5;
  double decay_factor = 0.8;
  double stdev_duration = 300;

  // This uses dynamic scaling of thresholds
  book_info_manager_->ComputeSumFactorSize(num_levels, decay_factor, stdev_duration);
  auto book_info_struct_ = book_info_manager_->GetBookInfoStruct(num_levels, decay_factor, stdev_duration);

  MvmTestsUtils::ConstructDummyBook(smv_);

  // This updates BestAsk/BidLevelInfo structs
  smv_->UpdateL1Prices();

  smv_->NotifyL2Listeners();

  CPPUNIT_ASSERT_DOUBLES_EQUAL(770.638467, book_info_struct_->sum_bid_factor_size_, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(6935.7462098, book_info_struct_->sum_ask_factor_size_, DOUBLE_ASSERT_PRECISION);

  MvmTestsUtils::ClearDummyBook(smv_);
}

/**
 * Test if SumSize is computed Correctly
 */

void BookInfoManagerTests::TestComputeSumPrice(void) {
  int num_levels = 3;
  double decay_vector = 0.1;
  double stdev_duration = 0;

  book_info_manager_->ComputeSumPrice(num_levels, decay_vector, stdev_duration);
  auto book_info_struct = book_info_manager_->GetBookInfoStruct(num_levels, decay_vector, stdev_duration);

  MvmTestsUtils::ConstructDummyBook(smv_);
  // This updates BestAsk/BidLevelInfo structs
  smv_->UpdateL1Prices();

  smv_->NotifyL2Listeners();

  // Check for computation of values
  CPPUNIT_ASSERT_DOUBLES_EQUAL(393.24, book_info_struct->sum_bid_price_, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(393.33, book_info_struct->sum_ask_price_, DOUBLE_ASSERT_PRECISION);

  MvmTestsUtils::ClearDummyBook(smv_);

  // BookInfoManager::RemoveUniqueInstance(common_smv_source_->getLogger(), *watch_, *smv_);
}

/**
 * Price-Size computation
 */
void BookInfoManagerTests::TestComputeSumFactorPriceSize(void) {
  int num_levels = 2;
  double decay_vector = 0.9;
  double stdev_duration = 300;  // TODO non zero stdev segfaults if used on multiple places, needs debugging

  book_info_manager_->ComputeSumFactorPriceSize(num_levels, decay_vector, stdev_duration);
  auto book_info_struct = book_info_manager_->GetBookInfoStruct(num_levels, decay_vector, stdev_duration);

  MvmTestsUtils::ConstructDummyBook(smv_);
  // This updates BestAsk/BidLevelInfo structs
  smv_->UpdateL1Prices();

  smv_->NotifyL2Listeners();

  // Check for computation of variables;
  CPPUNIT_ASSERT_DOUBLES_EQUAL(34819.902561, book_info_struct->sum_bid_factor_price_size_, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(313433.767212, book_info_struct->sum_ask_factor_price_size_, DOUBLE_ASSERT_PRECISION);

  MvmTestsUtils::ClearDummyBook(smv_);

  // BookInfoManager::RemoveUniqueInstance(common_smv_source_->getLogger(), *watch_, *smv_);
}

/**
 *
 */
void BookInfoManagerTests::TestComputeSumOrder(void) {
  int num_levels = 2;
  double decay_vector = 0.6;
  double stdev_duration = 0;

  book_info_manager_->ComputeSumFactorOrder(num_levels, decay_vector, stdev_duration);
  auto book_info_struct = book_info_manager_->GetBookInfoStruct(num_levels, decay_vector, stdev_duration);

  MvmTestsUtils::ConstructDummyBook(smv_);

  // This updates BestAsk/BidLevelInfo structs
  smv_->UpdateL1Prices();

  smv_->NotifyL2Listeners();

  // Check for computation of variables;
  CPPUNIT_ASSERT_DOUBLES_EQUAL(7.745966, book_info_struct->sum_ask_factor_order_, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(85.205633, book_info_struct->sum_bid_factor_order_, DOUBLE_ASSERT_PRECISION);

  MvmTestsUtils::ClearDummyBook(smv_);

  // BookInfoManager::RemoveUniqueInstance(common_smv_source_->getLogger(), *watch_, *smv_);
}

/**
 *
 */
void BookInfoManagerTests::TestComputeSumFactorPriceOrder(void) {
  int num_levels = 2;
  double decay_vector = 0.9;
  double stdev_duration = 300;
  book_info_manager_->ComputeSumFactorPriceOrder(num_levels, decay_vector, stdev_duration);
  auto book_info_struct = book_info_manager_->GetBookInfoStruct(num_levels, decay_vector, stdev_duration);

  MvmTestsUtils::ConstructDummyBook(smv_);

  // This updates BestAsk/BidLevelInfo structs
  smv_->UpdateL1Prices();

  smv_->NotifyL2Listeners();

  // Check for computation of variables;
  CPPUNIT_ASSERT_DOUBLES_EQUAL(1243.723803, book_info_struct->sum_ask_factor_price_order_, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(14425.924887, book_info_struct->sum_bid_factor_price_order_, DOUBLE_ASSERT_PRECISION);

  MvmTestsUtils::ClearDummyBook(smv_);
}

/**
 * Sum Factor
 */
void BookInfoManagerTests::TestComputeSumFactor(void) {
  int num_levels = 2;
  double decay_vector = 0.9;
  double stdev_duration = 300;
  book_info_manager_->ComputeSumFactor(num_levels, decay_vector, stdev_duration);
  auto book_info_struct = book_info_manager_->GetBookInfoStruct(num_levels, decay_vector, stdev_duration);

  MvmTestsUtils::ConstructDummyBook(smv_);

  // This updates BestAsk/BidLevelInfo structs
  smv_->UpdateL1Prices();

  smv_->NotifyL2Listeners();

  // Check for computation of variables;
  CPPUNIT_ASSERT_DOUBLES_EQUAL(1.802498, book_info_struct->sum_bid_factor_, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(1.802498, book_info_struct->sum_ask_factor_, DOUBLE_ASSERT_PRECISION);

  MvmTestsUtils::ClearDummyBook(smv_);
}

void BookInfoManagerTests::TestDynamicScaling(void) {
  // auto& sid_to_smvmap = common_smv_source_->getSMVMap();
  // smv_ = sid_to_smvmap[SecurityNameIndexer::GetUniqueInstance().GetIdFromString(shortcode_vec_[0].c_str())];
  // auto book_info_manager_ = BookInfoManager::GetUniqueInstance(common_smv_source_->getLogger(), *watch_, *smv_);
}

void BookInfoManagerTests::tearDown(void) {
  // This gets called after each TestCase
  // Remove the unique instances

  /**
   *  Can't put Remove instance in individual test cases as it would segfault when test-case failed
   *  Will need to check for nullptr for unimplemented functions
   *  Can't have one instance and then run it as each test case has its own variable
   *
   */
  if (book_info_manager_ != nullptr) {
    BookInfoManager::RemoveUniqueInstance(common_smv_source_->getLogger(), *watch_, *smv_);
  }

  HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
  HFSAT::SecurityDefinitions::RemoveUniqueInstance();
  HFSAT::SecurityNameIndexer::GetUniqueInstance().Clear();
  HFSAT::CurrencyConvertor::RemoveInstance();

  // deallocate the heap variables

  if (common_smv_source_ != nullptr) {
    delete common_smv_source_;
    common_smv_source_ = nullptr;
  }
}
}
