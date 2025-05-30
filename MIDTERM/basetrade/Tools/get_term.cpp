/**
    \file Tools/get_term.cpp
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

#include <iostream>
#include <stdlib.h>
#include "baseinfra/BaseUtils/curve_utils.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"

void ParseCommandLineParams(const int argc, const char **argv, std::string &shortcode_, int &yyyymmdd_) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " shc_code_ date " << std::endl;
    exit(0);
  } else {
    shortcode_ = argv[1];
    yyyymmdd_ = atoi(argv[2]);
  }
}

int main(int argc, char **argv) {
  std::string shortcode_ = "";
  int yyyymmdd_ = -1;

  ParseCommandLineParams(argc, (const char **)argv, shortcode_, yyyymmdd_);
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(yyyymmdd_);

  int term = 0;
  if (yyyymmdd_ > 0) {
    term = HFSAT::CurveUtils::_get_term_(yyyymmdd_, HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_));
  }
  printf("%d\n", term);
}
