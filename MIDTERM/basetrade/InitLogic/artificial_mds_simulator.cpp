/**
   \file InitLogic/artificial_mds_simulator.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include "basetrade/InitLogic/artificial_mds_simulator.hpp"
#include "baseinfra/MarketAdapter/mds_mcast.hpp"
#include "dvccode/Utils/CPUAffinity.hpp"
#include <map>
#include <set>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 20991225

HFSAT::DebugLogger dbglogger_(1024000, 1);

// One exchange can have multiple shortcodes added. So we need to have a map from shortcode -> filesource
// There is one filesource for each shortcode [ MarketViewMangers are one for each exchange. ]
std::map<std::string, HFSAT::CMELoggedMessageFileSource*> shortcode_cme_data_filesource_map_;
std::map<std::string, HFSAT::EUREXLoggedMessageFileSource*> shortcode_eurex_data_filesource_map_;
std::map<std::string, HFSAT::EOBIPriceFeedLoggedMessageFileSource*> shortcode_eobi_price_feed_source_map_;
std::map<std::string, HFSAT::LIFFELoggedMessageFileSource*> shortcode_liffe_data_filesource_map_;
std::map<std::string, HFSAT::ICELoggedMessageFileSource*> shortcode_ice_data_filesource_map_;
std::map<std::string, HFSAT::TMXPFLoggedMessageFileSource*> shortcode_tmx_pf_data_filesource_map_;
std::map<std::string, HFSAT::RTSLoggedMessageFileSource*> shortcode_rts_data_filesource_map_;
std::map<std::string, HFSAT::MICEXLoggedMessageFileSource*> shortcode_micex_data_filesource_map_;
std::map<std::string, HFSAT::HKOMDPFLoggedMessageFileSource*> shortcode_hkomd_pricefeed_filesource_map_;
std::map<std::string, HFSAT::HKEXLoggedMessageFileSource*> shortcode_hkex_data_filesource_map_;
std::map<std::string, HFSAT::OSEPriceFeedLoggedMessageFileSource*> shortcode_ose_data_filesource_map_;
std::map<std::string, HFSAT::CFELoggedMessageFileSource*> shortcode_cfe_data_filesource_map_;
std::map<std::string, HFSAT::ASXPFLoggedMessageFileSource*> shortcode_asx_pf_data_filesource_map_;
std::map<std::string, HFSAT::SGXPFLoggedMessageFileSource*> shortcode_sgx_pf_data_filesource_map_;
std::map<std::string, HFSAT::BSEPFLoggedMessageFileSource*> shortcode_bse_pf_data_filesource_map_;
std::map<std::string, HFSAT::CHIXL1LoggedMessageFileSource*> shortcode_chix_l1_data_filesource_map_;
std::map<std::string, HFSAT::OSEPFLoggedMessageFileSource*> shortcode_ose_pf_data_filesource_map_;
std::map<std::string, HFSAT::OSEL1LoggedMessageFileSource*> shortcode_ose_l1_data_filesource_map_;
std::map<std::string, HFSAT::KRXLoggedMessageFileSource*> shortcode_krx_data_filesource_map_;
std::map<std::string, HFSAT::ORSMessageFileSource*> shortcode_ors_data_filesource_map_;
std::map<std::string, HFSAT::NTPLoggedMessageFileSource*> shortcode_ntp_data_filesource_map_;
std::map<std::string, HFSAT::NSELoggedMessageFileSource*> shortcode_nse_data_filesource_map_;

bool IsHKEquity(std::string _shortcode) { return _shortcode.substr(0, 3) == "HK_"; }

void InitializeHistoricalDispatcher(HFSAT::HistoricalDispatcher& historical_dispatcher_,
                                    HFSAT::SecurityNameIndexer& sec_name_indexer_,
                                    std::vector<std::string>& real_shortcode_set_,
                                    std::set<std::string>& real_ors_shortcode_set, int tradingdate_,
                                    std::unique_ptr<HFSAT::MDSMcast>& mds_mcast, HFSAT::Watch& watch_) {
  for (auto it = real_shortcode_set_.begin(); it != real_shortcode_set_.end(); ++it) {
    std::string shortcode_ = *it;
    int secid = sec_name_indexer_.GetIdFromString(shortcode_);
    const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);
    HFSAT::ExchSource_t t_exch_source_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, tradingdate_);
    HFSAT::TradingLocation_t exch_trading_location_ =
        HFSAT::TradingLocationUtils::GetTradingLocationExch(t_exch_source_);

    // To Do :- Yet to add support for ORS, Control Msg & CFE File sources
    switch (t_exch_source_) {
      case HFSAT::kExchSourceCME: {
        if (shortcode_cme_data_filesource_map_.find(shortcode_) == shortcode_cme_data_filesource_map_.end()) {
          shortcode_cme_data_filesource_map_[shortcode_] = new HFSAT::CMELoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);
          shortcode_cme_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(mds_mcast.get());
          shortcode_cme_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_cme_data_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceBMF: {
        if (shortcode_ntp_data_filesource_map_.find(shortcode_) == shortcode_ntp_data_filesource_map_.end()) {
          shortcode_ntp_data_filesource_map_[shortcode_] =
              new HFSAT::NTPLoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, secid,
                                                    exchange_symbol_, exch_trading_location_, false, false, false);

          shortcode_ntp_data_filesource_map_[shortcode_]->SetNTPPriceLevelGlobalListener(mds_mcast.get());
          shortcode_ntp_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_ntp_data_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceEUREX:
      case HFSAT::kExchSourceEOBI: {
        if (shortcode_eobi_price_feed_source_map_.find(shortcode_) == shortcode_eobi_price_feed_source_map_.end()) {
          shortcode_eobi_price_feed_source_map_[shortcode_] = new HFSAT::EOBIPriceFeedLoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);

          shortcode_eobi_price_feed_source_map_[shortcode_]->SetPriceLevelGlobalListener(mds_mcast.get());
          shortcode_eobi_price_feed_source_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_eobi_price_feed_source_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceRTS: {
        if (shortcode_rts_data_filesource_map_.find(shortcode_) == shortcode_rts_data_filesource_map_.end()) {
          shortcode_rts_data_filesource_map_[shortcode_] = new HFSAT::RTSLoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);

          shortcode_rts_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(mds_mcast.get());
          shortcode_rts_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_rts_data_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceMICEX:
      case HFSAT::kExchSourceMICEX_CR:
      case HFSAT::kExchSourceMICEX_EQ: {
        if (shortcode_micex_data_filesource_map_.find(shortcode_) == shortcode_micex_data_filesource_map_.end()) {
          shortcode_micex_data_filesource_map_[shortcode_] = new HFSAT::MICEXLoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);

          shortcode_micex_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(mds_mcast.get());
          shortcode_micex_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_micex_data_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceHONGKONG:
      case HFSAT::kExchSourceHKOMD:
      case HFSAT::kExchSourceHKOMDCPF:
      case HFSAT::kExchSourceHKOMDPF: {
        if (shortcode_hkomd_pricefeed_filesource_map_.find(shortcode_) ==
            shortcode_hkomd_pricefeed_filesource_map_.end()) {
          shortcode_hkomd_pricefeed_filesource_map_[shortcode_] = new HFSAT::HKOMDPFLoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false,
              false, IsHKEquity(shortcode_));

          shortcode_hkomd_pricefeed_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(mds_mcast.get());
          shortcode_hkomd_pricefeed_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_hkomd_pricefeed_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceBATSCHI: {
        if (shortcode_chix_l1_data_filesource_map_.find(shortcode_) == shortcode_chix_l1_data_filesource_map_.end()) {
          shortcode_chix_l1_data_filesource_map_[shortcode_] = new HFSAT::CHIXL1LoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);

          shortcode_chix_l1_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(mds_mcast.get());
          shortcode_chix_l1_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_chix_l1_data_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceLIFFE: {
        if (shortcode_liffe_data_filesource_map_.find(shortcode_) == shortcode_liffe_data_filesource_map_.end()) {
          shortcode_liffe_data_filesource_map_[shortcode_] = new HFSAT::LIFFELoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);

          shortcode_liffe_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(mds_mcast.get());
          shortcode_liffe_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_liffe_data_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceICE: {
        if (shortcode_ice_data_filesource_map_.find(shortcode_) == shortcode_ice_data_filesource_map_.end()) {
          shortcode_ice_data_filesource_map_[shortcode_] =
              new HFSAT::ICELoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, secid,
                                                    exchange_symbol_, exch_trading_location_, false, false);

          shortcode_ice_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(mds_mcast.get());
          shortcode_ice_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_ice_data_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceTMX: {
        if (tradingdate_ >= USING_TMX_OBF_FROM) {
          if (shortcode_tmx_pf_data_filesource_map_.find(shortcode_) == shortcode_tmx_pf_data_filesource_map_.end()) {
            shortcode_tmx_pf_data_filesource_map_[shortcode_] = new HFSAT::TMXPFLoggedMessageFileSource(
                dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);

            shortcode_tmx_pf_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(mds_mcast.get());
            shortcode_tmx_pf_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
            historical_dispatcher_.AddExternalDataListener(shortcode_tmx_pf_data_filesource_map_[shortcode_]);
          }
        }
      } break;
      case HFSAT::kExchSourceASX: {
        if (shortcode_asx_pf_data_filesource_map_.find(shortcode_) == shortcode_asx_pf_data_filesource_map_.end()) {
          shortcode_asx_pf_data_filesource_map_[shortcode_] = new HFSAT::ASXPFLoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);

          shortcode_asx_pf_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(mds_mcast.get());
          shortcode_asx_pf_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_asx_pf_data_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceSGX: {
        if (shortcode_sgx_pf_data_filesource_map_.find(shortcode_) == shortcode_sgx_pf_data_filesource_map_.end()) {
          shortcode_sgx_pf_data_filesource_map_[shortcode_] = new HFSAT::SGXPFLoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);

          shortcode_sgx_pf_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(mds_mcast.get());
          shortcode_sgx_pf_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_sgx_pf_data_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceJPY: {
        // ose dependant shortcode OR CME dependant , use order-level data for ALL ose sources.
        if (shortcode_ose_pf_data_filesource_map_.find(shortcode_) == shortcode_ose_pf_data_filesource_map_.end()) {
          shortcode_ose_pf_data_filesource_map_[shortcode_] = new HFSAT::OSEPFLoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);

          shortcode_ose_pf_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(mds_mcast.get());
          shortcode_ose_pf_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_ose_pf_data_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceCFE: {
        if (shortcode_cfe_data_filesource_map_.find(shortcode_) == shortcode_cfe_data_filesource_map_.end()) {
          shortcode_cfe_data_filesource_map_[shortcode_] = new HFSAT::CFELoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);
          shortcode_cfe_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(mds_mcast.get());
          shortcode_cfe_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_cfe_data_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceNSE: {
        if (shortcode_nse_data_filesource_map_.find(shortcode_) == shortcode_nse_data_filesource_map_.end()) {
          shortcode_nse_data_filesource_map_[shortcode_] = new HFSAT::NSELoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);
          shortcode_nse_data_filesource_map_[shortcode_]->SetOrderGlobalListenerNSE(
              (HFSAT::OrderGlobalListenerNSE*)mds_mcast.get());
          shortcode_nse_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_nse_data_filesource_map_[shortcode_]);
        }
      } break;
      default: { } break; }
  }
}

void InitializeSecurityNameIndexer(HFSAT::SecurityNameIndexer& sec_name_indexer_, int& tradingdate_,
                                   std::vector<std::string> shortcode_vec_) {
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  for (unsigned int i = 0; i < shortcode_vec_.size(); i++) {
    const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_vec_[i]);
    if (!sec_name_indexer_.HasString(shortcode_vec_[i])) {
      sec_name_indexer_.AddString(exchange_symbol_, shortcode_vec_[i]);
    }
  }
}

int ConvertUTCStringToTime(char* time_string_hhmm_, int trading_date){
  char* tz_ = NULL;

  if ((strncmp(time_string_hhmm_, "EST_", 4) == 0) || (strncmp(time_string_hhmm_, "CST_", 4) == 0) ||
      (strncmp(time_string_hhmm_, "CET_", 4) == 0) || (strncmp(time_string_hhmm_, "BRT_", 4) == 0) ||
      (strncmp(time_string_hhmm_, "UTC_", 4) == 0) || (strncmp(time_string_hhmm_, "KST_", 4) == 0) ||
      (strncmp(time_string_hhmm_, "HKT_", 4) == 0) || (strncmp(time_string_hhmm_, "MSK_", 4) == 0) ||
      (strncmp(time_string_hhmm_, "IST_", 4) == 0) || (strncmp(time_string_hhmm_, "JST_", 4) == 0) ||
      (strncmp(time_string_hhmm_, "BST_", 4) == 0) || (strncmp(time_string_hhmm_, "AST_", 4) == 0)) {
    tz_ = time_string_hhmm_;
    time_string_hhmm_ = time_string_hhmm_ + 4;
    int hhmmss = atoi(time_string_hhmm_);
    if (hhmmss < 10000) {
      hhmmss *= 100;
    }
  }

  return HFSAT::DateTime::GetUTCHHMMFromTZHHMM(trading_date, atoi(time_string_hhmm_), tz_);
}

void ParseCommandLineArgs(int argc, char** argv, int& trading_date, std::vector<std::string>& input_shortcodes,
                          std::string& ip, int& port, std::string& iface, int& usecs_sleep, int& seek_time, int& end_time) {
  if (argc < 7) {
    std::cout << "Usage: <exec> <YYYYMMDD> <ShortcodeListFile> <IP> <Port> <Iface> <UsecsToSleep> <SeekTime> <EndTime(opt)>";
    exit(-1);
  } else {
    trading_date = atoi(argv[1]);

    std::string file = argv[2];

    char line[1024];
    std::ifstream sec_file_;
    sec_file_.open(file.c_str(), std::ifstream::in);
    if (!sec_file_.is_open()) {
      std::cerr << file << "  FILE DOESNOT EXIST " << std::endl;
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

      input_shortcodes.push_back(std::string(tokens[0]));
    }

    ip = argv[3];
    port = atoi(argv[4]);
    iface = argv[5];
    usecs_sleep = atoi(argv[6]);
    seek_time = 0;

    if (argc >= 8) {
      char* start_hhmm_ = argv[7];
      seek_time = ConvertUTCStringToTime(start_hhmm_, trading_date);
      if (argc >= 9) {
        char* end_hhmm_ = argv[8];
        end_time = ConvertUTCStringToTime(end_hhmm_, trading_date);
      }
    }
  }
}
/* Usage of the exec :- sim_real_packet_order_mismatch_detector trading_date_YYYYMMDD ExchangeTradingLocation
 */
