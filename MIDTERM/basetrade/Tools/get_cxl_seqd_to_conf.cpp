/**
   \file Tools/get_cxl_seqd_to_conf.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
*/
#include <iostream>
#include <stdlib.h>

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "baseinfra/LoggedSources/ors_message_stats_computer.hpp"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cout << " USAGE " << argv[0] << " SHORTCODE TRADINGDATE [ OUTPUTDIR ]" << std::endl;
    exit(0);
  }

  std::string shortcode_ = argv[1];
  const int tradingdate_ = atoi(argv[2]);

  std::string output_dir_ = ((argc > 3) ? argv[3] : "/spare/local/logs/alllogs");

  std::ostringstream t_oss_;
  t_oss_ << output_dir_ << "/glcstc_log." << shortcode_ << "." << tradingdate_;
  std::string filename_ = t_oss_.str();

  HFSAT::DebugLogger dbglogger_(1024000, 1);
  dbglogger_.OpenLogFile(filename_.c_str(), std::ofstream::out);

  dbglogger_.AddLogLevel(PVM_ERROR);
  dbglogger_.AddLogLevel(PVM_INFO);

  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

  sec_name_indexer_.AddString(exchange_symbol_, shortcode_);

  HFSAT::ExchSource_t exchange_source_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, tradingdate_);
  HFSAT::TradingLocation_t trading_location_ = HFSAT::TradingLocationUtils::GetTradingLocationExch(exchange_source_);

  HFSAT::ORSMessageStatsComputer::GetCxlSeqdToConfTimes(dbglogger_, sec_name_indexer_, tradingdate_, 0,
                                                        trading_location_,
                                                        true);  // Print verification info. to generate plots from.

  std::cout << filename_ << std::endl;

  return 0;
}
