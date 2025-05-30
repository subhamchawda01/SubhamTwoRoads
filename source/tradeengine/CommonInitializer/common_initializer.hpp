#define USE_UDP_DIRECT_SHM1_MERGE 1
#define USE_EXANIC_MERGE 0
#define USE_UNCONVERTED_DATA_DATE 20240805
#define BSE_USE_UNCONVERTED_DATA_DATE 20241121

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/MarketAdapter/market_orders_view.hpp"

#include "dvccode/ExternalData/historical_dispatcher.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"
#include "baseinfra/LoggedSources/filesource_list.hpp"
#include "baseinfra/LoggedSources/ors_message_filesource.hpp"
#include "baseinfra/MarketAdapter/indexed_bse_market_view_manager_2.hpp"
#include "baseinfra/MarketAdapter/indexed_nse_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_nse_market_view_manager_2.hpp"
#include "baseinfra/MarketAdapter/generic_l1_data_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_ntp_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_bmf_fpga_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/nse_market_order_manager.hpp"
#include "baseinfra/SimMarketMaker/base_sim_market_maker.hpp"
#include "baseinfra/SimMarketMaker/price_level_sim_market_maker.hpp"
#include "baseinfra/SimMarketMaker/order_level_sim.hpp"
#include "baseinfra/SimMarketMaker/order_level_sim_market_maker_2.hpp"
#include "baseinfra/BaseTrader/sim_trader_helper.hpp"
#include "baseinfra/OrderRouting/basic_order_manager.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_direct_processor.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_multi_shm_processor.hpp"

class CommonInitializer {
  std::vector<std::string> source_shortcode_vec_;
  std::vector<std::string> ors_shortcode_vec_;
  int tradingdate_;
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::TradingLocation_t dep_trading_location_;
  HFSAT::Watch watch_;
  std::vector<const char*> exchange_symbol_vec_;
  HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_;
  std::vector<HFSAT::ExchSource_t> sid_to_exch_source_map_;
  std::vector<HFSAT::MarketOrdersView*>& sid_to_mov_ptr_map_;
  std::vector<HFSAT::SecurityMarketView*> sid_to_sim_smv_ptr_map_;
  HFSAT::SecurityNameIndexer& sec_name_indexer_;
  HFSAT::SimTimeSeriesInfo* sim_time_series_info_;  // to be deleted
  std::string simconfig_filename_;
  HFSAT::MDSMessages::CombinedMDSMessagesDirectProcessor* combined_mds_messages_shm_processor_;
  HFSAT::MDSMessages::CombinedMDSMessagesMultiShmProcessor* combined_mds_messages_bmf_shm_processor_;

  std::vector<bool> sid_to_ors_needed_map_;
  std::vector<HFSAT::BaseSimMarketMaker*> sid_to_smm_map_;

  int global_start_utc_hhmm_;
  int global_end_utc_hhmm_;

  int progid_;

  unsigned int num_sec_id_;
  bool livetrading_;
  bool use_self_book_;
  HFSAT::HistoricalDispatcher historical_dispatcher_;
  HFSAT::SimpleLiveDispatcher simple_live_dispatcher_;
  std::unique_ptr<HFSAT::IndexedNSEMarketViewManager2> indexed_nse_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::GenericL1DataMarketViewManager> generic_l1_data_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedBSEMarketViewManager2> indexed_bse_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedBMFFpgaMarketViewManager> indexed_bmf_fpga_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedNtpMarketViewManager> indexed_ntp_market_view_manager_ = nullptr;

  std::map<std::string, HFSAT::ExternalDataListener*> shortcode_filesource_map_;
  std::map<std::string, std::unique_ptr<HFSAT::NSELoggedMessageFileSource> > shortcode_nse_data_filesource_map_;
  std::map<std::string, std::unique_ptr<HFSAT::NSEL1LoggedMessageFileSource> > shortcode_nse_l1_data_filesource_map_;
  std::map<std::string, std::unique_ptr<HFSAT::NSELoggedMessageFileSource2> > shortcode_nse_data_filesource_map2_;
  std::map<std::string, std::unique_ptr<HFSAT::BSELoggedMessageFileSource> > shortcode_bse_data_filesource_map_;
  std::map<std::string, std::unique_ptr<HFSAT::BSELoggedMessageFileSource2> > shortcode_bse_data_filesource_map2_;
  std::map<std::string, std::unique_ptr<HFSAT::BSEL1LoggedMessageFileSource> > shortcode_bse_l1_data_filesource_map_;
  std::map<std::string, std::unique_ptr<HFSAT::BMFFPGALoggedMessageFileSource> > shortcode_bmf_fpga_data_filesource_map_;
  std::map<std::string, std::unique_ptr<HFSAT::NTPLoggedMessageFileSource> > shortcode_ntp_data_filesource_map_;

