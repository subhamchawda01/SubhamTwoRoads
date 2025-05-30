// =====================================================================================
//
//       Filename:  Tools/check_cme_ors_mkt_arb_instances.cpp
//
//    Description:  Utility Functions for fetching the Sample Data values from the last 30 days
//
//        Version:  1.0
//        Created:  01/15/2016 12:38:39 PM
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

#include "baseinfra/MarketAdapter/cme_market_book_arbitrator.hpp"
#include "baseinfra/LoggedSources/cme_obf_logged_message_filesource.hpp"

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

#include "dvccode/ExternalData/historical_dispatcher.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cout << " <shortcode> <tradingdate>" << std::endl;
    exit(0);
  }

  std::string shortcode = std::string(argv[1]);
  int tradingdate = atoi(argv[2]);
  auto &shortcode_ors_data_filesource_map_ = HFSAT::ShortcodeORSMessageFilesourceMap::GetUniqueInstance();
  auto &sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  HFSAT::DebugLogger dbglogger(1024 * 1024, 1024 * 8);
  HFSAT::Watch watch(dbglogger, tradingdate);
  HFSAT::HistoricalDispatcher historical_dispatcher_;

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate);

  const char *exchange_symbol = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode);
  sec_name_indexer.AddString(exchange_symbol, shortcode);

  auto sec_id = sec_name_indexer.GetIdFromString(shortcode);

  HFSAT::ORSMessageFileSource *p_ors_message_filesource_ = new HFSAT::ORSMessageFileSource(
      dbglogger, sec_name_indexer, tradingdate, sec_id, exchange_symbol, HFSAT::kTLocCHI);

  p_ors_message_filesource_->SetExternalTimeListener(&watch);

  std::vector<HFSAT::MarketOrdersView *> sid_to_mov_ptr_map;
  sid_to_mov_ptr_map.resize(1, nullptr);

  HFSAT::MarketOrdersView *mov = HFSAT::MarketOrdersView::GetUniqueInstance(dbglogger, watch, sec_id);

  sid_to_mov_ptr_map[0] = mov;

  auto cme_book_arbitrator = new HFSAT::CMEMarketBookArbitrator(dbglogger, watch, sec_name_indexer, sid_to_mov_ptr_map);

  // Add confirmed and executed listener
  p_ors_message_filesource_->AddOrderConfirmedListener(cme_book_arbitrator);
  p_ors_message_filesource_->AddOrderExecutedListener(cme_book_arbitrator);

  shortcode_ors_data_filesource_map_.AddEntry(shortcode, p_ors_message_filesource_);

  historical_dispatcher_.AddExternalDataListener(p_ors_message_filesource_);

  auto filesource = new HFSAT::CMEOBFLoggedMessageFileSource(dbglogger, sec_name_indexer, tradingdate, sec_id,
                                                             exchange_symbol, HFSAT::kTLocCHI, false);

  // Update watch on order-book data as well
  filesource->SetExternalTimeListener(&watch);

  // Add as arbitrator as listener to filesource
  filesource->AddOrderLevelListener(cme_book_arbitrator);

  // Add filesource in historical dispatcher
  historical_dispatcher_.AddExternalDataListener(filesource, true);

  historical_dispatcher_.RunHist();

  return 0;
}
