#include <boost/program_options.hpp> 
#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"

void ParseCommandLineParams(const int argc, const char **argv, std::string &ex_file, int &input_date_, std::string &exchange_) {
  // expect :
  // 1. $0 ex date_YYYYMMDD
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " ex_file input_date_YYYYMMDD --exchange=[NSE]/BSE" << std::endl;
    exit(0);
  } else {
    ex_file = argv[1];
    input_date_ = atoi(argv[2]);
    boost::program_options::options_description desc("Allowed Options");
    desc.add_options()("help", "produce help message.")("exchange", boost::program_options::value<std::string>()->default_value("NSE"));

    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
    std::string exchange_flag = vm["exchange"].as<std::string>();
    exchange_ = exchange_flag; 
  }
}

/// input arguments : input_date
int main(int argc, char **argv) {
  std::string ex_file = "";
  int input_date_ = 20110101;
  std::string exchange_ = "NSE";
  ParseCommandLineParams(argc, (const char **)argv, ex_file, input_date_, exchange_);
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).SetExchangeType(exchange_);
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadSecurityDefinitions();
  std::ifstream FileShortCode(ex_file);
  std::map<std::string,int> ex_vec, ex_lot;
  std::string ex_ = "";
  std::vector<std::string> my_shc_list_;
  HFSAT::SecurityDefinitions::GetDefinedShortcodesVec(my_shc_list_, input_date_);
  char line_[1024];
  while (FileShortCode.good()) {
    memset(line_, 0, sizeof(line_));
    FileShortCode.getline(line_, sizeof(line_));
    std::vector<char *> tokens_;
    HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(line_, " ", tokens_);
    if (tokens_.size() < 2) {
      continue;
    }
    ex_ = tokens_[0];
    int lot_size = atoi(tokens_[1]);
    ex_vec[ex_] = 1;
    ex_lot[ex_] = lot_size;
  }
  for (unsigned int i = 0; i < my_shc_list_.size(); i++) {
    std::string this_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(my_shc_list_[i]);
     if (ex_vec.find(this_exchange_symbol_) != ex_vec.end() && ex_lot[this_exchange_symbol_] != -1) {
		std::replace(my_shc_list_[i].begin(), my_shc_list_[i].end(), '&', '~');
      //		  # AGGRE_NSE2241045 = 0,5.350000
                std::cout << "AGGRE_" << this_exchange_symbol_ << " = " << ex_lot[this_exchange_symbol_] <<"," ;
    
    if(HFSAT::SecurityDefinitions::IsOption(my_shc_list_[i])){
      std::cout << HFSAT::SecurityDefinitions::GetLastCloseForOptions(my_shc_list_[i]) << std::endl;
    }else if(HFSAT::SecurityDefinitions::IsFuture(my_shc_list_[i])){
      std::cout << HFSAT::SecurityDefinitions::GetLastClose(my_shc_list_[i]) << std::endl;
    }
		ex_lot[this_exchange_symbol_] = -1;
    }
    // else ignore as kGetContractSpecificationMissingCode
  }
return 0;
}
