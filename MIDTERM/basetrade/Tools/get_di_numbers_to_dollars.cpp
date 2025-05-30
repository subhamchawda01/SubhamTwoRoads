/**
    \file Tools/get_di_numbers_to_dollars.cpp

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

void ParseCommandLineParams(const int argc, const char **argv, std::string &shortcode_, int &r_tradingdate_,
                            double &price_) {
  // expect :
  // 1. $0 shortcode tradingdate
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0] << " shortcode tradingdate price" << std::endl;
    exit(0);
  } else {
    shortcode_ = argv[1];
    r_tradingdate_ = atoi(argv[2]);
    price_ = atof(argv[3]);
  }
}

/// input arguments : shortcode
int main(int argc, char **argv) {
  std::string shortcode_ = "";
  int tradingdate_ = 20130101;
  double price_ = 1.00;
  ParseCommandLineParams(argc, (const char **)argv, shortcode_, tradingdate_, price_);

  double unit_price_ = 0;
  int di_reserves_ = HFSAT::SecurityDefinitions::GetDIReserves(tradingdate_, shortcode_);
  double term_ = double(di_reserves_ / 252.0);

  if (term_ > 0.000) {
    unit_price_ = 100000 / std::pow((price_ / 100 + 1), term_);
  }

  printf("%f\n", unit_price_ * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyBRL, HFSAT::kCurrencyUSD) / price_);
}
