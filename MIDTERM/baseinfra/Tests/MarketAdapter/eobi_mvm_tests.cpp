/**
   \file Tests/dvctrade/MarketAdapter/eobi_mvm_tests.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/Tests/MarketAdapter/mvm_tests_utils.hpp"
#include "baseinfra/Tests/MarketAdapter/eobi_mvm_tests.hpp"

namespace HFTEST {

void EOBIMvmTests::setUp() {
  exchange_ = "EOBI";

  std::vector<std::string> shortcode_list_;

  MvmTestsUtils::ReadInstructionFile(exchange_, shortcode_list_, eobi_instruction_list_);
  common_smv_source_ = new CommonSMVSource(shortcode_list_, 20160310);
  common_smv_source_->SetDepShortcodeVector(shortcode_list_);
  common_smv_source_->SetSourceShortcodes(shortcode_list_);
  common_smv_source_->InitializeVariables();
  watch_ = &common_smv_source_->getWatch();
}

void EOBIMvmTests::TestEOBIMvm() {
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  auto eobi_book_manager = common_smv_source_->indexed_eobi_market_view_manager();

  std::string current_test_name = "";

  std::cout << std::endl;

  for (auto& instruction : eobi_instruction_list_) {
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
      if (instruction.size() > 2) {
        current_test_name = instruction[2];
      } else {
        current_test_name = "NameNotGiven";
      }

      bool reset_book = false;
      if (instruction.size() > 3) {
        reset_book = atoi(instruction[3].c_str()) != 0;
      }

      if (reset_book) eobi_book_manager->ResetBook(security_id);

      std::cout << "TESTCASE " << current_test_name << std::endl;

    } else if (instruction[2].compare("OnOrderAdd") == 0) {
      // New Level

      TradeType_t buysell = instruction[column_number++][0] == 'B' ? kTradeTypeBuy : kTradeTypeSell;
      double price = atof(instruction[column_number++].c_str());
      int size = atoi(instruction[column_number++].c_str());
      bool intermediate = atoi(instruction[column_number++].c_str()) != 0;

      eobi_book_manager->OnOrderAdd(security_id, buysell, price, size, intermediate);

    } else if (instruction[2].compare("OnOrderModify") == 0) {
      // Level Change Message

      TradeType_t buysell = instruction[column_number++][0] == 'B' ? kTradeTypeBuy : kTradeTypeSell;
      double price = atof(instruction[column_number++].c_str());
      int size = atoi(instruction[column_number++].c_str());
      double prev_price = atof(instruction[column_number++].c_str());
      int prev_size = atoi(instruction[column_number++].c_str());

      eobi_book_manager->OnOrderModify(security_id, buysell, price, size, prev_price, prev_size);

    } else if (instruction[2].compare("OnOrderDelete") == 0) {
      // Order Delete Message

      TradeType_t buysell = instruction[column_number++][0] == 'B' ? kTradeTypeBuy : kTradeTypeSell;
      double price = atof(instruction[column_number++].c_str());
      int size = atoi(instruction[column_number++].c_str());
      bool delete_order = atoi(instruction[column_number++].c_str()) != 0;
      bool intermediate = atoi(instruction[column_number++].c_str()) != 0;

      eobi_book_manager->OnOrderDelete(security_id, buysell, price, size, delete_order, intermediate);

    } else if (instruction[2].compare("OnOrderMassDelete") == 0) {
      //  Mass delete message

      eobi_book_manager->OnOrderMassDelete(security_id);

    } else if (instruction[2].compare("OnPartialOrderExecution") == 0) {
      // An order was partially executed

      TradeType_t buysell = instruction[column_number++][0] == 'B' ? kTradeTypeBuy : kTradeTypeSell;
      double price = atof(instruction[column_number++].c_str());
      int size = atoi(instruction[column_number++].c_str());

      eobi_book_manager->OnPartialOrderExecution(security_id, buysell, price, size);

    } else if (instruction[2].compare("OnFullOrderExecution") == 0) {
      // An Order was fully executed

      TradeType_t buysell = instruction[column_number++][0] == 'B' ? kTradeTypeBuy : kTradeTypeSell;
      double price = atof(instruction[column_number++].c_str());
      int size = atoi(instruction[column_number++].c_str());

      eobi_book_manager->OnFullOrderExecution(security_id, buysell, price, size);

    } else if (instruction[2].compare("OnExecutionSummary") == 0) {
      // Summary of trade

      TradeType_t buysell = instruction[column_number++][0] == 'B' ? kTradeTypeBuy : kTradeTypeSell;
      double price = atof(instruction[column_number++].c_str());
      int size = atoi(instruction[column_number++].c_str());

      eobi_book_manager->OnExecutionSummary(security_id, buysell, price, size);

    } else if (instruction[2].find("ASSERT") != std::string::npos) {
      // All type of assert statements here

      auto& sec_id_smv_map = common_smv_source_->getSMVMap();
      auto smv = sec_id_smv_map[security_id];

      // The asserts are actually run here ..
      MvmTestsUtils::ParseAndRunAssert(current_test_name, instruction, smv);
    }
  }
}

void EOBIMvmTests::tearDown() {
  HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
  HFSAT::SecurityDefinitions::RemoveUniqueInstance();
  HFSAT::SecurityNameIndexer::GetUniqueInstance().Clear();
  HFSAT::CurrencyConvertor::RemoveInstance();

  delete common_smv_source_;
  common_smv_source_ = nullptr;
}
}
