#include <iostream>
#include <stdlib.h>
#include <fstream>
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CDef/bse_security_definition.hpp"
#include "baseinfra/VolatileTradingInfo/base_commish.hpp"

double GetCommishPerContract(const std::string& _shortcode_, int _YYYYMMDD_, int location_path) {
  if (_shortcode_.find("BSE") == 0) {
	return HFSAT::BSESecurityDefinitions::GetBSECommission(_shortcode_);
  } else {
	std::cout << "Invalid Exchange Provided " << std::endl;
	exit(0);
  }
}


int main(int argc, char** argv) {
  if ( argc < 5) {
    std::cerr << "Usage : <exec> <date> <segment FO/CM> <input file with shortcodes> <output file> Location[0 (GRT), 1 (TwoRoads) ]" << std::endl;
    exit(0);
  }

  int input_date_ = atoi(argv[1]);
  std::string segment = argv[2];
  std::string input_file = argv[3];
  std::string output_file = argv[4];
  int location_path = 2; // 0 is GRT LIVE, 1 is Two Roads  // 2 backend With Stampduty
  if ( argc > 5) {
	location_path = atoi(argv[5]);
  }
  std::string line;
  std::ifstream infile;
  infile.open(input_file.c_str(), std::ifstream::in);
  std::ofstream outfile;
  outfile.open(output_file.c_str(), std::ofstream::out);
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadBSESecurityDefinitions();
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
             << GetCommishPerContract(short_code_, input_date_, location_path) << std::endl;
  }
  infile.close();
  outfile.close();
  return 0;
}

