/**
   \file InitLogic/sim_real_packet_order_mismatch_detector.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include <map>
#include <set>

#include "basetrade/InitLogic/sim_real_packet_order_mismatch_detector.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "baseinfra/MarketAdapter/sim_real_pf_manager.hpp"

#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 22201225

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
std::map<std::string, HFSAT::PUMALoggedMessageFileSource*> shortcode_puma_data_filesource_map_;
HFSAT::ControlMessageFileSource* control_message_file_source_ptr_ = NULL;
HFSAT::AFLASHLoggedMessageFileSource* aflash_message_file_source_ptr_ = NULL;
bool aflash_message_present_ = false;
bool control_message_present_ = false;

bool IsHKEquity(const std::string _shortcode) { return _shortcode.substr(0, 3) == "HK_"; }

template <class T>
T ReadNextEventFromStruct(HFSAT::BulkFileReader& bulk_file_reader_, std::string& exchange_symbol) {
  T next_event;
  size_t mds_available_len_;
  mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(T));
  if (mds_available_len_ == sizeof(T)) {
    exchange_symbol = std::string(next_event.getContract());
  } else {
    std::cerr << "Unable to read data from real file \n";
    exit(1);
  }
  return next_event;
}

void GetSimStartAndEndTime(const timeval& event_time, timeval& start_time, timeval& end_time) {
  // Initialize the earliest time with 0 sec & 0 usec
  // The earliest time is set as soon as the first event is found
  if (start_time.tv_sec == 0 && start_time.tv_usec == 0) {
    start_time = event_time;
  }
  end_time = event_time;
}

void InsertRealORSShortCode(const std::string exchange_symbol,
                            const std::map<std::string, std::string>& exch_symbol_short_code, const timeval& event_time,
                            timeval& start_time, timeval& end_time, std::set<std::string>& real_ors_shortcode_set) {
  std::string ors_prefix_("ORS_");
  std::string actual_exch_symbol_ = exchange_symbol.substr(ors_prefix_.size());

  auto it = exch_symbol_short_code.find(actual_exch_symbol_);
  if (it != exch_symbol_short_code.end()) {
    GetSimStartAndEndTime(event_time, start_time, end_time);
    real_ors_shortcode_set.insert(it->second);
  }
}

void PopulateShortCodeSetFromRealData(const std::map<std::string, std::string>& exch_symbol_short_code,
                                      int trading_date, HFSAT::BulkFileReader& bulk_file_reader_,
                                      std::set<std::string>& real_shortcode_set_,
                                      std::set<std::string>& real_ors_shortcode_set, timeval& start_time,
                                      timeval& end_time) {
  HFSAT::MDS_MSG::MDSMessageExchType exch_type;
  if (bulk_file_reader_.is_open()) {
    while (true) {
      size_t exch_available_len_ = bulk_file_reader_.read(&exch_type, sizeof(HFSAT::MDS_MSG::MDSMessageExchType));
      if (exch_available_len_ < sizeof(exch_type)) break;

      std::string exchange_symbol;
      timeval event_time{0, 0};
      switch (exch_type) {
        case HFSAT::MDS_MSG::CME_LS:
        case HFSAT::MDS_MSG::CME: {
          CME_MDS::CMECommonStruct next_event =
              ReadNextEventFromStruct<CME_MDS::CMECommonStruct>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.time_;
        } break;
        case HFSAT::MDS_MSG::LIFFE:
        case HFSAT::MDS_MSG::LIFFE_LS: {
          LIFFE_MDS::LIFFECommonStruct next_event =
              ReadNextEventFromStruct<LIFFE_MDS::LIFFECommonStruct>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.time_;
        } break;
        case HFSAT::MDS_MSG::EUREX_LS:
        case HFSAT::MDS_MSG::EUREX:
        case HFSAT::MDS_MSG::EOBI_LS: {
          EUREX_MDS::EUREXCommonStruct next_event =
              ReadNextEventFromStruct<EUREX_MDS::EUREXCommonStruct>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.time_;
        } break;
        case HFSAT::MDS_MSG::CONTROL: {
          HFSAT::GenericControlRequestStruct next_event =
              ReadNextEventFromStruct<HFSAT::GenericControlRequestStruct>(bulk_file_reader_, exchange_symbol);
          event_time.tv_sec = next_event.time_set_by_frontend_.tv_sec;
          event_time.tv_usec = next_event.time_set_by_frontend_.tv_usec;
          control_message_present_ = true;
        } break;
        case HFSAT::MDS_MSG::ORS_REPLY: {
          HFSAT::GenericORSReplyStructLive next_event =
              ReadNextEventFromStruct<HFSAT::GenericORSReplyStructLive>(bulk_file_reader_, exchange_symbol);
          event_time.tv_sec = next_event.time_set_by_server_.tv_sec;
          event_time.tv_usec = next_event.time_set_by_server_.tv_usec;
          InsertRealORSShortCode(exchange_symbol, exch_symbol_short_code, event_time, start_time, end_time,
                                 real_ors_shortcode_set);
          continue;
        } break;
        case HFSAT::MDS_MSG::RTS: {
          RTS_MDS::RTSCommonStruct next_event =
              ReadNextEventFromStruct<RTS_MDS::RTSCommonStruct>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.time_;
        } break;
        // Same Handling for MICEX_CR and MICEX_EQ
        case HFSAT::MDS_MSG::MICEX: {
          MICEX_MDS::MICEXCommonStruct next_event =
              ReadNextEventFromStruct<MICEX_MDS::MICEXCommonStruct>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.time_;
        } break;
        case HFSAT::MDS_MSG::NTP:
        case HFSAT::MDS_MSG::NTP_LS: {
          NTP_MDS::NTPCommonStruct next_event =
              ReadNextEventFromStruct<NTP_MDS::NTPCommonStruct>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.time_;
        } break;
        case HFSAT::MDS_MSG::BMF_EQ: {  // BMF_EQ and NTP have same common struct
          NTP_MDS::NTPCommonStruct next_event =
              ReadNextEventFromStruct<NTP_MDS::NTPCommonStruct>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.time_;
        } break;
        case HFSAT::MDS_MSG::EOBI_PF: {
          EUREX_MDS::EUREXCommonStruct next_event =
              ReadNextEventFromStruct<EUREX_MDS::EUREXCommonStruct>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.time_;
        } break;
        case HFSAT::MDS_MSG::OSE_PF: {
          OSE_MDS::OSEPriceFeedCommonStruct next_event =
              ReadNextEventFromStruct<OSE_MDS::OSEPriceFeedCommonStruct>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.time_;
        } break;
        case HFSAT::MDS_MSG::OSE_CF: {
          OSE_MDS::OSECombinedCommonStruct next_event =
              ReadNextEventFromStruct<OSE_MDS::OSECombinedCommonStruct>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.time_;
        } break;
        case HFSAT::MDS_MSG::CSM: {
          CSM_MDS::CSMCommonStruct next_event =
              ReadNextEventFromStruct<CSM_MDS::CSMCommonStruct>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.time_;
        } break;
        case HFSAT::MDS_MSG::OSE_L1: {
          OSE_MDS::OSEPLCommonStruct next_event =
              ReadNextEventFromStruct<OSE_MDS::OSEPLCommonStruct>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.time_;
        } break;
        case HFSAT::MDS_MSG::TMX:
        case HFSAT::MDS_MSG::TMX_LS: {
          ReadNextEventFromStruct<TMX_MDS::TMXLSCommonStruct>(bulk_file_reader_, exchange_symbol);
          // The event doesn't contain a time field in it
        } break;
        case HFSAT::MDS_MSG::TMX_OBF: {
          TMX_OBF_MDS::TMXPFCommonStruct next_event =
              ReadNextEventFromStruct<TMX_OBF_MDS::TMXPFCommonStruct>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.time_;
        } break;
        case HFSAT::MDS_MSG::ICE:
        case HFSAT::MDS_MSG::ICE_LS: {
          ICE_MDS::ICECommonStructLive next_event =
              ReadNextEventFromStruct<ICE_MDS::ICECommonStructLive>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.time_;
        } break;
        case HFSAT::MDS_MSG::ASX: {
          ASX_MDS::ASXPFCommonStruct next_event =
              ReadNextEventFromStruct<ASX_MDS::ASXPFCommonStruct>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.time_;
        } break;
        case HFSAT::MDS_MSG::SGX: {
          SGX_MDS::SGXPFCommonStruct next_event =
              ReadNextEventFromStruct<SGX_MDS::SGXPFCommonStruct>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.time_;
        } break;
        case HFSAT::MDS_MSG::OSE_ITCH_PF: {
          OSE_ITCH_MDS::OSEPFCommonStruct next_event =
              ReadNextEventFromStruct<OSE_ITCH_MDS::OSEPFCommonStruct>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.time_;
        } break;
        case HFSAT::MDS_MSG::HKOMDPF: {
          HKOMD_MDS::HKOMDPFCommonStruct next_event =
              ReadNextEventFromStruct<HKOMD_MDS::HKOMDPFCommonStruct>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.time_;
        } break;
        case HFSAT::MDS_MSG::AFLASH: {
          AFLASH_MDS::AFlashCommonStructLive next_event =
              ReadNextEventFromStruct<AFLASH_MDS::AFlashCommonStructLive>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.time_;
          aflash_message_present_ = true;
        } break;
        case HFSAT::MDS_MSG::RETAIL: {
          RETAIL_MDS::RETAILCommonStruct next_event =
              ReadNextEventFromStruct<RETAIL_MDS::RETAILCommonStruct>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.time_;
        } break;
        case HFSAT::MDS_MSG::NSE: {
          NSE_MDS::NSETBTDataCommonStruct next_event =
              ReadNextEventFromStruct<NSE_MDS::NSETBTDataCommonStruct>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.source_time;
        } break;
        case HFSAT::MDS_MSG::NSE_L1: {
          HFSAT::GenericL1DataStruct next_event =
              ReadNextEventFromStruct<HFSAT::GenericL1DataStruct>(bulk_file_reader_, exchange_symbol);
          event_time = {next_event.time.tv_sec, next_event.time.tv_usec};
        } break;
        case HFSAT::MDS_MSG::EOBI_OF: {
          EOBI_MDS::EOBICompactOrder next_event =
              ReadNextEventFromStruct<EOBI_MDS::EOBICompactOrder>(bulk_file_reader_, exchange_symbol);
          event_time = next_event.time_;
        } break;
        default: {
          std::cerr << " invalid exchange type read from the Real packets file. "
                    << "exch_type: " << exch_type << std::endl;
          exit(1);
        } break;
      }

      auto it = exch_symbol_short_code.find(exchange_symbol);
      if (it != exch_symbol_short_code.end()) {
        GetSimStartAndEndTime(event_time, start_time, end_time);
        real_shortcode_set_.insert(it->second);
      }
    }
    bulk_file_reader_.close();
  }
}

void InitializeHistoricalDispatcher(const int tradingdate_, const std::set<std::string>& real_shortcode_set_,
                                    const std::set<std::string>& real_ors_shortcode_set,
                                    HFSAT::SecurityNameIndexer& sec_name_indexer_, HFSAT::Watch& watch_,
                                    std::unique_ptr<HFSAT::SimRealPFManager>& sim_real_pf_manager,
                                    HFSAT::TradingLocation_t exch_trading_location_,
                                    HFSAT::HistoricalDispatcher& historical_dispatcher_) {
  for (auto it = real_shortcode_set_.begin(); it != real_shortcode_set_.end(); ++it) {
    std::string shortcode_ = *it;
    int secid = sec_name_indexer_.GetIdFromString(shortcode_);
    const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);
    HFSAT::ExchSource_t t_exch_source_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, tradingdate_);

    if (!control_message_file_source_ptr_ && control_message_present_) {
      bool control_file_present = true;
      int query_id = 1;
      control_message_file_source_ptr_ =
          new HFSAT::ControlMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_,
                                              exch_trading_location_, query_id, control_file_present);
      control_message_file_source_ptr_->AddControlMessageListener(sim_real_pf_manager.get());
      control_message_file_source_ptr_->SetExternalTimeListener(&watch_);
      historical_dispatcher_.AddExternalDataListener(control_message_file_source_ptr_);
    }
    if (!aflash_message_file_source_ptr_ && aflash_message_present_) {
      aflash_message_file_source_ptr_ =
          new HFSAT::AFLASHLoggedMessageFileSource(dbglogger_, tradingdate_, exch_trading_location_);
      aflash_message_file_source_ptr_->AddPriceLevelGlobalListener(sim_real_pf_manager.get());
      aflash_message_file_source_ptr_->SetExternalTimeListener(&watch_);
      historical_dispatcher_.AddExternalDataListener(aflash_message_file_source_ptr_);
    }

    // To Do :- Yet to add support for ORS, Control Msg & CFE File sources
    if (real_ors_shortcode_set.find(shortcode_) != real_ors_shortcode_set.end()) {
      if (shortcode_ors_data_filesource_map_.find(shortcode_) == shortcode_ors_data_filesource_map_.end()) {
        shortcode_ors_data_filesource_map_[shortcode_] = new HFSAT::ORSMessageFileSource(
            dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_);

        shortcode_ors_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(sim_real_pf_manager.get());
        shortcode_ors_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
        historical_dispatcher_.AddExternalDataListener(shortcode_ors_data_filesource_map_[shortcode_]);
      }
    }
    switch (t_exch_source_) {
      case HFSAT::kExchSourceCME: {
        if (shortcode_cme_data_filesource_map_.find(shortcode_) == shortcode_cme_data_filesource_map_.end()) {
          shortcode_cme_data_filesource_map_[shortcode_] = new HFSAT::CMELoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);
          shortcode_cme_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(sim_real_pf_manager.get());
          shortcode_cme_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_cme_data_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceEUREX:
      case HFSAT::kExchSourceEOBI: {
        if (shortcode_eobi_price_feed_source_map_.find(shortcode_) == shortcode_eobi_price_feed_source_map_.end()) {
          shortcode_eobi_price_feed_source_map_[shortcode_] = new HFSAT::EOBIPriceFeedLoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);
          shortcode_eobi_price_feed_source_map_[shortcode_]->SetPriceLevelGlobalListener(sim_real_pf_manager.get());
          shortcode_eobi_price_feed_source_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_eobi_price_feed_source_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceBMF: {
        if (shortcode_ntp_data_filesource_map_.find(shortcode_) == shortcode_ntp_data_filesource_map_.end()) {
          shortcode_ntp_data_filesource_map_[shortcode_] =
              new HFSAT::NTPLoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, secid,
                                                    exchange_symbol_, exch_trading_location_, false, false, false);

          shortcode_ntp_data_filesource_map_[shortcode_]->SetNTPPriceLevelGlobalListener(sim_real_pf_manager.get());
          shortcode_ntp_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_ntp_data_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceBMFEQ: {
        if (shortcode_puma_data_filesource_map_.find(shortcode_) == shortcode_puma_data_filesource_map_.end()) {
          shortcode_puma_data_filesource_map_[shortcode_] = new HFSAT::PUMALoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);

          shortcode_puma_data_filesource_map_[shortcode_]->SetNTPPriceLevelGlobalListener(sim_real_pf_manager.get());
          shortcode_puma_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_puma_data_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceRTS: {
        if (shortcode_rts_data_filesource_map_.find(shortcode_) == shortcode_rts_data_filesource_map_.end()) {
          shortcode_rts_data_filesource_map_[shortcode_] = new HFSAT::RTSLoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);

          shortcode_rts_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(sim_real_pf_manager.get());
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

          shortcode_micex_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(sim_real_pf_manager.get());
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

          shortcode_hkomd_pricefeed_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(sim_real_pf_manager.get());
          shortcode_hkomd_pricefeed_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_hkomd_pricefeed_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceBATSCHI: {
        if (shortcode_chix_l1_data_filesource_map_.find(shortcode_) == shortcode_chix_l1_data_filesource_map_.end()) {
          shortcode_chix_l1_data_filesource_map_[shortcode_] = new HFSAT::CHIXL1LoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);

          shortcode_chix_l1_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(sim_real_pf_manager.get());
          shortcode_chix_l1_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_chix_l1_data_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceLIFFE: {
        if (shortcode_liffe_data_filesource_map_.find(shortcode_) == shortcode_liffe_data_filesource_map_.end()) {
          shortcode_liffe_data_filesource_map_[shortcode_] = new HFSAT::LIFFELoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);

          shortcode_liffe_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(sim_real_pf_manager.get());
          shortcode_liffe_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_liffe_data_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceICE: {
        if (shortcode_ice_data_filesource_map_.find(shortcode_) == shortcode_ice_data_filesource_map_.end()) {
          shortcode_ice_data_filesource_map_[shortcode_] =
              new HFSAT::ICELoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, secid,
                                                    exchange_symbol_, exch_trading_location_, false, false);

          shortcode_ice_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(sim_real_pf_manager.get());
          shortcode_ice_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_ice_data_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceTMX: {
        if (tradingdate_ >= USING_TMX_OBF_FROM) {
          if (shortcode_tmx_pf_data_filesource_map_.find(shortcode_) == shortcode_tmx_pf_data_filesource_map_.end()) {
            shortcode_tmx_pf_data_filesource_map_[shortcode_] = new HFSAT::TMXPFLoggedMessageFileSource(
                dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);

            shortcode_tmx_pf_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(sim_real_pf_manager.get());
            shortcode_tmx_pf_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
            historical_dispatcher_.AddExternalDataListener(shortcode_tmx_pf_data_filesource_map_[shortcode_]);
          }
        }
      } break;
      case HFSAT::kExchSourceASX: {
        if (shortcode_asx_pf_data_filesource_map_.find(shortcode_) == shortcode_asx_pf_data_filesource_map_.end()) {
          shortcode_asx_pf_data_filesource_map_[shortcode_] = new HFSAT::ASXPFLoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);

          shortcode_asx_pf_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(sim_real_pf_manager.get());
          shortcode_asx_pf_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_asx_pf_data_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceSGX: {
        if (shortcode_sgx_pf_data_filesource_map_.find(shortcode_) == shortcode_sgx_pf_data_filesource_map_.end()) {
          shortcode_sgx_pf_data_filesource_map_[shortcode_] = new HFSAT::SGXPFLoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);

          shortcode_sgx_pf_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(sim_real_pf_manager.get());
          shortcode_sgx_pf_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_sgx_pf_data_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceJPY: {
        // ose dependant shortcode OR CME dependant , use order-level data for ALL ose sources.
        if (shortcode_ose_pf_data_filesource_map_.find(shortcode_) == shortcode_ose_pf_data_filesource_map_.end()) {
          shortcode_ose_pf_data_filesource_map_[shortcode_] = new HFSAT::OSEPFLoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);

          shortcode_ose_pf_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(sim_real_pf_manager.get());
          shortcode_ose_pf_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_ose_pf_data_filesource_map_[shortcode_]);
        }
      } break;
      case HFSAT::kExchSourceCFE: {
        if (shortcode_cfe_data_filesource_map_.find(shortcode_) == shortcode_cfe_data_filesource_map_.end()) {
          shortcode_cfe_data_filesource_map_[shortcode_] = new HFSAT::CFELoggedMessageFileSource(
              dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_, false);
          shortcode_cfe_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(sim_real_pf_manager.get());
          shortcode_cfe_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(shortcode_cfe_data_filesource_map_[shortcode_]);
        }
      } break;
      default: { } break; }
  }
  for (auto it = real_ors_shortcode_set.begin(); it != real_ors_shortcode_set.end(); ++it) {
    std::string shortcode_ = *it;
    int secid = sec_name_indexer_.GetIdFromString(shortcode_);
    const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

    if (shortcode_ors_data_filesource_map_.find(shortcode_) == shortcode_ors_data_filesource_map_.end()) {
      shortcode_ors_data_filesource_map_[shortcode_] = new HFSAT::ORSMessageFileSource(
          dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, exch_trading_location_);
    }
    shortcode_ors_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(sim_real_pf_manager.get());
    historical_dispatcher_.AddExternalDataListener(shortcode_ors_data_filesource_map_[shortcode_]);
  }
}

void InitializeSecurityNameIndexer(const std::vector<std::string> contract_spec_shortcode_vec_, const int& tradingdate_,
                                   HFSAT::SecurityNameIndexer& sec_name_indexer_,
                                   std::map<std::string, std::string>& exch_symbol_short_code) {
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  for (unsigned int i = 0; i < contract_spec_shortcode_vec_.size(); i++) {
    const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(contract_spec_shortcode_vec_[i]);
    int c_month_ = ((tradingdate_ / 100) % 100);
    if (c_month_ % 3 == 0) {
      int start_date_ = HFSAT::DateTime::CalcNextWeekDay(100 * (tradingdate_ / 100) + 7);  // 8th or next business day
      int end_date_ = HFSAT::DateTime::CalcNextWeekDay(100 * (tradingdate_ / 100) + 14);   // 15th or next business day
      if (contract_spec_shortcode_vec_[i].substr(0, 3) == "XTE" ||
          contract_spec_shortcode_vec_[i].substr(0, 3) == "YTE") {
        if (tradingdate_ < start_date_ || tradingdate_ > end_date_) continue;
      } else if (contract_spec_shortcode_vec_[i].substr(0, 2) == "XT" ||
                 contract_spec_shortcode_vec_[i].substr(0, 2) == "YT") {
        if (tradingdate_ > start_date_ && tradingdate_ < end_date_) continue;
      }

    } else {
      if (contract_spec_shortcode_vec_[i].substr(0, 3) == "XTE" ||
          contract_spec_shortcode_vec_[i].substr(0, 3) == "YTE")
        continue;
    }
    if (!sec_name_indexer_.HasString(contract_spec_shortcode_vec_[i])) {
      sec_name_indexer_.AddString(exchange_symbol_, contract_spec_shortcode_vec_[i]);
    }
    if (!std::string(exchange_symbol_).empty()) {
      exch_symbol_short_code.insert(
          std::pair<std::string, std::string>(exchange_symbol_, contract_spec_shortcode_vec_[i]));
    }
  }
}

void ExtractContractSpecShortCodes(const int tradingdate_, std::vector<std::string>& contract_spec_shortcode_vec_) {
  HFSAT::ShortcodeContractSpecificationMap& contract_specification_map =
      HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).contract_specification_map_;
  for (auto it = contract_specification_map.begin(); it != contract_specification_map.end(); it++) {
    contract_spec_shortcode_vec_.push_back(it->first);
  }
}

void ParseCommandLineArgs(int argc, char** argv, int& trading_date, std::string& exch_trading_loc) {
  if (argc < 3) {
    HFSAT::ExitVerbose(HFSAT::kSimRealPacketOrderMismatchDetectorArgsLess,
                       "Usage: "
                       "sim_real_packet_order_mismatch_detector trading_date_YYYYMMDD ExchangeTradingLocation");
  } else {
    trading_date = atoi(argv[1]);
    if ((trading_date < MIN_YYYYMMDD) || (trading_date > MAX_YYYYMMDD)) {
      std::cerr << "tradingdate_ " << trading_date << " out of range [ " << MIN_YYYYMMDD << " " << MAX_YYYYMMDD << " ] "
                << std::endl;
      exit(1);
    }
    exch_trading_loc = argv[2];
    if (HFSAT::TradingLocationUtils::GetTradingLocationFromLOC_NAME(exch_trading_loc) == HFSAT::kTLocMAX) {
      std::cerr << " The provided exchange location " << exch_trading_loc << " is invalid \n";
      exit(1);
    }
  }
}
/* Usage of the exec :- sim_real_packet_order_mismatch_detector trading_date_YYYYMMDD ExchangeTradingLocation
 */
