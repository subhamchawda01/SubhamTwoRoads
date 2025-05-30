// =====================================================================================
//
//       Filename:  baseinfra/Tests/dvctrade/MarketAdapter/base_market_view_manager_tests.cpp
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

#include "baseinfra/Tests/MarketAdapter/base_market_view_manager_tests.hpp"

namespace HFTEST {
using namespace HFSAT;

/**
 * Initilization before each test
 */
void BaseMarketViewManagerTests::setUp(void) {
  // Initialize the shortcode
  shortcode_vec_.push_back("FGBM_0");

  tradingdate_ = 20160310;

  common_smv_source_ = new CommonSMVSource(shortcode_vec_, tradingdate_, false);

  // Set the ors needed vec for computation of prom-order manager
  common_smv_source_->SetSourcesNeedingOrs(shortcode_vec_);

  common_smv_source_->InitializeVariables();
  watch_ = &common_smv_source_->getWatch();

  auto& sid_to_smvmap = common_smv_source_->getSMVMap();
  smv_ = sid_to_smvmap[SecurityNameIndexer::GetUniqueInstance().GetIdFromString(shortcode_vec_[0].c_str())];

  auto book_manager = common_smv_source_->indexed_eobi_price_level_market_view_manager();

  // Set the prom-order-manager pointer to SMV
  auto prom_order_manager = PromOrderManager::GetCreatedInstance(shortcode_vec_[0]);

  // Set book-manager as listener to prom-order manager so the prom-order book get's computed
  prom_order_manager->AddGlobalOrderChangeListener(book_manager);

  prom_order_manager->ManageOrdersAlso();
  // Smv needs POM to be used in MarketAdapter
  smv_->SetPromOrderManager(prom_order_manager);
}

/**
 * Tests if the new price goes beyond the size of vector ( for bid side, if the new price is higher than older best bid
 *
 */
void BaseMarketViewManagerTests::TestBidRebuildIndexHighAccess(void) {
  // Create dummy book
  MvmTestsUtils::ConstructDummyBook(smv_);

  // No need for exchange specific testing as this part is common to all exchanges
  // As long as the functions are called appropriately in book-manager, this should work fine
  auto book_manager = common_smv_source_->indexed_eobi_price_level_market_view_manager();

  auto best_bid_int_price = smv_->market_update_info().bestbid_int_price_;
  auto best_bid_size = smv_->market_update_info().bestbid_size_;

  auto old_base_bid_index = smv_->base_bid_index_;

  // Add try to create bid side book at a price which is at least initial tick-size difference from current book
  auto new_best_bid_int_price = best_bid_int_price + (int)smv_->initial_tick_size_ + 1;

  auto bid_index =
      smv_->base_bid_index_ -
      (smv_->market_update_info().bidlevels_[smv_->base_bid_index_].limit_int_price_ - new_best_bid_int_price);

  // See if new price is actually causing the rebuild index High
  CPPUNIT_ASSERT((int)bid_index >= (int)smv_->max_tick_range_);

  book_manager->RebuildIndexHighAccess(smv_->security_id(), kTradeTypeBuy, new_best_bid_int_price);

  /// logging
  if (IsLoggingEnabled()) {
    std::cout << "\nBook: " << smv_->shortcode() << " NewPx: " << new_best_bid_int_price << " Index: " << bid_index
              << " MaxRange: " << smv_->max_tick_range_ << " OldIndex: " << old_base_bid_index
              << " BaseBidIndex: " << smv_->base_bid_index_
              << " Px: " << smv_->market_update_info().bidlevels_[smv_->base_bid_index_].limit_int_price_ << std::endl;
  }

  // Check if current best-bid is set properly
  CPPUNIT_ASSERT_EQUAL(smv_->market_update_info().bidlevels_[smv_->base_bid_index_].limit_int_price_,
                       new_best_bid_int_price);

  // Get the location of last int price and see if it still exists
  bid_index = smv_->base_bid_index_ -
              (smv_->market_update_info().bidlevels_[smv_->base_bid_index_].limit_int_price_ - best_bid_int_price);

  if (bid_index > 0 && bid_index < smv_->max_tick_range_) {
    auto size = smv_->market_update_info().bidlevels_[bid_index].limit_size_;
    // If the index exists then the sizes should have been copied correcly
    CPPUNIT_ASSERT_EQUAL(size, best_bid_size);
  }
}

/**
 *  Same for ask side
 */
void BaseMarketViewManagerTests::TestAskRebuildIndexHighAccess(void) {
  // Create dummy book

  // Create dummy book
  MvmTestsUtils::ConstructDummyBook(smv_);

  // No need for exchange specific testing as this part is common to all exchanges
  // As long as the functions are called appropriately in book-manager, this should work fine
  auto book_manager = common_smv_source_->indexed_eobi_price_level_market_view_manager();

  auto best_ask_int_price = smv_->market_update_info().bestask_int_price_;
  auto best_ask_size = smv_->market_update_info().bestask_size_;

  // Add try to create bid side book at a price which is at least initial tick-size difference from current book
  auto new_best_ask_int_price = best_ask_int_price - (int)smv_->initial_tick_size_ - 1;

  auto ask_index =
      smv_->base_ask_index_ +
      (smv_->market_update_info().asklevels_[smv_->base_ask_index_].limit_int_price_ - new_best_ask_int_price);

  /// logging
  if (IsLoggingEnabled()) {
    std::cout << "\nBook: " << smv_->shortcode() << " NewPx: " << new_best_ask_int_price << " Index: " << ask_index
              << " MaxRange: " << smv_->max_tick_range_ << " BaseBidIndex: " << smv_->base_ask_index_
              << " Px: " << smv_->market_update_info().asklevels_[smv_->base_ask_index_].limit_int_price_ << std::endl;
  }

  // See if new price is actually causing the rebuild index High
  CPPUNIT_ASSERT((int)ask_index >= (int)smv_->max_tick_range_);

  book_manager->RebuildIndexHighAccess(smv_->security_id(), kTradeTypeSell, new_best_ask_int_price);

  // Check if current best-bid is set properly
  CPPUNIT_ASSERT_EQUAL(smv_->market_update_info().asklevels_[smv_->base_ask_index_].limit_int_price_,
                       new_best_ask_int_price);

  // Get the location of last int price and see if it still exists
  ask_index = smv_->base_ask_index_ +
              (smv_->market_update_info().asklevels_[smv_->base_ask_index_].limit_int_price_ - best_ask_int_price);

  if (ask_index > 0 && ask_index < smv_->max_tick_range_) {
    auto size = smv_->market_update_info().asklevels_[ask_index].limit_size_;
    // If the index exists then the sizes should have been copied correctly
    CPPUNIT_ASSERT_EQUAL(size, best_ask_size);
  }
}

/**
 * Testing for low access index i.e. When price has decreased
 */
void BaseMarketViewManagerTests::TestBidRebuildIndexLowAccess(void) {
  // Create dummy book
  MvmTestsUtils::ConstructDummyBook(smv_);

  // No need for exchange specific testing as this part is common to all exchanges
  // As long as the functions are called appropriately in book-manager, this should work fine
  auto book_manager = common_smv_source_->indexed_eobi_price_level_market_view_manager();

  auto best_bid_int_price = smv_->market_update_info().bestbid_int_price_;
  auto best_bid_size = smv_->market_update_info().bestbid_size_;

  // Add try to create bid side book at a price which is at least initial tick-size difference from current book
  auto new_best_bid_int_price = best_bid_int_price - (int)smv_->initial_tick_size_ - 1;

  auto bid_index =
      smv_->base_bid_index_ -
      (smv_->market_update_info().bidlevels_[smv_->base_bid_index_].limit_int_price_ - new_best_bid_int_price);

  /// logging
  if (IsLoggingEnabled()) {
    std::cout << "\nBook: " << smv_->shortcode() << " NewPx: " << new_best_bid_int_price << " Index: " << bid_index
              << " MaxRange: " << smv_->max_tick_range_ << " BaseBidIndex: " << smv_->base_bid_index_
              << " Px: " << smv_->market_update_info().bidlevels_[smv_->base_bid_index_].limit_int_price_ << std::endl;
  }

  // See if new price is actually causing the rebuild index High
  CPPUNIT_ASSERT((int)bid_index <= LOW_ACCESS_INDEX);

  book_manager->RebuildIndexLowAccess(smv_->security_id(), kTradeTypeBuy, new_best_bid_int_price);

  // Check if current best-bid is set properly
  CPPUNIT_ASSERT_EQUAL(smv_->market_update_info().bidlevels_[smv_->base_bid_index_].limit_int_price_,
                       new_best_bid_int_price);

  // Get the location of last int price and see if it still exists
  bid_index = smv_->base_bid_index_ -
              (smv_->market_update_info().bidlevels_[smv_->base_bid_index_].limit_int_price_ - best_bid_int_price);

  if (bid_index > 0 && bid_index < smv_->max_tick_range_) {
    auto size = smv_->market_update_info().bidlevels_[bid_index].limit_size_;
    // If the index exists then the sizes should have been copied correctly
    CPPUNIT_ASSERT_EQUAL(size, best_bid_size);
  }

  // Now the base-bid index should be moved from initial location
}

/**
 * Tests when ask side has dipped ( price has increased ), the index is built correctly
 */
void BaseMarketViewManagerTests::TestAskRebuildIndexLowAccess(void) {
  // Create dummy book

  // Create dummy book
  MvmTestsUtils::ConstructDummyBook(smv_);

  // No need for exchange specific testing as this part is common to all exchanges
  // As long as the functions are called appropriately in book-manager, this should work fine
  auto book_manager = common_smv_source_->indexed_eobi_price_level_market_view_manager();

  auto best_ask_int_price = smv_->market_update_info().bestask_int_price_;
  auto best_ask_size = smv_->market_update_info().bestask_size_;

  // Add try to create bid side book at a price which is at least initial tick-size difference from current book
  auto new_best_ask_int_price = best_ask_int_price + (int)smv_->initial_tick_size_ + 1;

  auto ask_index =
      smv_->base_ask_index_ +
      (smv_->market_update_info().asklevels_[smv_->base_ask_index_].limit_int_price_ - new_best_ask_int_price);

  /// logging
  if (IsLoggingEnabled()) {
    std::cout << "\nBook: " << smv_->shortcode() << " NewPx: " << new_best_ask_int_price << " Index: " << ask_index
              << " MaxRange: " << smv_->max_tick_range_ << " BaseBidIndex: " << smv_->base_ask_index_
              << " Px: " << smv_->market_update_info().asklevels_[smv_->base_ask_index_].limit_int_price_ << std::endl;
  }

  // See if new price is actually causing the rebuild index High
  CPPUNIT_ASSERT((int)ask_index <= LOW_ACCESS_INDEX);

  book_manager->RebuildIndexLowAccess(smv_->security_id(), kTradeTypeSell, new_best_ask_int_price);

  // Check if current best-bid is set properly
  CPPUNIT_ASSERT_EQUAL(smv_->market_update_info().asklevels_[smv_->base_ask_index_].limit_int_price_,
                       new_best_ask_int_price);

  // Get the location of last int price and see if it still exists
  ask_index = smv_->base_ask_index_ +
              (smv_->market_update_info().asklevels_[smv_->base_ask_index_].limit_int_price_ - best_ask_int_price);

  if (ask_index > 0 && ask_index < smv_->max_tick_range_) {
    auto size = smv_->market_update_info().asklevels_[ask_index].limit_size_;
    // If the index exists then the sizes should have been copied correctly
    CPPUNIT_ASSERT_EQUAL(size, best_ask_size);
  }
}

/**
 * Tests if rescaling of book works correctly.
 * The difference between RebuildIndex and RescaleBook is that in Recale, we keep all book
 * In order to do that we increase the size of vectors
 * In RebuildIndex, we loose the orders which are beyond the vector size after recentering
 */
void BaseMarketViewManagerTests::TestBidRescaleBook(void) {
  // Create dummy book
  MvmTestsUtils::ConstructDummyBook(smv_);

  // No need for exchange specific testing as this part is common to all exchanges
  // As long as the functions are called appropriately in book-manager, this should work fine
  auto book_manager = common_smv_source_->indexed_eobi_price_level_market_view_manager();

  auto best_bid_int_price = smv_->market_update_info().bestbid_int_price_;

  auto old_book_size = smv_->market_update_info().bidlevels_.size();

  // Add try to create bid side book at a price which is at least initial tick-size difference from current book
  auto new_best_bid_int_price = best_bid_int_price + smv_->initial_tick_size_ + 1;

  auto bid_index_ =
      smv_->base_bid_index_ -
      (smv_->market_update_info().bidlevels_[smv_->base_bid_index_].limit_int_price_ - new_best_bid_int_price);

  /// logging
  if (IsLoggingEnabled()) {
    std::cout << "\nBook: " << smv_->shortcode() << " NewPx: " << new_best_bid_int_price << " Index: " << bid_index_
              << " MaxRange: " << smv_->max_tick_range_ << " BaseBidIndex: " << smv_->base_bid_index_
              << " Px: " << smv_->market_update_info().bidlevels_[smv_->base_bid_index_].limit_int_price_ << std::endl;
  }

  // See if new price is actually causing the rebuild index High
  CPPUNIT_ASSERT((int)bid_index_ >= (int)smv_->max_tick_range_);

  book_manager->ReScaleBook(smv_->security_id(), kTradeTypeBuy, new_best_bid_int_price);

  CPPUNIT_ASSERT_EQUAL((old_book_size - 1) * 4 + 1, smv_->market_update_info().bidlevels_.size());

  /**
   * We can add more ASSERT lines checking for
   * prices are assigned correctly
   * Sizes are initialized to 0
   */

  // Checking if the last entry in the vector has appropriate price or not
  auto& bid_levels = smv_->market_update_info().bidlevels_;
  CPPUNIT_ASSERT_EQUAL(bid_levels.back().limit_int_price_, (((int)bid_levels.size() - 1) - (int)smv_->base_bid_index_ +
                                                            bid_levels[smv_->base_bid_index_].limit_int_price_));
}

/**
 * Tests if rescaling of book works correctly.
 * The difference between RebuildIndex and RescaleBook is that in Recale, we keep all book
 * In order to do that we increase the size of vectors
 * In RebuildIndex, we loose the orders which are beyond the vector size after recentering
 */
void BaseMarketViewManagerTests::TestAskRescaleBook(void) {
  // Create dummy book
  MvmTestsUtils::ConstructDummyBook(smv_);

  // No need for exchange specific testing as this part is common to all exchanges
  // As long as the functions are called appropriately in book-manager, this should work fine
  auto book_manager = common_smv_source_->indexed_eobi_price_level_market_view_manager();

  auto best_ask_int_price = smv_->market_update_info().bestask_int_price_;

  auto old_book_size = smv_->market_update_info().asklevels_.size();

  // Add try to create bid side book at a price which is at least initial tick-size difference from current book
  auto new_best_ask_int_price = best_ask_int_price - smv_->initial_tick_size_ - 1;

  auto ask_index_ =
      smv_->base_ask_index_ +
      (smv_->market_update_info().asklevels_[smv_->base_ask_index_].limit_int_price_ - new_best_ask_int_price);

  /// logging
  if (IsLoggingEnabled()) {
    std::cout << "\nBook: " << smv_->shortcode() << " NewPx: " << new_best_ask_int_price << " Index: " << ask_index_
              << " MaxRange: " << smv_->max_tick_range_ << " BaseBidIndex: " << smv_->base_ask_index_
              << " Px: " << smv_->market_update_info().asklevels_[smv_->base_ask_index_].limit_int_price_ << std::endl;
  }

  // See if new price is actually causing the rebuild index High
  CPPUNIT_ASSERT((int)ask_index_ >= (int)smv_->max_tick_range_);

  book_manager->ReScaleBook(smv_->security_id(), kTradeTypeSell, new_best_ask_int_price);

  CPPUNIT_ASSERT_EQUAL((old_book_size - 1) * 4 + 1, smv_->market_update_info().asklevels_.size());

  /**
   * We can add more ASSERT lines checking for
   * prices are assigned correctly
   * Sizes are initialized to 0
   */

  // Checking if the last entry in the vector has appropriate price or not
  auto& ask_levels = smv_->market_update_info().asklevels_;
  CPPUNIT_ASSERT_EQUAL(
      ask_levels.back().limit_int_price_,
      ((int)smv_->base_ask_index_ + ask_levels[smv_->base_ask_index_].limit_int_price_ - ((int)ask_levels.size() - 1)));
}

/**
 * Tests if the buld index for bid side was successful
 */
void BaseMarketViewManagerTests::TestBidBuildIndex(void) {
  double price = 131.09;
  int int_price = smv_->GetIntPx(price);

  // No need for exchange specific testing as this part is common to all exchanges
  // As long as the functions are called appropriately in book-manager, this should work fine
  auto book_manager = common_smv_source_->indexed_eobi_price_level_market_view_manager();

  // Builds the book with the given price set at smv_->base_bid_index_;
  book_manager->BuildIndex(smv_->security_id(), kTradeTypeBuy, int_price);

  // Test for price at one index ( in this case last )
  auto& bid_levels = smv_->market_update_info().bidlevels_;
  CPPUNIT_ASSERT_EQUAL(bid_levels.back().limit_int_price_, (((int)bid_levels.size() - 1) - (int)smv_->base_bid_index_ +
                                                            bid_levels[smv_->base_bid_index_].limit_int_price_));

  // Test for price at index 0
  CPPUNIT_ASSERT_EQUAL(bid_levels.front().limit_int_price_,
                       (bid_levels[smv_->base_bid_index_].limit_int_price_ - (int)smv_->base_bid_index_));
}

/**
 * Symmetric checking for ask side
 */
void BaseMarketViewManagerTests::TestAskBuildIndex(void) {
  double price = 131.10;
  int int_price = smv_->GetIntPx(price);

  // No need for exchange specific testing as this part is common to all exchanges
  // As long as the functions are called appropriately in book-manager, this should work fine
  auto book_manager = common_smv_source_->indexed_eobi_price_level_market_view_manager();

  // Builds the book with the given price set at smv_->base_bid_index_;
  book_manager->BuildIndex(smv_->security_id(), kTradeTypeSell, int_price);

  // Test for price at one index ( in this case last )
  auto& ask_levels = smv_->market_update_info().asklevels_;

  CPPUNIT_ASSERT_EQUAL(
      ask_levels.back().limit_int_price_,
      ((int)smv_->base_ask_index_ + ask_levels[smv_->base_ask_index_].limit_int_price_ - ((int)ask_levels.size() - 1)));

  // Test for price at index 0
  CPPUNIT_ASSERT_EQUAL(ask_levels.front().limit_int_price_,
                       (ask_levels[smv_->base_ask_index_].limit_int_price_ + (int)smv_->base_ask_index_));
}

/**
 * This function in base_market_view_manager doesn't get called anywhere, whoever feels like calling it, please
 * implement the tests
 */
void BaseMarketViewManagerTests::TestOrderExecuted(void) {}

/**
 * Tests following
 * If we have orders sitting at market best book then update the market_update_info in as way that it doesn't contain
 * our orders
 */
void BaseMarketViewManagerTests::TestUpdateBestBidVariablesUsingOurOrders(void) {
  MvmTestsUtils::ConstructDummyBook(smv_);

  // Prom order manager should be here
  CPPUNIT_ASSERT(smv_->p_prom_order_manager_ != nullptr);

  smv_->remove_self_orders_from_book_ = true;

  auto prom_order_manager = PromOrderManager::GetCreatedInstance(smv_->shortcode());

  auto sec_id = smv_->security_id();
  int original_size = smv_->market_update_info().bidlevels_[smv_->base_bid_index_].limit_size_;
  double px = smv_->market_update_info().bidlevels_[smv_->base_bid_index_].limit_int_price_;
  int intpx = smv_->market_update_info().bidlevels_[smv_->base_bid_index_].limit_int_price_;
  int size = 10;
  int saos = 0;

  prom_order_manager->OrderSequenced(0, saos, saos, sec_id, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0,
                                     watch_->tv());
  prom_order_manager->OrderConfirmed(0, saos, saos, sec_id, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0,
                                     watch_->tv());

  // Update the best bid with including our orders
  smv_->UpdateBestBidVariablesUsingOurOrders();

  CPPUNIT_ASSERT_EQUAL(original_size - size, smv_->market_update_info().bestbid_size_);
  CPPUNIT_ASSERT_EQUAL(intpx, smv_->market_update_info().bestbid_int_price_);

  saos++;
  size = original_size - size;  // Make remaing size as our size only

  prom_order_manager->OrderSequenced(0, saos, saos, sec_id, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0,
                                     watch_->tv());
  prom_order_manager->OrderConfirmed(0, saos, saos, sec_id, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0,
                                     watch_->tv());

  smv_->UpdateBestBidVariablesUsingOurOrders();

  // Now the price should have changed

  // Checking if price has changed
  CPPUNIT_ASSERT_ASSERTION_FAIL(CPPUNIT_ASSERT_EQUAL(intpx, smv_->market_update_info().bestbid_int_price_));
}

/**
 * Same As Bid
 */
void BaseMarketViewManagerTests::TestUpdateBestAskVariablesUsingOurOrders(void) {
  MvmTestsUtils::ConstructDummyBook(smv_);

  // Prom order manager should be here
  CPPUNIT_ASSERT(smv_->p_prom_order_manager_ != nullptr);

  smv_->remove_self_orders_from_book_ = true;

  auto prom_order_manager = PromOrderManager::GetCreatedInstance(smv_->shortcode());

  auto sec_id = smv_->security_id();
  int original_size = smv_->market_update_info().asklevels_[smv_->base_ask_index_].limit_size_;
  double px = smv_->market_update_info().asklevels_[smv_->base_ask_index_].limit_int_price_;
  int intpx = smv_->market_update_info().asklevels_[smv_->base_ask_index_].limit_int_price_;
  int size = 10;
  int saos = 0;

  prom_order_manager->OrderSequenced(0, saos, saos, sec_id, px, kTradeTypeSell, size, 0, 0, 0, intpx, 0, 0,
                                     watch_->tv());
  prom_order_manager->OrderConfirmed(0, saos, saos, sec_id, px, kTradeTypeSell, size, 0, 0, 0, intpx, 0, 0,
                                     watch_->tv());

  // Update the best bid with including our orders
  smv_->UpdateBestAskVariablesUsingOurOrders();

  CPPUNIT_ASSERT_EQUAL(original_size - size, smv_->market_update_info().bestask_size_);
  CPPUNIT_ASSERT_EQUAL(intpx, smv_->market_update_info().bestask_int_price_);

  saos++;
  size = original_size - size;  // Make remaing size as our size only

  prom_order_manager->OrderSequenced(0, saos, saos, sec_id, px, kTradeTypeSell, size, 0, 0, 0, intpx, 0, 0,
                                     watch_->tv());
  prom_order_manager->OrderConfirmed(0, saos, saos, sec_id, px, kTradeTypeSell, size, 0, 0, 0, intpx, 0, 0,
                                     watch_->tv());

  // Update the best bid with including our orders
  smv_->UpdateBestAskVariablesUsingOurOrders();

  // Now the price should have changed

  // Checking if price has changed
  CPPUNIT_ASSERT_ASSERTION_FAIL(CPPUNIT_ASSERT_EQUAL(intpx, smv_->market_update_info().bestask_int_price_));
}

void BaseMarketViewManagerTests::TestUpdateBestBidVariables(void) {}
void BaseMarketViewManagerTests::TestUpdateBestAskVariables(void) {}

/**
 *  Test if sanitization of book works correctly
 */
void BaseMarketViewManagerTests::TestSanitizeBidSide(void) {
  MvmTestsUtils::ConstructDummyBook(smv_);

  // No need for exchange specific testing as this part is common to all exchanges
  // As long as the functions are called appropriately in book-manager, this should work fine
  auto book_manager = common_smv_source_->indexed_eobi_price_level_market_view_manager();

  // Now change the ask price to be less than Bid side

  auto& ask_levels = smv_->market_update_info().asklevels_;
  for (auto&& level : ask_levels) {
    level.limit_int_price_ -= 2;
    level.limit_price_ = smv_->GetDoublePx(level.limit_int_price_);
  }

  // Sanitize the bid side now
  auto book_state = book_manager->SanitizeBidSide(smv_->security_id());

  // Book should be non empty on bid side, book_state should be OK
  CPPUNIT_ASSERT_EQUAL(kBookManagerOK, book_state);

  auto& bid_levels = smv_->market_update_info().bidlevels_;
  CPPUNIT_ASSERT_EQUAL(bid_levels[smv_->base_bid_index_].limit_int_price_,
                       ask_levels[smv_->base_ask_index_].limit_int_price_ - 1);

  // Now lets change the ask side so much that book becomes empty on bid side

  for (auto&& level : ask_levels) {
    level.limit_int_price_ -= 5;  // total of shift of 7
    level.limit_price_ = smv_->GetDoublePx(level.limit_int_price_);
  }

  book_state = book_manager->SanitizeBidSide(smv_->security_id());
  // Book should be empty on bid side, book_state should be Return

  CPPUNIT_ASSERT_EQUAL(kBookManagerReturn, book_state);
}

/**
 * Test if sanitization works on ask side
 */
void BaseMarketViewManagerTests::TestSanitizeAskSide(void) {
  MvmTestsUtils::ConstructDummyBook(smv_);

  // No need for exchange specific testing as this part is common to all exchanges
  // As long as the functions are called appropriately in book-manager, this should work fine
  auto book_manager = common_smv_source_->indexed_eobi_price_level_market_view_manager();

  // Now change the ask price to be less than Bid side

  auto& bid_levels = smv_->market_update_info().bidlevels_;
  for (auto&& level : bid_levels) {
    level.limit_int_price_ += 2;
    level.limit_price_ = smv_->GetDoublePx(level.limit_int_price_);
  }

  // Sanitize the bid side now
  auto book_state = book_manager->SanitizeAskSide(smv_->security_id());

  // Book should be non empty on bid side, book_state should be OK
  CPPUNIT_ASSERT_EQUAL(kBookManagerOK, book_state);

  auto& ask_levels = smv_->market_update_info().asklevels_;
  CPPUNIT_ASSERT_EQUAL(ask_levels[smv_->base_bid_index_].limit_int_price_,
                       bid_levels[smv_->base_ask_index_].limit_int_price_ + 1);

  // Now lets change the ask side so much that book becomes empty on bid side

  for (auto&& level : bid_levels) {
    level.limit_int_price_ += 5;  // total of shift of 7
    level.limit_price_ = smv_->GetDoublePx(level.limit_int_price_);
  }

  book_state = book_manager->SanitizeAskSide(smv_->security_id());
  // Book should be empty on bid side, book_state should be Return

  CPPUNIT_ASSERT_EQUAL(kBookManagerReturn, book_state);
}

/**
 * Functions below test BuildHigh/Low indices, ignoring them for now
 */
void BaseMarketViewManagerTests::TestAdjustAskIndex(void) {}

void BaseMarketViewManagerTests::TestAdjustBidIndex(void) {}

/**
 * Clean up after each test run
 */
void BaseMarketViewManagerTests::tearDown() {
  HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
  HFSAT::SecurityDefinitions::RemoveUniqueInstance();
  HFSAT::SecurityNameIndexer::GetUniqueInstance().Clear();
  HFSAT::CurrencyConvertor::RemoveInstance();
  HFSAT::PromOrderManager::RemoveUniqueInstance(smv_->shortcode());

  // deallocate the heap variables

  if (common_smv_source_ != nullptr) {
    delete common_smv_source_;
    common_smv_source_ = nullptr;
  }
}
}
