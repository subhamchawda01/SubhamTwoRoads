#include "baseinfra/Tests/MarketAdapter/fpga_mvm_tests.hpp"
#include "baseinfra/Tests/MarketAdapter/mvm_tests_utils.hpp"

namespace HFTEST {

FPGAMvmTests::FPGAMvmTests()
    : halfbook_update_cfg_filename_(GetTestDataFullPath("HalfBook_Update_data.txt", "baseinfra")),
      common_smv_source_(nullptr),
      watch_(nullptr) {}

void FPGAMvmTests::setUp() {
  exchange_ = "FPGA";

  std::vector<std::string> shortcode_list_;

  std::vector<std::vector<std::string> > instruction_vec_;

  MvmTestsUtils::ReadInstructionFile(exchange_, shortcode_list_, cme_instruction_list_);

  common_smv_source_ = new CommonSMVSource(shortcode_list_, 20160310);
  common_smv_source_->SetDepShortcodeVector(shortcode_list_);
  common_smv_source_->SetSourceShortcodes(shortcode_list_);
  common_smv_source_->SetBookType(false, false, false, false, false, false, false, false, true, false, false);
  common_smv_source_->InitializeVariables();
  watch_ = &common_smv_source_->getWatch();
  ReadFpgaHalfBookUpdatesFile();
}

void FPGAMvmTests::InitFpgaStruct(FPGAHalfBook& fpga_halfbook) {
  for (auto count = 0; count < 5; ++count) {
    fpga_halfbook.num_orders_[count] = 0;
    fpga_halfbook.prices_[count] = 0;
    fpga_halfbook.sizes_[count] = -1;
  }
}

void FPGAMvmTests::UpdateFpgaStruct(FPGAHalfBook* fpga_halfbook, HFSAT::TradeType_t buy_sell, int level, int price,
                                    int num_orders, int order_size) {
  int buy_sell_index = buy_sell == HFSAT::kTradeTypeBuy ? 0 : 1;
  fpga_halfbook[buy_sell_index].num_orders_[level] = num_orders;
  fpga_halfbook[buy_sell_index].prices_[level] = price;
  fpga_halfbook[buy_sell_index].sizes_[level] = order_size;
}

void FPGAMvmTests::ReadFpgaHalfBookUpdatesFile() {
  std::ifstream halfbook_update_file_;
  halfbook_update_file_.open(halfbook_update_cfg_filename_.c_str(), std::ifstream::in);

  if (halfbook_update_file_.is_open()) {
    int line_length = 1024;
    char line[line_length];
    bzero(line, line_length);

    while (halfbook_update_file_.good()) {
      bzero(line, line_length);
      halfbook_update_file_.getline(line, line_length);
      HFSAT::PerishableStringTokenizer st_(line, line_length);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() > 1) {
        if (tokens_[0][0] == '#') continue;

        if (tokens_.size() == 6) {
          FPGAHalfBook* halfbook_ptr(nullptr);
          halfbook_ptr = new FPGAHalfBook[2];
          InitFpgaStruct(halfbook_ptr[0]);
          InitFpgaStruct(halfbook_ptr[1]);

          int update_number = atoi(tokens_[0]);
          if (halfbook_update_map_.find(update_number) == halfbook_update_map_.end()) {
            halfbook_update_map_[update_number] = new FPGAHalfBook[2];
            halfbook_ptr = halfbook_update_map_[update_number];
            InitFpgaStruct(halfbook_ptr[0]);
            InitFpgaStruct(halfbook_ptr[1]);
          } else {
            halfbook_ptr = halfbook_update_map_[update_number];
          }
          HFSAT::TradeType_t buy_sell = std::string(tokens_[1]) == "B" ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;
          int level = atoi(tokens_[2]);
          int price = atoi(tokens_[3]);
          int num_orders = atoi(tokens_[4]);
          int order_size = atoi(tokens_[5]);
          UpdateFpgaStruct(halfbook_ptr, buy_sell, level, price, num_orders, order_size);
        }
      }
    }
  }
}

