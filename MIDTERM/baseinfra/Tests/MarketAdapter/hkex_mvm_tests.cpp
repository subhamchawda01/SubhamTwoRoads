/**
   \file Tests/dvctrade/MarketAdapter/hkex_mvm_tests.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/Tests/MarketAdapter/mvm_tests_utils.hpp"
#include "baseinfra/Tests/MarketAdapter/hkex_mvm_tests.hpp"

namespace HFTEST {

void HKEXMvmTests::setUp() {
  exchange_ = "HKEX";

  std::vector<std::string> shortcode_list_;

  std::vector<std::vector<std::string> > instruction_vec_;

  MvmTestsUtils::ReadInstructionFile(exchange_, shortcode_list_, hkex_instruction_list_);
  common_smv_source_ = new CommonSMVSource(shortcode_list_, 20160310);
  common_smv_source_->SetDepShortcodeVector(shortcode_list_);
  common_smv_source_->SetSourceShortcodes(shortcode_list_);
  common_smv_source_->InitializeVariables();
  watch_ = &common_smv_source_->getWatch();

  // HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_ = common_smv_source_->getSMVMap();
}

void HKEXMvmTests::TestHKEXMvm() {
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  auto hkex_book_manager = common_smv_source_->hkomd_price_level_market_view_manager();

  std::string current_test_name = "";
  std::cout << std::endl;
  for (auto& instruction : hkex_instruction_list_) {
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

      if (reset_book) hkex_book_manager->ResetBook(security_id);

      std::cout << "TESTCASE " << current_test_name << std::endl;

    } else if (instruction[2].compare("OnPriceLevelNew") == 0) {
      // New Level

      TradeType_t buysell = instruction[column_number++][0] == 'B' ? kTradeTypeBuy : kTradeTypeSell;
      int level_added = atoi(instruction[column_number++].c_str());
      double price = atof(instruction[column_number++].c_str());
      int size = atoi(instruction[column_number++].c_str());
      int ordercount = atoi(instruction[column_number++].c_str());
      bool intermediate = atoi(instruction[column_number++].c_str()) != 0;

      hkex_book_manager->OnPriceLevelNew(security_id, buysell, level_added, price, size, ordercount, intermediate);

    } else if (instruction[2].compare("OnPriceLevelChange") == 0) {
      // Level Change Message

      TradeType_t buysell = instruction[column_number++][0] == 'B' ? kTradeTypeBuy : kTradeTypeSell;
      int level_changed = atoi(instruction[column_number++].c_str());
      double price = atof(instruction[column_number++].c_str());
      int size = atoi(instruction[column_number++].c_str());
      int ordercount = atoi(instruction[column_number++].c_str());
      bool intermediate = atoi(instruction[column_number++].c_str()) != 0;

      hkex_book_manager->OnPriceLevelChange(security_id, buysell, level_changed, price, size, ordercount, intermediate);

    } else if (instruction[2].compare("OnPriceLevelDelete") == 0) {
      // Level Delete Message

      TradeType_t buysell = instruction[column_number++][0] == 'B' ? kTradeTypeBuy : kTradeTypeSell;
      int level_changed = atoi(instruction[column_number++].c_str());
      double price = atof(instruction[column_number++].c_str());
      bool intermediate = atoi(instruction[column_number++].c_str()) != 0;

      hkex_book_manager->OnPriceLevelDelete(security_id, buysell, level_changed, price, intermediate);

    } else if (instruction[2].compare("OnTrade") == 0) {
      // Trade Message

      // Aggressing side
      TradeType_t buysell = instruction[column_number++][0] == 'B' ? kTradeTypeBuy : kTradeTypeSell;
      double price = atof(instruction[column_number++].c_str());
      int size = atoi(instruction[column_number++].c_str());

      hkex_book_manager->OnTrade(security_id, price, size, buysell);

    } else if (instruction[2].find("ASSERT") != std::string::npos) {
      // All type of assert statements here

      auto& sec_id_smv_map = common_smv_source_->getSMVMap();
      auto smv = sec_id_smv_map[security_id];

      // The asserts are actually run here ..
      MvmTestsUtils::ParseAndRunAssert(current_test_name, instruction, smv);
    }
  }
}

void HKEXMvmTests::tearDown() {
  HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
  HFSAT::SecurityDefinitions::RemoveUniqueInstance();
  HFSAT::SecurityNameIndexer::GetUniqueInstance().Clear();
  HFSAT::CurrencyConvertor::RemoveInstance();

  delete common_smv_source_;
  common_smv_source_ = nullptr;
}
}
