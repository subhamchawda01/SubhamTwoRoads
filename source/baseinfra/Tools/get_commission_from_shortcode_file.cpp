#include <iostream>
#include <stdlib.h>
#include <fstream>
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CDef/bse_security_definition.hpp"
#include "baseinfra/VolatileTradingInfo/base_commish.hpp"


bool nse_secdef_init = 0;
bool bse_secdef_init = 0;

double GetCommishPerContract(const std::string& _shortcode_, int _YYYYMMDD_, int location_path) {
  if (_shortcode_.find("NSE") == 0) {
	  return HFSAT::NSESecurityDefinitions::GetNSECommission(_shortcode_, location_path);
  } else if (_shortcode_.find("BSE") == 0) {
    	return HFSAT::BSESecurityDefinitions::GetBSECommission(_shortcode_);
  } else{
	    std::cout << "Invalid Exchange Provided " << std::endl;
	    exit(0);
  }
}


int main(int argc, char** argv) {
  if ( argc < 4) {
    std::cerr << "Usage : <exec> <date> <segment FO/CM> <input file with shortcodes> Location[0 (GRT), 1 (TwoRoads) ]" << std::endl;
    exit(0);
  }

  int input_date_ = atoi(argv[1]);
  std::string segment = argv[2];
  std::string input_file = argv[3];
  
  int location_path = 2; // 0 is GRT LIVE, 1 is Two Roads  // 2 backend With Stampduty
  if ( segment == "FO") location_path = 0;
  
  if ( argc > 4) {
	  location_path = atoi(argv[4]);
  }

  std::string line;
  std::ifstream infile;
  infile.open(input_file.c_str(), std::ifstream::in);
  
  // Decide if we need both or which sec def.
  while(std::getline(infile, line)) {
    std::string trimmed_str_;
    HFSAT::PerishableStringTokenizer::TrimString(line.c_str(), trimmed_str_, ' ');

    std::string short_code_ = trimmed_str_;

    if (short_code_.find("NSE") == 0 && nse_secdef_init == 0) {
      HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
      nse_secdef_init = 1;
    } else if (short_code_.find("BSE") == 0 && bse_secdef_init == 0) {
      HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadBSESecurityDefinitions();
      bse_secdef_init = 1;
    }
  }

  infile.close();
  
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  
  std::vector<std::string> my_shc_list_;
  HFSAT::SecurityDefinitions::GetDefinedShortcodesVec(my_shc_list_, input_date_);
  std::map<std::string, std::string> shc_to_symbol_map_;

  for (unsigned i = 0; i < my_shc_list_.size(); i++) {
    const char* this_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(my_shc_list_[i]);
    shc_to_symbol_map_[this_exchange_symbol_] = my_shc_list_[i];
  }
  
  infile.open(input_file.c_str(), std::ifstream::in);
  while(std::getline(infile, line)) {
    std::string trimmed_str_;
    HFSAT::PerishableStringTokenizer::TrimString(line.c_str(), trimmed_str_, ' ');

    std::string short_code_ = trimmed_str_;
    if(shc_to_symbol_map_.find(trimmed_str_) != shc_to_symbol_map_.end()){
      short_code_ = shc_to_symbol_map_[trimmed_str_];
    }

    std::cout << short_code_ << " "
            << GetCommishPerContract(short_code_, input_date_, location_path)
            << std::endl;
  }
  infile.close();
  
  return 0;
}

