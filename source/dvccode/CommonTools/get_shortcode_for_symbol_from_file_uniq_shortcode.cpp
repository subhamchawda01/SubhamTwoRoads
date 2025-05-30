#include <iostream>
#include <stdlib.h>

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage : <exec> <file> <date> \n";
    exit(0);
  }
  std::string exchange_symbol_file = argv[1];
  int input_date_ = atoi(argv[2]);
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  std::ifstream FileShortCode(exchange_symbol_file);
  std::vector<std::string> shortcode_vec;
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
  std::string shortcode_="";
  std::vector<std::string> my_shc_list_;
  HFSAT::SecurityDefinitions::GetDefinedShortcodesVec(my_shc_list_, input_date_);
  std::map<std::string, int> shc_to_symbol_map_;

  if (FileShortCode.is_open()) {

	  while ( FileShortCode.good() ){
	  	getline(FileShortCode,shortcode_);
                std::replace(shortcode_.begin(), shortcode_.end(), '~', ' ');
		shc_to_symbol_map_[shortcode_] = 1;
	  }

          for (unsigned i = 0; i < my_shc_list_.size(); i++) {
            std::string this_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(my_shc_list_[i]);

//            std::string str(buffer, buffer + length);
            if (shc_to_symbol_map_.find(this_exchange_symbol_) != shc_to_symbol_map_.end() && shc_to_symbol_map_[this_exchange_symbol_] == 1) {
                std::replace(my_shc_list_[i].begin(), my_shc_list_[i].end(), '&', '~');
		shc_to_symbol_map_[this_exchange_symbol_] = 0;
                std::cout << this_exchange_symbol_ << " " << my_shc_list_[i] << std::endl;
            }
         }
  }
  else{
	  std::cout<<"Unable to read file"<<std::endl;
  }

  return 0;
}
