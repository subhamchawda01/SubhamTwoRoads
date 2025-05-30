/**
    \file Tools/get_min_price_increment.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717

*/

#include <iostream>
#include <stdlib.h>
#include "dvccode/CDef/security_definitions.hpp"

void ParseCommandLineParams(const int argc, const char **argv, std::string &shortcode_, int &yyyymmdd_,
                            std::string &_time_str_) {
  // expect :
  // 1. $0 shortcode
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " shortcode tradingdate [time=UTC_0000]" << std::endl;
    exit(0);
  } else {
    shortcode_ = argv[1];
    yyyymmdd_ = atoi(argv[2]);
    if (argc > 3) {
      _time_str_ = argv[3];
    }
  }
}

/// input arguments : shortcode
int main(int argc, char **argv) {
  std::string shortcode_ = "";
  int yyyymmdd_ = 20120101;
  std::string time_ = "";

  ParseCommandLineParams(argc, (const char **)argv, shortcode_, yyyymmdd_, time_);
  if (strncmp(shortcode_.c_str(), "NSE_", 4) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(yyyymmdd_).LoadNSESecurityDefinitions();
  }
  if (time_ != "") {
    HFSAT::SecurityDefinitions::ResetASXMpiIfNeeded(yyyymmdd_, time_);
  }
  printf("%f", HFSAT::SecurityDefinitions::GetContractMinPriceIncrement(shortcode_, yyyymmdd_));
}
