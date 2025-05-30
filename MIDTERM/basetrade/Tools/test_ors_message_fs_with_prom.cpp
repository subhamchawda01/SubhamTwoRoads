// CHecking for bug in the prom_order_manager and ors_message_filesource

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/random_channel.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"

#include "dvccode/ExternalData/historical_dispatcher.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "baseinfra/SmartOrderRouting/prom_pnl.hpp"

#include "dvccode/ExternalData/historical_dispatcher.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"

#include "baseinfra/LoggedSources/ors_message_filesource.hpp"
#include "dvccode/ORSMessages/ors_message_listener.hpp"
#include "dvccode/ORSMessages/ors_message_livesource.hpp"

#include "dvccode/CDef/ors_messages.hpp"
#include "baseinfra/OrderRouting/prom_order_manager.hpp"

// TEsting PromOrderManger and ORSMessageFileSrc
class TestPromWithORS : public HFSAT::GlobalOrderChangeListener

{
 public:
  TestPromWithORS(HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_,
                  HFSAT::PromOrderManager* _p_prom_order_manager_, HFSAT::ORSMessageFileSource* _ors_message_filesrc_,
                  HFSAT::PromPNL* t_prom_pnl_)
      : dbglogger_(_dbglogger_),
        watch_(_watch_),
        p_prom_order_manager_(_p_prom_order_manager_),
        ors_message_filesrc_(_ors_message_filesrc_),
        prom_pnl_(t_prom_pnl_) {
    // public OrderSequencedListener, public OrderConfirmedListener, public OrderConfCxlReplacedListener, public
    // OrderCanceledListener, public OrderExecutedListener
    //  {
    ors_message_filesrc_->AddOrderSequencedListener(p_prom_order_manager_);
    ors_message_filesrc_->AddOrderConfirmedListener(p_prom_order_manager_);
    ors_message_filesrc_->AddOrderConfCxlReplacedListener(p_prom_order_manager_);
    ors_message_filesrc_->AddOrderCanceledListener(p_prom_order_manager_);
    ors_message_filesrc_->AddOrderExecutedListener(p_prom_order_manager_);
    p_prom_order_manager_->AddGlobalOrderChangeListener(this);
    p_prom_order_manager_->ManageOrdersAlso();
  }

  ~TestPromWithORS() {}

  // Definitely implement
  void OnGlobalOrderChange(const unsigned int _security_id_, const HFSAT::TradeType_t _buysell_, const int _int_price_);

 private:
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::Watch& watch_;
  HFSAT::PromOrderManager* p_prom_order_manager_;
  HFSAT::ORSMessageFileSource* ors_message_filesrc_;
  HFSAT::PromPNL* prom_pnl_;
  std::map<unsigned int, int> sid_to_best_bid_size;
  std::map<unsigned int, int> sid_to_best_ask_size;
  std::map<unsigned int, int> sid_to_best_bid_price;
  std::map<unsigned int, int> sid_to_best_ask_price;
};

