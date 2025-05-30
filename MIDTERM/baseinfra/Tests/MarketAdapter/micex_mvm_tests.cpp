/**
   \file Tests/dvctrade/MarketAdapter/rts_mvm_tests.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/Tests/MarketAdapter/mvm_tests_utils.hpp"
#include "baseinfra/Tests/MarketAdapter/micex_mvm_tests.hpp"

namespace HFTEST {

void MICEXMvmTests::setUp() {
  exchange_ = "MICEX";

  std::vector<std::string> shortcode_list_;

  std::vector<std::vector<std::string> > instruction_vec_;

  MvmTestsUtils::ReadInstructionFile(exchange_, shortcode_list_, cme_instruction_list_);

  // remove this later on when we completely scrape away PF
  //  common_smv_source_->SetMICEXBookType(true);

  // date here is just a place holder
  common_smv_source_ = new CommonSMVSource(shortcode_list_, 20160310);
  common_smv_source_->SetDepShortcodeVector(shortcode_list_);
  common_smv_source_->SetSourceShortcodes(shortcode_list_);
  common_smv_source_->InitializeVariables();
  watch_ = &common_smv_source_->getWatch();

  of_cstr_ = new MICEX_OF_MDS::MICEXOFCommonStruct();

  // HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_ = common_smv_source_->getSMVMap();
}

void MICEXMvmTests::TestMICEXMvm() {
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();

  auto micex_book_manager = common_smv_source_->indexed_micex_of_market_view_manager();

  std::string current_test_name = "";
  std::cout << std::endl;
  for (auto& instruction : cme_instruction_list_) {
    //

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

      of_cstr_->msg_type = MICEX_OF_MDS::MICEXOFMsgType::kMICEXResetBegin;

      if (reset_book) micex_book_manager->Process(security_id, of_cstr_);

      std::cout << "TESTCASE " << current_test_name << std::endl;

    } else if (instruction[2].compare("OnOrderAdd") == 0) {
      of_cstr_->msg_type = MICEX_OF_MDS::MICEXOFMsgType::kMICEXAdd;
      of_cstr_->data.add.side = (instruction[column_number++][0] == 'B' ? '0' : '1');
      of_cstr_->data.add.order_id = atoi(instruction[column_number++].c_str());
      of_cstr_->data.add.price = atof(instruction[column_number++].c_str());
      of_cstr_->data.add.size = atoi(instruction[column_number++].c_str());
      of_cstr_->is_slower_ = atoi(instruction[column_number++].c_str()) != 0;
      of_cstr_->intermediate_ = atoi(instruction[column_number++].c_str()) != 0;

      micex_book_manager->Process(security_id, of_cstr_);

    } else if (instruction[2].compare("OnOrderModify") == 0) {
      of_cstr_->msg_type = MICEX_OF_MDS::MICEXOFMsgType::kMICEXModify;
      of_cstr_->data.mod.side = (instruction[column_number++][0] == 'B' ? '0' : '1');
      of_cstr_->data.mod.order_id = atoi(instruction[column_number++].c_str());
      of_cstr_->data.mod.size = atoi(instruction[column_number++].c_str());
      of_cstr_->is_slower_ = atoi(instruction[column_number++].c_str()) != 0;
      of_cstr_->intermediate_ = atoi(instruction[column_number++].c_str()) != 0;

      micex_book_manager->Process(security_id, of_cstr_);

    } else if (instruction[2].compare("OnOrderDelete") == 0) {
      of_cstr_->msg_type = MICEX_OF_MDS::MICEXOFMsgType::kMICEXDelete;
      of_cstr_->data.del.side = (instruction[column_number++][0] == 'B' ? '0' : '1');
      of_cstr_->data.del.order_id = atoi(instruction[column_number++].c_str());
      of_cstr_->is_slower_ = atoi(instruction[column_number++].c_str()) != 0;
      of_cstr_->intermediate_ = atoi(instruction[column_number++].c_str()) != 0;

      micex_book_manager->Process(security_id, of_cstr_);

    } else if (instruction[2].compare("OnOrderExec") == 0) {
      of_cstr_->msg_type = MICEX_OF_MDS::MICEXOFMsgType::kMICEXExec;
      of_cstr_->data.exec.side = instruction[column_number++][0];
      of_cstr_->data.exec.order_id = atoi(instruction[column_number++].c_str());
      of_cstr_->data.exec.price = atof(instruction[column_number++].c_str());
      of_cstr_->data.exec.size_exec = atoi(instruction[column_number++].c_str());
      of_cstr_->is_slower_ = atoi(instruction[column_number++].c_str()) != 0;
      of_cstr_->intermediate_ = atoi(instruction[column_number++].c_str()) != 0;
      of_cstr_->exchange_time_stamp = atoll(instruction[column_number++].c_str());

      micex_book_manager->Process(security_id, of_cstr_);

    } else if (instruction[2].compare("OnMSRL1Update") == 0) {
      of_cstr_->msg_type = MICEX_OF_MDS::MICEXOFMsgType::kMICEXBestLevelUpdate;
      of_cstr_->data.best_level_update.best_bid_price = atof(instruction[column_number++].c_str());
      of_cstr_->data.best_level_update.best_bid_size = atoi(instruction[column_number++].c_str());
      of_cstr_->data.best_level_update.best_ask_price = atof(instruction[column_number++].c_str());
      of_cstr_->data.best_level_update.best_ask_size = atoi(instruction[column_number++].c_str());
      of_cstr_->data.best_level_update.last_trade_price = atof(instruction[column_number++].c_str());
      of_cstr_->exchange_time_stamp = atoll(instruction[column_number++].c_str());

      micex_book_manager->Process(security_id, of_cstr_);

    } else if (instruction[2].find("ASSERT") != std::string::npos) {
      // All type of assert statements here

      auto& sec_id_smv_map = common_smv_source_->getSMVMap();
      auto smv = sec_id_smv_map[security_id];

      // The asserts are actually run here ..
      MvmTestsUtils::ParseAndRunAssert(current_test_name, instruction, smv);
    }
  }
}

void MICEXMvmTests::tearDown() {
  HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
  HFSAT::SecurityDefinitions::RemoveUniqueInstance();
  HFSAT::SecurityNameIndexer::GetUniqueInstance().Clear();
  HFSAT::CurrencyConvertor::RemoveInstance();

  delete of_cstr_;
  delete common_smv_source_;
  common_smv_source_ = nullptr;
}
}
