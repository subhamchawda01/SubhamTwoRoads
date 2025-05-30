// Simple Utility that shows the moving average based volume monitor for each product
// Idea is : it will startup , look at the market volumes since it started
//           compare the rate of trade (per minute ) from sampledata

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <list>

#include "dvccode/Utils/thread.hpp"

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/random_channel.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "baseinfra/MarketAdapter/indexed_hkomd_price_level_market_view_manager.hpp"

#include "baseinfra/MDSMessages/combined_mds_messages_shm_processor.hpp"

#include "dvccode/ExternalData/historical_dispatcher.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

#include "baseinfra/OrderRouting/prom_order_manager.hpp"

#include "baseinfra/MarketAdapter/market_adapter_list.hpp"
#include "dvctrade/RiskManager/risk_events_listener.hpp"
#include "dvctrade/RiskManager/admin_listener.hpp"
#include "dvctrade/RiskManager/query_listener.hpp"

#define _USE_NTP_LIVE_DATA_SOURCE_ false
#define _USE_NTP_RAW_SHM_ true

#define DEFAULT_VAL -1
bool _USE_CME_LIVE_DATA_SOURCE_ = false;
bool _USE_LIFFE_LIVE_DATA_SOURCE_ = false;
bool _USE_EUREX_LIVE_DATA_SOURCE_ = true;
bool _USE_CME_LIVE_SHM_DATA_SOURCE_ = false;
bool _USE_EUREX_LIVE_SHM_DATA_SOURCE_ = false;
bool _USE_HK_SHM_SOURCE_ = false;
typedef unsigned int uint32_t;

int get_hhmmss(int timeval, int period, int t) {
  int time_slot = ((timeval / 1000) / period + t) * period;
  int hh = time_slot / 3600;
  time_slot -= hh * 3600;
  int mm = time_slot / 60;
  time_slot -= mm * 60;
  int ss = time_slot;
  return 10000 * hh + 100 * mm + ss;
}
int get_sec_from_hhmmss(int hhmmss) {
  int ss = hhmmss % 100;
  hhmmss /= 100;
  ss += (hhmmss % 100) * 60;
  hhmmss /= 100;
  ss += hhmmss * 3600;
  return ss;
}

// Checks if any of the instruments are NSE symbols and if so adds NSE contract specs
bool CheckAndAddNSEDefinitions(std::vector<std::string>& t_shortcode_vec_) {
  bool is_nse_present_ = false;
  for (auto i = 0u; i < t_shortcode_vec_.size(); i++) {
    std::cout << " Shc:" << t_shortcode_vec_[i] << "\n";
    if (strncmp(t_shortcode_vec_[i].c_str(), "NSE_", 4) == 0) {
      is_nse_present_ = true;
    }
  }
  if (is_nse_present_) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal())
        .LoadNSESecurityDefinitions();
  }

  return is_nse_present_;
}

// capture SIGINT and SIGSEGV to dump all risk mapping
// will be used on next start for recovery

void termination_handler_segv(int signum) {
  HFSAT::RiskNotifier::getInstance().DumpAllRiskMappingOnSigInit();
  signal(signum, SIG_DFL);
  kill(getpid(), signum);
}

void termination_handler(int signum) {
  HFSAT::RiskNotifier::getInstance().DumpAllRiskMappingOnSigInit();
  exit(0);
}

