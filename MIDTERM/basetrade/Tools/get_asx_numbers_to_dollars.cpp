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
std::string shortcode_ = "";

double get_dollars(double quote_) {
  double j7_ = 3;
  double yield_ = 100.0 - quote_;
  double j8_ = yield_ / 200;
  double j9_ = 1 / (1 + j8_);
  double j10_ = 1000 * (j7_ * (1 - pow(j9_, 6)) / j8_ + 100 * pow(j9_, 6));
  std::cout << "j7: " << j7_ << " j8: " << j8_ << " j9: " << j9_ << " qt: " << quote_ << " j10: " << j10_ << std::endl;
  return j10_;
}

inline double GetASXBondPrice(double price_) {
  double term_ = 3;

  if (shortcode_ == "XT_0") {
    term_ = 10;
  }

  double yield_ = 100.0 - price_;
  double j8_ = yield_ / 200;
  double j9_ = 1 / (1 + j8_);
  double j10_ = 1000 * (3 * (1 - pow(j9_, (term_ * 2))) / j8_ + 100 * pow(j9_, (term_ * 2)));
  return j10_;
}

/// input arguments : shortcode
int main(int argc, char **argv) {
  int tradingdate_ = 20130101;
  double price_ = 1.00;
  ParseCommandLineParams(argc, (const char **)argv, shortcode_, tradingdate_, price_);

  double min_px_increment_ = HFSAT::SecurityDefinitions::GetContractMinPriceIncrement(shortcode_, tradingdate_);

  printf("%f\n", GetASXBondPrice(price_) - GetASXBondPrice(price_ - min_px_increment_));
}
