
/*
  Source: https://theboostcpplibraries.com/boost.lockfree
  Boost.Lockfree provides thread-safe and lock-free containers.
  Containers from this library can be accessed from multiple
  threads without having to synchronize access.
 */
#include <boost/lockfree/queue.hpp>

#include "basetrade/InitLogic/exchange_simulator.hpp"
#include "basetrade/BTUtils/ors_order_listener_thread.hpp"

HFSAT::ShortcodeRequestHelper* global_shc_request_helper;

void termHandler(int signum) {
  if (global_shc_request_helper) {
    global_shc_request_helper->RemoveAllShortcodesToListen();
    global_shc_request_helper = nullptr;
  }

  exit(0);
}

// Checks if any of the instruments are NSE symbols and if so adds NSE contract specs
bool CheckAndAddNSEDefinitions(std::vector<std::string>& t_shortcode_vec_, HFSAT::DebugLogger& dbglogger_) {
  bool is_nse_present_ = false;
  for (unsigned int i = 0; i < t_shortcode_vec_.size(); i++) {
    if (strncmp(t_shortcode_vec_[i].c_str(), "NSE_", 4) == 0) {
      is_nse_present_ = true;
    }
  }
  if (is_nse_present_) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(
        HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate()).LoadNSESecurityDefinitions();
  }

  return is_nse_present_;
}