void TestPromWithORS::OnGlobalOrderChange(const unsigned int _security_id_, const HFSAT::TradeType_t _buysell_,
                                          const int _int_price_) {
  const HFSAT::BidPriceSizeMap& intpx_2_sum_bid_confirmed_ = p_prom_order_manager_->intpx_2_sum_bid_confirmed();
  const HFSAT::AskPriceSizeMap& intpx_2_sum_ask_confirmed_ = p_prom_order_manager_->intpx_2_sum_ask_confirmed();

  // Best Bid, Assuming sorted decreasing
  HFSAT::BidPriceSizeMapConstIter_t intpx_2_sum_bid_confirmed_iter = intpx_2_sum_bid_confirmed_.begin();
  // Best Ask, Assuming sorted increasing
  HFSAT::AskPriceSizeMapConstIter_t intpx_2_sum_ask_confirmed_iter = intpx_2_sum_ask_confirmed_.begin();

  // We need to find out the first entry in the map with nonzero confirmed order
  // Max bidprice for nonzero size , min askprice for nonzero size
  int best_bid_size = -1;
  int best_bid_price = -1;
  int best_ask_size = -1;
  int best_ask_price = -1;

  dbglogger_ << "BID MAP: "
             << "----------\n";

  for (intpx_2_sum_bid_confirmed_iter = intpx_2_sum_bid_confirmed_.begin();
       intpx_2_sum_bid_confirmed_iter != intpx_2_sum_bid_confirmed_.end(); intpx_2_sum_bid_confirmed_iter++) {
    dbglogger_ << "SIZE : " << intpx_2_sum_bid_confirmed_iter->second
               << " PRICE: " << intpx_2_sum_bid_confirmed_iter->first << "\n";
    dbglogger_.DumpCurrentBuffer();
  }

  dbglogger_ << "ASK MAP: "
             << "----------\n";

  for (intpx_2_sum_ask_confirmed_iter = intpx_2_sum_ask_confirmed_.begin();
       intpx_2_sum_ask_confirmed_iter != intpx_2_sum_ask_confirmed_.end(); intpx_2_sum_ask_confirmed_iter++) {
    dbglogger_ << "SIZE : " << intpx_2_sum_ask_confirmed_iter->second
               << " PRICE: " << intpx_2_sum_ask_confirmed_iter->first << "\n";
    dbglogger_.DumpCurrentBuffer();
  }

  for (intpx_2_sum_bid_confirmed_iter = intpx_2_sum_bid_confirmed_.begin();
       intpx_2_sum_bid_confirmed_iter != intpx_2_sum_bid_confirmed_.end(); intpx_2_sum_bid_confirmed_iter++) {
    if (intpx_2_sum_bid_confirmed_iter->second > 0) {
      best_bid_size = intpx_2_sum_bid_confirmed_iter->second;
      best_bid_price = intpx_2_sum_bid_confirmed_iter->first;
      break;
    }
  }

  for (intpx_2_sum_ask_confirmed_iter = intpx_2_sum_ask_confirmed_.begin();
       intpx_2_sum_ask_confirmed_iter != intpx_2_sum_ask_confirmed_.end(); intpx_2_sum_ask_confirmed_iter++) {
    if (intpx_2_sum_ask_confirmed_iter->second > 0) {
      best_ask_size = intpx_2_sum_ask_confirmed_iter->second;
      best_ask_price = intpx_2_sum_ask_confirmed_iter->first;
      break;
    }
  }

  sid_to_best_bid_size[_security_id_] = best_bid_size;
  sid_to_best_ask_size[_security_id_] = best_ask_size;
  sid_to_best_bid_price[_security_id_] = best_bid_price;
  sid_to_best_ask_price[_security_id_] = best_ask_price;

  dbglogger_ << "BBS : " << best_bid_size << " BBP : " << best_bid_price << " BAS :" << best_ask_size
             << " BAP : " << best_ask_price << "\n";
  dbglogger_.DumpCurrentBuffer();

  dbglogger_ << "DIFF : " << best_ask_price - best_bid_price << "\n";
  dbglogger_.DumpCurrentBuffer();

  dbglogger_ << " PNL : " << prom_pnl_->total_pnl() << "\n";
  dbglogger_.DumpCurrentBuffer();
}

void ParseCommandLineOption(int argc, char** argv, std::string& shortcode_, int& yyyymmdd) {
  if (argc < 3) {
    std::cout << "USAGE: SHORTCODE(ZN_0 ) YYYYMMDD " << std::endl;
    exit(-1);
  }

  if (argc >= 3) {
    shortcode_ = std::string(argv[1]);
    yyyymmdd = atoi(argv[2]);
  }
}

