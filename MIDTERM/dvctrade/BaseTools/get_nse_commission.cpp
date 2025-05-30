#include <iostream>
#include <stdlib.h>
#include <fstream>
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "baseinfra/VolatileTradingInfo/base_commish.hpp"

int main(int argc, char** argv) {
  if ( argc < 5) {
    std::cerr << "Usage : <exec> <date> <segment FO/CM> <input file with shortcodes> <output file>" << std::endl;
    exit(0);
  }
  
  int input_date_ = atoi(argv[1]);
  std::string segment = argv[2];
  std::string input_file = argv[3];
  std::string output_file = argv[4];
  std::string line;
  std::ifstream infile;
  infile.open(input_file.c_str(), std::ifstream::in);
  std::ofstream outfile;
  outfile.open(output_file.c_str(), std::ofstream::out);
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  std::vector<std::string> my_shc_list_;
  HFSAT::SecurityDefinitions::GetDefinedShortcodesVec(my_shc_list_, input_date_);
  std::map<std::string, std::string> shc_to_symbol_map_;

  for (unsigned i = 0; i < my_shc_list_.size(); i++) {
    const char* this_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(my_shc_list_[i]);
    shc_to_symbol_map_[this_exchange_symbol_] = my_shc_list_[i];
  }
  outfile << std::fixed << std::setprecision(10);
  while(std::getline(infile, line)) {
     std::string trimmed_str_;
     HFSAT::PerishableStringTokenizer::TrimString(line.c_str(), trimmed_str_, ' '); 

     std::string short_code_ = trimmed_str_;
     //if ( segment == std::string("FO")) {
     if(shc_to_symbol_map_.find(trimmed_str_) != shc_to_symbol_map_.end()){ 
         short_code_ = shc_to_symbol_map_[trimmed_str_];
     }
     //outfile 

     outfile << trimmed_str_ << " " << short_code_ << " " 
             << HFSAT::BaseCommish::GetCommishPerContract(short_code_, input_date_) << std::endl;
  }
  infile.close();
  outfile.close();
  return 0;
}
