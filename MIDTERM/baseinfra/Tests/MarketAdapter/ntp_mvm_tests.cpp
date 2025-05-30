/**
   \file Tests/dvctrade/MarketAdapter/ntp_mvm_tests.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/Tests/MarketAdapter/mvm_tests_utils.hpp"
#include "baseinfra/Tests/MarketAdapter/ntp_mvm_tests.hpp"

namespace HFTEST {

//-----------------------------------------------------------------------------

/*
 * Test Case 1: testPriceLevelNew
 * We call the PriceLevelNew function of the market view manager with our Inputs
 * then check the smv with the expected output, using Assertions.
 */
void NtpMvmTests::TestPriceLevelNew(void) {
  // Get the variables
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();

  // Get the book manager
  auto ntp_book_manager = common_smv_source_->indexed_ntp_market_view_manager();

  // Get the sec id and smv
  unsigned int sec_id = sec_name_indexer.GetIdFromString("BR_DOL_0");
  auto& sid_smv_map = common_smv_source_->getSMVMap();
  auto smv = sid_smv_map[sec_id];

  // Call ntp-book manager
  ntp_book_manager->OnPriceLevelNew(sec_id, kTradeTypeBuy, 1, 2.5, 32, 12, false);

  int size = smv->market_update_info_.bestbid_size_;
  int num_orders = smv->market_update_info_.bestbid_ordercount_;
  int price = smv->market_update_info_.bestbid_int_price_;

  CPPUNIT_ASSERT(size == 32);
  CPPUNIT_ASSERT(num_orders == 12);
  CPPUNIT_ASSERT(price == 5);
}

/*
 * Test Case 2: testPriceLevelModify
 * We call the PriceLevelNew followed by PriceLevelChange function of the market view manager with our Inputs
 * then check the smv with the expected output, using Assertions.
 */
void NtpMvmTests::TestPriceLevelModify(void) {
  // Get the variables
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();

  // Get the book manager
  auto ntp_book_manager = common_smv_source_->indexed_ntp_market_view_manager();

  // Get the sec id and smv
  unsigned int sec_id = sec_name_indexer.GetIdFromString("BR_DOL_0");
  auto& sid_smv_map = common_smv_source_->getSMVMap();
  auto smv = sid_smv_map[sec_id];

  ntp_book_manager->OnPriceLevelNew(sec_id, kTradeTypeBuy, 1, 2.5, 32, 12, false);
  ntp_book_manager->OnPriceLevelChange(sec_id, kTradeTypeBuy, 1, 2.5, 34, 13, false);

  int size = smv->market_update_info_.bestbid_size_;
  int num_orders = smv->market_update_info_.bestbid_ordercount_;
  int price = smv->market_update_info_.bestbid_int_price_;

  CPPUNIT_ASSERT(size == 34);
  CPPUNIT_ASSERT(num_orders == 13);
  CPPUNIT_ASSERT(price == 5);
}

