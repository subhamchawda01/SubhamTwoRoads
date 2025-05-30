#include <iostream>
#include <stdlib.h>

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "baseinfra/VolatileTradingInfo/base_commish.hpp"

int main(int argc, char** argv) {
  if (argc <= 3) {
    std::cerr << "Usage : <exec> <shortcode/ALL> <date> "
                 "<details[EXCHANGE/N2D/TICKSIZE/LOTSIZE/COMMISH/SEGMENT/ALL/ALL_IN_ONELINE]> [load_nse_file=1]\n";
    exit(0);
  }
  int details_ = 0;
  if (argc >= 4) {
    if (!strncmp(argv[3], "EXCHANGE", 8)) {
      details_ = 1;
    } else if (!strncmp(argv[3], "N2D", 3)) {
      details_ = 2;
    } else if (!strncmp(argv[3], "TICKSIZE", 8)) {
      details_ = 3;
    } else if (!strncmp(argv[3], "LOTSIZE", 7)) {
      details_ = 4;
    } else if (!strncmp(argv[3], "COMMISH", 7)) {
      details_ = 5;
    } else if (!strcmp(argv[3], "ALL_IN_ONELINE")) {
      details_ = 6;
    } else if (!strcmp(argv[3], "LAST_CLOSE_PRICE")) {
      details_ = 7;
    } else if (!strcmp(argv[3], "CONTRACT_MULTIPLIER")) {
      details_ = 8;
    } else if (!strcmp(argv[3], "LAST_CLOSE_PRICE_OPTIONS")) {
      details_ = 9;
    } else if (!strcmp(argv[3], "SHC_EXCH")) {
      details_ = 10;
    } else if (!strcmp(argv[3], "SEGMENT")) {
      details_ = 11;
    }
  }
  int load_nse = 1;
  if (argc >= 5) {
    load_nse = atoi(argv[4]);
  }

  int input_date_ = atoi(argv[2]);
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);

  std::vector<std::string> shortcode_vec_;
  if (strcmp(argv[1], "ALL") == 0) {
    std::stringstream nse_filename_stream;
    if (load_nse) {
      HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
    }
    HFSAT::SecurityDefinitions::GetDefinedShortcodesVec(shortcode_vec_, input_date_);
  } else {
    shortcode_vec_.push_back(argv[1]);

    if (strncmp(argv[1], "NSE_", 4) == 0) {
      HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
    }
  }

  for (auto i = 0u; i < shortcode_vec_.size(); i++) {
    if (details_ == 1) {
      HFSAT::ExchSource_t exch_src_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_vec_[i], input_date_);
      std::cout << shortcode_vec_[i] << " " << HFSAT::ExchSourceStringForm(exch_src_) << std::endl;
    } else if (details_ == 2) {
      std::cout << shortcode_vec_[i] << " "
                << HFSAT::SecurityDefinitions::GetContractNumbersToDollars(shortcode_vec_[i], input_date_) << std::endl;
    } else if (details_ == 3) {
      std::cout << shortcode_vec_[i] << " " << HFSAT::SecurityDefinitions::GetContractMinPriceIncrement(
                                                   shortcode_vec_[i], input_date_) << std::endl;
    } else if (details_ == 4) {
      std::cout << shortcode_vec_[i] << " "
                << HFSAT::SecurityDefinitions::GetContractMinOrderSize(shortcode_vec_[i], input_date_) << std::endl;
    } else if (details_ == 5) {
      std::cout << shortcode_vec_[i] << " " << HFSAT::BaseCommish::GetCommishPerContract(shortcode_vec_[i], input_date_)
                << std::endl;
    } else if (details_ == 6) {
      HFSAT::ExchSource_t exch_src_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_vec_[i], input_date_);
      std::cout << shortcode_vec_[i] << " " << HFSAT::ExchSourceStringForm(exch_src_) << " "
                << HFSAT::SecurityDefinitions::GetContractNumbersToDollars(shortcode_vec_[i], input_date_) << " "
                << HFSAT::SecurityDefinitions::GetContractMinPriceIncrement(shortcode_vec_[i], input_date_) << " "
                << HFSAT::SecurityDefinitions::GetContractMinOrderSize(shortcode_vec_[i], input_date_) << " "
                << HFSAT::BaseCommish::GetCommishPerContract(shortcode_vec_[i], input_date_) << std::endl;
    } else if (details_ == 7) {
      // Nor calling for NSE spreads for now
      if (strncmp(shortcode_vec_[i].c_str(), "NSE_", 4) == 0 &&
          !HFSAT::NSESecurityDefinitions::IsSpreadShortcode(shortcode_vec_[i])) {
        std::cout << shortcode_vec_[i] << " "
                  << HFSAT::NSESecurityDefinitions::GetUniqueInstance(input_date_).GetLastClose(shortcode_vec_[i])
                  << std::endl;
      } else {
        std::cerr << "LAST_CLOSE_PRICE avaliable only for NSE non-sprd products" << std::endl;
      }
    } else if (details_ == 8) {
      if (strncmp(shortcode_vec_[i].c_str(), "NSE_", 4) == 0) {
        std::cout << shortcode_vec_[i] << " "
                  << HFSAT::NSESecurityDefinitions::GetUniqueInstance(input_date_)
                         .GetContractMultiplier(shortcode_vec_[i]) << std::endl;
      } else {
        std::cerr << "CONTRACT_MULTIPLIER avaliable only for NSE products" << std::endl;
      }
    } else if (details_ == 9) {
      // Nor calling for NSE spreads for now
      if (strncmp(shortcode_vec_[i].c_str(), "NSE_", 4) == 0 &&
          !HFSAT::NSESecurityDefinitions::IsSpreadShortcode(shortcode_vec_[i])) {
        std::cout << shortcode_vec_[i] << " "
                  << HFSAT::NSESecurityDefinitions::GetUniqueInstance(input_date_)
                         .GetLastCloseForOptions(shortcode_vec_[i]) << std::endl;
      } else {
        std::cerr << "LAST_CLOSE_PRICE_OPTIONS avaliable only for NSE non-sprd products" << std::endl;
      }
    } else if (details_ == 10) {
      HFSAT::ExchSource_t exch_src_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_vec_[i], input_date_);
      std::cout << shortcode_vec_[i] << " " << HFSAT::ExchSourceStringForm(exch_src_) << std::endl;
    } else if (details_ == 11) {
      if (!(strncmp(argv[1], "NSE_", 4) == 0)) {
        std::cout << "Valid for Only NSE exchange" << std::endl;
      } else {
        if (HFSAT::NSESecurityDefinitions::IsCurrency(shortcode_vec_[i])) {
          std::cout << "SEGMENT: "
                    << "CURRENCY" << std::endl;
        } else if (HFSAT::NSESecurityDefinitions::IsOption(shortcode_vec_[i])) {
          std::cout << "SEGMENT: "
                    << "OPTION" << std::endl;
        } else if (HFSAT::NSESecurityDefinitions::IsFuture(shortcode_vec_[i])) {
          std::cout << "SEGMENT: "
                    << "FUTURE" << std::endl;
        } else {
          std::cout << "SEGMENT: "
                    << "Invalid Segment" << std::endl;
        }
      }
    } else {
      HFSAT::ExchSource_t exch_src_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_vec_[i], input_date_);
      std::cout << "SHC: " << shortcode_vec_[i] << std::endl;
      std::cout << "EXCHANGE: " << HFSAT::ExchSourceStringForm(exch_src_) << std::endl;
      std::cout << "N2D: " << HFSAT::SecurityDefinitions::GetContractNumbersToDollars(shortcode_vec_[i], input_date_)
                << std::endl;
      std::cout << "TICKSIZE: " << HFSAT::SecurityDefinitions::GetContractMinPriceIncrement(shortcode_vec_[i],
                                                                                            input_date_) << std::endl;
      std::cout << "LOTSIZE: " << HFSAT::SecurityDefinitions::GetContractMinOrderSize(shortcode_vec_[i], input_date_)
                << std::endl;
      std::cout << "COMMISH: " << HFSAT::BaseCommish::GetCommishPerContract(shortcode_vec_[i], input_date_)
                << std::endl;
      // Nor calling for NSE spreads for now
      if (strncmp(shortcode_vec_[i].c_str(), "NSE_", 4) == 0 &&
          !HFSAT::NSESecurityDefinitions::IsSpreadShortcode(shortcode_vec_[i])) {
        std::cout << "LAST_CLOSE_PRICE: "
                  << HFSAT::NSESecurityDefinitions::GetUniqueInstance(input_date_).GetLastClose(shortcode_vec_[i])
                  << std::endl;
        std::cout << "CONTRACT_MULTIPLIER: "
                  << HFSAT::NSESecurityDefinitions::GetUniqueInstance(input_date_)
                         .GetContractMultiplier(shortcode_vec_[i]) << std::endl;
      }
    }
  }
  return 0;
}
