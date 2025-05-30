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
  std::string exchange_symbol_file = argv[1];
  std::ifstream FileShortCode(exchange_symbol_file);
  int input_date_ = atoi(argv[2]);
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);

  std::string exch_ = "";
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadBSESecurityDefinitions();
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
  if (FileShortCode.is_open()) {
    while (FileShortCode.good()) {
      getline(FileShortCode, exch_);
      if (exch_ != "") {
        std::replace(exch_.begin(), exch_.end(), '~', ' ');
        if (strncmp(exch_.c_str(), "NSE", 3) == 0) {
          std::cout << exch_ << " " << HFSAT::NSESecurityDefinitions::GetWeeklyShortCodeFromSymbol(exch_) << std::endl;
        } else {
          std::cout << exch_ << " " << HFSAT::BSESecurityDefinitions::GetWeeklyShortCodeFromSymbol(exch_) << std::endl;
        }
      }
    }
  } else {
    std::cout << "Unable to read file" << std::endl;
  }
  return 0;
}
