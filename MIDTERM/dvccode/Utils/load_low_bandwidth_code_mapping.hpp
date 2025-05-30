// =====================================================================================
//
//       Filename:  load_low_bandwidth_code_mapping.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  02/10/2014 11:31:23 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include <iostream>
#include <fstream>
#include <map>
#include <vector>

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/assumptions.hpp"

namespace HFSAT {
namespace Utils {

class LowBWCodeMappingLoader {
 public:
  static void LoadMappingForGivenFile(const char *file_name_,
                                      std::map<uint8_t, std::string> &product_code_to_shortcode_,
                                      std::map<std::string, std::string> &shortcode_to_exchange_symbol_) {
    std::ifstream livesource_contractcode_shortcode_mapping_file_;

    livesource_contractcode_shortcode_mapping_file_.open(file_name_);

    if (!livesource_contractcode_shortcode_mapping_file_.is_open()) {
      std::cerr << " Couldn't open broadcast product list file : " << file_name_ << "\n";
      exit(1);
    }

    std::cerr << " Read Cntract Code To Shortcode List File : " << file_name_ << "\n";
    char productcode_shortcode_line_[1024];

    int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
    HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);

    while (livesource_contractcode_shortcode_mapping_file_.good()) {
      livesource_contractcode_shortcode_mapping_file_.getline(productcode_shortcode_line_, 1024);

      std::string check_for_comment_ = productcode_shortcode_line_;

      if (check_for_comment_.find("#") != std::string::npos) continue;  // comments

      HFSAT::PerishableStringTokenizer st_(productcode_shortcode_line_, 1024);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() != 2) continue;  // mal formatted

      uint8_t this_contract_code_ = uint8_t(atoi(tokens_[0]));
      std::string this_shortcode_ = tokens_[1];
      std::string this_exch_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(this_shortcode_);

      // don't really need two maps, can use only one

      if (product_code_to_shortcode_.find(this_contract_code_) == product_code_to_shortcode_.end()) {
        product_code_to_shortcode_[this_contract_code_] = this_shortcode_;

        if (shortcode_to_exchange_symbol_.find(this_shortcode_) == shortcode_to_exchange_symbol_.end()) {
          shortcode_to_exchange_symbol_[this_shortcode_] = this_exch_symbol_;
        }
      }
    }

    livesource_contractcode_shortcode_mapping_file_.close();
  }

  static void GetEurexStructFromLowBWStruct(std::map<uint8_t, std::string> &product_code_to_shortcode_,
                                            std::map<std::string, std::string> &shortcode_to_exchange_symbol_,
                                            EUREX_MDS::EUREXCommonStruct &cstr_,
                                            EUREX_MDS::EUREXLSCommonStruct &minimal_cstr_) {
    memset((void *)&cstr_, 0, sizeof(EUREX_MDS::EUREXCommonStruct));
    cstr_.msg_ = minimal_cstr_.msg_;

    if (minimal_cstr_.msg_ == EUREX_MDS::EUREX_DELTA) {
      if (product_code_to_shortcode_.find(minimal_cstr_.data_.eurex_dels_.contract_code_) ==
          product_code_to_shortcode_.end()) {
        std::cerr << " Coulnd't find shortcode from product Code : " << minimal_cstr_.data_.eurex_dels_.contract_code_
                  << "\n";
        exit(1);
      }

      if (shortcode_to_exchange_symbol_.find(
              product_code_to_shortcode_[minimal_cstr_.data_.eurex_dels_.contract_code_]) ==
          shortcode_to_exchange_symbol_.end()) {
        std::cerr << " Coulnd't find exchange sym for shortcode : "
                  << product_code_to_shortcode_[minimal_cstr_.data_.eurex_dels_.contract_code_] << "\n";
        exit(1);
      }

      memcpy(cstr_.data_.eurex_dels_.contract_,
             shortcode_to_exchange_symbol_[product_code_to_shortcode_[minimal_cstr_.data_.eurex_dels_.contract_code_]]
                 .c_str(),
             std::min((int32_t)shortcode_to_exchange_symbol_
                          [product_code_to_shortcode_[minimal_cstr_.data_.eurex_dels_.contract_code_]].length(),
                      EUREX_MDS_CONTRACT_TEXT_SIZE));
      cstr_.data_.eurex_dels_.level_ = minimal_cstr_.data_.eurex_dels_.level_;
      cstr_.data_.eurex_dels_.size_ = minimal_cstr_.data_.eurex_dels_.size_;
      cstr_.data_.eurex_dels_.price_ = minimal_cstr_.data_.eurex_dels_.price_;
      cstr_.data_.eurex_dels_.type_ = minimal_cstr_.data_.eurex_dels_.type_;
      cstr_.data_.eurex_dels_.status_ = ' ';
      cstr_.data_.eurex_dels_.action_ = minimal_cstr_.data_.eurex_dels_.action_;
      cstr_.data_.eurex_dels_.intermediate_ = minimal_cstr_.data_.eurex_dels_.intermediate_;
      cstr_.data_.eurex_dels_.num_ords_ = minimal_cstr_.data_.eurex_dels_.num_ords_;

    }

    else if (minimal_cstr_.msg_ == EUREX_MDS::EUREX_TRADE) {
      if (product_code_to_shortcode_.find(minimal_cstr_.data_.eurex_trds_.contract_code_) ==
          product_code_to_shortcode_.end()) {
        std::cerr << " Coulnd't find shortcode from product Code : " << minimal_cstr_.data_.eurex_trds_.contract_code_
                  << "\n";
        exit(1);
      }

      if (shortcode_to_exchange_symbol_.find(
              product_code_to_shortcode_[minimal_cstr_.data_.eurex_trds_.contract_code_]) ==
          shortcode_to_exchange_symbol_.end()) {
        std::cerr << " Coulnd't find exchange sym for shortcode : "
                  << product_code_to_shortcode_[minimal_cstr_.data_.eurex_trds_.contract_code_] << "\n";
        exit(1);
      }

      memcpy(cstr_.data_.eurex_trds_.contract_,
             shortcode_to_exchange_symbol_[product_code_to_shortcode_[minimal_cstr_.data_.eurex_trds_.contract_code_]]
                 .c_str(),
             std::min((int32_t)shortcode_to_exchange_symbol_
                          [product_code_to_shortcode_[minimal_cstr_.data_.eurex_trds_.contract_code_]].length(),
                      EUREX_MDS_CONTRACT_TEXT_SIZE));

      cstr_.data_.eurex_trds_.trd_qty_ = minimal_cstr_.data_.eurex_trds_.trd_qty_;
      cstr_.data_.eurex_trds_.agg_side_ = minimal_cstr_.data_.eurex_trds_.agg_side_;
      cstr_.data_.eurex_trds_.trd_px_ = minimal_cstr_.data_.eurex_trds_.trd_px_;
    }
  }

