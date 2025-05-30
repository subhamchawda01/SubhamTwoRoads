#pragma once

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/Utils/holiday_manager.hpp"

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/currency_convertor.hpp"

#include "dvccode/Tests/TestUtils/cpptest_utils.hpp"
#include "dvccode/Tests/TestUtils/run_test.hpp"

namespace HFTEST {

/*
 * The Test class in which we write the test cases.
 */
class HolidayManagerTests : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(HolidayManagerTests);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition below.
  CPPUNIT_TEST(TestExchangeHolidays);
  CPPUNIT_TEST(TestExchangeStartDate);
  CPPUNIT_TEST(TestProductHolidays);
  CPPUNIT_TEST(TestProductStartDate);

  CPPUNIT_TEST_SUITE_END();

 public:
  void tearDown(void) { CurrencyConvertor::RemoveInstance(); }
  void setUp(void) {}

 protected:
  // Test Cases

  void TestExchangeHolidays(void) {
    try {
      const std::string exchange_holiday_tests(GetTestDataFullPath("exchange_holiday_tests", "dvccode"));

      if (HFSAT::FileUtils::ExistsAndReadable(exchange_holiday_tests)) {
        std::ifstream infile(exchange_holiday_tests, std::ifstream::in);

        if (infile.is_open()) {
          char line_buffer_[1024];

          while (infile.good()) {
            memset(line_buffer_, 0, 1024);
            infile.getline(line_buffer_, 1024);

            HFSAT::PerishableStringTokenizer string_tokenizer(line_buffer_, 1024, true);
            const std::vector<const char *> &tokens = string_tokenizer.GetTokens();
            if (!tokens.empty()) {
              std::string exchange(tokens[0]);
              std::string input_date(tokens[1]);
              std::string is_weekend_holiday(tokens[2]);
              std::string expected_out(tokens[3]);

              int actual_out = HFSAT::HolidayManager::GetUniqueInstance().IsExchangeHoliday(
                  exchange, std::atoi(input_date.c_str()), std::atoi(is_weekend_holiday.c_str()));
              if (std::atoi(expected_out.c_str()) != actual_out) {
                std::cout << "Test Failed : " << exchange << " " << std::atoi(input_date.c_str()) << " "
                          << std::atoi(is_weekend_holiday.c_str()) << " " << std::atoi(expected_out.c_str()) << " "
                          << actual_out << "\n";
                CPPUNIT_ASSERT(actual_out == std::atoi(expected_out.c_str()));
              }
            }
          }
        }
      } else {
        std::cerr << "File " << exchange_holiday_tests << " not exists or not readable. Exiting\n";
        CPPUNIT_ASSERT(0);
      }
    } catch (...) {
      CPPUNIT_ASSERT(0);
    }
  }

  void TestExchangeStartDate(void) {
    try {
      const std::string exchange_start_date_tests(GetTestDataFullPath("exchange_start_date_tests", "dvccode"));

      if (HFSAT::FileUtils::ExistsAndReadable(exchange_start_date_tests)) {
        std::ifstream infile(exchange_start_date_tests, std::ifstream::in);

        if (infile.is_open()) {
          char line_buffer_[1024];

          while (infile.good()) {
            memset(line_buffer_, 0, 1024);
            infile.getline(line_buffer_, 1024);

            HFSAT::PerishableStringTokenizer string_tokenizer(line_buffer_, 1024, true);
            const std::vector<const char *> &tokens = string_tokenizer.GetTokens();
            if (!tokens.empty()) {
              std::string exchange(tokens[0]);
              std::string input_date(tokens[1]);
              std::string is_weekend_holiday(tokens[2]);
              std::string expected_out(tokens[3]);

              int actual_out = HFSAT::HolidayManager::GetUniqueInstance().IsExchangeHoliday(
                  exchange, std::atoi(input_date.c_str()), std::atoi(is_weekend_holiday.c_str()));
              if (std::atoi(expected_out.c_str()) != actual_out) {
                std::cout << "Test Failed : " << exchange << " " << std::atoi(input_date.c_str()) << " "
                          << std::atoi(is_weekend_holiday.c_str()) << " " << std::atoi(expected_out.c_str()) << " "
                          << actual_out << "\n";
                CPPUNIT_ASSERT(actual_out == std::atoi(expected_out.c_str()));
              }
            }
          }
        }
      } else {
        std::cerr << "File " << exchange_start_date_tests << " not exists or not readable. Exiting\n";
        CPPUNIT_ASSERT(0);
      }
    } catch (...) {
      CPPUNIT_ASSERT(0);
    }
  }

