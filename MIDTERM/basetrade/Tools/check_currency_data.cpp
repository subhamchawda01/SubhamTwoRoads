/**
    \file Tools/check_currency_data.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717

*/

#include <iostream>
#include <stdlib.h>
#include "dvccode/CDef/currency_convertor.hpp"
#include "dvccode/CDef/security_definitions.hpp"

int main(int argc, char** argv) {
  int date_ = atoi(argv[1]);
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(date_);

  int i = 0;
  while (i < HFSAT::kCurrencyMAX) {
    HFSAT::Currency_t e_val_ = static_cast<HFSAT::Currency_t>(i);
    std::cout << HFSAT::CurrencyConvertor::GetStringFromEnum(e_val_) << " "
              << HFSAT::CurrencyConvertor::Convert(e_val_, HFSAT::kCurrencyUSD) << "\n";
    i++;
  }
}
