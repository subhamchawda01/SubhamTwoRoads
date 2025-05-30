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
#include "baseinfra/Tests/MarketAdapter/rts_of_mvm_tests.hpp"

namespace HFTEST {

void RTSOfMvmTests::setUp() {
  exchange_ = "RTSOF";

  std::vector<std::string> shortcode_list_;

  MvmTestsUtils::ReadInstructionFile(exchange_, shortcode_list_, rts_of_instruction_list_);
  order = new RTS_MDS::RTSOFCommonStructv2();
  order->md_flags = 0;  // empty bitmask for order/trade type
  common_smv_source_ = new CommonSMVSource(shortcode_list_, 20171130);

  // remove this later on when we completely scrape away PF, sets to use new OF
  common_smv_source_->SetRTSOFBookType();
  //////////////

  common_smv_source_->SetDepShortcodeVector(shortcode_list_);
  common_smv_source_->SetSourceShortcodes(shortcode_list_);
  common_smv_source_->InitializeVariables();
  watch_ = &common_smv_source_->getWatch();
}
void RTSOfMvmTests::showBook(int security_id) {
  auto& sec_id_smv_map = common_smv_source_->getSMVMap();
  auto this_smv_ = *sec_id_smv_map[security_id];

  for (int t_level_ = 0; t_level_ < 10; t_level_++) {
    printf("\033[2K%4d %10d %6d %22.9f %14d X %14d %22.9f %6d %10d %4d\n",
           std::min(9999, abs(this_smv_.bid_int_price_level(t_level_))), this_smv_.bid_size(t_level_),
           this_smv_.bid_order(t_level_), this_smv_.bid_price(t_level_), this_smv_.bid_int_price(t_level_),
           this_smv_.ask_int_price(t_level_), this_smv_.ask_price(t_level_), this_smv_.ask_order(t_level_),
           this_smv_.ask_size(t_level_), std::min(9999, abs(this_smv_.ask_int_price_level(t_level_))));
  }
  std::cout << std::endl;
}

void RTSOfMvmTests::TestRTSOfMvm() {
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  auto rts_of_book_manager = common_smv_source_->indexed_rts_of_market_view_manager();

  std::string current_test_name = "";

  std::cout << std::endl;

  for (auto& instruction : rts_of_instruction_list_) {
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

      if (reset_book) {
        order->msg_type = RTS_MDS::RTSOFMsgType::kRTSResetBegin;
        order->side = '0';
        rts_of_book_manager->Process(security_id, order);
      }
      std::cout << "TESTCASE " << current_test_name << std::endl;

    } else if (instruction[2].compare("OnOrderAdd") == 0) {
      // New Order
      // SYNTAX <shc> <OnOrderAdd>  <buysell> <order_id> <price> <size> <priority> <intermediate>

      uint8_t side = instruction[column_number++][0];
      uint64_t order_id = std::strtoul(instruction[column_number++].c_str(), nullptr, 10);

      double price = atof(instruction[column_number++].c_str());
      int size = atoi(instruction[column_number++].c_str());
      column_number++;
      bool intermediate = atoi(instruction[column_number++].c_str()) != 0;

      order->msg_type = RTS_MDS::RTSOFMsgType::kRTSAdd;
      order->order_id = order_id;
      order->side = side;
      order->price = price;
      order->size = size;
      order->is_intermediate = intermediate;
#if RTS_OF_MVM_DBG
      std::cout << "OnOrderAdd " << order->ToString() << std::endl;
#endif

      rts_of_book_manager->Process(security_id, order);

#if RTS_OF_MVM_DBG
      showBook(security_id);
#endif
    } else if (instruction[2].compare("OnOrderDelete") == 0) {
      // Order Delete Message
      // SYNTAX <shc> <OnOrderDelete> <buysell> <order_id> <intermediate>

      uint8_t side = instruction[column_number++][0];
      uint64_t order_id = std::strtoul(instruction[column_number++].c_str(), nullptr, 10);

      double price = atof(instruction[column_number++].c_str());
      int size = atoi(instruction[column_number++].c_str());
      column_number++;
      bool intermediate = atoi(instruction[column_number++].c_str()) != 0;

      order->msg_type = RTS_MDS::RTSOFMsgType::kRTSDelete;
      order->order_id = order_id;
      order->side = side;
      order->price = price;
      order->size = size;
      order->is_intermediate = intermediate;
#if RTS_OF_MVM_DBG
      std::cout << "OnOrderDelete " << order->ToString() << std::endl;
#endif

      rts_of_book_manager->Process(security_id, order);

#if RTS_OF_MVM_DBG
      showBook(security_id);
#endif

    } else if (instruction[2].compare("OnOrderExec") == 0) {
      // An order was partially executed
      // SYNTAX <shc> <OnOrderExec>   <buysell> <order_id> <price> <size_exec> <intermediate>

      uint8_t side = instruction[column_number++][0];
      uint64_t order_id = std::strtoul(instruction[column_number++].c_str(), nullptr, 10);
      double price = atof(instruction[column_number++].c_str());
      int size = atoi(instruction[column_number++].c_str());
      bool is_full_exec = atoi(instruction[column_number++].c_str()) != 0;
      bool intermediate = atoi(instruction[column_number++].c_str()) != 0;

      order->msg_type = RTS_MDS::RTSOFMsgType::kRTSExec;
      order->order_id = order_id;
      order->side = side;
      order->price = price;
      order->size = size;
      order->is_full_exec = is_full_exec;
      order->is_intermediate = intermediate;
#if RTS_OF_MVM_DBG
      std::cout << "OnOrderExec " << order->ToString() << std::endl;
#endif
      rts_of_book_manager->Process(security_id, order);

#if RTS_OF_MVM_DBG
      showBook(security_id);
#endif

    } else if (instruction[2].find("ASSERT") != std::string::npos) {
      // All type of assert statements here

      auto& sec_id_smv_map = common_smv_source_->getSMVMap();
      auto smv = sec_id_smv_map[security_id];

      // The asserts are actually run here ..
      MvmTestsUtils::ParseAndRunAssert(current_test_name, instruction, smv);
    }
  }
}

void RTSOfMvmTests::tearDown() {
  HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
  HFSAT::SecurityDefinitions::RemoveUniqueInstance();
  HFSAT::SecurityNameIndexer::GetUniqueInstance().Clear();
  HFSAT::CurrencyConvertor::RemoveInstance();

  delete common_smv_source_;
  common_smv_source_ = nullptr;
}
}
