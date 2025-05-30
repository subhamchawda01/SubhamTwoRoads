// =====================================================================================
//
//       Filename:  cboe_refdata_loader.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  20241013
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
#include "dvccode/CDef/cboe_mds_defines.hpp"
#include "dvccode/Utils/cboe_spot_token_generator.hpp"
#include "dvccode/Utils/cboe_combo_token_generator.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#define CBOE_REF_DATA_FILE_LOCATION "/spare/local/tradeinfo/CBOE_Files/RefData/"

namespace HFSAT {
namespace Utils {

class CBOERefDataLoader {
 private:
  std::map<int32_t, CBOE_UDP_MDS::CBOERefData> fo_token_to_cboe_refdata;
  std::map<int32_t, CBOE_UDP_MDS::CBOERefData> ix_token_to_cboe_refdata;
  std::map<int32_t, CBOE_UDP_MDS::CBOERefData> combo_token_to_cboe_refdata;
  std::map<int32_t, CBOE_UDP_MDS::CBOERefData> dummy_error_notif_map;
  
  void LoadDataFromFile(char const *ref_data_file_name, HFSAT::ExchSource_t exch_src) {

    if (FileUtils::ExistsAndReadable(ref_data_file_name)) {
      std::ifstream cboe_ref_file;
      cboe_ref_file.open(ref_data_file_name, std::ifstream::in);
      char readline_buffer_[1024];
      if (cboe_ref_file.is_open()) {
        while (cboe_ref_file.good()) {
          memset(readline_buffer_, 0, sizeof(readline_buffer_));
          cboe_ref_file.getline(readline_buffer_, sizeof(readline_buffer_));
  
          std::vector<char*> tokens_;
          // create a copy of line read before using non-const tokenizer
          char readline_buffer_copy_[1024];
          memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
          strcpy(readline_buffer_copy_, readline_buffer_);
    
          HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, ",", tokens_);
          if (tokens_.size() < 6) continue;
          std::string type_ = tokens_[1];
          // trim the inst_type field: contains spaces
          if (tokens_[0][0] != '#' && (type_ == "IDXOPT")) { // Only Index Options Support
            CBOE_UDP_MDS::CBOERefData ref_data;
            int64_t token = atoi(tokens_[0]);
            const char *instrument = tokens_[1];
            const char *symbol = tokens_[2];
            int64_t expiry = std::atoi(tokens_[3]);;
            double strike = atof(tokens_[4]);
            const char *option_type = tokens_[5];
      
            ref_data.Init(token, expiry, strike, instrument, symbol, option_type, exch_src);
      
            if (HFSAT::kExchSourceCBOE_FO == exch_src) {
              fo_token_to_cboe_refdata[token] = ref_data;
            }
  	      }else if((tokens_[0][0] != '#') && (type_ == "IDXSPT")){
            CBOE_UDP_MDS::CBOERefData ref_data;
            int64_t token = atoi(tokens_[0]);
            const char *instrument = tokens_[1];
            const char *symbol = tokens_[2];
            int64_t expiry = std::atoi(tokens_[3]);;
            double strike = atof(tokens_[4]);
            const char *option_type = tokens_[5];
      
            ref_data.Init(token, expiry, strike, instrument, symbol, option_type, exch_src);
      
            if (HFSAT::kExchSourceCBOE_IDX == exch_src) {
              ix_token_to_cboe_refdata[token] = ref_data;
            }
          }
        }
      }
      cboe_ref_file.close();
    }

    // //TODO - correct this 
    // CBOE_UDP_MDS::CBOERefData ref_data;
    // ref_data.Init(416904, -1, -1, (char const*)"IDXSPT",(char const*)"CBOE_IDX416904","I", exch_src);
    // fo_token_to_cboe_refdata[416904] = ref_data;
  }