bool FPGAMvmTests::IsSideUpdate(HFSAT::TradeType_t side_, FPGAHalfBook* halfbook_) {
  int index = (side_ == HFSAT::kTradeTypeBuy) ? 0 : 1;
  for (int level = 0; level < 5; ++level) {
    if (halfbook_[index].sizes_[level] != -1) return true;
  }
  return false;
}

void FPGAMvmTests::TestFpgaMvm() {
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  HFSAT::IndexedFpgaMarketViewManager* indexed_fpga_market_view_mgr =
      common_smv_source_->indexed_fpga_market_view_manager();

  std::string current_test_name = "";

  for (auto& instruction : cme_instruction_list_) {
    auto& shortcode = instruction[0];
    unsigned int security_id = sec_name_indexer.GetIdFromString(shortcode);

    // Parse the watch time
    if (instruction[1].compare("TESTCASE") != 0 && instruction.size() > 2) {
      int sec = atoi(instruction[1].substr(0, instruction[1].find(".")).c_str());
      int usec = atoi(instruction[1].substr(instruction[1].find(".")).c_str());
      watch_->OnTimeReceived(ttime_t(sec, usec));
    }

    if (instruction[1].compare("TESTCASE") == 0) {
      if (instruction.size() > 2)
        current_test_name = instruction[2];
      else
        current_test_name = "NameNotGiven";

      bool reset_book = false;
      if (instruction.size() > 3) {
        reset_book = atoi(instruction[3].c_str()) != 0;
      }

      if (reset_book) indexed_fpga_market_view_mgr->ResetBook(security_id);

    } else if (instruction[2].compare("HalfBookChange") == 0) {
      int update_num = atoi(instruction[3].c_str());
      if (halfbook_update_map_.find(update_num) != halfbook_update_map_.end()) {
        FPGAHalfBook* halfbook = halfbook_update_map_[update_num];
        bool is_buy = IsSideUpdate(HFSAT::kTradeTypeBuy, halfbook);
        bool is_sell = IsSideUpdate(HFSAT::kTradeTypeSell, halfbook);

        if (is_buy)
          indexed_fpga_market_view_mgr->OnHalfBookChange(security_id, HFSAT::kTradeTypeBuy, &halfbook[0],
                                                         is_buy && is_sell);
        if (is_sell)
          indexed_fpga_market_view_mgr->OnHalfBookChange(security_id, HFSAT::kTradeTypeSell, &halfbook[1], false);
      }
    } else if (instruction[2].compare("Trade") == 0) {
      HFSAT::TradeType_t side = (instruction[3] == "B") ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;
      int trade_price = atoi(instruction[4].c_str());
      int trade_size = atoi(instruction[5].c_str());
      indexed_fpga_market_view_mgr->OnTrade(security_id, trade_price, trade_size, side);
    } else if (instruction[2].compare("SyntheticDelete") == 0) {
      HFSAT::TradeType_t side = (instruction[3] == "B") ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;
      int price = atoi(instruction[4].c_str());
      indexed_fpga_market_view_mgr->OnPriceLevelDeleteSynthetic(security_id, side, price, false);
    } else if (instruction[2].find("ASSERT") != std::string::npos) {
      // All type of assert statements here

      auto& sec_id_smv_map = common_smv_source_->getSMVMap();
      auto smv = sec_id_smv_map[security_id];

      // The asserts are actually run here ..

      MvmTestsUtils::ParseAndRunAssert(current_test_name, instruction, smv);
    }
  }
}
void FPGAMvmTests::tearDown() {
  HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
  HFSAT::SecurityDefinitions::RemoveUniqueInstance();
  HFSAT::SecurityNameIndexer::GetUniqueInstance().Clear();
  HFSAT::CurrencyConvertor::RemoveInstance();
  for (auto it : halfbook_update_map_) {
    if (!it.second) delete it.second;
  }

  delete common_smv_source_;
  common_smv_source_ = nullptr;
}
}
