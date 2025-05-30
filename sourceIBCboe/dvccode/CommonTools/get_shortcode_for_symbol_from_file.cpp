#include <boost/program_options.hpp>
#include <iostream>
#include <stdlib.h>

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"

void ParseCommandLineParams(const int argc, const char **argv, std::string &ex_file, int &input_date_,
                            std::string &exchange_) {
  // expect :
  // 1. $0 ex date_YYYYMMDD
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " exchange_symbol_file input_date_YYYYMMDD --exchange=[NSE]/BSE" << std::endl;
    exit(0);
  } else {
    ex_file = argv[1];
    input_date_ = atoi(argv[2]);
    boost::program_options::options_description desc("Allowed Options");
    desc.add_options()("help", "produce help message.")(
        "exchange", boost::program_options::value<std::string>()->default_value("NSE"));

    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
    std::string exchange_flag = vm["exchange"].as<std::string>();
    exchange_ = exchange_flag;
  }
}

int main(int argc, char **argv) {
  std::string exchange_symbol_file = "";
  int input_date_ = 20110101;
  std::string exchange_ = "NSE";
  ParseCommandLineParams(argc, (const char **)argv, exchange_symbol_file, input_date_, exchange_);

  if ("NSE" == exchange_) HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
  if("BSE" == exchange_)  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadBSESecurityDefinitions(); 

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  std::ifstream FileShortCode(exchange_symbol_file);
  std::vector<std::string> shortcode_vec;

  std::string shortcode_ = "";
  std::vector<std::string> my_shc_list_;
  HFSAT::SecurityDefinitions::GetDefinedShortcodesVec(my_shc_list_, input_date_);
  std::map<std::string, int> shc_to_symbol_map_;

  if (FileShortCode.is_open()) {
    while (FileShortCode.good()) {
      getline(FileShortCode, shortcode_);
      std::replace(shortcode_.begin(), shortcode_.end(), '~', ' ');
      shc_to_symbol_map_[shortcode_] = 1;
    }

    for (unsigned i = 0; i < my_shc_list_.size(); i++) {
      std::string this_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(my_shc_list_[i]);
 
      if (shc_to_symbol_map_.find(this_exchange_symbol_) != shc_to_symbol_map_.end()) {
        std::replace(my_shc_list_[i].begin(), my_shc_list_[i].end(), '&', '~');
        std::cout << this_exchange_symbol_ << " " << my_shc_list_[i] << std::endl;
      }
    }
  } else {
    std::cout << "Unable to read file" << std::endl;
  }

  return 0;
}
