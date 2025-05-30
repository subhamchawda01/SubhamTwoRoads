// =====================================================================================
//
//       Filename:  bse_refdata_loader.hpp
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
#include "dvccode/CDef/bse_mds_defines.hpp"
#include "dvccode/Utils/bse_spot_token_generator.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#define BSE_REF_DATA_FILE_LOCATION "/spare/local/tradeinfo/BSE_Files/RefData/"

namespace HFSAT {
namespace Utils {

class BSERefDataLoader {
 private:
  std::map<int32_t, BSE_UDP_MDS::BSERefData> fo_token_to_bse_refdata;
  std::map<int32_t, BSE_UDP_MDS::BSERefData> cd_token_to_bse_refdata;
  std::map<int32_t, BSE_UDP_MDS::BSERefData> eq_token_to_bse_refdata;
  std::map<int32_t, BSE_UDP_MDS::BSERefData> dummy_error_notif_map;
  std::map<int32_t, BSE_UDP_MDS::BSERefData> ix_token_to_bse_refdata;
  
  int MONTH_INT(std::string _date_str_) {
    int mm=13;
    if (_date_str_ == "JAN")
        mm = 0;
    else if (_date_str_ == "FEB")
        mm = 1;
    else if (_date_str_ == "MAR")
        mm = 2;
    else if (_date_str_ == "APR")
        mm = 3;
    else if (_date_str_ == "MAY")
        mm = 4;
    else if (_date_str_ == "JUN")
        mm = 5;
    else if (_date_str_ == "JUL")
        mm = 6;
    else if (_date_str_ == "AUG")
        mm = 7;
    else if (_date_str_ == "SEP")
        mm = 8;
    else if (_date_str_ == "OCT")
        mm = 9;
    else if (_date_str_ == "NOV")
        mm = 10;
    else if (_date_str_ == "DEC")
        mm = 11;
    else 
        std::cerr << "Invalid Month" << std::endl;
    return mm;
  }

  void LoadDataFromFile(char const *ref_data_file_name, int32_t const &price_factor, HFSAT::ExchSource_t exch_src) {
    std::ifstream bse_ref_file;

    bse_ref_file.open(ref_data_file_name, std::ifstream::in);

    if (!(bse_ref_file.is_open())) {
      std::cerr << "Cannot open file " << ref_data_file_name << "\n";
      exit(-1);
    }

    char line[1024];
    while (!bse_ref_file.eof()) {
      bzero(line, 1024);
      bse_ref_file.getline(line, 1024);
      if (strlen(line) == 0 || strstr(line, "#") != NULL) continue;
      HFSAT::PerishableStringTokenizer st_(line, 1024);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() != 10) {
        
        std::cerr << "Ignoring Malformatted line in file " << line<<" " << tokens_.size()<<  ref_data_file_name << "\n";
        continue;
      }

      BSE_UDP_MDS::BSERefData ref_data;
      int32_t token = atoi(tokens_[0]);
      const char *instrument = tokens_[2];
      const char *symbol = tokens_[3];
      std::string expiry_str = tokens_[4];
      int64_t expiry = 0;
      struct tm tt;
      time_t t_of_day;
      if ( expiry_str.size() == 11){
        tt.tm_year = std::stoi(expiry_str.substr(7,4)) - 1900;
        tt.tm_mday = std::stoi(expiry_str.substr(0,2));
        tt.tm_mon =  MONTH_INT(expiry_str.substr(3,3));
        t_of_day = mktime(&tt);
        expiry = t_of_day;
      }

      double strike = atoi(tokens_[5]) / (double)price_factor;
      const char *option_type = tokens_[6];
      const char *exchange_symbol = tokens_[7];
      double lower_px = atoi(tokens_[8]) / (double)price_factor;
      double upper_px = atoi(tokens_[9]) / (double)price_factor;

      ref_data.Init(token, expiry, strike, instrument, symbol, option_type, exchange_symbol, price_factor, exch_src,
                    lower_px, upper_px);

      if (HFSAT::kExchSourceBSE_FO == exch_src) {
        fo_token_to_bse_refdata[token] = ref_data;
      } else if (HFSAT::kExchSourceBSE_CD == exch_src) {
        cd_token_to_bse_refdata[token] = ref_data;
      } else if (HFSAT::kExchSourceBSE_EQ == exch_src) {
        eq_token_to_bse_refdata[token] = ref_data;
      }
    }

    bse_ref_file.close();
  }


