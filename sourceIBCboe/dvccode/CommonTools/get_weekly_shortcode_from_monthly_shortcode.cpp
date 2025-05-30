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
  std::string monthly_shortcode = argv[1];
  std::replace(monthly_shortcode.begin(), monthly_shortcode.end(), '~', ' ');

  int input_date_ = atoi(argv[2]);
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadBSESecurityDefinitions();
  if (strncmp(monthly_shortcode.c_str(), "NSE_", 4) == 0) { 
  std::cout << monthly_shortcode << " "
            << HFSAT::NSESecurityDefinitions::GetWeeklyShortCodeFromMonthlyShortcode(monthly_shortcode) << std::endl;
  } else if (strncmp(monthly_shortcode.c_str(), "BSE_", 4) == 0) {
	std::cout << monthly_shortcode << " "
            << HFSAT::BSESecurityDefinitions::GetWeeklyShortCodeFromMonthlyShortcode(monthly_shortcode) << std::endl;
  }
  return 0;
}
