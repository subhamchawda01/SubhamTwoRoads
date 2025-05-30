#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"

void ParseCommandLineParams(const int argc, const char **argv, std::string &underlying, int &input_date_,
                            int &expiry_) {
  // expect :
  // 1. $0 shortcode date_YYYYMMDD expiry
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0] << " UnderLying input_date_YYYYMMDD ExpiryDate" << std::endl;
    exit(0);
  } else {
    underlying = argv[1];
    input_date_ = atoi(argv[2]);
    expiry_ = atoi(argv[3]);
  }
}

/// input arguments : input_date
int main(int argc, char **argv) {
  std::string underlying = "";
  int input_date_ = 20110101;
  int expiry = 20230101;
  ParseCommandLineParams(argc, (const char **)argv, underlying, input_date_, expiry);
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadBSESecurityDefinitions();
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  std::vector<std::string> shortcode_vec;
  std::string shortcode_ = "";
  if (strncmp(shortcode_.c_str(), "BSE_", 4) == 0) {
    std::cout << underlying << " " << expiry << " "
              << HFSAT::BSESecurityDefinitions::GetNumberItmForUnderlying(underlying, expiry) << std::endl;
  } else {
    std::cout << underlying << " " << expiry << " "
              << HFSAT::NSESecurityDefinitions::GetNumberItmForUnderlying(underlying, expiry) << std::endl;
  }
  return 0;
}
