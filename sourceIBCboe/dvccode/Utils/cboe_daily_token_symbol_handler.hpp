// =====================================================================================
//
//       Filename:  cboe_daily_token_symbol_handler.hpp
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
#include <string>
#include "dvccode/Utils/cboe_refdata_loader.hpp"
#include "dvccode/Utils/cboe_spot_token_generator.hpp"
#include "dvccode/Utils/cboe_combo_token_generator.hpp"
#include "dvccode/Utils/lock.hpp"

namespace HFSAT {
namespace Utils {

class CBOEDailyTokenSymbolHandler {
 private:
  CBOERefDataLoader& cboe_ref_data_loader_;
  HFSAT::CBOESpotTokenGenerator& cboe_spot_token_generator_;
  HFSAT::CBOEComboTokenGenerator& cboe_combo_token_generator_;
  std::map<int32_t, std::string> fo_token_to_symbol_mapping_;
  std::map<std::string, int32_t> fo_symbol_to_token_mapping_;
  std::map<int32_t, std::string> ix_token_to_symbol_mapping_;
  std::map<std::string, int32_t> ix_symbol_to_token_mapping_;
  std::map<int32_t, std::string> combo_token_to_symbol_mapping_;
  std::map<std::string, int32_t> combo_symbol_to_token_mapping_;
  std::map<int32_t, std::string> combo_token_to_symbol_mapping_mtx_;
  std::map<std::string, int32_t> combo_symbol_to_token_mapping_mtx_;
  
  char const* dummy_ptr;
  int32_t dummy_token;
  static HFSAT::Lock comboMapLock;

  CBOEDailyTokenSymbolHandler(CBOEDailyTokenSymbolHandler& disabled_copy_constructor) = delete;

  CBOEDailyTokenSymbolHandler(int32_t const& trading_date)
      : cboe_ref_data_loader_(CBOERefDataLoader::GetUniqueInstance(trading_date)),
        cboe_spot_token_generator_(HFSAT::CBOESpotTokenGenerator::GetUniqueInstance()),
        cboe_combo_token_generator_(HFSAT::CBOEComboTokenGenerator::GetUniqueInstance()),
        fo_token_to_symbol_mapping_(),
        fo_symbol_to_token_mapping_(),
        ix_token_to_symbol_mapping_(),
        ix_symbol_to_token_mapping_(),
        combo_token_to_symbol_mapping_(),
        combo_symbol_to_token_mapping_(),
        combo_token_to_symbol_mapping_mtx_(),
        combo_symbol_to_token_mapping_mtx_(),
        dummy_ptr("INVALID"),
        dummy_token(-1)

  {
    // FO
    std::map<int32_t, CBOE_UDP_MDS::CBOERefData> cboe_ref_data = cboe_ref_data_loader_.GetCBOERefData('F');

    if (0 == cboe_ref_data.size()) {
      std::cerr << "FO SEGMENT REF DATA NOT PRESENT" << std::endl;
      exit(-1);
    }

    for (auto& itr : cboe_ref_data) {
      std::ostringstream internal_symbol_str;

      internal_symbol_str << "CBOE"
                          << "_" << (itr.second).symbol << "_" << (itr.second).option_type << "_";
      internal_symbol_str << std::fixed << std::setprecision(2) << (itr.second).strike_price;
      internal_symbol_str << "_" << (itr.second).expiry;

      fo_token_to_symbol_mapping_[itr.first] = internal_symbol_str.str();
      fo_symbol_to_token_mapping_[internal_symbol_str.str()] = itr.first;
    }

    std::unordered_map<std::string, int> spot_data = cboe_spot_token_generator_.GetSpotIndexToTokenMap();
    if (0 == spot_data.size()) {
      std::cerr << "IX SEGMENT REF DATA NOT PRESENT" << std::endl;
      exit(-1);
    }
    for (auto& itr : spot_data) {
      // std::cout << "Spot Index Symbol : " << itr.first << " " << itr.second << std::endl;
      ix_token_to_symbol_mapping_[itr.second] = itr.first;
      ix_symbol_to_token_mapping_[itr.first] = itr.second;
    }

    std::unordered_map<std::string, int> combo_data = cboe_combo_token_generator_.GetComboToTokenMap();
    if (0 == spot_data.size()) {
      std::cerr << "CO SEGMENT REF DATA NOT PRESENT" << std::endl;
      exit(-1);
    }
    for (auto& itr : combo_data) {
      // std::cout << "Spot Index Symbol : " << itr.first << " " << itr.second << std::endl;
      combo_token_to_symbol_mapping_[itr.second] = itr.first;
      combo_symbol_to_token_mapping_[itr.first] = itr.second;
    }
  }

