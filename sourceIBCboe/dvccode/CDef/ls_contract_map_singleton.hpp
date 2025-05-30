// File: dvccode/CDef/ls_contract_map_singleton.hpp

#ifndef LS_CONTRACT_MAP_SINGLETON_H
#define LS_CONTRACT_MAP_SINGLETON_H

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/assumptions.hpp"
namespace HFSAT {
template <const char *const MAP_FILE>
class LsContractInfoMap {
 private:
  std::map<uint8_t, std::pair<std::string, const char *>>
      product_code_to_shortcode_exch_sym;  // shc - pair.first, exch_symbol - pair.second
  void InitializeMaps() {
    // Read mappings from file
    std::ifstream livesource_contractcode_shortcode_mapping_file_;
    livesource_contractcode_shortcode_mapping_file_.open(MAP_FILE);
    if (!livesource_contractcode_shortcode_mapping_file_.is_open()) {
      std::cerr << " Couldn't open broadcast product list file : " << MAP_FILE << "\n";
      exit(1);
    }

    char productcode_shortcode_line_[1024];

    int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
    HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);

    while (livesource_contractcode_shortcode_mapping_file_.good()) {
      livesource_contractcode_shortcode_mapping_file_.getline(productcode_shortcode_line_, 1024);
      std::string check_for_comment_ = productcode_shortcode_line_;
      if (check_for_comment_.find("#") != std::string::npos) continue;  // comments
      HFSAT::PerishableStringTokenizer st_(productcode_shortcode_line_, 1024);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      // Read product code and shc pair, fetch exchange symbol from shortcode
      if (tokens_.size() != 2) continue;  // mal formatted
      uint8_t this_contract_code_ = uint8_t(atoi(tokens_[0]));
      std::string this_shortcode_ = tokens_[1];
      std::string this_exch_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(this_shortcode_);

      // Populate the product_code to shc and exchange symbol map
      if (product_code_to_shortcode_exch_sym.find(this_contract_code_) == product_code_to_shortcode_exch_sym.end()) {
        // Check if duplicate entry of product code doesn't exist
        char *exch_sym_char = new char[16];
        bzero(exch_sym_char, 16);
        memcpy(static_cast<void *>(exch_sym_char), this_exch_symbol_.c_str(),
               std::min((int32_t)this_exch_symbol_.length(), CME_MDS_CONTRACT_TEXT_SIZE));

        product_code_to_shortcode_exch_sym[this_contract_code_] =
            std::pair<std::string, const char *>(this_shortcode_, exch_sym_char);
      }
    }

    livesource_contractcode_shortcode_mapping_file_.close();
  }
  LsContractInfoMap() {  // Make constructor private
    InitializeMaps();
  }

 public:
  LsContractInfoMap(const LsContractInfoMap &m1) = delete;             // Delete copy constructor
  LsContractInfoMap &operator=(const LsContractInfoMap &m1) = delete;  // Delete copy assignment
  static LsContractInfoMap &GetUniqueInstance() {
    static LsContractInfoMap unique_instance;
    return unique_instance;
  }
  inline std::string GetShcFromProductCode(uint8_t prod_code) {
    auto prod_map_itr = product_code_to_shortcode_exch_sym.find(prod_code);
    if (prod_map_itr != product_code_to_shortcode_exch_sym.end()) {
      return prod_map_itr->second.first;
    }
    // return empty string if not found
    return std::string("");
  }
  inline const char *GetChar16ExchSymbolFromProductCode(uint8_t prod_code) {
    // NOTE: this returns the char16 base pointer. The string will not be null terminated if the exch symbol exceeds 16
    // bytes. Returns nullptr if prod_code not found.
    auto prod_map_itr = product_code_to_shortcode_exch_sym.find(prod_code);
    if (prod_map_itr != product_code_to_shortcode_exch_sym.end()) {
      return prod_map_itr->second.second;
    }
    // return nullptr if not found
    return nullptr;
  }
};
}  // namespace HFSAT
#endif
