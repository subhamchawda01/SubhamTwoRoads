#include <iostream>
#include <stdlib.h>

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "Usage : <exec> <shortcode> <date>\n";
    exit(0);
  }
  std::string shortcode_ = argv[1];
  int input_date_ = atoi(argv[2]);
  if (strncmp(shortcode_.c_str(), "NSE_", 4) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
  }
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  HFSAT::ExchSource_t exch_src_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, input_date_);
  std::cout << HFSAT::ExchSourceStringForm(exch_src_) << std::endl;
  return 0;
}