  int64_t GetTokenFromComboSymbolMtx(char const* internal_symbol) {

    int64_t resultToken=dummy_token;
    CBOEDailyTokenSymbolHandler::comboMapLock.LockMutex();
    
    if(combo_symbol_to_token_mapping_mtx_.find(internal_symbol)!=combo_symbol_to_token_mapping_mtx_.end())
      resultToken=combo_symbol_to_token_mapping_mtx_[internal_symbol];
    
    CBOEDailyTokenSymbolHandler::comboMapLock.UnlockMutex();

    return resultToken;
  }

  const char* GetComboSymbolFromTokenMtx(int64_t const& token) {
    std::string symbol="INVALID";
    CBOEDailyTokenSymbolHandler::comboMapLock.LockMutex();
    
    if(combo_token_to_symbol_mapping_mtx_.find(token)!=combo_token_to_symbol_mapping_mtx_.end())
      symbol=combo_token_to_symbol_mapping_mtx_[token];
    
    CBOEDailyTokenSymbolHandler::comboMapLock.UnlockMutex();

    return symbol.c_str();
  }

  void UpdateComboTokenAndSymbolMapMtx(int64_t const& token,char const* symbol){
    CBOEDailyTokenSymbolHandler::comboMapLock.LockMutex();
    combo_symbol_to_token_mapping_mtx_[symbol]=token;
    combo_token_to_symbol_mapping_mtx_[token]=symbol;
    CBOEDailyTokenSymbolHandler::comboMapLock.UnlockMutex();
  }

  void UpdateComboTokenAndSymbolMap(int64_t const& token,char const* symbol){
    if(combo_symbol_to_token_mapping_.find(symbol)!=combo_symbol_to_token_mapping_.end()){
      std::cerr<<"Already have an entry for the required symbol"<<std::endl;
      return;
    }
    combo_symbol_to_token_mapping_[symbol]=token;
    combo_token_to_symbol_mapping_[token]=symbol;
  }
 public:
  static CBOEDailyTokenSymbolHandler& GetUniqueInstance(int32_t const& trading_date) {
    static CBOEDailyTokenSymbolHandler unique_instance(trading_date);
    return unique_instance;
  }

  //Not used yet might use when adding combo while trading is already happening
  void UpdateTokenAndSymbolMapMtx(int64_t const& token,char const* internal_symbol, char const& segment){
    if(CBOE_COMBO_SEGMENT_MARKING == segment){
      UpdateComboTokenAndSymbolMapMtx(token,internal_symbol);
    }else{
      std::cerr<<"This segments is not supported yet"<<std::endl;
    }
  }
  void UpdateTokenAndSymbolMap(int64_t token,const char* internal_symbol, char segment){
    if(CBOE_COMBO_SEGMENT_MARKING == segment){
      UpdateComboTokenAndSymbolMap(token,internal_symbol);
    }else{
      std::cerr<<"This segments is not supported yet"<<std::endl;
    }
  }
  //CBOE_COMBO_SEGMENT_MARKING
  int64_t GetTokenFromInternalSymbol(char const* internal_symbol, char const& segment) {

    if (CBOE_FO_SEGMENT_MARKING == segment) {
      return (fo_symbol_to_token_mapping_.find(std::string(internal_symbol)) == fo_symbol_to_token_mapping_.end()
                  ? dummy_token
                  : fo_symbol_to_token_mapping_[std::string(internal_symbol)]);
    } else if (CBOE_IX_SEGMENT_MARKING == segment) {
      return (ix_symbol_to_token_mapping_.find(std::string(internal_symbol)) == ix_symbol_to_token_mapping_.end()
                  ? dummy_token
                  : ix_symbol_to_token_mapping_[std::string(internal_symbol)]);
    } else if(CBOE_COMBO_SEGMENT_MARKING == segment){
      return (combo_symbol_to_token_mapping_.find(std::string(internal_symbol)) == combo_symbol_to_token_mapping_.end()
                  ? dummy_token
                  : combo_symbol_to_token_mapping_[std::string(internal_symbol)]);
    }

    return dummy_token;
  }

  char const* GetInternalSymbolFromToken(int64_t const& token, char const& segment) {
    if (fo_token_to_symbol_mapping_.find(token) != fo_token_to_symbol_mapping_.end()) {
          return (fo_token_to_symbol_mapping_[token]).c_str();
    } else if (CBOE_IX_SEGMENT_MARKING == segment) {
      return (ix_token_to_symbol_mapping_.find(token) == ix_token_to_symbol_mapping_.end()
                  ? dummy_ptr
                  : (ix_token_to_symbol_mapping_[token]).c_str());
    } else if(CBOE_COMBO_SEGMENT_MARKING == segment){
      return (combo_token_to_symbol_mapping_.find(token) == combo_token_to_symbol_mapping_.end()
                  ? dummy_ptr
                  : (combo_token_to_symbol_mapping_[token]).c_str());
    }

    return dummy_ptr;
  }
};

}
}