  static void GetCMEStructFromLowBWStruct(std::map<uint8_t, std::string> &product_code_to_shortcode_,
                                          std::map<std::string, std::string> &shortcode_to_exchange_symbol_,
                                          CME_MDS::CMECommonStruct &cstr_, CME_MDS::CMELSCommonStruct &minimal_cstr_) {
    memset((void *)&cstr_, 0, sizeof(CME_MDS::CMECommonStruct));

    cstr_.msg_ = minimal_cstr_.msg_;

    if (minimal_cstr_.msg_ == CME_MDS::CME_DELTA) {
      if (product_code_to_shortcode_.find(minimal_cstr_.data_.cme_dels_.contract_code_) ==
          product_code_to_shortcode_.end()) {
        std::cerr << " Coulnd't find shortcode from product Code : " << minimal_cstr_.data_.cme_dels_.contract_code_
                  << "\n";
        exit(1);
      }

      if (shortcode_to_exchange_symbol_.find(
              product_code_to_shortcode_[minimal_cstr_.data_.cme_dels_.contract_code_]) ==
          shortcode_to_exchange_symbol_.end()) {
        std::cerr << " Coulnd't find exchange sym for shortcode : "
                  << product_code_to_shortcode_[minimal_cstr_.data_.cme_dels_.contract_code_] << "\n";
        exit(1);
      }

      memcpy(cstr_.data_.cme_dels_.contract_,
             shortcode_to_exchange_symbol_[product_code_to_shortcode_[minimal_cstr_.data_.cme_dels_.contract_code_]]
                 .c_str(),
             std::min((int32_t)shortcode_to_exchange_symbol_
                          [product_code_to_shortcode_[minimal_cstr_.data_.cme_dels_.contract_code_]].length(),
                      CME_MDS_CONTRACT_TEXT_SIZE));

      cstr_.data_.cme_dels_.level_ = minimal_cstr_.data_.cme_dels_.level_;
      cstr_.data_.cme_dels_.size_ = minimal_cstr_.data_.cme_dels_.size_;
      cstr_.data_.cme_dels_.num_ords_ = minimal_cstr_.data_.cme_dels_.num_ords_;
      cstr_.data_.cme_dels_.price_ = minimal_cstr_.data_.cme_dels_.price_;
      cstr_.data_.cme_dels_.type_ = minimal_cstr_.data_.cme_dels_.type_;
      cstr_.data_.cme_dels_.status_ = 'A';
      cstr_.data_.cme_dels_.action_ = minimal_cstr_.data_.cme_dels_.action_;
      cstr_.data_.cme_dels_.intermediate_ = minimal_cstr_.data_.cme_dels_.intermediate_;

    } else if (minimal_cstr_.msg_ == CME_MDS::CME_TRADE) {
      if (product_code_to_shortcode_.find(minimal_cstr_.data_.cme_trds_.contract_code_) ==
          product_code_to_shortcode_.end()) {
        std::cerr << " Coulnd't find shortcode from product Code : " << minimal_cstr_.data_.cme_trds_.contract_code_
                  << "\n";
        exit(1);
      }

      if (shortcode_to_exchange_symbol_.find(
              product_code_to_shortcode_[minimal_cstr_.data_.cme_trds_.contract_code_]) ==
          shortcode_to_exchange_symbol_.end()) {
        std::cerr << " Coulnd't find exchange sym for shortcode : "
                  << product_code_to_shortcode_[minimal_cstr_.data_.cme_trds_.contract_code_] << "\n";
        exit(1);
      }

      memcpy(cstr_.data_.cme_trds_.contract_,
             shortcode_to_exchange_symbol_[product_code_to_shortcode_[minimal_cstr_.data_.cme_trds_.contract_code_]]
                 .c_str(),
             std::min((int32_t)shortcode_to_exchange_symbol_
                          [product_code_to_shortcode_[minimal_cstr_.data_.cme_trds_.contract_code_]].length(),
                      CME_MDS_CONTRACT_TEXT_SIZE));

      cstr_.data_.cme_trds_.trd_qty_ = minimal_cstr_.data_.cme_trds_.trd_qty_;
      cstr_.data_.cme_trds_.agg_side_ = minimal_cstr_.data_.cme_trds_.agg_side_;
      cstr_.data_.cme_trds_.trd_px_ = minimal_cstr_.data_.cme_trds_.trd_px_;
    }
  }
};
}
}
