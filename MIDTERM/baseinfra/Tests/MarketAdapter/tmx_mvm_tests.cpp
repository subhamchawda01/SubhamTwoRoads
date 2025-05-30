/**
   \file Tests/dvctrade/MarketAdapter/nse_mvm_tests.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/Tests/MarketAdapter/mvm_tests_utils.hpp"
#include "baseinfra/Tests/MarketAdapter/tmx_mvm_tests.hpp"

namespace HFTEST {

void TMXMvmTests::setUp() {
  exchange_ = "TMX";

  std::vector<std::string> shortcode_list_;

  MvmTestsUtils::ReadInstructionFile(exchange_, shortcode_list_, tmx_instruction_list_);
  common_smv_source_ = new CommonSMVSource(shortcode_list_, 20170710);

  // remove this later on when we completely scrape away PF
  common_smv_source_->SetTMXBookType(true);
  //////////////

  common_smv_source_->SetDepShortcodeVector(shortcode_list_);
  common_smv_source_->SetSourceShortcodes(shortcode_list_);
  common_smv_source_->InitializeVariables();
  watch_ = &common_smv_source_->getWatch();
}

void TMXMvmTests::TestTMXMvm() {
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  auto tmx_book_manager = common_smv_source_->indexed_tmx_obf_of_market_view_manager();

  tmx_book_manager->SetAuctionPeriodStateVec(true);
  std::string current_test_name = "";

  std::cout << std::endl;

  for (auto& instruction : tmx_instruction_list_) {
    //

    // for (auto& instr : instruction) std::cout << instr << " ";
    // std::cout << std::endl;

    auto& shortcode = instruction[0];
    unsigned int security_id = sec_name_indexer.GetIdFromString(shortcode);

    // If this is not a start of testcase, it the 3rd column would

    if (instruction[1].compare("TESTCASE") != 0 && instruction.size() > 2) {
      int sec = atoi(instruction[1].substr(0, instruction[1].find(".")).c_str());
      int usec = atoi(instruction[1].substr(instruction[1].find(".")).c_str());
      watch_->OnTimeReceived(ttime_t(sec, usec));
    }

    unsigned column_number = 3;

    if (instruction[1].compare("TESTCASE") == 0) {
      if (instruction.size() > 2) {
        current_test_name = instruction[2];
      } else {
        current_test_name = "NameNotGiven";
      }

      bool reset_book = false;
      if (instruction.size() > 3) {
        reset_book = atoi(instruction[3].c_str()) != 0;
      }

      if (reset_book) tmx_book_manager->OnOrderResetBegin(security_id);
      std::cout << "TESTCASE " << current_test_name << std::endl;

    } else if (instruction[2].compare("OnOrderAdd") == 0) {
      // New Order
      // SYNTAX <shc> <OnOrderAdd>  <buysell> <order_id> <price> <size> <priority> <intermediate>

      uint8_t side = instruction[column_number++][0];
      uint64_t order_id = std::strtoul(instruction[column_number++].c_str(), nullptr, 10);

      double price = atof(instruction[column_number++].c_str());
      int size = atoi(instruction[column_number++].c_str());
      int priority = atoi(instruction[column_number++].c_str());
      bool intermediate = atoi(instruction[column_number++].c_str()) != 0;

      tmx_book_manager->OnOrderAdd(security_id, order_id, side, price, size, priority, intermediate);

    } else if (instruction[2].compare("OnOrderModify") == 0) {
      // Level Change Message
      // SYNTAX <shc> <OnOrderModify> <buysell> <order_id> <new_price> <new_size> <new_order_id> <intermediate>

      uint8_t side = instruction[column_number++][0];
      uint64_t order_id = std::strtoul(instruction[column_number++].c_str(), nullptr, 10);
      double new_price = atof(instruction[column_number++].c_str());
      int new_size = atoi(instruction[column_number++].c_str());
      uint64_t new_order_id = std::strtoul(instruction[column_number++].c_str(), nullptr, 10);
      bool intermediate = atoi(instruction[column_number++].c_str()) != 0;

      tmx_book_manager->OnOrderReplace(security_id, order_id, side, new_price, new_size, new_order_id, intermediate);

    } else if (instruction[2].compare("OnOrderDelete") == 0) {
      // Order Delete Message
      // SYNTAX <shc> <OnOrderDelete> <buysell> <order_id> <intermediate>

      uint8_t side = instruction[column_number++][0];
      uint64_t order_id = std::strtoul(instruction[column_number++].c_str(), nullptr, 10);
      bool intermediate = atoi(instruction[column_number++].c_str()) != 0;

      tmx_book_manager->OnOrderDelete(security_id, order_id, side, intermediate);

    } else if (instruction[2].compare("OnOrderExec") == 0) {
      // An order was partially executed
      // SYNTAX <shc> <OnOrderExec>   <buysell> <order_id> <price> <size_exec> <intermediate>

      uint8_t side = instruction[column_number++][0];
      uint64_t order_id = std::strtoul(instruction[column_number++].c_str(), nullptr, 10);
      double price = atof(instruction[column_number++].c_str());
      int size_exec = atoi(instruction[column_number++].c_str());
      bool intermediate = atoi(instruction[column_number++].c_str()) != 0;

      tmx_book_manager->OnOrderExec(security_id, order_id, side, price, size_exec, intermediate);

    } else if (instruction[2].find("ASSERT") != std::string::npos) {
      // All type of assert statements here

      auto& sec_id_smv_map = common_smv_source_->getSMVMap();
      auto smv = sec_id_smv_map[security_id];

      // The asserts are actually run here ..
      MvmTestsUtils::ParseAndRunAssert(current_test_name, instruction, smv);
    }
  }
}

void TMXMvmTests::tearDown() {
  HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
  HFSAT::SecurityDefinitions::RemoveUniqueInstance();
  HFSAT::SecurityNameIndexer::GetUniqueInstance().Clear();
  HFSAT::CurrencyConvertor::RemoveInstance();

  delete common_smv_source_;
  common_smv_source_ = nullptr;
}
}
