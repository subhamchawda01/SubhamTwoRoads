/**
    \file Tools/get_di_unit_price.cpp

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
#include "dvctrade/FuturesUtils/DI1Utils_V1.hpp"

void ParseCommandLineParams(const int argc, const char **argv, std::string &shortcode_, int &date_, double &bprice_,
                            double &sprice_, int &qty_) {
  if (argc < 5) {
    std::cerr << "Usage: " << argv[0] << " exch_code_ date b_price s_price [ qty = 1 ] " << std::endl;
    exit(0);
  } else if (argc == 5) {
    shortcode_ = argv[1];
    date_ = atoi(argv[2]);
    bprice_ = atof(argv[3]);
    sprice_ = atof(argv[4]);
  } else if (argc == 6) {
    shortcode_ = argv[1];
    date_ = atoi(argv[2]);
    bprice_ = atof(argv[3]);
    sprice_ = atof(argv[4]);
    qty_ = atoi(argv[5]);
  }
}

inline double GetDIContractNumbersToDollars(std::string shortcode_, int date_, double price_) {
  double unit_price_ = 0;
  int di_reserves_ = HFSAT::SecurityDefinitions::GetDIReserves(date_, shortcode_);
  double term_ = double(di_reserves_ / 252.0);
  if (term_ > 0.000) {
    unit_price_ = 100000 / std::pow((price_ / 100 + 1), term_);
  }
  return (unit_price_ * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyBRL, HFSAT::kCurrencyUSD) / price_);
}

int main(int argc, char **argv) {
  std::string shortcode_ = "";
  int date_ = 0;
  double bprice_ = -1;
  double sprice_ = -1;
  int qty_ = 1;

  ParseCommandLineParams(argc, (const char **)argv, shortcode_, date_, bprice_, sprice_, qty_);

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(date_);

  double bvalue_ = -qty_ * bprice_ * GetDIContractNumbersToDollars(shortcode_, date_, bprice_);
  double svalue_ = -qty_ * sprice_ * GetDIContractNumbersToDollars(shortcode_, date_, sprice_);

  printf("%f\n", svalue_ - bvalue_);
}
