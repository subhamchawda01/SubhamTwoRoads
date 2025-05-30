/**
    \file Tools/get_token_from_shortcode.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717

*/

#include <iostream>
#include <stdlib.h>
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Utils/nse_refdata_loader.hpp"
#include "dvccode/Utils/bse_refdata_loader.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/Utils/bse_daily_token_symbol_handler.hpp"
#include "dvccode/Utils/nse_daily_token_symbol_handler.hpp"

void ParseCommandLineParams(const int argc, const char **argv, std::string &shortcode_, int &input_date_) {
  // expect :
  // 1. $0 shortcode date_YYYYMMDD
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " shortcode input_date_YYYYMMDD" << std::endl;
    exit(0);
  } else {
    shortcode_ = argv[1];
    input_date_ = atoi(argv[2]);
  }
}

/// input arguments : input_date
int main(int argc, char **argv) {
  std::string shortcode_ = "";
  int input_date_ = 20110101;
  ParseCommandLineParams(argc, (const char **)argv, shortcode_, input_date_);
  if (strncmp(shortcode_.c_str(), "NSE_", 4) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
  } else if (strncmp(shortcode_.c_str(), "BSE_", 4) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadBSESecurityDefinitions();
  } else if (shortcode_.substr(0, 3) == "HK_") {
    HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadHKStocksSecurityDefinitions();
  }
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);

  std::string exchange_symbol = HFSAT::ExchangeSymbolManager::GetUniqueInstance().GetExchSymbol(shortcode_);
  ;
  std::string internal_symbol = HFSAT::NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(exchange_symbol);
  if (strncmp(shortcode_.c_str(), "BSE_", 4) == 0) {
    internal_symbol = HFSAT::BSESecurityDefinitions::ConvertExchSymboltoDataSourceName(exchange_symbol);
  }
  if (internal_symbol == std::string("INVALID")) {
    std::cerr << "OnLiveProductsChange Error: Invalid Internal Symbol for " << exchange_symbol << std::endl;
    return 0;
  }

  HFSAT::Utils::NSEDailyTokenSymbolHandler &nse_daily_token_symbol_handler_ =
      HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(input_date_);
  HFSAT::Utils::BSEDailyTokenSymbolHandler &bse_daily_token_symbol_handler_ =
      HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(input_date_);
  char segment = HFSAT::NSESecurityDefinitions::GetSegmentTypeFromShortCode(shortcode_);
  int32_t token = nse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(internal_symbol.c_str(), segment);
  if (strncmp(shortcode_.c_str(), "BSE_", 4) == 0) {
    segment = HFSAT::BSESecurityDefinitions::GetSegmentTypeFromShortCode(shortcode_);
    token = bse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(internal_symbol.c_str(), segment);
  }

  if (token != -1 /*Dummy Token*/) {
    std::cout << token << std::endl;
  } else {
    std::cerr << " Invalid Token for shortcode " << shortcode_ << std::endl;
  }
  return 0;
}
