/**
    \file Tools/get_hist_stdev.cpp

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
#include "dvctrade/Indicators/historical_stdev_manager.hpp"

void ParseCommandLineParams(const int argc, const char **argv, std::string &shortcode_) {
  // expect :
  // 1. $0 shortcode
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " shortcode " << std::endl;
    exit(0);
  } else {
    shortcode_ = argv[1];
  }
}

/// input arguments : shortcode
int main(int argc, char **argv) {
  std::string shortcode_ = "";
  ParseCommandLineParams(argc, (const char **)argv, shortcode_);
  HFSAT::HistoricalStdevManager &historical_stdev_manager_ = HFSAT::HistoricalStdevManager::GetUniqueInstance();
  printf("%f", historical_stdev_manager_.GetStdev(shortcode_));
}
