// =====================================================================================
//
//       Filename:  bse_daily_token_symbol_handler.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  08/13/2015 05:53:54 AM
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
#include "dvccode/Utils/bse_refdata_loader.hpp"
#include "dvccode/Utils/bse_spot_token_generator.hpp"

#define EPOCH_TO_BSE_START_DATE_OFFSET 315532800

namespace HFSAT {
namespace Utils {

static std::string ConvertBSEExpiryInSecToDate(int32_t const& seconds_since_1980) {

  time_t time_computed = seconds_since_1980; // NOT REQUIRED + EPOCH_TO_BSE_START_DATE_OFFSET;
  struct tm* date_time_ptr = gmtime(&time_computed);

  std::ostringstream date_str;
  date_str << (date_time_ptr->tm_year + 1900) << std::setw(2) << std::setfill('0') << (date_time_ptr->tm_mon + 1)
           << std::setw(2) << std::setfill('0') << (date_time_ptr->tm_mday);

  return date_str.str();
}

class BSEDailyTokenSymbolHandler {
 private:
  BSERefDataLoader& bse_ref_data_loader_;
  HFSAT::BSESpotTokenGenerator& bse_spot_token_generator_;
  std::map<int32_t, std::string> eq_token_to_symbol_mapping_;
  std::map<std::string, int32_t> eq_symbol_to_token_mapping_;
  std::map<int32_t, std::string> fo_token_to_symbol_mapping_;
  std::map<std::string, int32_t> fo_symbol_to_token_mapping_;
  std::map<int32_t, std::string> cd_token_to_symbol_mapping_;
  std::map<std::string, int32_t> cd_symbol_to_token_mapping_;
  std::map<int32_t, std::string> ix_token_to_symbol_mapping_;
  std::map<std::string, int32_t> ix_symbol_to_token_mapping_;
  char const* dummy_ptr;
  int32_t dummy_token;

  BSEDailyTokenSymbolHandler(BSEDailyTokenSymbolHandler& disabled_copy_constructor) = delete;

  BSEDailyTokenSymbolHandler(int32_t const& trading_date)
      : bse_ref_data_loader_(BSERefDataLoader::GetUniqueInstance(trading_date)),
        bse_spot_token_generator_(HFSAT::BSESpotTokenGenerator::GetUniqueInstance()),
        eq_token_to_symbol_mapping_(),
        eq_symbol_to_token_mapping_(),
        fo_token_to_symbol_mapping_(),
        fo_symbol_to_token_mapping_(),
        cd_token_to_symbol_mapping_(),
        cd_symbol_to_token_mapping_(),
        dummy_ptr("INVALID"),
        dummy_token(-1)

