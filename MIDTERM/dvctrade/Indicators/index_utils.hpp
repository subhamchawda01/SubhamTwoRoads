// =====================================================================================
//
//       Filename:  index_utils.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  01/06/2015 09:17:34 AM
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
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <list>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#define INDEX_CONSTITUENT_LIST_BASE_DIR "/spare/local/tradeinfo/IndexInfo"
#define NSE_INDEX_INFO_DIR "/spare/local/tradeinfo/NSE_Files/IndexInfo"

namespace HFSAT {

std::string GetBovespaIndexConstituentFileList(int yyyymmdd);
void GetFractionInIbovespaIndex(int yyyymmdd, std::vector<double> &contituent_fraction);
void GetTheoreticalVolumeInIbovespaIndex(int yyyymmdd, std::vector<uint64_t> &contituent_fraction);
void GetConstituentListInIbovespaIndex(int yyyymmdd, std::vector<std::string> &const_list);
std::string GetSectorForBMFStock(std::string stock, int fraction_portfolio = 0);

/// NSE functions
void GetIthColumNIFTY(int yyyymmdd, std::vector<double> &value_vec, int idx, const std::string &portname);
std::string GetConstituentListFileNIFTY(int yyyymmdd, const std::string &portname);
void GetConstituentListInNIFTY(int yyyymmdd, std::vector<std::string> &const_list, const std::string &portname);

void GetConstituentFractionInNIFTY(int yyyymmdd, std::vector<double> &constituent_fraction,
                                   const std::string &portname);

void GetConstituentEquityVolNIFTY(int yyyymmdd, std::vector<double> &market_cap, const std::string &portname);

double GetIndexFactorNIFTY(int yyyymmdd, const std::string &portfolio);
}