 public:
  CommonInitializer(std::vector<std::string> source_shortcode_vec, std::vector<std::string> ors_shortcode_vec,
                    int tradingdate_, HFSAT::DebugLogger& _dbglogger_, HFSAT::TradingLocation_t _trade_loc_,
                    bool live_trading_ = false)
      : source_shortcode_vec_(source_shortcode_vec),
        ors_shortcode_vec_(ors_shortcode_vec),
        tradingdate_(tradingdate_),
        dbglogger_(_dbglogger_),
        dep_trading_location_(_trade_loc_),
        watch_(dbglogger_, tradingdate_),
        sid_to_smv_ptr_map_(HFSAT::sid_to_security_market_view_map()),
        sid_to_exch_source_map_(),
        sid_to_mov_ptr_map_(HFSAT::sid_to_market_orders_view_map()),
        sid_to_sim_smv_ptr_map_(),
        sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()),
        sim_time_series_info_(),
        simconfig_filename_(),
        combined_mds_messages_shm_processor_(),
        combined_mds_messages_bmf_shm_processor_(),
        sid_to_ors_needed_map_(),
        sid_to_smm_map_(),
        global_start_utc_hhmm_(-1),
        global_end_utc_hhmm_(-1),
        progid_(0),
        num_sec_id_(1),
        livetrading_(live_trading_),
        use_self_book_(false) {}

  void SetStartEndTime(int datagen_start_utc_hhmm, int datagen_end_utc_hhmm) {
    global_start_utc_hhmm_ = datagen_start_utc_hhmm;
    global_end_utc_hhmm_ = datagen_end_utc_hhmm;
  }

  void SetRuntimeID(int _progid_) { progid_ = _progid_; }

  void SetSelfBook(bool _use_self_book_) { use_self_book_ = _use_self_book_; };

  void GetSimConfig(uint32_t secid, std::string shortcode) {
    if (!simconfig_filename_.empty()) {
      if (sim_time_series_info_->sid_to_sim_config_.size() <= secid) {
        sim_time_series_info_->sid_to_sim_config_.push_back(
            HFSAT::SimConfig::GetSimConfigsForShortcode(dbglogger_, watch_, shortcode, simconfig_filename_));
      }
    } else {
      if (sim_time_series_info_->sid_to_sim_config_.size() <= secid) {
        sim_time_series_info_->sid_to_sim_config_.push_back(
            HFSAT::SimConfig::GetSimConfigsForShortcode(dbglogger_, watch_, shortcode));
      }
    }
  }

  void InitializeVariables(bool _use_l1_book_ = false) {
    // Get exchange symbols corresponding to the shortcodes of interest (sources)
    // Add exchange symbols to SecurityNameIndexer
    for (auto i = 0u; i < source_shortcode_vec_.size(); i++) {
      if (!sec_name_indexer_.HasString(source_shortcode_vec_[i])) {  // need to add this source to sec_name_indexer_
                                                                     // since it was not added already
        // A unique instance of ExchangeSymbolManager gets the current symbol that the exchange knows this shortcode
        // as
        // and also allocates permanent storage to this instrument, that allows read access from outside.
        const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(source_shortcode_vec_[i]);
        sec_name_indexer_.AddString(exchange_symbol_, source_shortcode_vec_[i]);
        exchange_symbol_vec_.push_back(exchange_symbol_);
        sid_to_ors_needed_map_.push_back(false);
      }
    }

    // Get exchange symbols corresponding to the shortcodes of interest (trading)
    // Add exchange symbols to SecurityNameIndexer
    for (auto i = 0u; i < ors_shortcode_vec_.size(); i++) {
      int sec_id_ = sec_name_indexer_.GetIdFromString(ors_shortcode_vec_[i]);
      if (sec_id_ == -1) {  // need to add this source to sec_name_indexer_
                            // since it was not added already
        // A unique instance of ExchangeSymbolManager gets the current symbol that the exchange knows this shortcode
        // as
        // and also allocates permanent storage to this instrument, that allows read access from outside.
        const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(ors_shortcode_vec_[i]);
        sec_name_indexer_.AddString(exchange_symbol_, ors_shortcode_vec_[i]);
        exchange_symbol_vec_.push_back(exchange_symbol_);
        sid_to_ors_needed_map_.push_back(true);
      } else {
        sid_to_ors_needed_map_[sec_id_] = true;
      }
    }

    ///< Unique Instance of map from shortcode to p_smv_
    HFSAT::ShortcodeSecurityMarketViewMap& shortcode_smv_map_ =
        HFSAT::ShortcodeSecurityMarketViewMap::GetUniqueInstance();

    if (!livetrading_) {
      sim_time_series_info_ = new HFSAT::SimTimeSeriesInfo(sec_name_indexer_.NumSecurityId());
    } else {
      if (HFSAT::kTLocBMF == HFSAT::TradingLocationUtils::GetTradingLocationFromHostname()) {
      } else {
        combined_mds_messages_shm_processor_ = new HFSAT::MDSMessages::CombinedMDSMessagesDirectProcessor(
            dbglogger_, sec_name_indexer_, HFSAT::kComShmConsumer);
      }

      std::string stats_log_filepath = std::string("/spare/local/logs/tradelogs/shm_stats.") +
                                       std::to_string(tradingdate_) + "." + std::to_string(progid_);
      //      combined_mds_messages_shm_processor_->EnableQueueStats(stats_log_filepath);
    }

    // Making all the smvs and adding to sid_to_smv_map
    for (auto i = 0u; i < sec_name_indexer_.NumSecurityId(); i++) {
      std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(i);
      const char* _this_exchange_symbol_ = sec_name_indexer_.GetSecurityNameFromId(i);
      HFSAT::ExchSource_t t_exch_source_ =
          HFSAT::SecurityDefinitions::GetContractExchSource(_this_shortcode_, tradingdate_);
      sid_to_exch_source_map_.push_back(t_exch_source_);
      HFSAT::SecurityMarketView* p_smv_ =
          new HFSAT::SecurityMarketView(dbglogger_, watch_, sec_name_indexer_, _this_shortcode_, _this_exchange_symbol_,
                                        i, sid_to_exch_source_map_[i], true, "INVALID", "INVALID", "INVALID");
      p_smv_->SetL1OnlyFlag(false);
      sid_to_smv_ptr_map_.push_back(p_smv_);                  // add to security_id_ to SMV* map
      shortcode_smv_map_.AddEntry(_this_shortcode_, p_smv_);  // add to shortcode_ to SMV* map
      if (false == _use_l1_book_) {
        p_smv_->InitializeSMVForIndexedBook();
      }
    }

    num_sec_id_ = sec_name_indexer_.NumSecurityId();
  }

  void Initialize(bool _use_l1_book_ = false) {
    InitializeVariables(_use_l1_book_);
    // Go over all the security ids.
    // Create it's filesource and link it to the shortcode's exchange's MarketViewManager
    // Link the Filesource to the HistoricalDispatcher
    for (unsigned int secid = 0; secid < sec_name_indexer_.NumSecurityId(); secid++) {
      const char* exchange_symbol_ = sec_name_indexer_.GetSecurityNameFromId(secid);
      std::string shortcode_ = sec_name_indexer_.GetShortcodeFromId(secid);
      switch (sid_to_exch_source_map_[secid]) {
        case HFSAT::kExchSourceNSE: {
          std::string l1_data_filename =
              HFSAT::NSEL1LoggedMessageFileNamer::GetName(exchange_symbol_, tradingdate_, dep_trading_location_);

          // Book Manager Creation
          if (!indexed_nse_market_view_manager_ && (false == _use_l1_book_)) {
            indexed_nse_market_view_manager_ =
                std::unique_ptr<HFSAT::IndexedNSEMarketViewManager2>(new HFSAT::IndexedNSEMarketViewManager2(
                    dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_, use_self_book_));
          } else if ((true == _use_l1_book_ ) && !generic_l1_data_market_view_manager_) {
              generic_l1_data_market_view_manager_ =
                  std::unique_ptr<HFSAT::GenericL1DataMarketViewManager>(new HFSAT::GenericL1DataMarketViewManager(
                      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
          
          if (!livetrading_) {
            if (false == _use_l1_book_){
              //Using Unconverted data for running sim from 20240408
              if( tradingdate_ >= USE_UNCONVERTED_DATA_DATE){
                shortcode_nse_data_filesource_map2_[shortcode_] =
                std::unique_ptr<HFSAT::NSELoggedMessageFileSource2>(new HFSAT::NSELoggedMessageFileSource2(
                dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_));

                shortcode_nse_data_filesource_map2_[shortcode_]->SetExternalTimeListener(&watch_);
                shortcode_nse_data_filesource_map2_[shortcode_]->SetOrderGlobalListenerNSE(
                indexed_nse_market_view_manager_.get());
                shortcode_filesource_map_[shortcode_] = shortcode_nse_data_filesource_map2_[shortcode_].get();
                historical_dispatcher_.AddExternalDataListener(shortcode_nse_data_filesource_map2_[shortcode_].get());

              } else{
                if (shortcode_nse_data_filesource_map_.find(shortcode_) == shortcode_nse_data_filesource_map_.end()) {
                    //std::cout << "NSE logged : " << dep_trading_location_ << std::endl ;
                    shortcode_nse_data_filesource_map_[shortcode_] =
                    std::unique_ptr<HFSAT::NSELoggedMessageFileSource>(new HFSAT::NSELoggedMessageFileSource(
                      dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_));

                    shortcode_nse_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                    shortcode_nse_data_filesource_map_[shortcode_]->SetOrderGlobalListenerNSE(
                    indexed_nse_market_view_manager_.get());
                    shortcode_filesource_map_[shortcode_] = shortcode_nse_data_filesource_map_[shortcode_].get();
                    historical_dispatcher_.AddExternalDataListener(shortcode_nse_data_filesource_map_[shortcode_].get());
                }
              }
            } else if (true == _use_l1_book_) {
                if (shortcode_nse_l1_data_filesource_map_.find(shortcode_) ==
                    shortcode_nse_l1_data_filesource_map_.end()) {
                  shortcode_nse_l1_data_filesource_map_[shortcode_] =
                      std::unique_ptr<HFSAT::NSEL1LoggedMessageFileSource>(new HFSAT::NSEL1LoggedMessageFileSource(
                          dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_));
                  shortcode_nse_l1_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                  shortcode_nse_l1_data_filesource_map_[shortcode_]->SetL1DataListener(
                      generic_l1_data_market_view_manager_.get());
                  shortcode_filesource_map_[shortcode_] = shortcode_nse_l1_data_filesource_map_[shortcode_].get();
                  historical_dispatcher_.AddExternalDataListener(
                      shortcode_nse_l1_data_filesource_map_[shortcode_].get());
                } else {
                  dbglogger_ << "L1 DATA ABSENT FOR : " << shortcode_ << " FILENAME : " << l1_data_filename << "\n";
                  dbglogger_.DumpCurrentBuffer();
                }

            } 

            HFSAT::SecurityMarketView* p_smv_sim_ = sid_to_smv_ptr_map_[secid];
            sid_to_sim_smv_ptr_map_.push_back(p_smv_sim_);
            sid_to_mov_ptr_map_.push_back(nullptr);
            GetSimConfig(secid, shortcode_);
            if (sid_to_ors_needed_map_[secid]) {
              HFSAT::OrderLevelSim* olsmm = nullptr;
              olsmm =
                HFSAT::OrderLevelSim::GetUniqueInstance(dbglogger_, watch_, *p_smv_sim_, 0, *sim_time_series_info_);
              if (sim_time_series_info_->sid_to_sim_config_[secid].use_order_level_sim_) {
                HFSAT::MarketOrdersView* mov = HFSAT::MarketOrdersView::GetUniqueInstance(dbglogger_, watch_, secid);
                sid_to_mov_ptr_map_[secid] = mov;
  
                bool is_hidden_order_available_ =
                  HFSAT::NSESecurityDefinitions::IsHiddenOrderAvailable(p_smv_sim_->shortcode());
                HFSAT::NSEMarketOrderManager* nse_market_order_manager = new HFSAT::NSEMarketOrderManager(
                  dbglogger_, watch_, sec_name_indexer_, sid_to_mov_ptr_map_, is_hidden_order_available_);
  
                if( tradingdate_ >= USE_UNCONVERTED_DATA_DATE){
                      auto filesource2 = shortcode_nse_data_filesource_map2_[shortcode_].get();
                      filesource2->SetOrderLevelListenerSim(nse_market_order_manager);
                      filesource2->SetExternalTimeListener(&watch_);
                      historical_dispatcher_.AddExternalDataListener(filesource2, true);
                }
                else{
                    auto filesource = shortcode_nse_data_filesource_map_[shortcode_].get();
                    filesource->SetOrderLevelListenerSim(nse_market_order_manager);
                    filesource->SetExternalTimeListener(&watch_);
                    historical_dispatcher_.AddExternalDataListener(filesource, true);
                }
                sid_to_smm_map_.push_back(olsmm);
              }
              else {
                HFSAT::PriceLevelSimMarketMaker* plsmm = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(
                  dbglogger_, watch_, *p_smv_sim_, 0, *sim_time_series_info_);
                plsmm->SubscribeL2Events(*p_smv_sim_);
                sid_to_smm_map_.push_back(plsmm);
              }
            }
            else {
              sim_time_series_info_->sid_to_sim_config_.resize(secid + 1);
              sid_to_smm_map_.push_back(nullptr);
            }
          }
          else {
            if (sid_to_ors_needed_map_[secid]) {
              combined_mds_messages_shm_processor_->AddORSreplySourceForProcessing(HFSAT::MDS_MSG::ORS_REPLY, &watch_,
                sid_to_smv_ptr_map_);
            }
            combined_mds_messages_shm_processor_->AddDataSourceForProcessing(
              HFSAT::MDS_MSG::NSE,
              (void*)((HFSAT::OrderGlobalListenerNSE*)(indexed_nse_market_view_manager_.get())), &watch_);
            if (use_self_book_) {
              combined_mds_messages_shm_processor_->GetProShmORSReplyProcessor()->AddOrderConfirmedListener(
                secid, (HFSAT::OrderConfirmedListener*)indexed_nse_market_view_manager_.get());
              combined_mds_messages_shm_processor_->GetProShmORSReplyProcessor()->AddOrderCanceledListener(
                secid, (HFSAT::OrderCanceledListener*)indexed_nse_market_view_manager_.get());
              combined_mds_messages_shm_processor_->GetProShmORSReplyProcessor()->AddOrderExecutedListener(
                secid, (HFSAT::OrderExecutedListener*)indexed_nse_market_view_manager_.get());
            }
          }
        } break;
        //
        case HFSAT::kExchSourceBSE: {
          std::string l1_data_filename =
              HFSAT::BSEL1LoggedMessageFileNamer::GetName(exchange_symbol_, tradingdate_, dep_trading_location_);

          if (!indexed_bse_market_view_manager_ && (false == _use_l1_book_)) {
            indexed_bse_market_view_manager_ =
              std::unique_ptr<HFSAT::IndexedBSEMarketViewManager2>(new HFSAT::IndexedBSEMarketViewManager2(
                dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_, use_self_book_));
          } else if ((true == _use_l1_book_ ) && !generic_l1_data_market_view_manager_) {
              generic_l1_data_market_view_manager_ =
                  std::unique_ptr<HFSAT::GenericL1DataMarketViewManager>(new HFSAT::GenericL1DataMarketViewManager(
                      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }

          if (!livetrading_) {
	    if (false == _use_l1_book_) {
              if( tradingdate_ >= BSE_USE_UNCONVERTED_DATA_DATE){
                //std::cout << "Running bse sim on unconverted data" << std::endl;
                if (shortcode_bse_data_filesource_map2_.find(shortcode_) == shortcode_bse_data_filesource_map2_.end()) {
                  //std::cout << "BSE logged : " << dep_trading_location_ << std::endl ;
                  shortcode_bse_data_filesource_map2_[shortcode_] =
                    std::unique_ptr<HFSAT::BSELoggedMessageFileSource2>(new HFSAT::BSELoggedMessageFileSource2(
                      dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_));
                  shortcode_bse_data_filesource_map2_[shortcode_]->SetExternalTimeListener(&watch_);
                  shortcode_bse_data_filesource_map2_[shortcode_]->SetOrderGlobalListenerBSE(
                    indexed_bse_market_view_manager_.get());
                  shortcode_filesource_map_[shortcode_] = shortcode_bse_data_filesource_map2_[shortcode_].get();
                  historical_dispatcher_.AddExternalDataListener(shortcode_bse_data_filesource_map2_[shortcode_].get());
                  //std::cout << "shortcode_" << shortcode_ << " FIle " << "BSELoggedMessageFileSource" << std::endl;
                }
              }else{
                if (shortcode_bse_data_filesource_map_.find(shortcode_) == shortcode_bse_data_filesource_map_.end()) {
                  //std::cout << "BSE logged : " << dep_trading_location_ << std::endl ;
                  shortcode_bse_data_filesource_map_[shortcode_] =
                    std::unique_ptr<HFSAT::BSELoggedMessageFileSource>(new HFSAT::BSELoggedMessageFileSource(
                      dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_));
                  shortcode_bse_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                  shortcode_bse_data_filesource_map_[shortcode_]->SetOrderGlobalListenerBSE(
                    indexed_bse_market_view_manager_.get());
                  shortcode_filesource_map_[shortcode_] = shortcode_bse_data_filesource_map_[shortcode_].get();
                  historical_dispatcher_.AddExternalDataListener(shortcode_bse_data_filesource_map_[shortcode_].get());
                }
              }
            } else if (true == _use_l1_book_) {
                if (shortcode_bse_l1_data_filesource_map_.find(shortcode_) ==
                    shortcode_bse_l1_data_filesource_map_.end()) {
                  shortcode_bse_l1_data_filesource_map_[shortcode_] =
                      std::unique_ptr<HFSAT::BSEL1LoggedMessageFileSource>(new HFSAT::BSEL1LoggedMessageFileSource(
                          dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_));
                  shortcode_bse_l1_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                  shortcode_bse_l1_data_filesource_map_[shortcode_]->SetL1DataListener(
                      generic_l1_data_market_view_manager_.get());
                  shortcode_filesource_map_[shortcode_] = shortcode_bse_l1_data_filesource_map_[shortcode_].get();
                  historical_dispatcher_.AddExternalDataListener(
                      shortcode_bse_l1_data_filesource_map_[shortcode_].get());
                } else {
                  dbglogger_ << "L1 DATA ABSENT FOR : " << shortcode_ << " FILENAME : " << l1_data_filename << "\n";
                  dbglogger_.DumpCurrentBuffer();
                }
            } 
  
            HFSAT::SecurityMarketView* p_smv_sim_ = sid_to_smv_ptr_map_[secid];
            sid_to_sim_smv_ptr_map_.push_back(p_smv_sim_);
            sid_to_mov_ptr_map_.push_back(nullptr);
            GetSimConfig(secid, shortcode_);
            if (sid_to_ors_needed_map_[secid]) {
              HFSAT::OrderLevelSim* olsmm = nullptr;
              olsmm =
                HFSAT::OrderLevelSim::GetUniqueInstance(dbglogger_, watch_, *p_smv_sim_, 0, *sim_time_series_info_);
              if (sim_time_series_info_->sid_to_sim_config_[secid].use_order_level_sim_) {
                HFSAT::MarketOrdersView* mov = HFSAT::MarketOrdersView::GetUniqueInstance(dbglogger_, watch_, secid);
                sid_to_mov_ptr_map_[secid] = mov;
  
  
                if( tradingdate_ >= BSE_USE_UNCONVERTED_DATA_DATE){
                      auto filesource2 = shortcode_bse_data_filesource_map2_[shortcode_].get();
                      filesource2->SetExternalTimeListener(&watch_);
                      historical_dispatcher_.AddExternalDataListener(filesource2, true);
                }
                else{
                    auto filesource = shortcode_bse_data_filesource_map_[shortcode_].get();
                    filesource->SetExternalTimeListener(&watch_);
                    historical_dispatcher_.AddExternalDataListener(filesource, true);
                }
                
                sid_to_smm_map_.push_back(olsmm);
              }
              else {
                HFSAT::PriceLevelSimMarketMaker* plsmm = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(
                  dbglogger_, watch_, *p_smv_sim_, 0, *sim_time_series_info_);
                plsmm->SubscribeL2Events(*p_smv_sim_);
                sid_to_smm_map_.push_back(plsmm);
              }
            } 
            else {
              sim_time_series_info_->sid_to_sim_config_.resize(secid + 1);
              sid_to_smm_map_.push_back(nullptr);
            }
          } else {
            if (sid_to_ors_needed_map_[secid]) {
              combined_mds_messages_shm_processor_->AddORSreplySourceForProcessing(HFSAT::MDS_MSG::ORS_REPLY, &watch_,
                                                                                   sid_to_smv_ptr_map_);
            }
            combined_mds_messages_shm_processor_->AddDataSourceForProcessing(
                HFSAT::MDS_MSG::BSE, (void*)((HFSAT::OrderGlobalListenerBSE*)(indexed_bse_market_view_manager_.get())),
                &watch_);
            if (use_self_book_) {
              combined_mds_messages_shm_processor_->GetProShmORSReplyProcessor()->AddOrderConfirmedListener(
                  secid, (HFSAT::OrderConfirmedListener*)indexed_bse_market_view_manager_.get());
              combined_mds_messages_shm_processor_->GetProShmORSReplyProcessor()->AddOrderCanceledListener(
                  secid, (HFSAT::OrderCanceledListener*)indexed_bse_market_view_manager_.get());
              combined_mds_messages_shm_processor_->GetProShmORSReplyProcessor()->AddOrderExecutedListener(
                  secid, (HFSAT::OrderExecutedListener*)indexed_bse_market_view_manager_.get());
            }
          }
        } break;
  
            //
        case HFSAT::kExchSourceBMF: {
          if (!indexed_ntp_market_view_manager_) {
            indexed_ntp_market_view_manager_ = std::unique_ptr<HFSAT::IndexedNtpMarketViewManager>(
                new HFSAT::IndexedNtpMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
  
          if (!indexed_bmf_fpga_market_view_manager_) {
            indexed_bmf_fpga_market_view_manager_ = std::unique_ptr<HFSAT::IndexedBMFFpgaMarketViewManager>(
                new HFSAT::IndexedBMFFpgaMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
          if (!livetrading_) {
            if (tradingdate_ >= USING_BMF_FPGA_FROM) {
              if (shortcode_bmf_fpga_data_filesource_map_.find(shortcode_) ==
                  shortcode_bmf_fpga_data_filesource_map_.end()) {
                shortcode_bmf_fpga_data_filesource_map_[shortcode_] =
                    std::unique_ptr<HFSAT::BMFFPGALoggedMessageFileSource>(
                        new HFSAT::BMFFPGALoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, secid,
                                                                  exchange_symbol_, dep_trading_location_, false));
                shortcode_bmf_fpga_data_filesource_map_[shortcode_]->SetFullBookGlobalListener(
                    indexed_bmf_fpga_market_view_manager_.get());
                shortcode_bmf_fpga_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                shortcode_filesource_map_[shortcode_] = shortcode_bmf_fpga_data_filesource_map_[shortcode_].get();
                historical_dispatcher_.AddExternalDataListener(
                    shortcode_bmf_fpga_data_filesource_map_[shortcode_].get());
              }
            } else {
              if (shortcode_ntp_data_filesource_map_.find(shortcode_) == shortcode_ntp_data_filesource_map_.end()) {
                shortcode_ntp_data_filesource_map_[shortcode_] =
                    std::unique_ptr<HFSAT::NTPLoggedMessageFileSource>(new HFSAT::NTPLoggedMessageFileSource(
                        dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                        false, false, false));
                shortcode_ntp_data_filesource_map_[shortcode_]->SetNTPPriceLevelGlobalListener(
                    indexed_ntp_market_view_manager_.get());
                shortcode_ntp_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                shortcode_filesource_map_[shortcode_] = shortcode_ntp_data_filesource_map_[shortcode_].get();
                historical_dispatcher_.AddExternalDataListener(shortcode_ntp_data_filesource_map_[shortcode_].get());
              }
            }
            HFSAT::SecurityMarketView* p_smv_sim_ = sid_to_smv_ptr_map_[secid];
            sid_to_sim_smv_ptr_map_.push_back(p_smv_sim_);
            sid_to_mov_ptr_map_.push_back(nullptr);
            GetSimConfig(secid, shortcode_);
            if (sid_to_ors_needed_map_[secid]) {
              HFSAT::OrderLevelSimMarketMaker2* olsmm = nullptr;
              if (sim_time_series_info_->sid_to_sim_config_[secid].use_order_level_sim_) {
                HFSAT::MarketOrdersView* mov = HFSAT::MarketOrdersView::GetUniqueInstance(dbglogger_, watch_, secid);
                sid_to_mov_ptr_map_[secid] = mov;
                olsmm = HFSAT::OrderLevelSimMarketMaker2::GetUniqueInstance(dbglogger_, watch_, *p_smv_sim_, 0,
                                                                            *sim_time_series_info_);
                HFSAT::NTPLoggedMessageFileSource* ntp_filesource =
                    new HFSAT::NTPLoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, secid,
                                                          p_smv_sim_->secname(), HFSAT::kTLocBMF, true, false, false);
                ntp_filesource->SetOrderLevelListenerSim((HFSAT::OrderLevelSimMarketMaker2*)olsmm);
                ntp_filesource->SetExternalTimeListener(&watch_);
                historical_dispatcher_.AddExternalDataListener(ntp_filesource, true);
                sid_to_smm_map_.push_back(olsmm);
              } else {
                HFSAT::PriceLevelSimMarketMaker* plsmm = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(
                    dbglogger_, watch_, *p_smv_sim_, 0, *sim_time_series_info_);
                plsmm->SubscribeL2Events(*p_smv_sim_);
                sid_to_smm_map_.push_back(plsmm);
              }
            } else {
              sim_time_series_info_->sid_to_sim_config_.resize(secid + 1);
              sid_to_smm_map_.push_back(nullptr);
            }
          } else {
            if (sid_to_ors_needed_map_[secid]) {
              combined_mds_messages_bmf_shm_processor_->AddORSreplySourceForProcessing(HFSAT::MDS_MSG::ORS_REPLY,
                                                                                       &watch_, sid_to_smv_ptr_map_);
            }
  
            // TODO - MULTISHM
            combined_mds_messages_bmf_shm_processor_->AddFPGADataSourceForProcessing(
                HFSAT::MDS_MSG::NTP,
                (void*)((HFSAT::BMFFPGAFullBookGlobalListener*)(indexed_bmf_fpga_market_view_manager_.get())), &watch_);
          }
        } break;
        default: { } break; 
      }
    }
  }

  void SetToActualSeek(bool _actual_skip_) {
    if( tradingdate_ >= USE_UNCONVERTED_DATA_DATE){
      for(auto & itr : shortcode_nse_data_filesource_map2_){
        (itr.second)->SetActualDataSkip(_actual_skip_);
      }
      return;
    }

    for(auto & itr : shortcode_nse_data_filesource_map_){
      (itr.second)->SetActualDataSkip(_actual_skip_);
    }
  }

  void Seek(HFSAT::ttime_t seek_time = HFSAT::ttime_t(0, 0)) {
    // If seek time is given as proper input use that, else use the date, hhmm given as input
    if (seek_time.tv_sec > 0) {
      historical_dispatcher_.SeekHistFileSourcesTo(seek_time);
      if (HFSAT::DateTime::Get_UTC_YYYYMMDD_from_ttime(seek_time) != tradingdate_) {
        watch_.ResetWatch(HFSAT::DateTime::Get_UTC_YYYYMMDD_from_ttime(seek_time));
      }
      return;
    }
#define MINUTES_TO_PREP 30
    // To only process data starting MINUTES_TO_PREP minutes before datagen_start_utc_hhmm_
    int event_process_start_utc_hhmm_ = global_start_utc_hhmm_;
    {
      event_process_start_utc_hhmm_ =
          ((event_process_start_utc_hhmm_ / 100) * 60) + (event_process_start_utc_hhmm_ % 100);
      event_process_start_utc_hhmm_ = std::max(0, event_process_start_utc_hhmm_ - MINUTES_TO_PREP);
      event_process_start_utc_hhmm_ =
          (event_process_start_utc_hhmm_ % 60) + ((event_process_start_utc_hhmm_ / 60) * 100);
    }
#undef MINUTES_TO_PREP

    historical_dispatcher_.SeekHistFileSourcesTo(
        HFSAT::ttime_t(HFSAT::DateTime::GetTimeUTC(tradingdate_, event_process_start_utc_hhmm_), 0));
  }

  void Run(HFSAT::ttime_t end_time = HFSAT::ttime_t(0, 0)) {
    // // start event loop
    try {
      if (livetrading_) {
        simple_live_dispatcher_.RunLive();
      } else {
        if (end_time.tv_sec > 0) {
          std::cout << "Run Histrical" << std::endl;
          historical_dispatcher_.RunHist(end_time);
        } else {
          if (global_end_utc_hhmm_ == -1) {
            historical_dispatcher_.RunHist();
          } else {
            historical_dispatcher_.RunHist(
                HFSAT::ttime_t(HFSAT::DateTime::GetTimeUTC(tradingdate_, global_end_utc_hhmm_), 0));
          }
        }
      }
    } catch (int e) {
      std::cerr << "Unable to run historical dispatcher\n";
    }
    std::cout << "Delete Sources" << std::endl;
    historical_dispatcher_.DeleteSources();
  }

  HFSAT::SecurityMarketViewPtrVec& getSMVMap() { return sid_to_smv_ptr_map_; }

  std::vector<HFSAT::MarketOrdersView*>& getMOVMap() { return sid_to_mov_ptr_map_; }

  HFSAT::SecurityMarketViewPtrVec& getSimSMVMap() { return sid_to_sim_smv_ptr_map_; }

  std::vector<HFSAT::BaseSimMarketMaker*>& getSMMMap() { return sid_to_smm_map_; }

  unsigned int getNumSecId() { return num_sec_id_; }

  HFSAT::TradingLocation_t getDepTradingLocation() { return dep_trading_location_; }

  HFSAT::HistoricalDispatcher& getHistoricalDispatcher() { return historical_dispatcher_; }

  HFSAT::Watch& getWatch() { return watch_; }

  HFSAT::DebugLogger& getLogger() { return dbglogger_; }

  HFSAT::MDSMessages::CombinedMDSMessagesDirectProcessor* getShmProcessor() {
    return combined_mds_messages_shm_processor_;
  }
  HFSAT::MDSMessages::CombinedMDSMessagesMultiShmProcessor* getBMFShmProcessor() {
    return combined_mds_messages_bmf_shm_processor_;
  }

  const char* getExchangleSymbol() { return exchange_symbol_vec_[0]; }

  void cleanup() {
    historical_dispatcher_.DeleteSources();
    for (auto i = 0u; i < sid_to_smv_ptr_map_.size(); i++) {
      if (sid_to_smv_ptr_map_[i] != NULL) {
        // std::cerr << "deleting " << sid_to_smv_ptr_map_[i]->shortcode() << "\n";
        delete (sid_to_smv_ptr_map_[i]);
      }
    }
    // this points to static reference !
    sid_to_smv_ptr_map_.clear();

    for (auto i = 0u; i < sid_to_sim_smv_ptr_map_.size(); i++) {
      if (sid_to_sim_smv_ptr_map_[i] != NULL) {
        // std::cerr << "deleting " << sid_to_sim_smv_ptr_map_[i]->shortcode() << "\n";
        delete (sid_to_sim_smv_ptr_map_[i]);
      }
    }
    sid_to_sim_smv_ptr_map_.clear();
  }

  HFSAT::IndexedNSEMarketViewManager2* indexed_nse_market_view_manager() {
    return indexed_nse_market_view_manager_.get();
  }

  HFSAT::GenericL1DataMarketViewManager* generic_l1_data_market_view_manager() {
    return generic_l1_data_market_view_manager_.get();
  }

  HFSAT::IndexedBSEMarketViewManager2* indexed_bse_market_view_manager() {
    return indexed_bse_market_view_manager_.get();
  }

  std::map<std::string, HFSAT::ExternalDataListener*>& getShcToFileSrcMap() { return shortcode_filesource_map_; }

  ~CommonInitializer() { cleanup(); };
};
