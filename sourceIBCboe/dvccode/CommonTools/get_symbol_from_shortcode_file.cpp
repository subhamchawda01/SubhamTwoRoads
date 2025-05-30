#include <iostream>
#include <stdlib.h>

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"

void split(const std::string& s, char delim, std::vector<std::string>& elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
}

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
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadBSESecurityDefinitions();
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
  std::string shortcode_ = "";
  std::vector<std::string> my_shc_list_;
  HFSAT::SecurityDefinitions::GetDefinedShortcodesVec(my_shc_list_, input_date_);
  std::map<std::string, int> shc_to_symbol_map_;

  if (FileShortCode.is_open()) {
    while (FileShortCode.good()) {
      getline(FileShortCode, shortcode_);
      std::replace(shortcode_.begin(), shortcode_.end(), '~', ' ');
      if (strncmp(shortcode_.c_str(), "BSE_", 4) == 0) {
        if (HFSAT::BSESecurityDefinitions::IsShortcode(shortcode_)) {
          std::vector<std::string> tokens1_;
          split(shortcode_, '_', tokens1_);
          if (HFSAT::BSESecurityDefinitions::IsEquity(shortcode_)) {
            std::cout << shortcode_ << " " << shortcode_ << std::endl;
          } else if (HFSAT::BSESecurityDefinitions::IsFuture(shortcode_)) {
            std::cout << shortcode_ << " BSE_" << tokens1_[1] << "_FUT_"
                      << HFSAT::BSESecurityDefinitions::GetExpiryFromShortCode(shortcode_) << std::endl;
          } else if (HFSAT::BSESecurityDefinitions::IsSpotIndex(shortcode_)) {
            std::cout << shortcode_ << " " << shortcode_ << std::endl;
          } else {
            std::cout << shortcode_ << " BSE_" << tokens1_[1] << "_" << tokens1_[2][0] << "E_" << std::fixed
                      << std::setprecision(2) << HFSAT::BSESecurityDefinitions::GetStrikePriceFromShortCode(shortcode_)
                      << "_" << HFSAT::BSESecurityDefinitions::GetExpiryFromShortCode(shortcode_) << std::endl;
          }
        } else {
          std::cout << "Not Valid ShortCode: " << shortcode_ << std::endl;
        }
      } else {
        if (HFSAT::NSESecurityDefinitions::IsShortcode(shortcode_)) {
          std::vector<std::string> tokens1_;
          split(shortcode_, '_', tokens1_);
          if (HFSAT::NSESecurityDefinitions::IsEquity(shortcode_)) {
            std::cout << shortcode_ << " " << shortcode_ << std::endl;
          } else if (HFSAT::NSESecurityDefinitions::IsFuture(shortcode_)) {
            std::cout << shortcode_ << " NSE_" << tokens1_[1] << "_FUT_"
                      << HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(shortcode_) << std::endl;
          } else if (HFSAT::NSESecurityDefinitions::IsSpotIndex(shortcode_)) {
            std::cout << shortcode_ << " " << shortcode_ << std::endl;
          } else {
            std::cout << shortcode_ << " NSE_" << tokens1_[1] << "_" << tokens1_[2][0] << "E_" << std::fixed
                      << std::setprecision(2) << HFSAT::NSESecurityDefinitions::GetStrikePriceFromShortCode(shortcode_)
                      << "_" << HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(shortcode_) << std::endl;
          }
        } else {
          std::cout << "Not Valid ShortCode: " << shortcode_ << std::endl;
        }
      }
    }
  } else {
    std::cout << "Unable to read file" << std::endl;
  }

  return 0;
}
