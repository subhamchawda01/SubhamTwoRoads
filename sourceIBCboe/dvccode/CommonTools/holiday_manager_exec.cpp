#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "dvccode/Utils/holiday_manager.hpp"

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "USAGE : '<exec> EXCHANGE' or '<exec> PRODUCT'\n";
    exit(1);
  }

  try {
    if (!std::strcmp(argv[1], "EXCHANGE")) {
      if (argc < 5) {
        std::cerr << "USAGE : <exec> EXCHANGE <exchange-name> <date> <is-weekend-holiday>(T/F)\n";
        exit(1);
      }

      std::string exchange_name = argv[2];
      std::string date = argv[3];
      bool is_num = true;
      for (std::string::const_iterator iter = date.begin(); iter != date.end(); iter++) {
        if (!std::isdigit(*iter)) {
          is_num = false;
          break;
        }
      }
      if (!is_num) {
        std::cerr << "Date entered " << date << " is not a number. Exiting.\n";
        exit(1);
      }
      bool is_weekend_holiday = !std::strcmp(argv[4], "T");
      bool is_holiday = HFSAT::HolidayManager::GetUniqueInstance().IsExchangeHoliday(
          exchange_name, std::atoi(date.c_str()), is_weekend_holiday);
      if (is_holiday) {
        std::cout << "1\n";
      } else {
        std::cout << "2\n";
      }

    } else if (!std::strcmp(argv[1], "PRODUCT")) {
      if (argc < 5) {
        std::cerr << "USAGE : <exec> PRODUCT <product-name> <date> <is-weekend-holiday>(T/F)\n";
        exit(1);
      }
      std::string product_name = argv[2];
      std::string date = argv[3];
      bool is_num = true;
      for (std::string::const_iterator iter = date.begin(); iter != date.end(); iter++) {
        if (!std::isdigit(*iter)) {
          is_num = false;
          break;
        }
      }
      if (!is_num) {
        std::cerr << "Date entered " << date << " is not a number. Exiting.\n";
        exit(1);
      }
      bool is_weekend_holiday = !std::strcmp(argv[4], "T");
      bool is_holiday = HFSAT::HolidayManager::GetUniqueInstance().IsProductHoliday(
          product_name, std::atoi(date.c_str()), is_weekend_holiday);
      if (is_holiday) {
        std::cout << "1\n";
      } else {
        std::cout << "2\n";
      }

    } else {
      std::cerr << "USAGE : '<exec> EXCHANGE' or '<exec> PRODUCT'\n";
      exit(1);
    }

  } catch (...) {
    std::cout << "Error\n";
  }

  return 0;
}
