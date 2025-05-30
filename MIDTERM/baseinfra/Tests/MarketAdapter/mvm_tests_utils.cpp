/**
  \file Tests/dvctrade/MarketAdapter/mvm_tests_utils.cpp

  \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
*/

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "baseinfra/Tests/MarketAdapter/mvm_tests_utils.hpp"

namespace HFTEST {

namespace MvmTestsUtils {

/**
 * Reads the instruction lines for book mangers
 * @param exchange
 * @param shortcode_list
 * @param instruction_list
 */

void ReadInstructionFile(std::string exchange, std::vector<std::string>& shortcode_list,
                         std::vector<std::vector<std::string> >& instruction_list) {
  // for now storing the instructions in repo
  // later we can add these things somewhere in database or one single read only place

  std::string exchange_filename_ = GetTestDataFullPath(exchange + "_mvm_test_data.txt", "baseinfra");

  std::ifstream instruction_list_file_;
  instruction_list_file_.open(exchange_filename_.c_str(), std::ifstream::in);

  if (instruction_list_file_.is_open()) {
    // Fix maximum number of words to be red

    int kInstructionLineLength = 1024;
    char line[kInstructionLineLength];
    bzero(line, kInstructionLineLength);

    // read instructions from the file
    while (instruction_list_file_.good()) {
      // Read lines from the instructon files

      bzero(line, kInstructionLineLength);
      instruction_list_file_.getline(line, kInstructionLineLength);

      HFSAT::PerishableStringTokenizer st_(line, kInstructionLineLength);
      // Each instruction is vector of tokens
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() > 1) {
        // adding all the non empty lines for now
        // will check for validity of instructions later when we have all defined sets
        // assumption - First column is shortcode
        if (tokens_[0][0] == '#') {
          continue;
        }

        shortcode_list.push_back(std::string(tokens_[0]));
        std::vector<std::string> instruction_line_;

        for (auto token : tokens_) {
          instruction_line_.push_back(std::string(token));
        }

        instruction_list.push_back(instruction_line_);
      }
    }
  }
}

/**
 *
 * @param assert_line
 * @param smv
 */

void ParseAndRunAssert(std::string current_test_name, std::vector<std::string> assert_line,
                       HFSAT::SecurityMarketView* smv) {
  std::string assert_line_string = "";
  for (auto word : assert_line) {
    assert_line_string += " " + word;
  }

  if (IsLoggingEnabled()) {
    std::cout << assert_line_string << std::endl;
    std::cout << smv->shortcode() << " " << smv->min_price_increment() << " | "
              << smv->market_update_info_.bestbid_size_ << " " << smv->market_update_info_.bestbid_ordercount_ << " "
              << smv->market_update_info_.bestbid_int_price_ << " " << smv->market_update_info_.bestbid_price_ << " | "
              << smv->market_update_info_.bestask_price_ << " " << smv->market_update_info_.bestask_int_price_ << " "
              << smv->market_update_info_.bestask_ordercount_ << " " << smv->market_update_info_.bestask_size_
              << std::endl
              << std::endl;

    for (auto i = 0; i < 3; i++) {
      std::cout << smv->shortcode() << " " << smv->min_price_increment() << " | "
                << smv->market_update_info_.bidlevels_[smv->base_bid_index_ - i].limit_size_ << " "
                << smv->market_update_info_.bidlevels_[smv->base_bid_index_ - i].limit_ordercount_ << " "
                << smv->market_update_info_.bidlevels_[smv->base_bid_index_ - i].limit_int_price_ << " "
                << smv->market_update_info_.bidlevels_[smv->base_bid_index_ - i].limit_price_ << " | "
                << smv->market_update_info_.asklevels_[smv->base_ask_index_ - i].limit_price_ << " "
                << smv->market_update_info_.asklevels_[smv->base_ask_index_ - i].limit_int_price_ << " "
                << smv->market_update_info_.asklevels_[smv->base_ask_index_ - i].limit_ordercount_ << " "
                << smv->market_update_info_.asklevels_[smv->base_ask_index_ - i].limit_size_ << std::endl;
    }
    std::cout << std::endl;
  }

  std::string assert_type = assert_line[2];
  std::string value_type = assert_line[3];
  unsigned int current_column = 4;
  std::stringstream message;

  if (assert_type.compare("DOUBLE_ASSERT") == 0) {
    // Get the appropriate values

    HFSAT::TradeType_t buysell = assert_line[current_column++][0] == 'B' ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;

    // In book managers, we pass level as 1, while here ( accessing through index) we start with 0
    int level = atoi(assert_line[current_column++].c_str()) - 1;
    double expected_value = atof(assert_line[current_column++].c_str());
    double given_value = 0.0;

    if (value_type.compare("PRICE") == 0) {
      // Currently calling smv bid_price functions,
      // Here assumption is that smv is functioning correctly
      // We can directly use market_update_info_struct variables as well

      given_value = buysell == HFSAT::kTradeTypeBuy ? smv->bid_price(level) : smv->ask_price(level);
    }

    // Setup message and process assert
    message.clear();
    message << " | " << current_test_name << " | " << assert_line_string << " | " << given_value;

    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message.str(), expected_value, given_value, smv->min_price_increment() * 0.01);

  } else if (assert_type.compare("INT_ASSERT") == 0) {
    // Parse the values here

    HFSAT::TradeType_t buysell = assert_line[current_column++][0] == 'B' ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;
    // In book managers, we pass level as 1, while here ( accessing through index) we start with 0

    int level = atoi(assert_line[current_column++].c_str()) - 1;
    int expected_value = atoi(assert_line[current_column++].c_str());

    int given_value = 0;

    // get appropriate features
    if (value_type.compare("INT_PRICE") == 0) {
      // checks int price at given level
      given_value = buysell == HFSAT::kTradeTypeBuy ? smv->bid_int_price(level) : smv->ask_int_price(level);
    } else if (value_type.compare("SIZE") == 0) {
      // checks total size at given level
      given_value = buysell == HFSAT::kTradeTypeBuy ? smv->bid_size(level) : smv->ask_size(level);
    } else if (value_type.compare("ORDER_COUNT") == 0) {
      // Checks the number of orders at given level
      given_value = buysell == HFSAT::kTradeTypeBuy ? smv->bid_order(level) : smv->ask_order(level);
    } else {
      CPPUNIT_FAIL("Invalid String Provided " + value_type);
    }

    // Setup message and do assert
    message.clear();
    message << " | " << current_test_name << " | " << assert_line_string << " | " << given_value;

    CPPUNIT_ASSERT_EQUAL_MESSAGE(message.str(), expected_value, given_value);

  } else if (assert_type.compare("ASSERT") == 0) {
    // Checking general assert types
    if (value_type.compare("EMPTY_BOOK") == 0) {
      // Checks if the book on one side is empty

      // Get the side to check
      HFSAT::TradeType_t buysell =
          assert_line[current_column++][0] == 'B' ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;

      bool current_status = smv->IsBidBookEmpty();
      if (buysell == HFSAT::kTradeTypeSell) {
        current_status = smv->IsAskBookEmpty();
      }

      // If there's value given in the line then check
      // Otherwise default the book should be empty
      bool given_value = true;
      if (current_column < assert_line.size()) {
        given_value = atoi(assert_line[current_column++].c_str()) != 0;
      }

      message.clear();
      message << " | " << current_test_name << " | " << assert_line_string << " | " << current_status << " # "
              << given_value;

      CPPUNIT_ASSERT_MESSAGE(message.str(), current_status == given_value);

    } else if (value_type.compare("CROSSED_BOOK") == 0) {
      // Checks the price as well as int price for bid and ask
      // Ask_Price > Bid_Price

      double bid_price = smv->bid_price(0);
      double ask_price = smv->ask_price(0);

      // Setup Message and Assert
      message.clear();
      message << " | " << current_test_name << " | " << assert_line_string << " | BidPx " << bid_price << " AskPx "
              << ask_price;

      CPPUNIT_ASSERT_MESSAGE(message.str(), bid_price < ask_price);

      // Checking the same for int price
      // Though this condition should already be met as long as GetDoublePx works fine

      int bid_int_price = smv->bid_int_price(0);
      int ask_int_price = smv->ask_int_price(0);

      // Setup message and do assert
      message.clear();
      message << " | " << current_test_name << " | " << assert_line_string << " | BidIntPx " << bid_int_price
              << " AskIntPx " << ask_int_price;

      CPPUNIT_ASSERT_MESSAGE(message.str(), bid_int_price < ask_int_price);

    } else if (value_type.compare("CROSSED_BEST_BOOK") == 0) {
      // Checks the price as well as int price for best levels

      double bid_price = smv->bestbid_price();
      double ask_price = smv->bestask_price();

      // Setup Message and Assert
      message.clear();
      message << " | " << current_test_name << " | " << assert_line_string << " | BidPx " << bid_price << " AskPx "
              << ask_price;

      CPPUNIT_ASSERT_MESSAGE(message.str(), bid_price < ask_price);

      // Checking the same for int price
      // Though this condition should be met as long as GetDoublePx works fine

      int bid_int_price = smv->bestbid_int_price();
      int ask_int_price = smv->bestask_int_price();

      // Setup Message and Assert
      message.clear();
      message << " | " << current_test_name << " | " << assert_line_string << " | BidIntPx " << bid_int_price
              << " AskIntPx " << ask_int_price;

      CPPUNIT_ASSERT_MESSAGE(message.str(), bid_int_price < ask_int_price);

    } else if (value_type.compare("SIZE_ORDER_CHECK") == 0) {
      // Checks if bid_ordercount is <= bid_size

      HFSAT::TradeType_t buysell =
          assert_line[current_column++][0] == 'B' ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;

      int level = 0;
      if (assert_line.size() > current_column) {
        level = atoi(assert_line[current_column++].c_str());
      }

      // Get the size and order at the level
      int size = smv->bid_size(level);
      int ordercount = smv->bid_order(level);

      // Change the values in case of sell
      if (buysell == HFSAT::kTradeTypeSell) {
        size = smv->ask_size(level);
        ordercount = smv->ask_order(level);
      }

      // Setup the message and Assert
      message.clear();
      message << " | " << current_test_name << " | " << assert_line_string << " | Size " << size
              << " Order: " << ordercount;

      CPPUNIT_ASSERT_MESSAGE(message.str(), size >= ordercount);
    }
  }
}

