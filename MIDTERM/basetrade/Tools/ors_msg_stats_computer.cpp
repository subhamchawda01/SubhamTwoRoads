/**
    \file ors_msg_stats_calculator.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include "baseinfra/LoggedSources/ors_message_stats_computer.hpp"
#include "baseinfra/LoggedSources/ors_message_filesource.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/ExternalData/historical_dispatcher.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "usage: " << argv[0] << " <short_code> <trading_date_yyyymmdd>\n";
    exit(0);
  }

  std::string short_code_ = argv[1];
  int tradingdate_ = atoi(argv[2]);

  HFSAT::HistoricalDispatcher historical_dispatcher_;
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  HFSAT::DebugLogger dbglogger_(128 * 1024);  // 128 kB

  const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(short_code_);
  sec_name_indexer_.AddString(exchange_symbol_, short_code_);
  HFSAT::TradingLocation_t trading_location_ = HFSAT::TradingLocationUtils::GetTradingLocationExch(
      HFSAT::SecurityDefinitions::GetContractExchSource(short_code_, tradingdate_));
  HFSAT::ORSMessageFileSource* p_ors_message_file_source_ = new HFSAT::ORSMessageFileSource(
      dbglogger_, sec_name_indexer_, tradingdate_, sec_name_indexer_.GetIdFromChar16(exchange_symbol_),
      exchange_symbol_, trading_location_);

  HFSAT::Watch watch_(dbglogger_, tradingdate_);

  p_ors_message_file_source_->SetExternalTimeListener(&watch_);

  historical_dispatcher_.AddExternalDataListener(p_ors_message_file_source_);

  HFSAT::ORSMessageStatsComputer* p_ors_message_stats_computer_ =
      new HFSAT::ORSMessageStatsComputer(dbglogger_, watch_, false);
  p_ors_message_stats_computer_->ComputeRejectionPeriods(true);

  p_ors_message_file_source_->AddOrderRejectedListener(p_ors_message_stats_computer_);

  try {
    historical_dispatcher_.RunHist();
  } catch (int e) {
  }

  std::map<HFSAT::ttime_t, unsigned> time_to_num_rejections_ =
      std::map<HFSAT::ttime_t, unsigned>(p_ors_message_stats_computer_->TimeToNumRejections());
}