  {
    // FO
    std::map<int32_t, BSE_UDP_MDS::BSERefData> bse_ref_data = bse_ref_data_loader_.GetBSERefData('F');

    if (0 == bse_ref_data.size()) {
      std::cerr << "FO SEGMENT REF DATA NOT PRESENT" << std::endl;
      exit(-1);
    }

    for (auto& itr : bse_ref_data) {
      std::ostringstream internal_symbol_str;

      if (std::string("XX") == std::string((itr.second).option_type)) {
        internal_symbol_str << "BSE"
                            << "_" << (itr.second).symbol << "_FUT_"
                            << ConvertBSEExpiryInSecToDate((itr.second).expiry);

      } else {
        internal_symbol_str << "BSE"
                            << "_" << (itr.second).symbol << "_" << (itr.second).option_type << "_";
        internal_symbol_str << std::fixed << std::setprecision(2) << (itr.second).strike_price;
        internal_symbol_str << "_" << ConvertBSEExpiryInSecToDate((itr.second).expiry);
      }

      fo_token_to_symbol_mapping_[itr.first] = internal_symbol_str.str();
      fo_symbol_to_token_mapping_[internal_symbol_str.str()] = itr.first;
      ix_token_to_symbol_mapping_[itr.first] = internal_symbol_str.str();
      ix_symbol_to_token_mapping_[internal_symbol_str.str()] = itr.first;
    }

    // CD
    bse_ref_data = bse_ref_data_loader_.GetBSERefData('C');

    if (0 == bse_ref_data.size()) {
      std::cerr << "CD SEGMENT REF DATA NOT PRESENT" << std::endl;
      exit(-1);
    }

    for (auto& itr : bse_ref_data) {
      std::ostringstream internal_symbol_str;

      if (std::string("XX") == std::string((itr.second).option_type)) {
        internal_symbol_str << "BSE"
                            << "_" << (itr.second).symbol << "_FUT_"
                            << ConvertBSEExpiryInSecToDate((itr.second).expiry);

      } else {
        internal_symbol_str << "BSE"
                            << "_" << (itr.second).symbol << "_" << (itr.second).option_type << "_";
        internal_symbol_str << std::fixed << std::setprecision(2) << (itr.second).strike_price;
        internal_symbol_str << "_" << ConvertBSEExpiryInSecToDate((itr.second).expiry);
      }

      cd_token_to_symbol_mapping_[itr.first] = internal_symbol_str.str();
      cd_symbol_to_token_mapping_[internal_symbol_str.str()] = itr.first;
    }

    // EQ
    bse_ref_data = bse_ref_data_loader_.GetBSERefData('E');

    if (0 == bse_ref_data.size()) {
      std::cerr << "EQ SEGMENT REF DATA NOT PRESENT" << std::endl;
      exit(-1);
    }

    for (auto& itr : bse_ref_data) {
      std::ostringstream internal_symbol_str;

      internal_symbol_str << "BSE"
                          << "_" << (itr.second).symbol;

      eq_token_to_symbol_mapping_[itr.first] = internal_symbol_str.str();
      eq_symbol_to_token_mapping_[internal_symbol_str.str()] = itr.first;
    }

    std::unordered_map<std::string, int> spot_data = bse_spot_token_generator_.GetSpotIndexToTokenMap();
    if (0 == bse_ref_data.size()) {
      std::cerr << "IX SEGMENT REF DATA NOT PRESENT" << std::endl;
      exit(-1);
    }
    for (auto& itr : spot_data) {

      ix_token_to_symbol_mapping_[itr.second] = itr.first;
      ix_symbol_to_token_mapping_[itr.first] = itr.second;
    }

  }

 public:
  static BSEDailyTokenSymbolHandler& GetUniqueInstance(int32_t const& trading_date) {
    static BSEDailyTokenSymbolHandler unique_instance(trading_date);
    return unique_instance;
  }

  int64_t GetTokenFromInternalSymbol(char const* internal_symbol, char const& segment) {
    if (BSE_FO_SEGMENT_MARKING == segment) {
      return (fo_symbol_to_token_mapping_.find(std::string(internal_symbol)) == fo_symbol_to_token_mapping_.end()
                  ? dummy_token
                  : fo_symbol_to_token_mapping_[std::string(internal_symbol)]);

    } else if (BSE_CD_SEGMENT_MARKING == segment) {
      return (cd_symbol_to_token_mapping_.find(std::string(internal_symbol)) == cd_symbol_to_token_mapping_.end()
                  ? dummy_token
                  : cd_symbol_to_token_mapping_[std::string(internal_symbol)]);

    } else if (BSE_EQ_SEGMENT_MARKING == segment) {
      return (eq_symbol_to_token_mapping_.find(std::string(internal_symbol)) == eq_symbol_to_token_mapping_.end()
                  ? dummy_token
                  : eq_symbol_to_token_mapping_[std::string(internal_symbol)]);
    }
    else if (BSE_IX_SEGMENT_MARKING == segment) {
      return (ix_symbol_to_token_mapping_.find(std::string(internal_symbol)) == ix_symbol_to_token_mapping_.end()
                  ? dummy_token
                  : ix_symbol_to_token_mapping_[std::string(internal_symbol)]);
    }

    return dummy_token;
  }

  char const* GetInternalSymbolFromToken(int64_t const& token, char const& segment) {
    if (fo_token_to_symbol_mapping_.find(token) != fo_token_to_symbol_mapping_.end())
          return (fo_token_to_symbol_mapping_[token]).c_str();
    else if (cd_token_to_symbol_mapping_.find(token) != cd_token_to_symbol_mapping_.end())
          return (cd_token_to_symbol_mapping_[token]).c_str();
    else if (eq_token_to_symbol_mapping_.find(token) != eq_token_to_symbol_mapping_.end())
          return (eq_token_to_symbol_mapping_[token]).c_str();
    else if (BSE_IX_SEGMENT_MARKING == segment) {
      return (ix_token_to_symbol_mapping_.find(token) == ix_token_to_symbol_mapping_.end()
                  ? dummy_ptr
                  : (ix_token_to_symbol_mapping_[token]).c_str());
    }
    

    return dummy_ptr;
  }
};
}
}