int main(int argc, char** argv) {
  signal(SIGINT, termHandler);
  signal(SIGSEGV, termHandler);

  std::vector<std::string> sec_list_vec;
  std::vector<const char*> exchange_symbol_vec;

  if (argc < 2) {
    std::cerr << " usage : Input file name containing the Shotcodes" << std::endl;
    exit(0);
  }

  std::string filename_input(argv[1]);

  int prog_id = -100;

  HFSAT::DebugLogger dbglogger_(1024000, 1);
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "/spare/local/logs/alllogs/exchange_simulator." << HFSAT::DateTime::GetCurrentIsoDateLocal();
  std::string logfilename_ = t_temp_oss_.str();
  dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out);

  int tradingdate_ = HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate();

  struct timeval current_time;
  gettimeofday(&current_time, NULL);

  // forcing prog_id to be between -200 and -101
  prog_id = (current_time.tv_usec % 100) - 200;

  // Make Sure This Is Done Above All Other Classe's Initialization, Needed For ASX TICK Changes
  HFSAT::SecurityDefinitions::ResetASXMpiIfNeeded(tradingdate_, (time_t)current_time.tv_sec);

  /////////FILE INPUT OF SHORTCODES///////////
  char line[1024];
  std::ifstream sec_file_;
  sec_file_.open(filename_input.c_str(), std::ifstream::in);
  if (!sec_file_.is_open()) {
    std::cerr << filename_input << "  FILE DOESNOT EXIST " << std::endl;
    exit(-1);
  }

  while (!sec_file_.eof()) {
    bzero(line, 1024);
    sec_file_.getline(line, 1024);
    if (strstr(line, "#") || strlen(line) == 0) continue;
    HFSAT::PerishableStringTokenizer st(line, 1024);
    const std::vector<const char*>& tokens = st.GetTokens();
    if (tokens.size() < 1) {
      std::cerr << " Bad file..See #entries " << std::endl;
      exit(-1);
    }

    if (tokens.size() == 2) {
      if (strcmp(tokens[0], "QUERY_ID") == 0) {
        int temp = atoi(tokens[1]);
        // making sure that prog_id fro oebu is between -100 and -200
        if (temp <= -100 && temp >= -200) {
          prog_id = temp;
        }
        continue;
      }
    }

    std::cout << " SHC:  " << tokens[0] << std::endl;
    sec_list_vec.push_back(std::string(tokens[0]));
  }
  ////////@end FILE INPUT////////////////

  bool is_nse_added = CheckAndAddNSEDefinitions(sec_list_vec, dbglogger_);

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);

  // Get exchange symbols for all securities
  for (auto security : sec_list_vec) {
    exchange_symbol_vec.push_back(HFSAT::ExchangeSymbolManager::GetExchSymbol(security));
  }

  // The following class requests data for the required shortcodes from the CombinedShmWriter
  HFSAT::ShortcodeRequestHelper shc_req_helper(prog_id);
  global_shc_request_helper = &shc_req_helper;
  shc_req_helper.AddShortcodeListToListen(sec_list_vec);

  HFSAT::Watch watch_(dbglogger_, HFSAT::DateTime::GetCurrentIsoDateLocal());

  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  for (unsigned int sec_id = 0; sec_id < (unsigned int)sec_list_vec.size(); sec_id++)
    sec_name_indexer_.AddString(exchange_symbol_vec[sec_id], sec_list_vec[sec_id]);

  HFSAT::TradingLocation_t curr_location_ = HFSAT::TradingLocationUtils::GetTradingLocationFromHostname();

  bool use_ose_l1_ = false;
  bool use_nse_l1 = false;

  if (curr_location_ == HFSAT::kTLocSPR) {
    use_nse_l1 = true;
  }

  // This is the main class. This is a TCP based interface between ORS and SimMarketMaker
  HFSAT::ORSOrderListenerThread ors_listener_thread_(dbglogger_, watch_);

  // Create sim market maker for required securities
  std::map<int, HFSAT::PriceLevelSimMarketMaker*> sec_id_to_sim_market_maker_;

  HFSAT::SimTimeSeriesInfo sim_time_series_info_ = HFSAT::SimTimeSeriesInfo(sec_name_indexer_.NumSecurityId());

  HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_ = HFSAT::sid_to_security_market_view_map();

  HFSAT::ShortcodeSecurityMarketViewMap& shortcode_smv_map_ =
      HFSAT::ShortcodeSecurityMarketViewMap::GetUniqueInstance();

  /*
   * 1) Create SMV for input securities
   * 2) Create SimMarketMaker for them
   * 3) Subscribe SimMarketMaker and ORSListener thread to all SMVs
   * 4) Subscribe ORSListenerThread to broadcasts from SimMarketMaker
   */
  for (unsigned int sec_id = 0; sec_id < (unsigned int)sec_name_indexer_.NumSecurityId(); sec_id++) {
    if (sim_time_series_info_.sid_to_sim_config_.size() < sec_id) {
      sim_time_series_info_.sid_to_sim_config_.resize(sec_id);
    }

    if (sim_time_series_info_.sid_to_sim_config_.size() <= sec_id) {
      sim_time_series_info_.sid_to_sim_config_.push_back(
          HFSAT::SimConfig::GetSimConfigsForShortcode(dbglogger_, watch_, sec_list_vec[sec_id]));
    }

    HFSAT::ExchSource_t exch_source =
        HFSAT::SecurityDefinitions::GetContractExchSource(sec_list_vec[sec_id], tradingdate_);
    int _this_sid_ = sec_name_indexer_.GetIdFromString(sec_list_vec[sec_id]);

    if (exch_source == HFSAT::kExchSourceEUREX) {
      exch_source = HFSAT::kExchSourceEOBI;
    }

    if (exch_source == HFSAT::kExchSourceHONGKONG) {
      exch_source = HFSAT::kExchSourceHKOMDPF;
    }

    bool set_temporary_bool_checking_if_this_is_an_indexed_book_ =
        HFSAT::CommonSimIndexedBookBool(exch_source, sec_list_vec[sec_id], curr_location_);

    if (exch_source == HFSAT::kExchSourceJPY && use_ose_l1_) {
      set_temporary_bool_checking_if_this_is_an_indexed_book_ = false;
    }

    HFSAT::SecurityMarketView* p_smv_ = new HFSAT::SecurityMarketView(
        dbglogger_, watch_, sec_name_indexer_, sec_list_vec[sec_id], exchange_symbol_vec[sec_id], _this_sid_,
        exch_source, set_temporary_bool_checking_if_this_is_an_indexed_book_);
    sid_to_smv_ptr_map_.push_back(p_smv_);                      // add to security_id_ to SMV* map
    shortcode_smv_map_.AddEntry(sec_list_vec[sec_id], p_smv_);  // add to shortcode_ to SMV* map

    HFSAT::PriceLevelSimMarketMaker* plsmm =
        HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(dbglogger_, watch_, *p_smv_, 0, sim_time_series_info_);

    plsmm->Connect();

    sec_id_to_sim_market_maker_[sec_id] = plsmm;
    plsmm->SubscribeL2Events(*p_smv_);

    ors_listener_thread_.SubscribeSMV(*p_smv_);

    plsmm->AddOrderNotFoundListener(&ors_listener_thread_);
    plsmm->AddOrderSequencedListener(&ors_listener_thread_);
    plsmm->AddOrderConfirmedListener(&ors_listener_thread_);
    plsmm->AddOrderConfCxlReplacedListener(&ors_listener_thread_);
    plsmm->AddOrderCanceledListener(&ors_listener_thread_);
    plsmm->AddOrderExecutedListener(&ors_listener_thread_);
    plsmm->AddOrderRejectedListener(&ors_listener_thread_);

    plsmm->AddSecIdToSACI(0, sec_id);
  }

  // Set the sim market maker map in ORSListenerThread
  ors_listener_thread_.SetSimMarketMakerMap(&sec_id_to_sim_market_maker_);

  // Creating IndexedBook for all exchanges
  HFSAT::IndexedCmeMarketViewManager indexed_cme_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedEobiPriceLevelMarketViewManager indexed_eobi_price_feed_market_view_manager_(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);
  HFSAT::IndexedLiffePriceLevelMarketViewManager indexed_liffe_price_level_market_view_manager_(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);
  HFSAT::IndexedRtsMarketViewManager indexed_rts_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedMicexMarketViewManager indexed_micex_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                          sid_to_smv_ptr_map_);
  HFSAT::IndexedNtpMarketViewManager indexed_ntp_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedNtpMarketViewManager indexed_puma_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                       sid_to_smv_ptr_map_);
  HFSAT::IndexedCfeMarketViewManager indexed_cfe_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::HKEXIndexedMarketViewManager hkex_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                sid_to_smv_ptr_map_);
  HFSAT::IndexedIceMarketViewManager indexed_ice_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedHKOMDPriceLevelMarketViewManager indexed_hkomd_price_level_market_view_manager(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);
  HFSAT::IndexedTmxMarketViewManager indexed_tmx_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);

  // HFSAT::
  HFSAT::OSEOrderLevelMarketViewManager ose_order_level_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                             sid_to_smv_ptr_map_);

  HFSAT::OSEL1PriceMarketViewManager ose_l1_price_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                       sid_to_smv_ptr_map_);
  HFSAT::OSEPriceFeedMarketViewManager ose_pricefeed_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                          sid_to_smv_ptr_map_);
  HFSAT::IndexedOsePriceFeedMarketViewManager indexed_ose_price_feed_market_view_manager_(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);

  HFSAT::IndexedNSEMarketViewManager indexed_nse_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::GenericL1DataMarketViewManager indexed_nse_l1_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                            sid_to_smv_ptr_map_);

  // Adding processing for all exchanges in combined mds shm processor

  HFSAT::MDSMessages::CombinedMDSMessagesShmProcessor combined_mds_messages_shm_processor_(
      dbglogger_, sec_name_indexer_, HFSAT::kComShmConsumer);

  for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++) {
    HFSAT::ExchSource_t _this_exch_source_ =
        HFSAT::SecurityDefinitions::GetContractExchSource(sec_list_vec[ii], tradingdate_);
    if (_this_exch_source_ == HFSAT::kExchSourceJPY && use_ose_l1_) {
      sid_to_smv_ptr_map_[ii]->market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_ = false;
    } else {
      sid_to_smv_ptr_map_[ii]->market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_ = true;
      sid_to_smv_ptr_map_[ii]->InitializeSMVForIndexedBook();
    }
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
      HFSAT::MDS_MSG::OSE_ITCH_PF, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ice_market_view_manager_)),
      &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::OSE_L1, (void*)((HFSAT::FullBookGlobalListener*)&(ose_l1_price_market_view_manager_)), &watch_);
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
    if (use_nse_l1) {
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::NSE_L1, (void*)((HFSAT::L1DataListener*)&(indexed_nse_l1_market_view_manager_)), &watch_);
    } else {
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::NSE, (void*)((HFSAT::OrderGlobalListenerNSE*)&(indexed_nse_market_view_manager_)), &watch_);
    }
  }

  combined_mds_messages_shm_processor_.AddORSreplySourceForProcessing(HFSAT::MDS_MSG::ORS_REPLY, &watch_,
                                                                      sid_to_smv_ptr_map_);

  // Run event loop
  ors_listener_thread_.Run();
  ors_listener_thread_.run();
  combined_mds_messages_shm_processor_.RunLiveShmSource();

  return 0;
}
