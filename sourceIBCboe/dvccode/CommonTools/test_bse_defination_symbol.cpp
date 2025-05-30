/**
    \file Tools/get_exchange_symbol.cpp

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
#include "dvccode/Utils/bse_daily_token_symbol_handler.hpp"

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
void getDetails(std::string shortcode_, int input_date_) {
  std::cout << "--------------------------- " << shortcode_ << " ------------------------------- " << std::endl;
  HFSAT::Utils::BSERefDataLoader &ref_data_loader = HFSAT::Utils::BSERefDataLoader::GetUniqueInstance(input_date_);
  char segment = HFSAT::BSESecurityDefinitions::GetSegmentTypeFromShortCode(shortcode_);
  HFSAT::Utils::BSEDailyTokenSymbolHandler &bse_daily_token_symbol_handler_ =
      HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(input_date_);
  std::string exchange_symbol = HFSAT::ExchangeSymbolManager::GetUniqueInstance().GetExchSymbol(shortcode_);
  std::string internal_symbol = HFSAT::BSESecurityDefinitions::ConvertExchSymboltoDataSourceName(exchange_symbol);
  int32_t token = bse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(internal_symbol.c_str(), segment);
  HFSAT::ExchSource_t exch = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, input_date_);
  double minprice = HFSAT::SecurityDefinitions::GetContractMinPriceIncrement(shortcode_, input_date_);
  int order_size = HFSAT::SecurityDefinitions::GetContractMinOrderSize(shortcode_, input_date_);
  std::cout << "Token " << token << " Exchanage " << HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_)
            << std::endl;
  std::cout << "Datasymbol " << internal_symbol << " ExchangeS: " << HFSAT::BSESecurityDefinitions::GetExchSymbolBSE(shortcode_) << std::endl;
  std::cout << "LastClose " << HFSAT::BSESecurityDefinitions::GetLastClose(shortcode_) << " GetOptionType: " << HFSAT::BSESecurityDefinitions::GetOptionType(shortcode_) << std::endl;
  std::cout << "IsFut" << HFSAT::BSESecurityDefinitions::IsFuture(shortcode_) << " IsOpt: "<< HFSAT::BSESecurityDefinitions::IsOption(shortcode_) << std::endl;
  std::cout << "Expiry " << HFSAT::BSESecurityDefinitions::GetExpiryFromShortCode(shortcode_) << std::endl;
  std::cout << "RefDataLeg1: " << ref_data_loader.GetBSERefData(segment)[token].ToString() << "\n";
  std::cout << "DATACONTRACT : " << ExchSourceStringForm(exch) << " Priceinceremen : " << minprice << " Minsize "
            << order_size << "\n";
//  GetOpenInterestFromExchangeSymbol() << " COMM: "<< GetBSECommission();
  std::cout << "---------------------------------------------------------- " << std::endl;
}

/// input arguments : input_date
int main(int argc, char **argv) {
  std::string shortcode_ = "";
  int input_date_ = 20110101;
  ParseCommandLineParams(argc, (const char **)argv, shortcode_, input_date_);
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadBSESecurityDefinitions();
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  shortcode_ = "BSX";
  getDetails("BSE_" + shortcode_ + "_FUT0", input_date_);
  getDetails("BSE_" + shortcode_ + "_FUT1", input_date_);
  getDetails("BSE_" + shortcode_ + "_C0_A", input_date_);
  getDetails("BSE_" + shortcode_ + "_C0_A_W", input_date_);
  getDetails("BSE_" + shortcode_ + "_C1_A_W", input_date_);
  getDetails("BSE_" + shortcode_ + "_P0_A", input_date_);
  getDetails("BSE_" + shortcode_ + "_P0_A_W", input_date_);
  getDetails("BSE_" + shortcode_ + "_P1_A_W", input_date_);
  shortcode_ = "BKX";
  getDetails("BSE_" + shortcode_ + "_FUT0", input_date_);
  getDetails("BSE_" + shortcode_ + "_FUT1", input_date_);
  getDetails("BSE_" + shortcode_ + "_C0_A", input_date_);
  getDetails("BSE_" + shortcode_ + "_C0_A_W", input_date_);
  getDetails("BSE_" + shortcode_ + "_C1_A_W", input_date_);
  getDetails("BSE_" + shortcode_ + "_P0_A", input_date_);
  getDetails("BSE_" + shortcode_ + "_P0_A_W", input_date_);
  getDetails("BSE_" + shortcode_ + "_P1_A_W", input_date_);
}
