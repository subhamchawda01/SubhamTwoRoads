// =====================================================================================
//
//       Filename:  combined_source_book_viewer.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  02/02/2014 06:49:23 AM
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

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/random_channel.hpp"
#include "dvccode/CDef/mds_messages.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/global_sim_data_manager.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"

#include "dvccode/ExternalData/simple_live_dispatcher.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "baseinfra/MarketAdapter/market_defines.hpp"

#include "baseinfra/MarketAdapter/market_adapter_list.hpp"

#include "baseinfra/MarketAdapter/book_init_utils.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_shm_processor.hpp"

#include "dvccode/Profiler/cpucycle_profiler.hpp"

#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 22201225
#define BOOK_CHECK

class TestBook : public HFSAT::SecurityMarketViewChangeListener {
  const HFSAT::SecurityMarketView& this_smv_;
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::Watch& watch_;

  /// 0 : keep changing book and showing current book
  /// 1 : event by event
  /// 2 : keep changing book and refreshing screen till the specified time is reached
  int mode_;
  int pause_mfm_;
  int max_levels_;
  bool stuck_for_go_;
  unsigned long long traded_volume_;

 public:
  TestBook(const HFSAT::SecurityMarketView& _this_smv_, HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_,
           const int r_max_levels_, const int r_mode_)
      : this_smv_(_this_smv_),
        dbglogger_(_dbglogger_),
        watch_(_watch_),
        mode_(r_mode_),
        pause_mfm_(0),
        max_levels_(r_max_levels_),
        stuck_for_go_(false),
        traded_volume_(0) {
    printf("\033[3;1H");  // move to 3rd line

    {
      printf("%2s %5s %3s %11s %7s X %7s %11s %3s %5s %2s\n", "BL", "BS", "BO", "BP", "BIP", "AIP", "AP", "AO", "AS",
             "AL");
    }
  }

  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) {
    DefLoop();
  }

  inline void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                           const HFSAT::MarketUpdateInfo& _market_update_info_) {
    traded_volume_ += _trade_print_info_.size_traded_;
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  void DefLoop() {
    switch (mode_) {
      case 0: {
        ShowBook();
      } break;
      case 1: {
        ShowBook();
        stuck_for_go_ = true;
        AskForUserInput();
        while (stuck_for_go_) {
          sleep(1);
        }
      } break;
      case 2: {
        if ((pause_mfm_ <= 0) || (watch_.msecs_from_midnight() > pause_mfm_)) {
          /// after reaching specified time it should permit event
          /// by event lookup like mode 1
          if (pause_mfm_ > 0) {
            mode_ = 1;
            printf("\033[%d;1H", (max_levels_ + 4));
            printf("                                                                ");
            return;
          }
          ShowBook();
          stuck_for_go_ = true;
          AskForUserInput();
          while (stuck_for_go_) {
            sleep(1);
          }
        } else {
          ShowBook();
        }
      } break;
      case 3: {
        CheckBook();
      } break;
      default: { } break; }
  }

  void CheckBook() {
    if (this_smv_.IsError()) {
      // 	printf ( "ERROR \n" );
      // 	exit ( 0 );
    }
  }

  void ShowBook() {
    printf("\033[1;1H");  // move to 1st line
    printf("Time: %s\n", watch_.time_string());
    printf("Secname: %s   Traded Volume: %llu\n", this_smv_.secname(), traded_volume_);
    printf("\033[4;1H");  // move to 4th line

    max_levels_ = 15;

    int m_m_levels = std::min(max_levels_, std::min(this_smv_.NumBidLevels(), this_smv_.NumAskLevels()));
    for (int t_level_ = 0; t_level_ < m_m_levels; t_level_++) {
      switch (this_smv_.secname()[0]) {
        case 'Z':
        case 'U': {
          {
            printf("%2d %5d %3d %11.7f %7d X %7d %11.7f %3d %5d %2d\n", this_smv_.bid_int_price_level(t_level_),
                   this_smv_.bid_size(t_level_), this_smv_.bid_order(t_level_), this_smv_.bid_price(t_level_),
                   this_smv_.bid_int_price(t_level_), this_smv_.ask_int_price(t_level_), this_smv_.ask_price(t_level_),
                   this_smv_.ask_order(t_level_), this_smv_.ask_size(t_level_),
                   this_smv_.ask_int_price_level(t_level_));
          }
        } break;
        default: {
          {
            printf("%2d %5d %3d %11.7f %7d X %7d %11.7f %3d %5d %2d\n", this_smv_.bid_int_price_level(t_level_),
                   this_smv_.bid_size(t_level_), this_smv_.bid_order(t_level_), this_smv_.bid_price(t_level_),
                   this_smv_.bid_int_price(t_level_), this_smv_.ask_int_price(t_level_), this_smv_.ask_price(t_level_),
                   this_smv_.ask_order(t_level_), this_smv_.ask_size(t_level_),
                   this_smv_.ask_int_price_level(t_level_));
          }
        } break;
      }
    }

    if ((m_m_levels < max_levels_) && (m_m_levels < this_smv_.NumBidLevels())) {
      for (int t_level_ = m_m_levels; t_level_ < std::min(max_levels_, this_smv_.NumBidLevels()); t_level_++) {
        switch (this_smv_.secname()[0]) {
          case 'Z':
          case 'U': {
            {
              printf("%2d %5d %3d %11.7f %7d X %7s %11s %3s %5s %2s \n", this_smv_.bid_int_price_level(t_level_),
                     this_smv_.bid_size(t_level_), this_smv_.bid_order(t_level_), this_smv_.bid_price(t_level_),
                     this_smv_.bid_int_price(t_level_), "-", "-", "-", "-", "-");
            }
          } break;
          default: {
            {
              printf("%2d %5d %3d %11.7f %7d X %7s %11s %3s %5s %2s \n", this_smv_.bid_int_price_level(t_level_),
                     this_smv_.bid_size(t_level_), this_smv_.bid_order(t_level_), this_smv_.bid_price(t_level_),
                     this_smv_.bid_int_price(t_level_), "-", "-", "-", "-", "-");
            }
          } break;
        }
      }
    }

    if ((m_m_levels < max_levels_) && (m_m_levels < this_smv_.NumAskLevels())) {
      for (int t_level_ = m_m_levels; t_level_ < std::min(max_levels_, this_smv_.NumAskLevels()); t_level_++) {
        switch (this_smv_.secname()[0]) {
          case 'Z':
          case 'U': {
            {
              printf("%2s %5s %3s %11s %7s X %7d %11.7f %3d %5d %2d \n", "-", "-", "-", "-", "-",
                     this_smv_.ask_int_price(t_level_), this_smv_.ask_price(t_level_), this_smv_.ask_order(t_level_),
                     this_smv_.ask_size(t_level_), this_smv_.ask_int_price_level(t_level_));
            }
          } break;
          default: {
            {
              printf("%2s %5s %3s %11s %7s X %7d %11.7f %3d %5d %2d \n", "-", "-", "-", "-", "-",
                     this_smv_.ask_int_price(t_level_), this_smv_.ask_price(t_level_), this_smv_.ask_order(t_level_),
                     this_smv_.ask_size(t_level_), this_smv_.ask_int_price_level(t_level_));
            }
          } break;
        }
      }
    }
  }

  void PrintCmdStr() {
    printf("\033[%d;1H", (max_levels_ + 4));  // move to max_levels_ + 4th line
    if (mode_ != 2)
      printf("Enter command: ");
    else
      printf("Enter time to go to: ");
  }

  void AskForUserInput() {
    if (mode_ == 0) {
      sleep(1);
      return;
    }

    PrintCmdStr();

    const int kCmdLineBufferLen = 256;
    char readline_buffer_[kCmdLineBufferLen];
    std::cin.getline(readline_buffer_, kCmdLineBufferLen);
    // TODO process cmd

    HFSAT::PerishableStringTokenizer st_(readline_buffer_, kCmdLineBufferLen);
    const std::vector<const char*>& tokens_ = st_.GetTokens();
    if (mode_ == 1) {
      stuck_for_go_ = false;
    } else {
      if (mode_ == 2) {
        if (tokens_.size() > 0) {
          pause_mfm_ = atoi(tokens_[0]);
          stuck_for_go_ = false;
        }
      }
    }
  }
};

