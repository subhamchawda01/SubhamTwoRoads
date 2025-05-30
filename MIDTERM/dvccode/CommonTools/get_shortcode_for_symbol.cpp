#include <iostream>
#include <stdlib.h>

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage : <exec> <symbol> <date> \n";
    exit(0);
  }
  std::string exchange_symbol_queried_ = argv[1];
  std::replace(exchange_symbol_queried_.begin(), exchange_symbol_queried_.end(), '~', ' ');

  int input_date_ = atoi(argv[2]);
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);

  if (strncmp(exchange_symbol_queried_.c_str(), "NSE", 3) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
  }
  std::vector<std::string> my_shc_list_;
  HFSAT::SecurityDefinitions::GetDefinedShortcodesVec(my_shc_list_, input_date_);
  std::map<std::string, std::string> shc_to_symbol_map_;
  for (unsigned i = 0; i < my_shc_list_.size(); i++) {
    const char* this_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(my_shc_list_[i]);
    if (strcmp(exchange_symbol_queried_.c_str(), this_exchange_symbol_) == 0) {
      std::replace(my_shc_list_[i].begin(), my_shc_list_[i].end(), '&', '~');
      std::cout << my_shc_list_[i] << std::endl;
      break;
    }
  }
  return 0;
}