// Tests the general cases, reading from file
void NtpMvmTests::TestNTPMvm(void) {
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  auto ntp_book_manager = common_smv_source_->indexed_ntp_market_view_manager();

  std::string current_test_name = "";
  std::cout << std::endl;
  for (auto& instruction : ntp_instruction_list_) {
    //

    // for (auto& instr : instruction) std::cout << instr << " ";
    // std::cout << std::endl;

    auto& shortcode = instruction[0];
    unsigned int security_id = sec_name_indexer.GetIdFromString(shortcode);

    // Parse the watch time
    if (instruction[1].compare("TESTCASE") != 0 && instruction.size() > 2) {
      int sec = atoi(instruction[1].substr(0, instruction[1].find(".")).c_str());
      int usec = atoi(instruction[1].substr(instruction[1].find(".")).c_str());
      watch_->OnTimeReceived(ttime_t(sec, usec));
    }

    unsigned column_number = 3;

    if (instruction[1].compare("TESTCASE") == 0) {
      if (instruction.size() > 2)
        current_test_name = instruction[2];
      else
        current_test_name = "NameNotGiven";

      bool reset_book = false;
      if (instruction.size() > 3) {
        reset_book = atoi(instruction[3].c_str()) != 0;
      }

      if (reset_book) ntp_book_manager->ResetBook(security_id);

      std::cout << "TESTCASE " << current_test_name << std::endl;

    } else if (instruction[2].compare("OnPriceLevelNew") == 0) {
      // New Level

      TradeType_t buysell = instruction[column_number++][0] == 'B' ? kTradeTypeBuy : kTradeTypeSell;
      int level_added = atoi(instruction[column_number++].c_str());
      double price = atof(instruction[column_number++].c_str());
      int size = atoi(instruction[column_number++].c_str());
      int ordercount = atoi(instruction[column_number++].c_str());
      bool intermediate = atoi(instruction[column_number++].c_str()) != 0;

      ntp_book_manager->OnPriceLevelNew(security_id, buysell, level_added, price, size, ordercount, intermediate);

    } else if (instruction[2].compare("OnPriceLevelChange") == 0) {
      // Level Change Message

      TradeType_t buysell = instruction[column_number++][0] == 'B' ? kTradeTypeBuy : kTradeTypeSell;
      int level_changed = atoi(instruction[column_number++].c_str());
      double price = atof(instruction[column_number++].c_str());
      int size = atoi(instruction[column_number++].c_str());
      int ordercount = atoi(instruction[column_number++].c_str());
      bool intermediate = atoi(instruction[column_number++].c_str()) != 0;

      ntp_book_manager->OnPriceLevelChange(security_id, buysell, level_changed, price, size, ordercount, intermediate);

    } else if (instruction[2].compare("OnPriceLevelDelete") == 0) {
      // Level Delete Message

      TradeType_t buysell = instruction[column_number++][0] == 'B' ? kTradeTypeBuy : kTradeTypeSell;
      int level_changed = atoi(instruction[column_number++].c_str());
      double price = atof(instruction[column_number++].c_str());
      bool intermediate = atoi(instruction[column_number++].c_str()) != 0;

      ntp_book_manager->OnPriceLevelDelete(security_id, buysell, level_changed, price, intermediate);

    } else if (instruction[2].compare("OnTrade") == 0) {
      // Trade Message
      // not reading the side
      column_number++;
      double price = atof(instruction[column_number++].c_str());
      int size = atoi(instruction[column_number++].c_str());

      ntp_book_manager->OnTrade(security_id, price, size);

    } else if (instruction[2].compare("OnPriceLevelDeleteThru") == 0) {
      // All levels till given level are deleted ( including the given level)

      TradeType_t buysell = instruction[column_number++][0] == 'B' ? kTradeTypeBuy : kTradeTypeSell;

      bool intermediate = (atoi(instruction[column_number++].c_str()) != 0);

      ntp_book_manager->OnPriceLevelDeleteThru(security_id, buysell, intermediate);
    } else if (instruction[2].compare("OnPriceLevelDeleteFrom") == 0) {
      // All levels starting from given level are deleted

      TradeType_t buysell = instruction[column_number++][0] == 'B' ? kTradeTypeBuy : kTradeTypeSell;
      int min_level_deleted = atoi(instruction[column_number++].c_str());
      double price = atof(instruction[column_number++].c_str());
      bool intermediate = (atoi(instruction[column_number++].c_str()) != 0);

      ntp_book_manager->OnPriceLevelDeleteFrom(security_id, buysell, min_level_deleted, price, intermediate);

    } else if (instruction[2].find("ASSERT") != std::string::npos) {
      // All type of assert statements here

      auto& sec_id_smv_map = common_smv_source_->getSMVMap();
      auto smv = sec_id_smv_map[security_id];

      // The asserts are actually run here ..
      MvmTestsUtils::ParseAndRunAssert(current_test_name, instruction, smv);
    }
  }
}

/**
 * Like Constructor : Initialize the book, which initializes all the commonly required variables/
 * Then we can add as many shortcodes as we want for testing
 * Make the object for ntp_mvm_
 */
void NtpMvmTests::setUp(void) {
  exchange_ = "NTP";
  std::vector<std::string> shortcode_list = {"BR_DOL_0"};

  MvmTestsUtils::ReadInstructionFile(exchange_, shortcode_list, ntp_instruction_list_);

  // Create smv source with the shortcodes
  common_smv_source_ = new CommonSMVSource(shortcode_list, 20160310);

  common_smv_source_->SetDepShortcodeVector(shortcode_list);
  common_smv_source_->SetSourceShortcodes(shortcode_list);
  common_smv_source_->InitializeVariables();

  watch_ = &common_smv_source_->getWatch();
}

// Like Destructor
void NtpMvmTests::tearDown(void) {
  HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
  HFSAT::SecurityDefinitions::RemoveUniqueInstance();
  HFSAT::SecurityNameIndexer::GetUniqueInstance().Clear();
  HFSAT::CurrencyConvertor::RemoveInstance();

  delete common_smv_source_;
  common_smv_source_ = nullptr;
}

//-----------------------------------------------------------------------------
}
