#include "dvccode/Tests/ExchangeSymbolManager/exchange_symbol_manager_tests.hpp"
#include <fstream>

namespace HFTEST {

void ExchangeSymbolManagerTests::TestGetExchangeSymbol() {
  try {
    const std::string exchange_symbol_manager_tests(GetTestDataFullPath("exchange_symbol_manager_tests", "dvccode"));

    if (HFSAT::FileUtils::ExistsAndReadable(exchange_symbol_manager_tests)) {
      std::ifstream test_file(exchange_symbol_manager_tests, std::ifstream::in);

      if (test_file.is_open()) {
        std::string test_input;
        while (getline(test_file, test_input)) {
          if (test_input.size()) {
            if (test_input[0] == '#')
              continue;  // Comments begin with '#'
            else {
              std::string short_code, _date, expected_output, extended_expected_output;
              std::stringstream input(test_input);
              std::vector<std::string> input_tokens;
              std::string word;
              while (getline(input, word, ' ')) {
                input_tokens.push_back(word);
              }
              short_code = input_tokens[0];
              _date = input_tokens[1];
              expected_output = input_tokens[2];
              if (input_tokens.size() > 3) {
                extended_expected_output = input_tokens[3];
              } else
                extended_expected_output = "";
              HFSAT::ExchangeSymbolManager::SetUniqueInstance(std::atoi(_date.c_str()));
              std::string actual_output_str = HFSAT::ExchangeSymbolManager::GetExchSymbol(short_code);
              HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();

              std::string actual_output = "", actual_otput_extended = "";
              bool flag = false;
              for (char c : actual_output_str) {
                if (c == ' ') {
                  flag = true;
                }
                if (!flag) {
                  actual_output += c;
                } else {
                  if (c != ' ') {
                    actual_otput_extended += c;
                  }
                }
              }

              if (expected_output != actual_output || actual_otput_extended != extended_expected_output) {
                std::cout << " Test Failed : "
                          << " " << short_code << " " << _date << " " << expected_output << " "
                          << extended_expected_output << " " << actual_output << " " << actual_otput_extended << "\n";
                CPPUNIT_ASSERT(actual_output == expected_output && actual_otput_extended == extended_expected_output);
              }
            }
          }
        }
      }
    } else {
      std::cerr << "File " << exchange_symbol_manager_tests << " not exists or not readable. Exiting\n";
      CPPUNIT_ASSERT(0);
    }

  } catch (...) {
    std::cout << " In catch block\n";
    CPPUNIT_ASSERT(0);
  }

}  // end of TestGetExchangeSymbol

}  // end of namespace HFTEST