int main(int argc, char** argv) {
  std::string shortcode = "";
  int yyyymmdd = 0;
  ParseCommandLineOption(argc, argv, shortcode, yyyymmdd);

  std::cout << "SHORTCODE :" << shortcode << " : " << yyyymmdd << std::endl;
  // Get ShortCode to ExchangeSYmbol
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(yyyymmdd);

  std::string exch_symbol = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode);
  HFSAT::ExchSource_t dep_exch_source_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode, yyyymmdd);

  // Setup logger & watch
  HFSAT::DebugLogger dbglogger_(1024000, 1);

  {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/logs/alllogs/testing_ors_mslv_prom." << yyyymmdd;
    std::string logfilename_ = t_temp_oss_.str();
    dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out);
  }

  dbglogger_.AddLogLevel(BOOK_ERROR);
  dbglogger_.AddLogLevel(BOOK_INFO);
  dbglogger_.AddLogLevel(BOOK_TEST);

  HFSAT::Watch watch_(dbglogger_, yyyymmdd);

  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  sec_name_indexer_.AddString(exch_symbol.c_str(), shortcode);

  unsigned int sec_id = sec_name_indexer_.GetIdFromString(shortcode);

  HFSAT::HistoricalDispatcher historical_dispatcher_;

  HFSAT::TradingLocation_t trading_loc_ = HFSAT::TradingLocationUtils::GetTradingLocationExch(dep_exch_source_);

  HFSAT::ORSMessageFileSource* ors_message_filesrc = NULL;

  // Setup ors_message_filesource
  ors_message_filesrc = new HFSAT::ORSMessageFileSource(dbglogger_, sec_name_indexer_, yyyymmdd, sec_id,
                                                        exch_symbol.c_str(), trading_loc_);
  ors_message_filesrc->SetExternalTimeListener(&watch_);

  historical_dispatcher_.AddExternalDataListener(ors_message_filesrc);

  // Setup prom_order_manager

  HFSAT::PromOrderManager* p_prom_order_manager_ = NULL;

  p_prom_order_manager_ = HFSAT::PromOrderManager::GetUniqueInstance(dbglogger_, watch_, sec_name_indexer_, shortcode,
                                                                     sec_id, exch_symbol.c_str());

  HFSAT::BulkFileWriter bulkwriter("/spare/local/logs/alllogs/test_ors_pnl_BULK.log");

  bool set_temporary_bool_checking_if_this_is_an_indexed_book_ = false;
  if ((dep_exch_source_ == HFSAT::kExchSourceLIFFE) || (dep_exch_source_ == HFSAT::kExchSourceEUREX)
      // || ( dep_exch_source_ == HFSAT::kExchSourceCME )
      ) {
    set_temporary_bool_checking_if_this_is_an_indexed_book_ = true;
  }

  // if ( ( shortcode.compare ( 0 , 3 , "LFI" ) == 0 ) ||
  //      ( shortcode.compare ( 0 , 6 , "SP_LFI" ) == 0 ) ||
  //      ( shortcode.compare ( 0 , 6 , "SP_LFL" ) == 0 ) ||
  //      ( shortcode.compare ( 0 , 3 , "LFL" ) == 0 ) )
  //   { // Using original liffe-book for products requiring order-level computations ...
  //     // currently only in simtrader
  //     set_temporary_bool_checking_if_this_is_an_indexed_book_ = false ;
  //   }
  HFSAT::SecurityMarketView* p_smv_ =
      new HFSAT::SecurityMarketView(dbglogger_, watch_, sec_name_indexer_, shortcode, exch_symbol.c_str(), sec_id,
                                    dep_exch_source_, set_temporary_bool_checking_if_this_is_an_indexed_book_);

  HFSAT::PromPNL* prom_pnl = new HFSAT::PromPNL(dbglogger_, watch_, *p_prom_order_manager_, *p_smv_,
                                                101,  // TODO {} runtime_id_
                                                bulkwriter);

  // Setup the main class

  TestPromWithORS test_prom_with_ors =
      TestPromWithORS(dbglogger_, watch_, p_prom_order_manager_, ors_message_filesrc, prom_pnl);

  try {
    historical_dispatcher_.RunHist();
  } catch (int e) {
  }

  return 0;
}
