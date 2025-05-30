// =====================================================================================
//
//       Filename:  nse_refdata_loader.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  07/14/2015 10:46:00 AM
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

#pragma once

#include <iostream>
#include <fstream>
#include <string.h>
#include <unordered_map>
#include "dvccode/CDef/nse_mds_defines.hpp"
#include "dvccode/Utils/spot_token_generator.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#define NSE_REF_DATA_FILE_LOCATION "/spare/local/tradeinfo/NSE_Files/RefData/"

namespace HFSAT {
namespace Utils {

class NSERefDataLoader {
 private:
  std::map<int32_t, NSE_UDP_MDS::NSERefData> fo_token_to_nse_refdata;
  std::map<int32_t, NSE_UDP_MDS::NSERefData> cd_token_to_nse_refdata;
  std::map<int32_t, NSE_UDP_MDS::NSERefData> eq_token_to_nse_refdata;
  std::map<int32_t, NSE_UDP_MDS::NSERefData> ix_token_to_nse_refdata;

  std::map<int32_t, NSE_UDP_MDS::NSERefData> dummy_error_notif_map;

  void LoadDataFromFile(char const *ref_data_file_name, int32_t const &price_factor, HFSAT::ExchSource_t exch_src) {
    std::ifstream nse_ref_file;

    nse_ref_file.open(ref_data_file_name, std::ifstream::in);

    if (!(nse_ref_file.is_open())) {
      std::cerr << "Cannot open file " << ref_data_file_name << "\n";
      exit(-1);
    }

    char line[1024];
    while (!nse_ref_file.eof()) {
      bzero(line, 1024);
      nse_ref_file.getline(line, 1024);
      if (strlen(line) == 0 || strstr(line, "#") != NULL) continue;
      HFSAT::PerishableStringTokenizer st_(line, 1024);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() != 10) {
        std::cerr << "Ignoring Malformatted line in file " << ref_data_file_name << "\n";
        continue;
      }

      NSE_UDP_MDS::NSERefData ref_data;

      int32_t token = atoi(tokens_[0]);
      const char *instrument = tokens_[2];
      const char *symbol = tokens_[3];
      int64_t expiry = atoi(tokens_[4]);
      double strike = atoi(tokens_[5]) / (double)price_factor;
      const char *option_type = tokens_[6];
      const char *exchange_symbol = tokens_[7];
      double lower_px = atoi(tokens_[8]) / (double)price_factor;
      double upper_px = atoi(tokens_[9]) / (double)price_factor;

      ref_data.Init(token, expiry, strike, instrument, symbol, option_type, exchange_symbol, price_factor, exch_src,
                    lower_px, upper_px);

      if (HFSAT::kExchSourceNSE_FO == exch_src) {
        fo_token_to_nse_refdata[token] = ref_data;
      } else if (HFSAT::kExchSourceNSE_CD == exch_src) {
        cd_token_to_nse_refdata[token] = ref_data;
      } else if (HFSAT::kExchSourceNSE_EQ == exch_src) {
        eq_token_to_nse_refdata[token] = ref_data;
      }
    }

    nse_ref_file.close();
  }

  void LoadIxData(int price_factor){
    std::unordered_map <std::string, int> spot_index_2_token_map_ = HFSAT::SpotTokenGenerator::GetUniqueInstance().GetSpotIndexToTokenMap();
    for (auto& itr : spot_index_2_token_map_) {
        NSE_UDP_MDS::NSERefData ref_data;
// 16921 0 STK 20MICRONS 0 0 XX STK 0 0
        int32_t token = itr.second;
      const char *instrument = "IX";
      const char *symbol = itr.first.c_str();
      int64_t expiry = 0;
      double strike = 0;
      const char *option_type = "XX";
      const char *exchange_symbol = "IX";
      double lower_px = 0;
      double upper_px = 0;

        ref_data.Init(token, expiry, strike, instrument, symbol, option_type, exchange_symbol, price_factor, HFSAT::kExchSourceNSE_IDX,
                    lower_px, upper_px);
        ix_token_to_nse_refdata[token] = ref_data;
      }
  }

  void LoadRefData(int32_t const &trading_date) {
    // FUT File Loading
    std::ostringstream nse_fo_file_name_str;

    // We provide default value(=-1) as trading date, for ToString call in MDS struct
    int32_t adjusted_trading_date = (trading_date == -1) ? HFSAT::DateTime::GetCurrentIsoDateLocal() : trading_date;

    nse_fo_file_name_str << NSE_REF_DATA_FILE_LOCATION << "nse_fo_" << adjusted_trading_date << "_contracts.txt";
    LoadDataFromFile(nse_fo_file_name_str.str().c_str(), 100, HFSAT::kExchSourceNSE_FO);

    // CUR File Handling
    std::ostringstream nse_cd_file_name_str;
    nse_cd_file_name_str << NSE_REF_DATA_FILE_LOCATION << "nse_cd_" << adjusted_trading_date << "_contracts.txt";

    LoadDataFromFile(nse_cd_file_name_str.str().c_str(), 10000000, HFSAT::kExchSourceNSE_CD);

    // Stocks File Handling
    std::ostringstream nse_eq_file_name_str;
    nse_eq_file_name_str << NSE_REF_DATA_FILE_LOCATION << "nse_eq_" << adjusted_trading_date << "_contracts.txt";

    LoadDataFromFile(nse_eq_file_name_str.str().c_str(), 100, HFSAT::kExchSourceNSE_EQ);

    // IX Handling
    LoadIxData(100);    
  }

  NSERefDataLoader(int32_t const &trading_date) { LoadRefData(trading_date); }

  NSERefDataLoader(NSERefDataLoader const &disabled_copy_constructor) = delete;

 public:
  static NSERefDataLoader &GetUniqueInstance(int32_t const &trading_date) {
    static NSERefDataLoader unique_instance(trading_date);
    return unique_instance;
  }

  std::map<int32_t, NSE_UDP_MDS::NSERefData> &GetNSERefData(char const &segment) {
    if (NSE_EQ_SEGMENT_MARKING == segment)
      return eq_token_to_nse_refdata;
    else if (NSE_CD_SEGMENT_MARKING == segment)
      return cd_token_to_nse_refdata;
    else if (NSE_FO_SEGMENT_MARKING == segment)
      return fo_token_to_nse_refdata;
    else if (NSE_IX_SEGMENT_MARKING == segment)
      return ix_token_to_nse_refdata;

    return dummy_error_notif_map;
  }

  double GetPriceMultiplier(const int &token, const char &segment) {
    double price_multiplier = -1;
    if (NSE_EQ_SEGMENT_MARKING == segment && eq_token_to_nse_refdata.find(token) != eq_token_to_nse_refdata.end()) {
      return eq_token_to_nse_refdata[token].price_multiplier;
    } else if (NSE_CD_SEGMENT_MARKING == segment &&
               cd_token_to_nse_refdata.find(token) != cd_token_to_nse_refdata.end()) {
      return cd_token_to_nse_refdata[token].price_multiplier;
    } else if (NSE_FO_SEGMENT_MARKING == segment &&
               fo_token_to_nse_refdata.find(token) != fo_token_to_nse_refdata.end()) {
      return fo_token_to_nse_refdata[token].price_multiplier;
    } else if (NSE_IX_SEGMENT_MARKING == segment &&
               ix_token_to_nse_refdata.find(token) != ix_token_to_nse_refdata.end()) {
      return ix_token_to_nse_refdata[token].price_multiplier;
    } 

    return price_multiplier;
  }
};
}
}
