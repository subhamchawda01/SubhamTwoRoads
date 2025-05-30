#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"

void ParseCommandLineParams(const int argc, const char **argv, std::string &shortcode_file, int &input_date_) {
  // expect :
  // 1. $0 shortcode date_YYYYMMDD
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " shortcode_file input_date_YYYYMMDD" << std::endl;
    exit(0);
  } else {
    shortcode_file = argv[1];
    input_date_ = atoi(argv[2]);
  }
}

/// input arguments : input_date
int main(int argc, char **argv) {
  std::string shortcode_file = "";
  int input_date_ = 20110101;
  ParseCommandLineParams(argc, (const char **)argv, shortcode_file, input_date_);

  bool load_nse_sec_def = false, load_bse_sec_def = false, load_cboe_sec_def = false;

  std::ifstream FileShortCode(shortcode_file);
  std::vector<std::string> shortcode_vec;
  std::string shortcode_ = "";
  if (FileShortCode.is_open()) {
    while (FileShortCode.good()) {
      getline(FileShortCode, shortcode_);
      shortcode_vec.push_back(shortcode_);
    }

    for (unsigned int i = 0; i < shortcode_vec.size(); i++) {
      if (strncmp(shortcode_vec[i].c_str(), "NSE_", 4) == 0)
        load_nse_sec_def = true;
      else if (strncmp(shortcode_vec[i].c_str(), "BSE_", 4) == 0)
        load_bse_sec_def = true;
      else if (strncmp(shortcode_vec[i].c_str(), "CBOE_", 4) == 0)
        load_cboe_sec_def = true;
    }

    if (load_nse_sec_def) HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
    if (load_bse_sec_def) HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadBSESecurityDefinitions();
    if (load_cboe_sec_def) HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadCBOESecurityDefinitions();
    HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);

    for (unsigned int i = 0; i < shortcode_vec.size(); i++) {
      bool Kexist = HFSAT::ExchangeSymbolManager::CheckIfContractSpecExists(shortcode_vec[i]);
      if (Kexist == true) {
        std::cout << shortcode_vec[i] << " " << HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_vec[i])
                  << std::endl;
      }
      // else ignore as kGetContractSpecificationMissingCode
    }
  } else {
    std::cout << "Unable to read file" << std::endl;
  }
  return 0;
}
