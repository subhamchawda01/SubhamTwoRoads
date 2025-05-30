// =====================================================================================
//
//       Filename:  l1_sender.cpp
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

#include "Utils/thread.hpp"

#include "CDef/defines.hpp"
#include "CDef/security_definitions.hpp"
#include "CDef/file_utils.hpp"
#include "CDef/trading_location_manager.hpp"
#include "CDef/exchange_symbol_manager.hpp"
#include "CDef/debug_logger.hpp"
#include "CDef/stored_market_data_common_message_defines.hpp"
#include "CDef/random_channel.hpp"
#include "CDef/mds_messages.hpp"

#include "CommonDataStructures/security_name_indexer.hpp"
#include "CommonDataStructures/perishable_string_tokenizer.hpp"

#include "CommonTradeUtils/date_time.hpp"
#include "CommonTradeUtils/watch.hpp"
#include "TradingInfo/network_account_info_manager.hpp"

#include "MarketAdapter/shortcode_security_market_view_map.hpp"
#include "MarketAdapter/market_defines.hpp"

#include "MarketAdapter/indexed_nse_market_view_manager.hpp"

#include "MarketAdapter/book_init_utils.hpp"
#include "MDSMessages/combined_mds_messages_shm_processor.hpp"

#include "Profiler/cpucycle_profiler.hpp"
#include "Utils/multicast_sender_socket.hpp"

class L1DataGenerator : public HFSAT::SecurityMarketViewChangeListener {
  const HFSAT::SecurityMarketView& this_smv_;
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::Watch& watch_;
  HFSAT::GenericL1DataStruct l1_struct_;
  HFSAT::MulticastSenderSocket* socket_;
  int pkt_size_;

 public:
  L1DataGenerator(const HFSAT::SecurityMarketView& _this_smv_, HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_,
                  HFSAT::MulticastSenderSocket* socket)
      : this_smv_(_this_smv_),
        dbglogger_(_dbglogger_),
        watch_(_watch_),
        l1_struct_(),
        socket_(socket),
        pkt_size_(sizeof(HFSAT::GenericL1DataStruct)) {
    memcpy(l1_struct_.symbol, _this_smv_.secname(), strlen(_this_smv_.secname()));
  }

  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) {
    l1_struct_.time = watch_.tv();
    l1_struct_.type = HFSAT::GenericL1DataType::L1_DELTA;

    l1_struct_.delta.bestbid_size = _market_update_info_.bestbid_size_;
    l1_struct_.delta.bestbid_price = _market_update_info_.bestbid_price_;
    l1_struct_.delta.bestbid_ordercount = _market_update_info_.bestbid_ordercount_;

    l1_struct_.delta.bestask_size = _market_update_info_.bestask_size_;
    l1_struct_.delta.bestask_price = _market_update_info_.bestask_price_;
    l1_struct_.delta.bestask_ordercount = _market_update_info_.bestask_ordercount_;

    socket_->WriteN(pkt_size_, &l1_struct_);
  }

  inline void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                           const HFSAT::MarketUpdateInfo& _market_update_info_) {
    l1_struct_.time = watch_.tv();
    l1_struct_.type = HFSAT::GenericL1DataType::L1_TRADE;

    l1_struct_.trade.price = _trade_print_info_.trade_price_;
    l1_struct_.trade.size = _trade_print_info_.size_traded_;
    l1_struct_.trade.side = _trade_print_info_.buysell_;

    socket_->WriteN(pkt_size_, &l1_struct_);

    OnMarketUpdate(_security_id_, _market_update_info_);
  }
};

/// signal handler
void sighandler(int signum) { exit(0); }

