/**
   \file CDefCode/hk_stocks_security_definition.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#include "dvccode/CDef/hk_stocks_security_definition.hpp"
#include "dvccode/CDef/currency_convertor.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include <algorithm>

// filepaths are relative to /spare/local/tradeinfo/
#define HK_REF_FILENAME_PREFIX "HKStocks/ref"
#define HK_SHORTCODE_EXCHSYMBOL_FILE "HKStocks/shortcode_exch_symbol.txt"

#define HK_CHINA_RES_BEER_NAME_CHANGE_DATE 20151023

namespace HFSAT {

HKStocksSecurityDefinitions* HKStocksSecurityDefinitions::p_uniqueinstance_ = NULL;
int HKStocksSecurityDefinitions::intdate_ = 0;
std::vector<HKStocksParams*> HKStocksSecurityDefinitions::stock_params_;

std::map<std::string, std::string> HKStocksSecurityDefinitions::shortcode_2_exchsymbol_;
std::map<std::string, std::string> HKStocksSecurityDefinitions::exchsymbol_2_shortcode_;

HKStocksSecurityDefinitions::HKStocksSecurityDefinitions(int intdate) {
  CurrencyConvertor::SetUniqueInstance(intdate);

  intdate_ = intdate;
  stock_params_.clear();
  LoadInstParams(intdate_);
  LoadShortcodeExchangeSymbolMap();
}

std::string HKStocksSecurityDefinitions::GetExchSymbolHKStocks(const std::string& shortcode) {
  std::string local_shc = shortcode;
  if (shortcode_2_exchsymbol_.find(shortcode) != shortcode_2_exchsymbol_.end()) {
    return (shortcode_2_exchsymbol_[local_shc]);
  } else {
    return "INVALID";
  }
}

std::string HKStocksSecurityDefinitions::GetShortCodeFromExchSymbol(const std::string& exch_symbol) {
  if (exchsymbol_2_shortcode_.find(exch_symbol) != exchsymbol_2_shortcode_.end()) {
    return (exchsymbol_2_shortcode_[exch_symbol]);
  } else {
    return "INVALID";
  }
}

// Reads entries from HK Ref file - every line is of format
// Shortcode SecurityCode MinPriceIncrement LotSize
void HKStocksSecurityDefinitions::LoadInstParams(const int date) {
  std::ostringstream hk_ref_file_name_oss;
  std::string hk_ref_file_name = "";
  hk_ref_file_name_oss << HK_REF_FILENAME_PREFIX << "." << date;
  hk_ref_file_name = std::string("/spare/local/tradeinfo/") + hk_ref_file_name_oss.str();

  if (FileUtils::ExistsAndReadable(hk_ref_file_name)) {
    std::ifstream hk_ref_file;
    hk_ref_file.open(hk_ref_file_name.c_str(), std::ifstream::in);
    char buffer[1024];
    if (hk_ref_file.is_open()) {
      while (hk_ref_file.good()) {
        memset(buffer, 0, sizeof(buffer));
        hk_ref_file.getline(buffer, sizeof(buffer));
        PerishableStringTokenizer st_(buffer, sizeof(buffer));

        const std::vector<const char*>& tokens_ = st_.GetTokens();
        if (tokens_.size() >= 4 && tokens_[0][0] != '#') {
          HKStocksParams* t_config_ = new HKStocksParams();
          memset(t_config_->shortcode, 0, sizeof(t_config_->shortcode));
          // We will only use HK_CHINA_RES_BEER all the time.
          // Removing HK_CHINA_RESOURCES instead adding HK_CHINA_RES_BEER
          if (std::string(tokens_[0]) == "HK_CHINA_RESOURCES") {
            strcpy(t_config_->shortcode, "HK_CHINA_RES_BEER");
          } else {
            strcpy(t_config_->shortcode, tokens_[0]);
          }
          t_config_->security_code = atoi(tokens_[1]);
          t_config_->min_tick = atof(tokens_[2]);
          t_config_->lot_size = atoi(tokens_[3]);

          stock_params_.push_back(t_config_);
        }
      }  // end while
    }
    hk_ref_file.close();
  } else {
    std::cerr << "Fatal error - could not read HK Ref file " << hk_ref_file_name << '\n';
    exit(0);
  }
}

// Shortcode assumed to be HK_EQTY_NAME like HK_CHINA_MOBILE
void HKStocksSecurityDefinitions::AddHKStocksContractSpecifications(
    ShortcodeContractSpecificationMap& contract_spec_map) {
  for (auto param : stock_params_) {
    contract_spec_map[std::string(param->shortcode)] = ContractSpecification(
        param->min_tick, (1.0 / param->min_tick) * CurrencyConvertor::Convert(kCurrencyHKD, kCurrencyUSD),
        kExchSourceHONGKONG, param->lot_size);
  }
}

void HKStocksSecurityDefinitions::LoadShortcodeExchangeSymbolMap() {
  std::string hk_exchsymbol_filepath = std::string("/spare/local/tradeinfo/") + HK_SHORTCODE_EXCHSYMBOL_FILE;
  if (FileUtils::ExistsAndReadable(hk_exchsymbol_filepath)) {
    std::ifstream hk_shc_exch_file;
    hk_shc_exch_file.open(hk_exchsymbol_filepath.c_str(), std::ifstream::in);
    char buffer[1024];
    std::ostringstream t_oss_;
    t_oss_ << intdate_;
    if (hk_shc_exch_file.is_open()) {
      while (hk_shc_exch_file.good()) {
        memset(buffer, 0, sizeof(buffer));
        hk_shc_exch_file.getline(buffer, sizeof(buffer));
        PerishableStringTokenizer st_(buffer, sizeof(buffer));

        const std::vector<const char*>& tokens = st_.GetTokens();
        if (tokens.size() == 2 && tokens[0][0] != '#') {
          shortcode_2_exchsymbol_[std::string(tokens[0])] = std::string(tokens[1]);
          exchsymbol_2_shortcode_[std::string(tokens[1])] = std::string(tokens[0]);
        }
      }
    }
    // exchsymbol_2_shortcode_ is just used to get LoggedData file name
    // This is changed here as we just intend to use HK_CHINA_RES_BEER shortcode
    // But, before HK_CHINA_RES_BEER_NAME_CHANGE_DATE, it's data is stored in HK_CHINA_RESOURCES filename
    if (intdate_ < HK_CHINA_RES_BEER_NAME_CHANGE_DATE) {
      std::string exch_sym = shortcode_2_exchsymbol_["HK_CHINA_RES_BEER"];
      exchsymbol_2_shortcode_[exch_sym] = "HK_CHINA_RESOURCES";
    }
    hk_shc_exch_file.close();
  } else {
    std::cerr << "Fatal error - could not read HK ShortcodeExchSymbol file " << hk_exchsymbol_filepath << '\n';
    exit(0);
  }
}
}