int main(int argc, char** argv) {
  // signal handling, Interrupts and seg faults
  signal(SIGINT, termination_handler);
  signal(SIGSEGV, termination_handler_segv);
  signal(SIGPIPE, SIG_IGN);

  std::vector<std::string> sec_list_vec;
  std::vector<const char*> exchange_symbol_vec;
  if (argc < 2) {
    std::cerr << " usage : <exec> <Input file name containing the Symbols E.g ~/infracore_install/files/mkt_sec.txt>"
              << std::endl;
    exit(0);
  }

  int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();

  struct timeval current_time;
  gettimeofday(&current_time, NULL);

  // Make Sure This Is Done Above All Other Classe's Initialization, Needed For ASX TICK Changes
  HFSAT::SecurityDefinitions::ResetASXMpiIfNeeded(tradingdate_, (time_t)current_time.tv_sec);

  std::string filename_input(argv[1]);

  /////////FILE INPUT OF SHORTCODES///////////
  char line[1024];
  std::ifstream sec_file_;
  sec_file_.open(filename_input.c_str(), std::ifstream::in);
  if (!sec_file_.is_open()) {
    std::cerr << filename_input << "  FILE DOESNOT EXIST " << std::endl;
    exit(-1);
  }

  std::map<std::string, bool> found_shc;
  while (!sec_file_.eof()) {
    bzero(line, 1024);
    sec_file_.getline(line, 1024);
    if (strstr(line, "#") || strlen(line) == 0) continue;
    HFSAT::PerishableStringTokenizer st(line, 1024);
    const std::vector<const char*>& tokens = st.GetTokens();
    if (tokens.size() < 4) {
      std::cerr << " Bad file..See #entries " << std::endl;
      exit(-1);
    }
    if (!strncmp(tokens[3], "C_", 2)) {  // look for multiple shortcodes in this line
      for (unsigned int itr = 4; itr < tokens.size(); itr++) {
        if (strchr(tokens[itr], '_') && !found_shc[tokens[itr]]) {
          std::cerr << " SEC_SHORTCODE " << tokens[itr] << std::endl;
          sec_list_vec.push_back(tokens[itr]);
          found_shc[tokens[itr]] = true;
        }
      }
    } else if (!found_shc[tokens[3]]) {
      std::string shc = std::string(tokens[3]);
      shc.erase(std::remove(shc.begin(), shc.end(), '\\'), shc.end());  // remove '\' from NSE_M\&M_FUT0
      shc.erase(std::remove(shc.begin(), shc.end(), '"'), shc.end());   // remove '"' from NSE_M\&M_FUT0
      std::cerr << " SEC_SHORTCODE " << tokens[3] << std::endl;
      sec_list_vec.push_back(shc);
      found_shc[tokens[3]] = true;
    }
  }
  ////////@end FILE INPUT////////////////
  bool is_nse_added = CheckAndAddNSEDefinitions(sec_list_vec);

  // filter out products whose contract specs not found
  for (std::vector<std::string>::iterator itr = sec_list_vec.begin(); itr != sec_list_vec.end();) {
    if (!HFSAT::SecurityDefinitions::CheckIfContractSpecExists(*itr, tradingdate_)) {
      std::cout << "spec not found deleting " << *itr << std::endl;
      sec_list_vec.erase(itr);
    } else
      itr++;
  }

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);

  HFSAT::TradingLocation_t curr_location_ = HFSAT::TradingLocationUtils::GetTradingLocationFromHostname();
  if (curr_location_ == HFSAT::kTLocM1 || curr_location_ == HFSAT::kTLocSYD) {
    _USE_CME_LIVE_DATA_SOURCE_ = true;
    _USE_LIFFE_LIVE_DATA_SOURCE_ = true;
    _USE_CME_LIVE_SHM_DATA_SOURCE_ = true;
    _USE_EUREX_LIVE_SHM_DATA_SOURCE_ = true;
  }
  if (curr_location_ == HFSAT::kTLocHK) {
    _USE_CME_LIVE_DATA_SOURCE_ = true;
    _USE_LIFFE_LIVE_DATA_SOURCE_ = true;
    _USE_EUREX_LIVE_DATA_SOURCE_ = true;
    _USE_EUREX_LIVE_SHM_DATA_SOURCE_ = true;
  }
  if (curr_location_ == HFSAT::kTLocJPY) {
    _USE_HK_SHM_SOURCE_ = false;
    _USE_CME_LIVE_DATA_SOURCE_ = true;
    _USE_LIFFE_LIVE_DATA_SOURCE_ = true;
    _USE_EUREX_LIVE_DATA_SOURCE_ = true;
    _USE_EUREX_LIVE_SHM_DATA_SOURCE_ = true;
  }

  for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++)
    exchange_symbol_vec.push_back(HFSAT::ExchangeSymbolManager::GetExchSymbol(sec_list_vec[ii]));

  HFSAT::DebugLogger dbglogger_(1024000, 1);
  // setup DebugLogger
  {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/logs/risk_logs/risk_client_log_" << HFSAT::DateTime::GetCurrentIsoDateLocal();
    std::string logfilename_ = t_temp_oss_.str();
    dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out);
  }

  HFSAT::Watch watch_(dbglogger_, tradingdate_);

  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++)
    sec_name_indexer_.AddString(exchange_symbol_vec[ii], sec_list_vec[ii]);

  HFSAT::SimpleLiveDispatcher simple_live_dispatcher_;

  HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_ = HFSAT::sid_to_security_market_view_map();
  HFSAT::ShortcodeSecurityMarketViewMap& shortcode_smv_map_ =
      HFSAT::ShortcodeSecurityMarketViewMap::GetUniqueInstance();

  for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++) {
    HFSAT::ExchSource_t _this_exch_source_ =
        HFSAT::SecurityDefinitions::GetContractExchSource(sec_list_vec[ii], tradingdate_);
    int _this_sid_ = sec_name_indexer_.GetIdFromString(sec_list_vec[ii]);

    if (_this_exch_source_ == HFSAT::kExchSourceEUREX) {
      _this_exch_source_ = HFSAT::kExchSourceEOBI;
    }

    bool set_temporary_bool_checking_if_this_is_an_indexed_book_ =
        HFSAT::CommonSimIndexedBookBool(_this_exch_source_, sec_list_vec[ii], curr_location_);

    HFSAT::SecurityMarketView* p_smv_ = new HFSAT::SecurityMarketView(
        dbglogger_, watch_, sec_name_indexer_, sec_list_vec[ii], exchange_symbol_vec[ii], _this_sid_,
        _this_exch_source_, set_temporary_bool_checking_if_this_is_an_indexed_book_);
    std::cerr << " SID : for SMV " << _this_sid_ << std::endl;
    sid_to_smv_ptr_map_.push_back(p_smv_);                  // add to security_id_ to SMV* map
    shortcode_smv_map_.AddEntry(sec_list_vec[ii], p_smv_);  // add to shortcode_ to SMV* map
  }

  HFSAT::IndexedLiffePriceLevelMarketViewManager indexed_liffe_price_level_market_view_manager_(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);
  HFSAT::IndexedIceMarketViewManager indexed_ice_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::HKEXIndexedMarketViewManager indexed_hkex_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                        sid_to_smv_ptr_map_);
  HFSAT::OSEOrderLevelMarketViewManager ose_order_level_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                             sid_to_smv_ptr_map_);
  HFSAT::OSEL1PriceMarketViewManager ose_l1_price_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                       sid_to_smv_ptr_map_);
  HFSAT::IndexedEobiPriceLevelMarketViewManager indexed_eobi_price_feed_market_view_manager_(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);
  HFSAT::IndexedCmeMarketViewManager indexed_cme_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedNtpMarketViewManager indexed_ntp_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedNtpMarketViewManager indexed_puma_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                       sid_to_smv_ptr_map_);
  HFSAT::IndexedRtsMarketViewManager indexed_rts_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedMicexMarketViewManager indexed_micex_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                          sid_to_smv_ptr_map_);
  HFSAT::IndexedOsePriceFeedMarketViewManager indexed_ose_price_feed_market_view_manager_(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);
  HFSAT::IndexedCfeMarketViewManager indexed_cfe_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedTmxMarketViewManager indexed_tmx_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedHKOMDPriceLevelMarketViewManager indexed_hkomd_price_level_market_view_manager(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);
  HFSAT::IndexedNSEMarketViewManager indexed_nse_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);

  HFSAT::MDSMessages::CombinedMDSMessagesShmProcessor combined_mds_messages_shm_processor_(
      dbglogger_, sec_name_indexer_, HFSAT::kComShmConsumer);

  for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++) {
    sid_to_smv_ptr_map_[ii]->market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_ = true;
    sid_to_smv_ptr_map_[ii]->InitializeSMVForIndexedBook();
  }

  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::CME, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_cme_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::CME_LS, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_cme_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::EUREX, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_cme_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::EOBI_PF,
      (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_eobi_price_feed_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::EOBI_LS,
      (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_eobi_price_feed_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::EUREX_LS, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_cme_market_view_manager_)),
      &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::ICE, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ice_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::ASX, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ice_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::SGX, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ice_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::LIFFE,
      (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_liffe_price_level_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::LIFFE_LS,
      (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_liffe_price_level_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::RTS, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_cme_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::MICEX, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_micex_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::NTP, (void*)((HFSAT::NTPPriceLevelGlobalListener*)(&indexed_ntp_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::BMF_EQ, (void*)((HFSAT::NTPPriceLevelGlobalListener*)(&indexed_ntp_market_view_manager_)),
      &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::OSE_CF, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ose_price_feed_market_view_manager_)),
      &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::HKOMDPF,
      (void*)((HFSAT::HKHalfBookGlobalListener*)(&indexed_hkomd_price_level_market_view_manager)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::CSM, (void*)((HFSAT::CFEPriceLevelGlobalListener*)&(indexed_cfe_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::TMX, (void*)((HFSAT::FullBookGlobalListener*)(&indexed_tmx_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::TMX_LS, (void*)((HFSAT::FullBookGlobalListener*)(&indexed_tmx_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::TMX_OBF, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ice_market_view_manager_)), &watch_);
  // Since NSE requires some files to load which may/may not be present on a given server
  if (true == is_nse_added) {
    combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
        HFSAT::MDS_MSG::NSE, (void*)((HFSAT::OrderGlobalListenerNSE*)&(indexed_nse_market_view_manager_)), &watch_);
  }
  combined_mds_messages_shm_processor_.AddORSreplySourceForProcessing(HFSAT::MDS_MSG::ORS_REPLY, &watch_,
                                                                      sid_to_smv_ptr_map_);

  // create RiskNotifier instance
  HFSAT::RiskNotifier& risk_notifier = HFSAT::RiskNotifier::setInstance(dbglogger_, sec_list_vec);

  // Add listeners for various updates affecting our risk values
  for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++) {
    const unsigned int _this_sid_ = (unsigned int)sec_name_indexer_.GetIdFromString(sec_list_vec[ii]);

    std::string _this_shortcode_ = sec_list_vec[ii];
    HFSAT::RiskEventsListener* risk_listener =
        new HFSAT::RiskEventsListener(dbglogger_, watch_, sec_name_indexer_, *sid_to_smv_ptr_map_[_this_sid_]);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderExecutedListener(_this_sid_, risk_listener);
  }

  // Start listening to admin (getflat) updates from the central server
  HFSAT::AdminListener admin_listener(&risk_notifier);
  admin_listener.run();
  // Start listening to admin (getflat) updates from the central server
  HFSAT::QueryListener query_listener(&risk_notifier, QUERY_LISTENER_PORT);
  query_listener.run();

  combined_mds_messages_shm_processor_.RunLiveShmSource();

  return 0;
}