  void LoadIxData(){

    std::unordered_map <std::string, int> cboe_spot_index_2_token_map_ = HFSAT::CBOESpotTokenGenerator::GetUniqueInstance().GetSpotIndexToTokenMap();
    for (auto& itr : cboe_spot_index_2_token_map_) {
         CBOE_UDP_MDS::CBOERefData ref_data;
// 16921 0 STK 20MICRONS 0 0 XX STK 0 0
      int32_t token = itr.second;
      const char *instrument = "IX";
      const char *symbol = itr.first.c_str();
      int64_t expiry = 0;
      double strike = 0;
      const char *option_type = "IX";

      ref_data.Init(token, expiry, strike, instrument, symbol, option_type, HFSAT::kExchSourceCBOE_IDX);

      ix_token_to_cboe_refdata[token] = ref_data;
    }
  }
  //For now this is how we store the symbol for combo refdata due to the size restriction of 11
  std::string getSymbolForCombo(std::string combo_shc_){
    size_t pos = combo_shc_.rfind('_'); // Find the last occurrence of '_'
    if (pos == std::string::npos) {
        // If '_' is not found, return an empty string or handle error
        return "";
    }
    return combo_shc_.substr(pos + 1);
  }
  void LoadCOData(){

    std::unordered_map <std::string, int> cboe_combo_2_token_map_ = HFSAT::CBOEComboTokenGenerator::GetUniqueInstance().GetComboToTokenMap();
    for (auto& itr : cboe_combo_2_token_map_) {
         CBOE_UDP_MDS::CBOERefData ref_data;

      int32_t token = itr.second;
      const char *instrument = "CO";
      const char *symbol = getSymbolForCombo(itr.first).c_str();
      int64_t expiry = 0;
      double strike = 0;
      const char *option_type = "XX";

      ref_data.Init(token, expiry, strike, instrument, symbol, option_type, HFSAT::kExchSourceCBOE_CO);

      combo_token_to_cboe_refdata[token] = ref_data;
      // std::cout<<itr.first<<" "<<"Ref Data"<<" "<<ref_data.symbol<<std::endl;
    }
  }

  void LoadRefData(int32_t const &trading_date) { //Not loading for combo as of now
    // FUT File Loading
    std::ostringstream cboe_fo_file_name_str;

    // //Combo File Loading if present
    // std::ostringstream cboe_combo_file_name_str;


    // We provide default value(=-1) as trading date, for ToString call in MDS struct
    int32_t adjusted_trading_date = (trading_date == -1) ? HFSAT::DateTime::GetCurrentIsoDateLocal() : trading_date;

    cboe_fo_file_name_str << CBOE_REF_DATA_FILE_LOCATION << "cboe_fo_" << adjusted_trading_date << "_contracts.txt";
    LoadDataFromFile(cboe_fo_file_name_str.str().c_str(), HFSAT::kExchSourceCBOE_FO);

    // cboe_combo_file_name_str << CBOE_REF_DATA_FILE_LOCATION << "cboe_combo_" << adjusted_trading_date << "_contracts.txt";
    // LoadDataFromFile(cboe_combo_file_name_str.str().c_str(), HFSAT::kExchSourceCBOE_CO); 

    // IX Handling
    LoadIxData();
    LoadCOData();
  }

  CBOERefDataLoader(int32_t const &trading_date) { LoadRefData(trading_date); }

  CBOERefDataLoader(CBOERefDataLoader const &disabled_copy_constructor) = delete;

 public:
  static CBOERefDataLoader &GetUniqueInstance(int32_t const &trading_date) {
    static CBOERefDataLoader unique_instance(trading_date);
    return unique_instance;
  }
  
  void AddComboRefData(std::string combo_shc_){

    int32_t token = HFSAT::CBOEComboTokenGenerator::GetUniqueInstance().GetTokenOrUpdate(combo_shc_);

    if(combo_token_to_cboe_refdata.find(token)!=combo_token_to_cboe_refdata.end()){
      std::cerr<<"CBOE Ref data already added for this combo :"<< combo_shc_ << "and token is: "<<token <<std::endl;
      std::cout<<"Symbol: "<<combo_token_to_cboe_refdata[token].symbol<<std::endl;
      return;
    }

    CBOE_UDP_MDS::CBOERefData ref_data;
    const char *instrument = "CO";
    const char *symbol = getSymbolForCombo(combo_shc_).c_str();
    int64_t expiry = 0;
    double strike = 0;
    const char *option_type = "XX";

    ref_data.Init(token, expiry, strike, instrument, symbol, option_type, HFSAT::kExchSourceCBOE_CO);

    combo_token_to_cboe_refdata[token] = ref_data;
  }
  std::map<int32_t, CBOE_UDP_MDS::CBOERefData> &GetCBOERefData(char const &segment) {
    if (CBOE_FO_SEGMENT_MARKING == segment)
      return fo_token_to_cboe_refdata;
    else if (CBOE_IX_SEGMENT_MARKING == segment)
      return ix_token_to_cboe_refdata;
    else if(CBOE_COMBO_SEGMENT_MARKING == segment){
      return combo_token_to_cboe_refdata;
    }
    return dummy_error_notif_map;
  }

};
}
}
