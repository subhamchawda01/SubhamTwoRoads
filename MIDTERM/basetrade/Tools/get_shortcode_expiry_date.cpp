/**
   \file Tools/get_shortcode_expiry_date.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2017
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/

#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iomanip>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "USAGE: " + std::string(argv[0]) + " SHORTCODE START_DATE END_DATE [NUM_DAYS_PER_EXPIRY=1]"
              << std::endl;
    exit(0);
  }

  std::string shortcode = argv[1];
  int start_yyyymmdd = atoi(argv[2]);
  int end_yyyymmdd = atoi(argv[3]);
  int num_days = 1;
  if (argc > 4) {
    num_days = atoi(argv[4]);
  }

  if (end_yyyymmdd < start_yyyymmdd) {
    std::cerr << " End_date > Start_date " << std::endl;
    exit(0);
  }

  int current_yyyymmdd = end_yyyymmdd;
  std::string last_exchange_symbol = std::string("");

  while (current_yyyymmdd > start_yyyymmdd) {
    // Get todays' exchange sym manager
    HFSAT::ExchangeSymbolManager* exchange_sym_manager =
        HFSAT::ExchangeSymbolManager::GetSingleInstance(current_yyyymmdd);
    std::string exchange_symbol = exchange_sym_manager->GetExchSymbolSingleInstance(shortcode);

    if (!last_exchange_symbol.empty() && exchange_symbol != last_exchange_symbol) {
      std::cout << current_yyyymmdd << std::endl;
      if (num_days > 1) {
        // If we need more than 1 days then just print the dates continuously
        int num = 1;
        while (num < num_days) {
          current_yyyymmdd = HFSAT::DateTime::CalcPrevWeekDay(current_yyyymmdd);
          std::cout << current_yyyymmdd << std::endl;
          num++;
        }
      }
    }

    last_exchange_symbol = exchange_symbol;
    delete exchange_sym_manager;
    current_yyyymmdd = HFSAT::DateTime::CalcPrevWeekDay(current_yyyymmdd);
  }
}
