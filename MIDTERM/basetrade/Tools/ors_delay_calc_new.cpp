/**
   \file Tools/ors_delay_calc_new.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <signal.h>
#include <getopt.h>
#include <map>
#include <algorithm>
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/file_utils.hpp"  // To create the directory
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/ors_defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"

#include "dvccode/CDef/ors_messages.hpp"

#include "baseinfra/SimMarketMaker/security_delay_stats.hpp"

/**
 * prints the order_info details with sequence, cancel-sequence tag
 *
 * @param order_info
 * @param cxl_order
 */
void PrintStruct(HFSAT::OrsOrderInfo order_info, bool cxl_order) {
  //  Get the delay strings
  std::string cxl_seq_rtt = (order_info.cxl_time_ - order_info.cxlseq_time_).ToString();
  std::string seq_rtt = (order_info.conf_time_ - order_info.seq_time_).ToString();
  std::string seq_time = order_info.seq_time_.ToString();
  std::string mkt_time = order_info.mkt_time_.ToString();
  std::string cxl_seq_time = order_info.cxlseq_time_.ToString();
  std::string cxl_mkt_time = order_info.cxlmkt_time_.ToString();

  if (!cxl_order) {
    printf("SEQ %s %s %s\n", seq_time.c_str(), seq_rtt.c_str(), mkt_time.c_str());
  } else {
    printf("CXL %s %s %s\n", cxl_seq_time.c_str(), cxl_seq_rtt.c_str(), cxl_mkt_time.c_str());
  }
}

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cout << " USAGE: EXEC <shortcode> <tradedate>" << std::endl;
    std::cout << " STDCOUT will contain the entire output, please redirect" << std::endl;
    exit(0);
  }

  std::string shortcode = argv[1];
  int tradingdate = atoi(argv[2]);

  // Utility structs require by SecurityDelayStats

  HFSAT::DebugLogger dbglogger(4 * 1024 * 1024, 256 * 1024);

  HFSAT::Watch watch(dbglogger, tradingdate);
  HFSAT::SecurityNameIndexer& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate);
  const char* exch_symbol = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode);
  auto exch_source = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode, tradingdate);
  auto smv = new HFSAT::SecurityMarketView(dbglogger, watch, sec_name_indexer, shortcode, exch_symbol, 0, exch_source,
                                           false, "INVALIDFILE", "INVALIDFILE", "INVALID");
  if (strncmp(shortcode.c_str(), "NSE_", 4) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate).LoadNSESecurityDefinitions();
  } else if (shortcode.substr(0, 3) == "HK_") {
    HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate).LoadHKStocksSecurityDefinitions();
  }
  // Add to sec_name_indexer to get sec_id required by security_delay_stats
  sec_name_indexer.AddString(exch_symbol, shortcode);

  HFSAT::TradingLocation_t dep_trading_location = HFSAT::TradingLocationUtils::GetTradingLocationExch(exch_source);

  HFSAT::SecurityDelayStats& security_delay_stats =
      HFSAT::SecurityDelayStats::GetUniqueInstance(smv->security_id(), watch, dep_trading_location, 0);

  // Get the maps of time to seqd-conf or time_to cxl_seqd-conf values
  const auto& time_to_seqd_ors_info = security_delay_stats.GetSeqTimeToOrsOrderMap();
  const auto& time_to_cxl_ors_info = security_delay_stats.GetCxlSeqTimeToOrsOrderMap();

  auto seqd_iter = time_to_seqd_ors_info.begin();
  auto cxl_iter = time_to_cxl_ors_info.begin();

  // Print the seqd-conf, cxl_seq-cxl values in chronological order
  while (seqd_iter != time_to_seqd_ors_info.end() && cxl_iter != time_to_cxl_ors_info.end()) {
    if (seqd_iter->first < cxl_iter->first) {
      PrintStruct(seqd_iter->second, false);
      seqd_iter++;
    } else if (seqd_iter->first >= cxl_iter->first) {
      PrintStruct(cxl_iter->second, true);
      cxl_iter++;
    }
  }

  // Print remaining seqd-conf values if at all
  while (seqd_iter != time_to_seqd_ors_info.end()) {
    PrintStruct(seqd_iter->second, false);
    seqd_iter++;
  }

  // Print remaining cxl_seqd-cxl values if at all
  while (cxl_iter != time_to_cxl_ors_info.end()) {
    PrintStruct(cxl_iter->second, true);
    cxl_iter++;
  }

  return 0;
}