/// signal handler
void sighandler(int signum) {
  fprintf(stderr, "Received signal %d \n", signum);
  printf("\033[%d;1H", 20);
  std::cout << HFSAT::CpucycleProfiler::GetUniqueInstance().GetCpucycleSummaryString();
  HFSAT::CpucycleProfiler::GetUniqueInstance().FreeMem();  // free up cpu profiler mem
  exit(0);
}

int main(int argc, char** argv) {
  // intialize profiler
  HFSAT::CpucycleProfiler::SetUniqueInstance(30);
  HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(6, "On-book-update");

  /// set signal handler .. add other signals later
  struct sigaction sigact;
  sigact.sa_handler = sighandler;
  sigaction(SIGINT, &sigact, NULL);

  int max_levels_ = 50;
  int defmode_ = 0;
  if (argc < 2) {
    std::cerr << " usage : <exec> <shortcode> < MODE [ default = 0, NSE_L1 = 1]>\n";
    exit(0);
  }

  bool nse_l1_mode = false;

  std::string _this_shortcode_ = argv[1];

  int mode = 0;
  if (argc >= 3) {
    mode = atoi(argv[2]);
  }

  if (mode == 1) {
    nse_l1_mode = true;
  }

  HFSAT::DebugLogger dbglogger_(1024000, 1);
  // setup DebugLogger
  {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/logs/alllogs/combined_book_viewer_log." << _this_shortcode_ << "."
                << HFSAT::DateTime::GetCurrentIsoDateLocal();
    std::string logfilename_ = t_temp_oss_.str();
    dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out);
  }
  dbglogger_.AddLogLevel(BOOK_ERROR);
  dbglogger_.AddLogLevel(BOOK_INFO);
  dbglogger_.AddLogLevel(BOOK_TEST);

  int tradingdate_ = HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate();
  if (0 == strncmp(_this_shortcode_.c_str(), "NSE_", 4)) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();
  }

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);

  const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(_this_shortcode_);
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  sec_name_indexer_.AddString(exchange_symbol_, _this_shortcode_);

  HFSAT::Watch watch_(dbglogger_, HFSAT::DateTime::GetCurrentIsoDateLocal());
  unsigned int _this_sid_ = 0;

  HFSAT::ExchSource_t _this_exch_source_ =
      HFSAT::SecurityDefinitions::GetContractExchSource(_this_shortcode_, tradingdate_);

  if (_this_exch_source_ == HFSAT::kExchSourceEUREX) {
    _this_exch_source_ = HFSAT::kExchSourceEOBI;
  }

  /// This will only be used in historical
  /// In real life this should not matter since all locations receive
  //  HFSAT::TradingLocation_t r_trading_location_ = HFSAT::TradingLocationUtils::GetTradingLocationExch (
  //  _this_exch_source_ );

  HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_ =
      HFSAT::sid_to_security_market_view_map();  ///< Unique Instance of map from sid to p_smv_
  HFSAT::ShortcodeSecurityMarketViewMap& shortcode_smv_map_ =
      HFSAT::ShortcodeSecurityMarketViewMap::GetUniqueInstance();  ///< Unique Instance of map from shortcode to p_smv_
  bool set_temporary_bool_checking_if_this_is_an_indexed_book_ = HFSAT::CommonSimIndexedBookBool(
      _this_exch_source_, std::string("NOMATCH"));  // moved to a common function due to conflicts with smv analyser ;

  HFSAT::SecurityMarketView* p_smv_ = new HFSAT::SecurityMarketView(
      dbglogger_, watch_, sec_name_indexer_, _this_shortcode_, exchange_symbol_, _this_sid_, _this_exch_source_,
      set_temporary_bool_checking_if_this_is_an_indexed_book_);
  sid_to_smv_ptr_map_.push_back(p_smv_);                  // add to security_id_ to SMV* map
  shortcode_smv_map_.AddEntry(_this_shortcode_, p_smv_);  // add to shortcode_ to SMV* map

  HFSAT::IndexedLiffePriceLevelMarketViewManager indexed_liffe_price_level_market_view_manager_(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);
  HFSAT::IndexedMicexMarketViewManager indexed_micex_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                          sid_to_smv_ptr_map_);
  HFSAT::IndexedCmeMarketViewManager indexed_cme_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedNtpMarketViewManager indexed_ntp_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedBMFFpgaMarketViewManager indexed_bmf_fpga_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                               sid_to_smv_ptr_map_);
  HFSAT::IndexedEobiMarketViewManager indexed_eobi_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                        sid_to_smv_ptr_map_);
  HFSAT::IndexedEobiPriceLevelMarketViewManager indexed_eobi_price_feed_market_view_manager_(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);
  HFSAT::IndexedOsePriceFeedMarketViewManager indexed_ose_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                               sid_to_smv_ptr_map_);
  HFSAT::HKEXIndexedMarketViewManager indexed_hkex_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                        sid_to_smv_ptr_map_);
  HFSAT::IndexedCfeMarketViewManager indexed_cfe_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedIceMarketViewManager indexed_ice_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedOseOrderFeedMarketViewManager indexed_ose_order_feed_market_view_manager_(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);
  HFSAT::IndexedHKOMDPriceLevelMarketViewManager indexed_hkomd_market_book_manager_(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);
  HFSAT::IndexedNSEMarketViewManager indexed_nse_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::GenericL1DataMarketViewManager generic_l1_data_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                             sid_to_smv_ptr_map_);

  HFSAT::SimpleLiveDispatcher simple_live_dispatcher_;

  HFSAT::MDSMessages::CombinedMDSMessagesShmProcessor combined_mds_messages_shm_processor_(
      dbglogger_, sec_name_indexer_, HFSAT::kComShmConsumer);

  if (_this_exch_source_ == HFSAT::kExchSourceHONGKONG) {
    _this_exch_source_ = HFSAT::kExchSourceHKOMDPF;
  }
  switch (_this_exch_source_) {
    case HFSAT::kExchSourceCME: {
      p_smv_->InitializeSMVForIndexedBook();
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::CME, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_cme_market_view_manager_)), &watch_);
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::CME_LS, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_cme_market_view_manager_)),
          &watch_);

    } break;

    case HFSAT::kExchSourceEOBI: {
      p_smv_->InitializeSMVForIndexedBook();
      // At a time only 1 source will provide data
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::EUREX, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_cme_market_view_manager_)),
          &watch_);
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::EOBI_PF,
          (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_eobi_price_feed_market_view_manager_)), &watch_);
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::EOBI_LS,
          (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_eobi_price_feed_market_view_manager_)), &watch_);
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::EUREX_LS, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_cme_market_view_manager_)),
          &watch_);
    } break;

    case HFSAT::kExchSourceICE: {
      p_smv_->InitializeSMVForIndexedBook();
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::ICE, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ice_market_view_manager_)), &watch_);
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::ICE_LS, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ice_market_view_manager_)),
          &watch_);
    } break;
    case HFSAT::kExchSourceASX: {
      p_smv_->InitializeSMVForIndexedBook();
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::ASX, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ice_market_view_manager_)), &watch_);
    } break;
    case HFSAT::kExchSourceJPY: {
      p_smv_->InitializeSMVForIndexedBook();
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::OSE_ITCH_OF,
          (void*)((HFSAT::OrderLevelGlobalListener*)(&indexed_ose_order_feed_market_view_manager_)), &watch_);
    } break;
    case HFSAT::kExchSourceTMX: {
      p_smv_->InitializeSMVForIndexedBook();
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::TMX_OBF, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ice_market_view_manager_)),
          &watch_);
    } break;
    case HFSAT::kExchSourceLIFFE: {
      p_smv_->InitializeSMVForIndexedBook();
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::LIFFE,
          (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_liffe_price_level_market_view_manager_)), &watch_);
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::LIFFE_LS,
          (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_liffe_price_level_market_view_manager_)), &watch_);
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::ICE, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ice_market_view_manager_)), &watch_);
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::ICE_LS, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ice_market_view_manager_)),
          &watch_);
    } break;

    case HFSAT::kExchSourceRTS: {
      p_smv_->InitializeSMVForIndexedBook();
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::RTS, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_cme_market_view_manager_)), &watch_);
    } break;

    case HFSAT::kExchSourceMICEX:
    case HFSAT::kExchSourceMICEX_CR:
    case HFSAT::kExchSourceMICEX_EQ: {
      p_smv_->InitializeSMVForIndexedBook();
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::MICEX, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_micex_market_view_manager_)),
          &watch_);
    } break;

    case HFSAT::kExchSourceNTP:
    case HFSAT::kExchSourceBMF: {
      p_smv_->InitializeSMVForIndexedBook();
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::NTP, (void*)((HFSAT::NTPPriceLevelGlobalListener*)(&indexed_ntp_market_view_manager_)),
          &watch_);
      combined_mds_messages_shm_processor_.AddFPGADataSourceForProcessing(
          HFSAT::MDS_MSG::NTP, (void*)((HFSAT::BMFFPGAFullBookGlobalListener*)(&indexed_bmf_fpga_market_view_manager_)),
          &watch_);
    } break;
    case HFSAT::kExchSourceBMFEQ: {
      p_smv_->InitializeSMVForIndexedBook();
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::BMF_EQ, (void*)((HFSAT::NTPPriceLevelGlobalListener*)(&indexed_ntp_market_view_manager_)),
          &watch_);
    } break;

    //    case HFSAT::kExchSourceJPY: {
    //      p_smv_->InitializeSMVForIndexedBook();
    //      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
    //          HFSAT::MDS_MSG::OSE_CF, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ose_market_view_manager_)),
    //          &watch_);
    //    } break;

    case HFSAT::kExchSourceHONGKONG: {
      p_smv_->InitializeSMVForIndexedBook();
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::HKEX, (void*)((HFSAT::HKHalfBookGlobalListener*)(&indexed_hkex_market_view_manager_)),
          &watch_);
    } break;
    case HFSAT::kExchSourceHKOMDPF: {
      p_smv_->InitializeSMVForIndexedBook();
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::HKOMDPF, (void*)((HFSAT::HKHalfBookGlobalListener*)(&indexed_hkomd_market_book_manager_)),
          &watch_);
    } break;
    case HFSAT::kExchSourceCFE: {
      p_smv_->InitializeSMVForIndexedBook();
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::CSM, (void*)((HFSAT::CFEPriceLevelGlobalListener*)&(indexed_cfe_market_view_manager_)),
          &watch_);
    } break;
    case HFSAT::kExchSourceNSE: {
      if (nse_l1_mode) {
        p_smv_->InitializeSMVForIndexedBook();
        combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::NSE_L1, (void*)((HFSAT::L1DataListener*)&(generic_l1_data_market_view_manager_)), &watch_);
      } else {
        p_smv_->InitializeSMVForIndexedBook();
        combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::NSE, (void*)((HFSAT::OrderGlobalListenerNSE*)&(indexed_nse_market_view_manager_)), &watch_);
      }
    } break;
    case HFSAT::kExchSourceSGX: {
      p_smv_->InitializeSMVForIndexedBook();
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::SGX, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ice_market_view_manager_)), &watch_);
    } break;
    default:
      fprintf(stderr, "Not implemented for exchange %d", _this_exch_source_);
      exit(1);
  }

  printf("\033c");  // clear screen

  TestBook test_book_(*p_smv_, dbglogger_, watch_, max_levels_, defmode_);

  p_smv_->subscribe_price_type(&test_book_, HFSAT::kPriceTypeMktSizeWPrice);
  if (max_levels_ > 1) {
    p_smv_->subscribe_L2(&test_book_);
  }

  combined_mds_messages_shm_processor_.RunLiveShmSource();

  return 0;
}