  void TestProductHolidays(void) {
    try {
      const std::string tests(GetTestDataFullPath("product_holiday_tests", "dvccode"));

      if (HFSAT::FileUtils::ExistsAndReadable(tests)) {
        std::ifstream infile(tests, std::ifstream::in);

        if (infile.is_open()) {
          char line_buffer_[1024];

          while (infile.good()) {
            memset(line_buffer_, 0, 1024);
            infile.getline(line_buffer_, 1024);

            HFSAT::PerishableStringTokenizer string_tokenizer(line_buffer_, 1024, true);
            const std::vector<const char *> &tokens = string_tokenizer.GetTokens();
            if (!tokens.empty()) {
              std::string product(tokens[0]);
              std::string input_date(tokens[1]);
              std::string is_weekend_holiday(tokens[2]);
              std::string expected_out(tokens[3]);

              int actual_out = HFSAT::HolidayManager::GetUniqueInstance().IsProductHoliday(
                  product, std::atoi(input_date.c_str()), std::atoi(is_weekend_holiday.c_str()));
              if (std::atoi(expected_out.c_str()) != actual_out) {
                std::cout << "Test Failed : " << product << " " << std::atoi(input_date.c_str()) << " "
                          << std::atoi(is_weekend_holiday.c_str()) << " " << std::atoi(expected_out.c_str()) << " "
                          << actual_out << "\n";
                CPPUNIT_ASSERT(actual_out == std::atoi(expected_out.c_str()));
              }
            }
          }
        }
      } else {
        std::cerr << "File " << tests << " not exists or not readable. Exiting\n";
        CPPUNIT_ASSERT(0);
      }
    } catch (...) {
      CPPUNIT_ASSERT(0);
    }
  }

  void TestProductStartDate(void) {
    try {
      const std::string tests(GetTestDataFullPath("product_start_date_tests", "dvccode"));

      if (HFSAT::FileUtils::ExistsAndReadable(tests)) {
        std::ifstream infile(tests, std::ifstream::in);

        if (infile.is_open()) {
          char line_buffer_[1024];

          while (infile.good()) {
            memset(line_buffer_, 0, 1024);
            infile.getline(line_buffer_, 1024);

            HFSAT::PerishableStringTokenizer string_tokenizer(line_buffer_, 1024, true);
            const std::vector<const char *> &tokens = string_tokenizer.GetTokens();
            if (!tokens.empty()) {
              std::string product(tokens[0]);
              std::string input_date(tokens[1]);
              std::string is_weekend_holiday(tokens[2]);
              std::string expected_out(tokens[3]);

              int actual_out = HFSAT::HolidayManager::GetUniqueInstance().IsProductHoliday(
                  product, std::atoi(input_date.c_str()), std::atoi(is_weekend_holiday.c_str()));
              if (std::atoi(expected_out.c_str()) != actual_out) {
                std::cout << "Test Failed : " << product << " " << std::atoi(input_date.c_str()) << " "
                          << std::atoi(is_weekend_holiday.c_str()) << " " << std::atoi(expected_out.c_str()) << " "
                          << actual_out << "\n";
                CPPUNIT_ASSERT(actual_out == std::atoi(expected_out.c_str()));
              }
            }
          }
        }
      } else {
        std::cerr << "File " << tests << " not exists or not readable. Exiting\n";
        CPPUNIT_ASSERT(0);
      }
    } catch (...) {
      CPPUNIT_ASSERT(0);
    }
  }

 private:
};
}
