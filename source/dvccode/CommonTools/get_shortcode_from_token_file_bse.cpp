#include <iostream>
#include <stdlib.h>
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Utils/bse_daily_token_symbol_handler.hpp"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage : <exec> <file> <date> \n";
    exit(0);
  }
  std::string token_file = argv[1];
  int input_date_ = atoi(argv[2]);
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadBSESecurityDefinitions();
  HFSAT::Utils::BSEDailyTokenSymbolHandler &bse_daily_token_symbol_handler_ = HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(input_date_);
  std::ifstream FileToken(token_file);
  std::string line;
  int64_t token_;

  if (FileToken.is_open()) {
    while (std::getline(FileToken, line)) {
      std::istringstream iss(line);
        
      if (!(iss >> token_)) {
         std::cerr << "Error reading data from file." << std::endl;
         return 1;
      } 
      std::string datasoucename = bse_daily_token_symbol_handler_.GetInternalSymbolFromToken(token_, 'S');
      std::string exchsymbol = HFSAT::BSESecurityDefinitions::ConvertDataSourceNametoExchSymbol(datasoucename);
      std::string shortcode = HFSAT::BSESecurityDefinitions::GetShortCodeFromExchangeSymbol(exchsymbol);
      std::cout << token_ << " " << datasoucename << " " << exchsymbol << " " << shortcode << std::endl;
    }
  }
  else{
    std::cout<<"Unable to read file"<<std::endl;
  }

  return 0;
}