int main(int argc, char** argv) {
  HFSAT::HistoricalDispatcher historical_dispatcher_;
  HFSAT::BulkFileReader bulk_inorder_live_data_file_reader_;
  /// SecurityNameIndexer is needed to extract the short code from a given Exchange symbol
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();

  std::unique_ptr<HFSAT::SimRealPFManager> sim_real_pf_manager = nullptr;
  // Vector of all the short codes defined in the Contract Specification Map
  std::vector<std::string> contract_spec_shortcode_vec_;
  // Stores the shortcodes extracted for given exchange symbols found in the in Order live data file
  std::set<std::string> real_shortcode_set;
  std::set<std::string> real_ors_shortcode_set;
  std::map<std::string, std::string> exch_symbol_short_code;
  timeval start_time{0};
  timeval end_time;

  // Command line arguments
  int trading_date;
  std::string exch_trading_loc;

  ParseCommandLineArgs(argc, argv, trading_date, exch_trading_loc);

  HFSAT::Watch watch_(dbglogger_, trading_date);
  HFSAT::TradingLocation_t exch_trading_location =
      HFSAT::TradingLocationUtils::GetTradingLocationFromLOC_NAME(exch_trading_loc);

  // The file sources shouldn't fallback to the primary location in case the file is not present in the trading location
  bool skip_primary_loc = true;
  HFSAT::LoggedMessageFileNamer::SetSkipPrimaryLoc(skip_primary_loc);
  std::string real_packets_order_fileName =
      HFSAT::RealPacketsOrderFileNamer::GetName(trading_date, exch_trading_location);

  bulk_inorder_live_data_file_reader_.open(real_packets_order_fileName);
  ExtractContractSpecShortCodes(trading_date, contract_spec_shortcode_vec_);
  InitializeSecurityNameIndexer(contract_spec_shortcode_vec_, trading_date, sec_name_indexer_, exch_symbol_short_code);
  PopulateShortCodeSetFromRealData(exch_symbol_short_code, trading_date, bulk_inorder_live_data_file_reader_,
                                   real_shortcode_set, real_ors_shortcode_set, start_time, end_time);
  bulk_inorder_live_data_file_reader_.close();
  sim_real_pf_manager =
      std::unique_ptr<HFSAT::SimRealPFManager>(new HFSAT::SimRealPFManager(real_packets_order_fileName));

  InitializeHistoricalDispatcher(trading_date, real_shortcode_set, real_ors_shortcode_set, sec_name_indexer_, watch_,
                                 sim_real_pf_manager, exch_trading_location, historical_dispatcher_);

  // Set the start time of Historical Dispatcher 1usec before the time of earliest event
  // This is done to ensure that all events prior to the first event in the Real packets file are skipped
  HFSAT::ttime_t hist_dispatcher_start_time(start_time.tv_sec, start_time.tv_usec - 1);

  historical_dispatcher_.SeekHistFileSourcesTo(hist_dispatcher_start_time);
  historical_dispatcher_.RunHist(end_time);
  int mismatch_count = sim_real_pf_manager->GetMismatchCount();
  if (!mismatch_count) {
    std::cout << "No packet order mismatches found. Comparison Successful \n";
  } else {
    std::cerr << "Sim Real Packet Order mismatch detected \n";
  }

  return 0;
}
