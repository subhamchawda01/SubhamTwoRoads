/**
    \file Tools/get_numbers_to_dollars.cpp

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

void ParseCommandLineParams(const int argc, const char **argv, std::string &shortcode_, int &r_tradingdate_) {
  // expect :
  // 1. $0 shortcode tradingdate
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " shortcode tradingdate" << std::endl;
    exit(0);
  } else {
    shortcode_ = argv[1];
    r_tradingdate_ = atoi(argv[2]);
  }
}

/// input arguments : shortcode
int main(int argc, char **argv) {
  std::string shortcode_ = "";
  int tradingdate_ = 20130101;
  ParseCommandLineParams(argc, (const char **)argv, shortcode_, tradingdate_);
  if (strncmp(shortcode_.c_str(), "NSE_", 4) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();
  }
  printf("%f", HFSAT::SecurityDefinitions::GetContractNumbersToDollars(shortcode_, tradingdate_));
}
