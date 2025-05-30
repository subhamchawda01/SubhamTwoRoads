/**
    \file CommonTools/get_segment_from_shortcode.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717

*/

#include <iostream>
#include <stdlib.h>
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"

void ParseCommandLineParams(const int argc, const char **argv, std::string &shortcode_, int &input_date_) {
  // expect :
  // 1. $0 shortcode date_YYYYMMDD
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " shortcode input_date_YYYYMMDD" << std::endl;
    exit(0);
  } else {
    shortcode_ = argv[1];
    input_date_ = atoi(argv[2]);
  }
}

/// input arguments : input_date
int main(int argc, char **argv) {
  std::string shortcode_ = "";
  int input_date_ = 20110101;
  ParseCommandLineParams(argc, (const char **)argv, shortcode_, input_date_);
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  if (strncmp(shortcode_.c_str(), "NSE_", 4) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
    HFSAT::NSESecurityDefinitions &nse_sec_def = HFSAT::NSESecurityDefinitions::GetUniqueInstance(input_date_);
    char segment = nse_sec_def.GetSegmentTypeFromShortCode(shortcode_); 
    std::cout << "SEGMENT:  " << segment << std::endl;
  } else if (shortcode_.substr(0, 3) == "HK_") {
    HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadHKStocksSecurityDefinitions();
  } else if (strncmp(shortcode_.c_str(), "BSE_", 4) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadBSESecurityDefinitions();
    HFSAT::BSESecurityDefinitions &bse_sec_def = HFSAT::BSESecurityDefinitions::GetUniqueInstance(input_date_);
    char segment = bse_sec_def.GetSegmentTypeFromShortCode(shortcode_);
    std::cout << "SEGMENT:  " << segment << std::endl;
  }
}