int main(int argc, char** argv) {
  /// set signal handler .. add other signals later
  struct sigaction sigact;
  sigact.sa_handler = sighandler;
  sigaction(SIGINT, &sigact, NULL);

  if (argc < 3) {
    std::cerr << " usage : <exec> <exch> <config_file> \n";
    exit(0);
  }

  int tradingdate = HFSAT::DateTime::GetCurrentIsoDateLocal();
  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate).LoadNSESecurityDefinitions();

  std::vector<std::string> shc_list;
  std::string exch = argv[1];
  std::string cfg_filename = argv[2];

  std::ifstream ifile;
  ifile.open(cfg_filename.c_str());

  if (!ifile.is_open()) {
    std::cerr << "Unable to open config file : " << cfg_filename << std::endl;
  }

  char line[1024];

  while (ifile.getline(line, sizeof(line))) {
    if (strlen(line) == 0 || strstr(line, "#") != NULL) continue;
    HFSAT::PerishableStringTokenizer st(line, 1024);
    const std::vector<const char*>& tokens = st.GetTokens();
    if (tokens.size() != 1) {
      std::cerr << "Malformatted line in " << cfg_filename << std::endl;
      exit(-1);
    }
    if (!HFSAT::SecurityDefinitions::CheckIfContractSpecExists(tokens[0], tradingdate)) {
      std::cerr << "Shortcode in config doesn't exist in SecDef " << tokens[0] << std::endl;
      exit(-1);
    }
    shc_list.push_back(tokens[0]);
  }

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate);

  HFSAT::DebugLogger dbglogger(1024000, 1);
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "/spare/local/logs/alllogs/l1_sender." << HFSAT::DateTime::GetCurrentIsoDateLocal();
  std::string logfilename_ = t_temp_oss_.str();
  dbglogger.OpenLogFile(logfilename_.c_str(), std::ofstream::out);

  HFSAT::MulticastSenderSocket* sender_socket;

  if (exch == "NSE-SGX") {
    // Hard coding for now, as we would need extra support in network_account file to support this
    sender_socket = new HFSAT::MulticastSenderSocket(
        "239.23.0.29", 22999,
        HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceSGX, HFSAT::k_MktDataLive));
  } else {
    std::cerr << "Not Implemented for Exch : " << exch << " Exiting. " << std::endl;
    exit(-1);
  }

  HFSAT::Watch watch(dbglogger, tradingdate);

  HFSAT::SecurityNameIndexer& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();

  HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map = HFSAT::sid_to_security_market_view_map();

  HFSAT::ShortcodeSecurityMarketViewMap& shortcode_smv_map = HFSAT::ShortcodeSecurityMarketViewMap::GetUniqueInstance();

  for (auto shc : shc_list) {
    const char* exchange_symbol = HFSAT::ExchangeSymbolManager::GetExchSymbol(shc);
    sec_name_indexer.AddString(exchange_symbol, shc);
    unsigned int sec_id = sec_name_indexer.GetIdFromSecname(exchange_symbol);

    HFSAT::ExchSource_t exch_source = HFSAT::SecurityDefinitions::GetContractExchSource(shc, tradingdate);

    if (exch_source == HFSAT::kExchSourceEUREX) {
      exch_source = HFSAT::kExchSourceEOBI;
    }

    bool set_temporary_bool_checking_if_this_is_an_indexed_book_ =
        HFSAT::CommonSimIndexedBookBool(exch_source, std::string("NOMATCH"));

    HFSAT::SecurityMarketView* p_smv =
        new HFSAT::SecurityMarketView(dbglogger, watch, sec_name_indexer, shc, exchange_symbol, sec_id, exch_source,
                                      set_temporary_bool_checking_if_this_is_an_indexed_book_);
    sid_to_smv_ptr_map.push_back(p_smv);     // add to security_id_ to SMV* map
    shortcode_smv_map.AddEntry(shc, p_smv);  // add to shortcode_ to SMV* map
  }

  HFSAT::IndexedNSEMarketViewManager indexed_nse_market_view_manager(dbglogger, watch, sec_name_indexer,
                                                                     sid_to_smv_ptr_map);

  HFSAT::MDSMessages::CombinedMDSMessagesShmProcessor combined_mds_messages_shm_processor(dbglogger, sec_name_indexer,
                                                                                          HFSAT::kComShmConsumer);

  for (unsigned int i = 0; i < sec_name_indexer.GetNumSecurityId(); i++) {
    std::string shc = sec_name_indexer.GetShortcodeFromId(i);
    HFSAT::ExchSource_t exch_source = HFSAT::SecurityDefinitions::GetContractExchSource(shc, tradingdate);
    HFSAT::SecurityMarketView* p_smv = sid_to_smv_ptr_map[i];

    if (exch_source == HFSAT::kExchSourceHONGKONG) {
      exch_source = HFSAT::kExchSourceHKOMDPF;
    }
    switch (exch_source) {
      case HFSAT::kExchSourceNSE: {
        p_smv->InitializeSMVForIndexedBook();
        combined_mds_messages_shm_processor.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::NSE, (void*)((HFSAT::OrderGlobalListenerNSE*)&(indexed_nse_market_view_manager)), &watch);
      } break;

      default:
        fprintf(stderr, "Not implemented for exchange %d", exch_source);
        exit(1);
    }

    L1DataGenerator* test_book = new L1DataGenerator(*p_smv, dbglogger, watch, sender_socket);

    p_smv->subscribe_price_type(test_book, HFSAT::kPriceTypeMktSizeWPrice);
  }

  combined_mds_messages_shm_processor.RunLiveShmSource();

  return 0;
}
