#include "dvccode/Tests/SecurityDefinitions/security_definitions_tests.hpp"
#include <fstream>

namespace HFTEST {
std::stringstream assert_message;
void SecurityDefinitionsTests::TestGetContractSpecifications() {
  // Accessing the input test file.
  const std::string contract_specifications_tests(
      GetTestDataFullPath("security_definitions_contract_specs_tests", "dvccode"));

  if (HFSAT::FileUtils::ExistsAndReadable(contract_specifications_tests)) {
    std::ifstream test_file(contract_specifications_tests, std::ifstream::in);

    if (test_file.is_open()) {
      std::string test_input;
      while (getline(test_file, test_input)) {
        if (test_input.size()) {
          if (test_input[0] == '#')
            continue;  // Comments begin with '#', we skip them!
          else {
            std::string short_code_, input_date_, expected__exch_source_, expected_min_order_size_;
            std::stringstream input(test_input);
            std::vector<std::string> input_tokens;
            std::string word;

            while (getline(input, word, ' ')) {
              input_tokens.push_back(word);
            }

            if (int(input_tokens.size()) < 6) {
              // The test case is erred.
              continue;
            }

            // Fetch the test input from the test file.
            short_code_ = input_tokens[0];
            input_date_ = input_tokens[1];
            double expected_min_price_increment_ = std::atof(input_tokens[2].c_str());
            double expected_numbers_to_dollars_ = std::atof(input_tokens[3].c_str());
            expected__exch_source_ = input_tokens[4];
            expected_min_order_size_ = input_tokens[5];

            // Create an instance of the SecurityDefinition Class and initialize y=the contract specifications hash
            // map for all exchange short_codes
            HFSAT::SecurityDefinitions::GetUniqueInstance(std::atoi(input_date_.c_str()));

            // Contract Specifications
            double actual_min_price_increment_;
            double actual_numbers_to_dollars_;
            HFSAT::ExchSource_t actual_exch_source_;
            int actual_min_order_size_;

            actual_min_price_increment_ =
                HFSAT::SecurityDefinitions::GetContractMinPriceIncrement(short_code_, std::atoi(input_date_.c_str()));
            actual_numbers_to_dollars_ =
                HFSAT::SecurityDefinitions::GetContractNumbersToDollars(short_code_, std::atoi(input_date_.c_str()));
            actual_exch_source_ =
                HFSAT::SecurityDefinitions::GetContractExchSource(short_code_, std::atoi(input_date_.c_str()));
            actual_min_order_size_ =
                HFSAT::SecurityDefinitions::GetContractMinOrderSize(short_code_, std::atoi(input_date_.c_str()));

            assert_message << "Test case failed , actual_min_price_increment does not match to expected " << short_code_
                           << " " << input_date_ << " Expected : " << expected_min_price_increment_
                           << " Actual: " << actual_min_price_increment_ << "\n";

            CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(assert_message.str(), expected_min_price_increment_,
                                                 actual_min_price_increment_, 0.1);

            assert_message.clear();
            assert_message.str("");
            assert_message << "Test case failed , actual_numbers_to_dollars does not match to expected" << short_code_
                           << " " << input_date_ << " "
                           << "Expected : " << expected_numbers_to_dollars_ << " "
                           << "Actual: " << actual_numbers_to_dollars_ << std::endl;

            CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(assert_message.str(), expected_numbers_to_dollars_,
                                                 actual_numbers_to_dollars_, 0.1);

            assert_message.clear();
            assert_message.str("");
            assert_message << "Test case failed , actual_exch_source_ does not match to expected" << short_code_ << " "
                           << input_date_ << " "
                           << "Expected : " << expected__exch_source_ << " "
                           << "Actual: " << actual_exch_source_ << std::endl;

            CPPUNIT_ASSERT_MESSAGE(assert_message.str(),
                                   atoi(expected__exch_source_.c_str()) == int(actual_exch_source_));

            assert_message.clear();
            assert_message.str("");
            assert_message << "Test case failed , actual_min_order_size does not match to expected" << short_code_
                           << " " << input_date_ << " "
                           << "Expected : " << expected_min_order_size_ << " "
                           << "Actual: " << actual_min_order_size_ << std::endl;

            CPPUNIT_ASSERT_MESSAGE(assert_message.str(),
                                   atoi(expected_min_order_size_.c_str()) == actual_min_order_size_);
            assert_message.clear();
            assert_message.str("");

            HFSAT::SecurityDefinitions::RemoveUniqueInstance();
          }
        }
      }
    }
  } else {
    assert_message << "File contract_specifications_tests not exists or not readable. Exiting\n";
    CPPUNIT_ASSERT_MESSAGE(assert_message.str(), 0);
    assert_message.clear();
    assert_message.str("");
  }

}  // end of TestGetContractSpecifications

void SecurityDefinitionsTests::TestGetASXBondPrice() {
  double test_price_ = 69.50;  // Random double test value
  int test_asx_term_ = 10;     // Value is constant at 10
  double expected_asx_bond_price_ = 24371.6;

  double actual_asx_bond_price_ =
      HFSAT::SecurityDefinitions::GetUniqueInstance(20170612).GetASXBondPrice(test_price_, test_asx_term_);

  assert_message << "Test failed : "
                 << "Test_price : " << test_price_ << " Test ASX term : " << test_asx_term_
                 << " Expected ASX bond price : " << expected_asx_bond_price_
                 << " Actual ASX bond price : " << actual_asx_bond_price_ << std::endl;

  CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(assert_message.str(), expected_asx_bond_price_, actual_asx_bond_price_, 0.1);
  assert_message.clear();
  assert_message.str("");

}  // end of TestGetASXBondPrice

void SecurityDefinitionsTests::TestGetDIReserves() {
  // Accessing the input test file.
  const std::string get_DI_reserves_tests(GetTestDataFullPath("security_definitions_get_DI_reserves_tests", "dvccode"));

  if (HFSAT::FileUtils::ExistsAndReadable(get_DI_reserves_tests)) {
    std::ifstream test_file(get_DI_reserves_tests, std::ifstream::in);

    if (test_file.is_open()) {
      std::string test_input;
      while (getline(test_file, test_input)) {
        if (test_input.size()) {
          if (test_input[0] == '#')
            continue;  // Comments begin with '#', we skip them!
          else {
            std::string short_code_, input_date_, expected_DI_reserve_value_;
            std::stringstream input(test_input);
            std::vector<std::string> input_tokens;
            std::string word;

            while (getline(input, word, ' ')) {
              input_tokens.push_back(word);
            }
            if (input_tokens.size() < 3) {
              // test case is erred;
              continue;
            }

            // Fetch the test input from the test file.
            short_code_ = input_tokens[0];
            input_date_ = input_tokens[1];
            expected_DI_reserve_value_ = input_tokens[2];

            // Create an instance of the SecurityDefinition Class and initialize y=the contract specifications hash
            // map for all exchange short_codes
            HFSAT::SecurityDefinitions::GetUniqueInstance(std::atoi(input_date_.c_str()));

            int actual_DI_reserve_value_ =
                HFSAT::SecurityDefinitions::GetDIReserves(atoi(input_date_.c_str()), short_code_);

            assert_message << "Test case Failed : " << short_code_ << " "
                           << " " << input_date_ << " "
                           << "Expected : " << expected_DI_reserve_value_ << " Actual : " << actual_DI_reserve_value_
                           << std::endl;
            CPPUNIT_ASSERT_MESSAGE(assert_message.str(),
                                   atoi(expected_DI_reserve_value_.c_str()) == actual_DI_reserve_value_);
            assert_message.clear();
            assert_message.str("");

            HFSAT::SecurityDefinitions::RemoveUniqueInstance();
          }
        }
      }
    }
  } else {
    assert_message << "File get_DI_reserves_tests not exists or not readable. Exiting\n";
    CPPUNIT_ASSERT_MESSAGE(assert_message.str(), 0);
    assert_message.clear();
    assert_message.str("");
  }

}  // end of TestGetDIReserves

void SecurityDefinitionsTests::TestGetExpiryPriceFactor() {
  // Accessing the input test file.
  const std::string contract_specifications_tests(
      GetTestDataFullPath("security_definitions_get_expity_price_factor_tests", "dvccode"));

  if (HFSAT::FileUtils::ExistsAndReadable(contract_specifications_tests)) {
    std::ifstream test_file(contract_specifications_tests, std::ifstream::in);

    if (test_file.is_open()) {
      std::string test_input;
      while (getline(test_file, test_input)) {
        if (test_input.size()) {
          if (test_input[0] == '#')
            continue;  // Comments begin with '#', we skip them!
          else {
            std::string short_code_, input_date_;
            std::stringstream input(test_input);
            std::vector<std::string> input_tokens;
            std::string word;

            while (getline(input, word, ' ')) {
              input_tokens.push_back(word);
            }
            if (input_tokens.size() < 3) {
              // Tests case is erred
              continue;
            }

            // Fetch the test input from the test file.
            short_code_ = input_tokens[0];
            input_date_ = input_tokens[1];
            double expected_expiry_price_factor_ = atof(input_tokens[2].c_str());

            // Create an instance of the SecurityDefinition Class and initialize the contract specifications hash
            // map for all exchange short_codes
            double actual_expiry_price_factor_ =
                HFSAT::SecurityDefinitions::GetExpiryPriceFactor(short_code_, atoi(input_date_.c_str()));

            assert_message << "Test case Failed : " << short_code_ << " " << input_date_ << " "
                           << "Expected expiry price factor : " << expected_expiry_price_factor_
                           << " Actual expiry price factor : " << actual_expiry_price_factor_ << std::endl;

            CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(assert_message.str(), expected_expiry_price_factor_,
                                                 actual_expiry_price_factor_, 0.01);
            assert_message.clear();
            assert_message.str("");

            HFSAT::SecurityDefinitions::RemoveUniqueInstance();
          }
        }
      }
    }
  } else {
    assert_message << "File security_definitions_get_expity_price_factor_tests not exists or not readable. Exiting\n";
    CPPUNIT_ASSERT_MESSAGE(assert_message.str(), 0);
    assert_message.clear();
  }

}  // end of TestGetExpiryPriceFactor

void SecurityDefinitionsTests::TestNonSelfEnabledEntities() {
  std::ifstream t_non_self_securities_file_;
  t_non_self_securities_file_.open("/spare/local/tradeinfo/non_self_securities.txt", std::ifstream::in);

  if (t_non_self_securities_file_.is_open()) {
    std::string test_input;
    while (getline(t_non_self_securities_file_, test_input)) {
      if (test_input.size()) {
        if (test_input[0] == '#')
          continue;  // Comments begin with '#', we skip them!
        else {
          std::string short_code_;
          std::stringstream input(test_input);
          std::vector<std::string> input_tokens;
          std::string word;

          while (getline(input, word, ' ')) {
            input_tokens.push_back(word);
          }

          // Create an instance of the SecurityDefinition Class and initialize y=the contract specifications hash
          // map for all exchange short_codes
          HFSAT::SecurityDefinitions::GetUniqueInstance(20170612);

          std::vector<std::string> actual_non_self_enabled_securities_ =
              HFSAT::SecurityDefinitions::GetNonSelfEnabledSecurities(20170612);

          bool flag = true;
          if (input_tokens.size() != actual_non_self_enabled_securities_.size()) {
            flag = false;
          } else {
            std::sort(input_tokens.begin(), input_tokens.end());
            std::sort(actual_non_self_enabled_securities_.begin(), actual_non_self_enabled_securities_.end());
            for (int itr = 0; itr < int(input_tokens.size()); itr++) {
              if (input_tokens[itr] != actual_non_self_enabled_securities_[itr]) {
                flag = false;
                break;
              }
            }
          }
          assert_message << "Test case failed : non_self_securities_ vector of class Security Definitions not "
                            "initialized properly\n"
                         << std::endl;
          CPPUNIT_ASSERT_MESSAGE(assert_message.str(), flag == true);
          assert_message.clear();
          assert_message.str("");

          HFSAT::SecurityDefinitions::RemoveUniqueInstance();
        }
      }
    }
  } else {
    assert_message << "File non_self_securities_tests not exists or not readable. Exiting\n";
    CPPUNIT_ASSERT_MESSAGE(assert_message.str(), 0);
    assert_message.clear();
    assert_message.str("");
  }

}  // end of TestNonSelfEnabledEntities

}  // end of namespace HFTEST