/**
 * Constructs dummy book
 */
void ConstructDummyBook(HFSAT::SecurityMarketView* smv) {
  // Setup the book variables

  smv->market_update_info().bestbid_price_ = 131.09;
  smv->market_update_info().bestask_price_ = 131.10;
  smv->market_update_info().bestbid_int_price_ = smv->GetIntPx(smv->market_update_info().bestbid_price_);
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);
  smv->market_update_info().bestbid_size_ = 100;
  smv->market_update_info().bestask_size_ = 900;
  smv->market_update_info().bestbid_ordercount_ = 98;
  smv->market_update_info().bestask_ordercount_ = 10;

  /*
   *  For now setting any number, not need to go through book manager
   *  Could have set this through book-manager as well but that would have to
   *  be separate for separate exchanges
   *  */

  smv->base_bid_index_ = smv->market_update_info().bidlevels_.size() / 2;
  smv->base_ask_index_ = smv->market_update_info().asklevels_.size() / 2;
  auto bid_index = smv->base_bid_index_;
  auto ask_index = smv->base_ask_index_;

  if (IsLoggingEnabled()) {
    std::cout << " BidIndex: " << bid_index << " AskIndex: " << ask_index << std::endl;
  }

  auto mpi = smv->min_price_increment();
  for (auto count = 1u; count <= 5; count++) {
    smv->market_update_info().bidlevels_[bid_index].limit_price_ = 131.09 - (count - 1) * mpi;
    smv->market_update_info().asklevels_[ask_index].limit_price_ = 131.10 + (count - 1) * mpi;
    smv->market_update_info().bidlevels_[bid_index].limit_size_ = count * 100;
    smv->market_update_info().asklevels_[ask_index].limit_size_ = count * 900;

    smv->market_update_info().bidlevels_[bid_index].limit_ordercount_ = 100 - count * 2;
    smv->market_update_info().bidlevels_[ask_index].limit_ordercount_ = count * 10;

    smv->market_update_info().bidlevels_[bid_index].limit_int_price_ =
        smv->GetIntPx(smv->market_update_info().bidlevels_[bid_index].limit_price_);
    smv->market_update_info().asklevels_[ask_index].limit_int_price_ =
        smv->GetIntPx(smv->market_update_info().asklevels_[ask_index].limit_price_);

    bid_index--;
    ask_index--;
  }

  if (IsLoggingEnabled()) {
    // Print all the levels of book
    std::cout << "\nBook: " << smv->shortcode() << " Symbol: " << smv->secname() << std::endl;
    for (auto count = 0; count < 5; count++) {
      std::cout << "[ " << smv->bid_size(count) << " " << smv->bid_order(count) << " " << smv->bid_price(count) << " x "
                << smv->ask_price(count) << " " << smv->ask_order(count) << " " << smv->ask_size(count) << " ]"
                << std::endl;
    }
  }
  smv->is_ready_ = true;
}

void ClearDummyBook(HFSAT::SecurityMarketView* smv_) {
  // Reset the values in the vector

  std::fill(smv_->market_update_info().bidlevels_.begin(), smv_->market_update_info().bidlevels_.end(),
            HFSAT::MarketUpdateInfoLevelStruct(0, kInvalidIntPrice, kInvalidPrice, 0, 0, HFSAT::ttime_t(0, 0)));

  std::fill(smv_->market_update_info().asklevels_.begin(), smv_->market_update_info().asklevels_.end(),
            HFSAT::MarketUpdateInfoLevelStruct(0, kInvalidIntPrice, kInvalidPrice, 0, 0, HFSAT::ttime_t(0, 0)));
}
}
}
