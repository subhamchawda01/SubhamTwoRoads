#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Utils/nse_daily_token_symbol_handler.hpp"
#include "dvccode/Utils/bse_daily_token_symbol_handler.hpp"

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
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadBSESecurityDefinitions();
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  HFSAT::Utils::BSERefDataLoader::GetUniqueInstance(input_date_);
  HFSAT::Utils::NSERefDataLoader::GetUniqueInstance(input_date_);
  HFSAT::Utils::NSEDailyTokenSymbolHandler &bse_daily_token_symbol_handler_ =
      HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(input_date_);
  HFSAT::Utils::BSEDailyTokenSymbolHandler &nse_daily_token_symbol_handler_ =
      HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(input_date_);
  std::ifstream FileShortCode(shortcode_file);
  std::vector<std::string> shortcode_vec;
  std::string shortcode_ = "";
  if (FileShortCode.is_open()) {
    while (FileShortCode.good()) {
      getline(FileShortCode, shortcode_);
      if (shortcode_ == "") continue;
      shortcode_vec.push_back(shortcode_);
    }
    for (unsigned int i = 0; i < shortcode_vec.size(); i++) {
      bool Kexist = HFSAT::ExchangeSymbolManager::CheckIfContractSpecExists(shortcode_vec[i]);
      if (Kexist == true) {
        std::string exchange_symbol = HFSAT::ExchangeSymbolManager::GetUniqueInstance().GetExchSymbol(shortcode_vec[i]);
        int32_t token = -1;
        if (strncmp(shortcode_vec[i].c_str(), "NSE_", 4) == 0) {
          std::string internal_symbol =
              HFSAT::NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(exchange_symbol);
          char segment = HFSAT::NSESecurityDefinitions::GetSegmentTypeFromShortCode(shortcode_vec[i]);
          token = nse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(internal_symbol.c_str(), segment);
        } else if (strncmp(shortcode_vec[i].c_str(), "BSE_", 4) == 0) {
          std::string internal_symbol =
              HFSAT::BSESecurityDefinitions::ConvertExchSymboltoDataSourceName(exchange_symbol);
          char segment = HFSAT::BSESecurityDefinitions::GetSegmentTypeFromShortCode(shortcode_vec[i]);
          token = bse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(internal_symbol.c_str(), segment);
        }
        if (token != -1 /*Dummy Token*/) {
          std::cout << shortcode_vec[i] << " Valid" << std::endl;
        } else {
          std::cout << shortcode_vec[i] << " Invalid" << std::endl;
        }
      } else {
        std::cout << shortcode_vec[i] << " Invalid" << std::endl;
      }
    }
  } else {
    std::cout << "Unable to read file" << std::endl;
  }
  return 0;
}
