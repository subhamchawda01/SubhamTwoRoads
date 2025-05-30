/**
   \file CommonDataStructures/simple_security_symbol_indexer.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   India
   +91 80 4190 3551
*/
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "dvccode/Utils/thread.hpp"

#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/ttime.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"

#include "dvccode/Utils/bulk_file_reader.hpp"

#include "dvccode/ExternalData/historical_dispatcher.hpp"
#include "baseinfra/LoggedSources/eobi_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/eobi_price_feed_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ose_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ose_pricefeed_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ice_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/hkomd_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/asx_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/asx_pf_logged_message_filesource.hpp"

#include "baseinfra/MarketAdapter/market_adapter_list.hpp"

#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 22141225

class TestBook : public HFSAT::Thread,
                 public HFSAT::QueuePositionChangeListener,
                 public HFSAT::SecurityMarketViewChangeListener {
  HFSAT::MarketOrdersView* mov_;
  HFSAT::SecurityMarketView* smv_;
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::Watch& watch_;
  bool day_over_;

 public:
  TestBook(HFSAT::MarketOrdersView* this_mov_, HFSAT::SecurityMarketView* this_smv_, HFSAT::DebugLogger& _dbglogger_,
           HFSAT::Watch& _watch_)
      : mov_(this_mov_), smv_(this_smv_), dbglogger_(_dbglogger_), watch_(_watch_), day_over_(false) {}

  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) {
    if (mov_->IsReady()) {
      std::cout << watch_.tv() << " OnMarketUpdate " << mov_->GetMarketString() << " "
                << smv_->MarketUpdateInfoToString() << std::endl;
    }
  }

  inline void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                           const HFSAT::MarketUpdateInfo& _market_update_info_) {
    if (mov_->IsReady()) {
      std::cout << watch_.tv() << " OnTradePrint " << mov_->GetMarketString() << " " << _trade_print_info_.ToString()
                << " " << smv_->MarketUpdateInfoToString() << std::endl;
    }
  }

  void QueuePosChange(HFSAT::QueuePositionUpdate position_update_) {
    if (mov_->IsReady()) {
      std::cout << watch_.tv() << " OrderChange " << mov_->GetMarketString() << " " << smv_->MarketUpdateInfoToString()
                << std::endl;
    }
  }

  void thread_main() {
    while (!day_over_) {
      sleep(1);
    }
  }

  void DayOver() { day_over_ = true; }
};

void PrintUsage() { std::cout << "Usage: EXEC SHORTCODE YYYYMMDD" << std::endl; }