int main(int argc, char** argv) {
  std::vector<std::string> affinity_process_list_vec_;
  process_type_map process_and_type_;
  process_and_type_ = AffinityAllocator::parseProcessListFile(affinity_process_list_vec_);

  int threadId = ((int)(syscall(SYS_gettid)));
  std::string name = "ArtificalMDSSimulator";
  int core_alloced_ =
      CPUManager::allocateFirstBestAvailableCore(process_and_type_, affinity_process_list_vec_, threadId, name, true);

  std::cout << "Thread : " << name << " Thread ID : " << threadId << " CORE # " << core_alloced_ << std::endl;

  HFSAT::HistoricalDispatcher historical_dispatcher_;
  /// SecurityNameIndexer is needed to extract the short code from a given Exchange symbol
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();

  std::unique_ptr<HFSAT::MDSMcast> mds_mcast = nullptr;
  // Vector of all the short codes defined in the Contract Specification Map
  std::vector<std::string> contract_spec_shortcode_vec_;
  // Stores the shortcodes extracted for given exchange symbols found in the in Order live data file
  std::set<std::string> real_ors_shortcode_set;
  std::map<std::string, std::string> exch_symbol_short_code;

  // Command line arguments
  int trading_date;
  std::vector<std::string> input_shcs_;
  std::string ip, iface;
  int port;
  int usecs_sleep;
  int seek_time;
  int end_time = -1;

  ParseCommandLineArgs(argc, argv, trading_date, input_shcs_, ip, port, iface, usecs_sleep, seek_time, end_time);

  HFSAT::Watch watch_(dbglogger_, trading_date);

  // The file sources shouldn't fallback to the primary location in case the file is not present in the trading location
  bool skip_primary_loc = false;
  HFSAT::LoggedMessageFileNamer::SetSkipPrimaryLoc(skip_primary_loc);

  HFSAT::SecurityDefinitions::GetUniqueInstance(trading_date).LoadNSESecurityDefinitions();

  InitializeSecurityNameIndexer(sec_name_indexer_, trading_date, input_shcs_);

  mds_mcast = std::unique_ptr<HFSAT::MDSMcast>(new HFSAT::MDSMcast(ip, port, iface, usecs_sleep));

  InitializeHistoricalDispatcher(historical_dispatcher_, sec_name_indexer_, input_shcs_, real_ors_shortcode_set,
                                 trading_date, mds_mcast, watch_);

  // Set the start time of Historical Dispatcher 1usec before the time of earliest event
  // This is done to ensure that all events prior to the first event in the Real packets file are skipped
  historical_dispatcher_.SeekHistFileSourcesTo(HFSAT::ttime_t(HFSAT::DateTime::GetTimeUTC(trading_date, seek_time), 0));
  if (end_time != -1)
    historical_dispatcher_.RunHist(HFSAT::ttime_t(HFSAT::DateTime::GetTimeUTC(trading_date, end_time), 0));
  else
    historical_dispatcher_.RunHist();

  return 0;
}