  void LoadIxData(int price_factor){

    //std::cout << "FILLING LoadIxData " << std::endl;
    std::unordered_map <std::string, int> bse_spot_index_2_token_map_ = HFSAT::BSESpotTokenGenerator::GetUniqueInstance().GetSpotIndexToTokenMap();
    for (auto& itr : bse_spot_index_2_token_map_) {
         BSE_UDP_MDS::BSERefData ref_data;
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

        ref_data.Init(token, expiry, strike, instrument, symbol, option_type, exchange_symbol, price_factor, HFSAT::kExchSourceBSE_IDX,
                    lower_px, upper_px);

        //std::cout << "FILLING FOR " << token << std::endl;
        ix_token_to_bse_refdata[token] = ref_data;
      }
      //std::cout << "SIZE OF MAP IS " << ix_token_to_bse_refdata.size() << std::endl;
      //std::cout << "FILLING LoadIxData Completed" << std::endl;
  }

  void LoadRefData(int32_t const &trading_date) {
    // FUT File Loading
    std::ostringstream bse_fo_file_name_str;

    // We provide default value(=-1) as trading date, for ToString call in MDS struct
    int32_t adjusted_trading_date = (trading_date == -1) ? HFSAT::DateTime::GetCurrentIsoDateLocal() : trading_date;

    bse_fo_file_name_str << BSE_REF_DATA_FILE_LOCATION << "bse_fo_" << adjusted_trading_date << "_contracts.txt";
    LoadDataFromFile(bse_fo_file_name_str.str().c_str(), 100, HFSAT::kExchSourceBSE_FO);

    // CUR File Handling
    std::ostringstream bse_cd_file_name_str;
    bse_cd_file_name_str << BSE_REF_DATA_FILE_LOCATION << "bse_cd_" << adjusted_trading_date << "_contracts.txt";

    LoadDataFromFile(bse_cd_file_name_str.str().c_str(), 10000000, HFSAT::kExchSourceBSE_CD);

    // Stocks File Handling
    std::ostringstream bse_eq_file_name_str;
    bse_eq_file_name_str << BSE_REF_DATA_FILE_LOCATION << "bse_eq_" << adjusted_trading_date << "_contracts.txt";

    LoadDataFromFile(bse_eq_file_name_str.str().c_str(), 100, HFSAT::kExchSourceBSE_EQ);

    // IX Handling
    LoadIxData(100);   
  
  }

  BSERefDataLoader(int32_t const &trading_date) { LoadRefData(trading_date); }

  BSERefDataLoader(BSERefDataLoader const &disabled_copy_constructor) = delete;

 public:
  static BSERefDataLoader &GetUniqueInstance(int32_t const &trading_date) {
    static BSERefDataLoader unique_instance(trading_date);
    return unique_instance;
  }

  std::map<int32_t, BSE_UDP_MDS::BSERefData> &GetBSERefData(char const &segment) {
    if (BSE_EQ_SEGMENT_MARKING == segment)
      return eq_token_to_bse_refdata;
    else if (BSE_CD_SEGMENT_MARKING == segment)
      return cd_token_to_bse_refdata;
    else if (BSE_FO_SEGMENT_MARKING == segment)
      return fo_token_to_bse_refdata;
    else if (BSE_IX_SEGMENT_MARKING == segment)
      return ix_token_to_bse_refdata;

    return dummy_error_notif_map;
  }

  double GetPriceMultiplier(const int &token, const char &segment) {
    double price_multiplier = -1;
    if (BSE_EQ_SEGMENT_MARKING == segment && eq_token_to_bse_refdata.find(token) != eq_token_to_bse_refdata.end()) {
      return eq_token_to_bse_refdata[token].price_multiplier;
    } else if (BSE_CD_SEGMENT_MARKING == segment &&
               cd_token_to_bse_refdata.find(token) != cd_token_to_bse_refdata.end()) {
      return cd_token_to_bse_refdata[token].price_multiplier;
    } else if (BSE_FO_SEGMENT_MARKING == segment &&
               fo_token_to_bse_refdata.find(token) != fo_token_to_bse_refdata.end()) {
      return fo_token_to_bse_refdata[token].price_multiplier;
    }  else if (BSE_IX_SEGMENT_MARKING == segment &&
               ix_token_to_bse_refdata.find(token) != ix_token_to_bse_refdata.end()) {
      return ix_token_to_bse_refdata[token].price_multiplier;
    }

    return price_multiplier;
  }
};
}
}