int main(int argc, char** argv) {
  if (argc < 3) {
    PrintUsage();
    exit(-1);
  }

  std::string shortcode_ = argv[1];
  int tradingdate_ = atoi(argv[2]);

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

  HFSAT::DebugLogger dbglogger_(1024000, 1);
  // setup DebugLogger
  {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/logs/alllogs/print_queue_pos." << shortcode_ << "." << tradingdate_;
    std::string logfilename_ = t_temp_oss_.str();
    dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out);
  }

  HFSAT::Watch watch_(dbglogger_, tradingdate_);
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  sec_name_indexer_.AddString(exchange_symbol_, shortcode_);
  HFSAT::ExchSource_t exch_source_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, tradingdate_);
  HFSAT::TradingLocation_t r_trading_location_ = HFSAT::TradingLocationUtils::GetTradingLocationExch(exch_source_);

  HFSAT::MarketOrdersView* p_mov_ = new HFSAT::MarketOrdersView(dbglogger_, watch_, 0);

  std::vector<HFSAT::MarketOrdersView*> market_orders_view_map_;
  market_orders_view_map_.push_back(p_mov_);

  HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_ = HFSAT::sid_to_security_market_view_map();

  bool set_temporary_bool_checking_if_this_is_an_indexed_book_ =
      HFSAT::CommonSimIndexedBookBool(exch_source_, shortcode_, r_trading_location_);

  HFSAT::SecurityMarketView* p_smv_ =
      new HFSAT::SecurityMarketView(dbglogger_, watch_, sec_name_indexer_, shortcode_, exchange_symbol_, 0,
                                    exch_source_, set_temporary_bool_checking_if_this_is_an_indexed_book_);
  sid_to_smv_ptr_map_.push_back(p_smv_);

  p_smv_->market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_ = true;
  p_smv_->InitializeSMVForIndexedBook();

  HFSAT::AsxMarketOrderManager* asx_market_order_manager =
      new HFSAT::AsxMarketOrderManager(dbglogger_, watch_, sec_name_indexer_, market_orders_view_map_);
  HFSAT::EOBIMarketOrderManager* p_eurex_market_order_manager_ =
      new HFSAT::EOBIMarketOrderManager(dbglogger_, watch_, sec_name_indexer_, market_orders_view_map_);
  HFSAT::OseMarketOrderManager* p_ose_market_order_manager_ =
      new HFSAT::OseMarketOrderManager(dbglogger_, watch_, sec_name_indexer_, market_orders_view_map_);
  HFSAT::IceMarketOrderManager* p_ice_market_order_manager_ =
      new HFSAT::IceMarketOrderManager(dbglogger_, watch_, sec_name_indexer_, market_orders_view_map_);
  HFSAT::HKMarketOrderManager* p_hk_market_order_manager_ =
      new HFSAT::HKMarketOrderManager(dbglogger_, watch_, sec_name_indexer_, market_orders_view_map_);
  HFSAT::NtpMarketOrderManager* p_ntp_market_order_manager_ =
      new HFSAT::NtpMarketOrderManager(dbglogger_, watch_, sec_name_indexer_, market_orders_view_map_);

  HFSAT::IndexedEobiPriceLevelMarketViewManager indexed_eobi_price_level_market_view_manager_(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);
  HFSAT::IndexedOsePriceFeedMarketViewManager indexed_ose_pricefeed_market_view_manager_(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);
  HFSAT::IndexedIceMarketViewManager indexed_ice_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedHKOMDPriceLevelMarketViewManager indexed_hkomd_pf_market_view_manager_(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);
  HFSAT::IndexedNtpMarketViewManager indexed_ntp_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);

  HFSAT::HistoricalDispatcher historical_dispatcher_;

  switch (exch_source_) {
    case HFSAT::kExchSourceASX: {
      auto filesource = new HFSAT::ASXLoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, 0,
                                                              exchange_symbol_, r_trading_location_);

      filesource->SetExternalTimeListener(&watch_);
      filesource->AddASXListener(asx_market_order_manager);

      historical_dispatcher_.AddExternalDataListener(filesource);

      auto pf_filesource = new HFSAT::ASXPFLoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, 0,
                                                                   exchange_symbol_, r_trading_location_);
      pf_filesource->SetExternalTimeListener(&watch_);
      pf_filesource->AddPriceLevelGlobalListener(&indexed_ice_market_view_manager_);

      historical_dispatcher_.AddExternalDataListener(pf_filesource);
      break;
    }
    case HFSAT::kExchSourceEUREX:
    case HFSAT::kExchSourceEOBI: {
      HFSAT::EOBILoggedMessageFileSource* eobi_logged_message_file_source_ = new HFSAT::EOBILoggedMessageFileSource(
          dbglogger_, sec_name_indexer_, tradingdate_, 0, exchange_symbol_, r_trading_location_);

      eobi_logged_message_file_source_->SetExternalTimeListener(&watch_);
      eobi_logged_message_file_source_->SetOrderLevelListenerSim(p_eurex_market_order_manager_);

      historical_dispatcher_.AddExternalDataListener(eobi_logged_message_file_source_);

      HFSAT::EOBIPriceFeedLoggedMessageFileSource* eobi_price_feed_logged_message_file_source_ =
          new HFSAT::EOBIPriceFeedLoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, 0,
                                                          exchange_symbol_, r_trading_location_);
      eobi_price_feed_logged_message_file_source_->SetExternalTimeListener(&watch_);

      eobi_price_feed_logged_message_file_source_->SetPriceLevelGlobalListener(
          &indexed_eobi_price_level_market_view_manager_);

      historical_dispatcher_.AddExternalDataListener(eobi_price_feed_logged_message_file_source_);
    } break;
    case HFSAT::kExchSourceJPY: {
      HFSAT::OSELoggedMessageFileSource* ose_logged_message_file_source_ = new HFSAT::OSELoggedMessageFileSource(
          dbglogger_, sec_name_indexer_, tradingdate_, 0, exchange_symbol_, r_trading_location_);

      ose_logged_message_file_source_->SetExternalTimeListener(&watch_);
      ose_logged_message_file_source_->SetOrderLevelListenerSim(p_ose_market_order_manager_);

      historical_dispatcher_.AddExternalDataListener(ose_logged_message_file_source_);

      HFSAT::OSEPriceFeedLoggedMessageFileSource* ose_price_feed_logged_message_file_source_ =
          new HFSAT::OSEPriceFeedLoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, 0,
                                                         exchange_symbol_, r_trading_location_);
      ose_price_feed_logged_message_file_source_->SetExternalTimeListener(&watch_);

      ose_price_feed_logged_message_file_source_->SetPriceLevelGlobalListener(
          &indexed_ose_pricefeed_market_view_manager_);

      historical_dispatcher_.AddExternalDataListener(ose_price_feed_logged_message_file_source_);
    } break;
    case HFSAT::kExchSourceICE: {
      HFSAT::ICELoggedMessageFileSource* ice_logged_message_file_source_ = new HFSAT::ICELoggedMessageFileSource(
          dbglogger_, sec_name_indexer_, tradingdate_, 0, exchange_symbol_, r_trading_location_, true);

      ice_logged_message_file_source_->SetExternalTimeListener(&watch_);
      ice_logged_message_file_source_->AddOrderLevelListener(p_ice_market_order_manager_);

      historical_dispatcher_.AddExternalDataListener(ice_logged_message_file_source_);

      HFSAT::ICELoggedMessageFileSource* ice_pf_logged_message_file_source_ = new HFSAT::ICELoggedMessageFileSource(
          dbglogger_, sec_name_indexer_, tradingdate_, 0, exchange_symbol_, r_trading_location_, false);
      ice_pf_logged_message_file_source_->SetExternalTimeListener(&watch_);

      ice_pf_logged_message_file_source_->SetPriceLevelGlobalListener(&indexed_ice_market_view_manager_);

      historical_dispatcher_.AddExternalDataListener(ice_pf_logged_message_file_source_);
    } break;
    case HFSAT::kExchSourceHONGKONG:
    case HFSAT::kExchSourceHKOMD:
    case HFSAT::kExchSourceHKOMDCPF:
    case HFSAT::kExchSourceHKOMDPF: {
      HFSAT::HKOMDLoggedMessageFileSource* hkomd_logged_message_file_source_ = new HFSAT::HKOMDLoggedMessageFileSource(
          dbglogger_, sec_name_indexer_, tradingdate_, 0, exchange_symbol_, r_trading_location_);

      hkomd_logged_message_file_source_->SetExternalTimeListener(&watch_);
      hkomd_logged_message_file_source_->AddOrderLevelListener(p_hk_market_order_manager_);

      historical_dispatcher_.AddExternalDataListener(hkomd_logged_message_file_source_);

      HFSAT::HKOMDPFLoggedMessageFileSource* hkomdpf_logged_message_file_source_ =
          new HFSAT::HKOMDPFLoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, 0, exchange_symbol_,
                                                    r_trading_location_);
      hkomdpf_logged_message_file_source_->SetExternalTimeListener(&watch_);

      hkomdpf_logged_message_file_source_->SetPriceLevelGlobalListener(&indexed_hkomd_pf_market_view_manager_);

      historical_dispatcher_.AddExternalDataListener(hkomdpf_logged_message_file_source_);
    } break;
    case HFSAT::kExchSourceBMF:
    case HFSAT::kExchSourceNTP: {
      HFSAT::NTPLoggedMessageFileSource* ntp_logged_message_file_source_ = new HFSAT::NTPLoggedMessageFileSource(
          dbglogger_, sec_name_indexer_, tradingdate_, 0, exchange_symbol_, r_trading_location_, false, true);

      ntp_logged_message_file_source_->SetExternalTimeListener(&watch_);
      ntp_logged_message_file_source_->SetOrderLevelListenerSim(p_ntp_market_order_manager_);

      historical_dispatcher_.AddExternalDataListener(ntp_logged_message_file_source_);

      HFSAT::NTPLoggedMessageFileSource* ntp_pf_logged_message_file_source_ = new HFSAT::NTPLoggedMessageFileSource(
          dbglogger_, sec_name_indexer_, tradingdate_, 0, exchange_symbol_, r_trading_location_);
      ntp_pf_logged_message_file_source_->SetExternalTimeListener(&watch_);

      ntp_pf_logged_message_file_source_->SetNTPPriceLevelGlobalListener(&indexed_ntp_market_view_manager_);

      historical_dispatcher_.AddExternalDataListener(ntp_pf_logged_message_file_source_);
    } break;
    default:
      break;
  }

  TestBook test_book_(p_mov_, p_smv_, dbglogger_, watch_);
  test_book_.run();

  p_smv_->subscribe_L2(&test_book_);
  p_mov_->SubscribeL1QueuePosChange(&test_book_);

  historical_dispatcher_.RunHist();
}
