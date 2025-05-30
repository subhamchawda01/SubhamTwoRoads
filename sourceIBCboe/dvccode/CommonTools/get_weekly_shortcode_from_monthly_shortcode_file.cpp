// =====================================================================================
//
//       Filename:  GetWeekly_Shortcode_fromMonthly_Shortcode.cpp
//
//    Description:  Get Weekly Shortcode from monlthy
//
//        Version:  1.0
//        Created:  10/04/2022 04:49:29 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================
#include <iostream>
#include <stdlib.h>

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage : <exec> <symbol> <date> \n";
    exit(0);
  }
  std::string monthly_shortcode_file = argv[1];
  std::ifstream FileShortCode(monthly_shortcode_file);
  int input_date_ = atoi(argv[2]);
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  std::string shortcode = "";
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadBSESecurityDefinitions();
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();

  if (FileShortCode.is_open()) {
    while (FileShortCode.good()) {
      getline(FileShortCode, shortcode);
      if (shortcode != "") {
        if (strncmp(shortcode.c_str(), "NSE_", 4) == 0) {
          std::cout << shortcode << " "
                    << HFSAT::NSESecurityDefinitions::GetWeeklyShortCodeFromMonthlyShortcode(shortcode) << std::endl;
        } else {
          std::cout << shortcode << " "
                    << HFSAT::BSESecurityDefinitions::GetWeeklyShortCodeFromMonthlyShortcode(shortcode) << std::endl;
        }
      }
    }
  } else {
    std::cout << "Unable to read file" << std::endl;
  }

  return 0;
}
