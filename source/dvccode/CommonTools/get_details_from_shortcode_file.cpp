#include <boost/program_options.hpp>
#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"

void ParseCommandLineParams(const int argc, const char **argv, std::string &shortcode_file, int &input_date_, std::string &exchange_) {
  // expect :
  // 1. $0 shortcode date_YYYYMMDD
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " shortcode_file input_date_YYYYMMDD --exchange=[NSE]/BSE" << std::endl;
    exit(0);
  } else {
    shortcode_file = argv[1];
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
  std::string shortcode_file = "";
  int input_date_ = 20110101;
  std::string exchange_ = "NSE";
  ParseCommandLineParams(argc, (const char **)argv, shortcode_file, input_date_, exchange_);
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).SetExchangeType(exchange_);
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadSecurityDefinitions();
  std::ifstream FileShortCode(shortcode_file);
  std::vector<std::string> shortcode_vec;
  std::string shortcode_ = "";
  if (FileShortCode.is_open()) {
    while (FileShortCode.good()) {
      getline(FileShortCode, shortcode_);
      if (shortcode_ == "") break;
      shortcode_vec.push_back(shortcode_);
    }
    for (unsigned int i = 0; i < shortcode_vec.size(); i++) {
      bool Kexist = HFSAT::ExchangeSymbolManager::CheckIfContractSpecExists(shortcode_vec[i]);
      if (Kexist == true) {
        if (strncmp(shortcode_vec[i].c_str(), "NSE_", 4) == 0) {
          int k = HFSAT::NSESecurityDefinitions::GetOptionType(shortcode_vec[i]);
          std::string optionType = "FUT";
          if (k == 1)
            optionType = "CE";
          else if (k == -1)
            optionType = "PE";
          std::cout << shortcode_vec[i] << " " << HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_vec[i]) << " "
                    << HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(shortcode_vec[i]) << " "
                    << HFSAT::NSESecurityDefinitions::GetStrikePriceFromShortCode(shortcode_vec[i]) << " " << optionType
                    << std::endl;

        } else if (strncmp(shortcode_vec[i].c_str(), "BSE_", 4) == 0) {
          int k = HFSAT::BSESecurityDefinitions::GetOptionType(shortcode_vec[i]);
          std::string optionType = "FUT";
          if (k == 1)
            optionType = "CE";
          else if (k == -1)
            optionType = "PE";
          std::cout << shortcode_vec[i] << " " << HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_vec[i]) << " "
                    << HFSAT::BSESecurityDefinitions::GetExpiryFromShortCode(shortcode_vec[i]) << " "
                    << HFSAT::BSESecurityDefinitions::GetStrikePriceFromShortCode(shortcode_vec[i]) << " " << optionType
                    << std::endl;
        }
      }
    }
  } else {
    std::cout << "Unable to read file" << std::endl;
  }
  return 0;
}
