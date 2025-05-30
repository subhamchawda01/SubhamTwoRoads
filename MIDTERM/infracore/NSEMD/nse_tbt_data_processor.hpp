// =====================================================================================
//
//       Filename:  nse_tbt_data_processor.hpp
//
//    Description:  This class is a processed events handler and based on input mode performs actions,
//                  i.e Logger mode logs data to files, ComShm mode enables write to combined shm queue etc.
//                  Simply and interface of what to do with the processed events
//
//        Version:  1.0
//        Created:  09/18/2015 10:17:36 AM
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

#pragma once

#include <iostream>
#include <cstdlib>
#include <unordered_set>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CombinedControlUtils/combined_control_message_livesource.hpp"
#include "dvccode/Utils/mds_logger.hpp"
#include "dvccode/Utils/mds_shm_interface.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/Utils/lock.hpp"
#include "infracore/NSEMD/nse_tbt_templates.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/Listeners/live_products_manager_listener.hpp"
#include "infracore/Tools/live_products_manager.hpp"
#include "dvccode/Utils/data_daemon_config.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/Utils/nse_daily_token_symbol_handler.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/Utils/combined_source_generic_logger.hpp"
#include "dvccode/Utils/rdtscp_timer.hpp"

#define MAX_BUFFERED_EVENTS_STORAGE 8192

#define CALL_MEMBER_FN(object, ptrToMember) ((object).*(ptrToMember))

namespace HFSAT {
namespace NSEMD {

class NSESegmentWiseHistoryManager {
 private:
  HFSAT::DebugLogger& dbglogger_;
  //  std::unordered_set<int32_t>& processing_tokens_;

  // For RecoveryMode
  std::map<int32_t, std::map<uint64_t, NSE_MDS::NSETBTDataCommonStruct>*> token_to_live_orders_map_;
  std::map<int32_t, HFSAT::Lock*> token_to_control_lock_;

  NSESegmentWiseHistoryManager();

 public:
  NSESegmentWiseHistoryManager(HFSAT::DebugLogger& dbglogger)
      : dbglogger_(dbglogger), token_to_live_orders_map_(), token_to_control_lock_() {}

  bool IsTokenAvailableForRecovery(int32_t token) {
    return token_to_live_orders_map_.end() == token_to_live_orders_map_.find(token) ? false : true;
  }

  void UpdateProcessedLiveOrdersMap(int32_t token, NSE_MDS::NSETBTDataCommonStruct const& bufferd_event) {
    if (token_to_control_lock_.end() == token_to_control_lock_.find(token)) {
      token_to_control_lock_[token] = new HFSAT::Lock();
    }

    // Lock token control
    token_to_control_lock_[token]->LockMutex();

    std::map<uint64_t, NSE_MDS::NSETBTDataCommonStruct>* live_orders_map = NULL;

    if (token_to_live_orders_map_.find(token) == token_to_live_orders_map_.end()) {
      std::map<uint64_t, NSE_MDS::NSETBTDataCommonStruct>* new_live_orders_map =
          new std::map<uint64_t, NSE_MDS::NSETBTDataCommonStruct>();
      token_to_live_orders_map_[token] = new_live_orders_map;
    }

    live_orders_map = token_to_live_orders_map_[token];

    if (ORDER_MESSAGE_TYPE_NEW == bufferd_event.activity_type ||
        ORDER_MESSAGE_TYPE_MODIFY == bufferd_event.activity_type ||
        SPREAD_ORDER_MESSAGE_TYPE_NEW == bufferd_event.activity_type ||
        SPREAD_ORDER_MESSAGE_TYPE_MODIFY == bufferd_event.activity_type) {
      (*live_orders_map)[bufferd_event.data.nse_order.order_id] = bufferd_event;

    } else if (ORDER_MESSAGE_TYPE_DELETE == bufferd_event.activity_type) {
      if (live_orders_map->end() != live_orders_map->find(bufferd_event.data.nse_order.order_id)) {
        live_orders_map->erase(live_orders_map->find(bufferd_event.data.nse_order.order_id));
      }
    } else {  // Trade

      if (live_orders_map->end() != live_orders_map->find(bufferd_event.data.nse_trade.buy_order_id)) {
        NSE_MDS::NSETBTDataCommonStruct& this_event = (*live_orders_map)[bufferd_event.data.nse_trade.buy_order_id];
        this_event.data.nse_order.order_qty -= bufferd_event.data.nse_trade.trade_qty;

        if (this_event.data.nse_order.order_qty <= 0) {
          live_orders_map->erase(live_orders_map->find(bufferd_event.data.nse_trade.buy_order_id));
        }
      }

      if (live_orders_map->end() != live_orders_map->find(bufferd_event.data.nse_trade.sell_order_id)) {
        NSE_MDS::NSETBTDataCommonStruct& this_event = (*live_orders_map)[bufferd_event.data.nse_trade.sell_order_id];
        this_event.data.nse_order.order_qty -= bufferd_event.data.nse_trade.trade_qty;

        if (this_event.data.nse_order.order_qty <= 0) {
          live_orders_map->erase(live_orders_map->find(bufferd_event.data.nse_trade.sell_order_id));
        }
      }
    }

    // Time to release this token lock
    token_to_control_lock_[token]->UnlockMutex();
  }

  bool FetchProcessedLiveOrdersMap(int32_t token, char* data_packet, int32_t const& MAX_BUFFER_SIZE) {
    // Acquire lock to copy events
    if (token_to_control_lock_.end() == token_to_control_lock_.find(token)) {
      token_to_control_lock_[token] = new HFSAT::Lock();
    }

    // Lock token control
    token_to_control_lock_[token]->LockMutex();
    // It's time to now construct buffer
    std::map<uint64_t, NSE_MDS::NSETBTDataCommonStruct> live_orders = *(token_to_live_orders_map_[token]);
    token_to_control_lock_[token]->UnlockMutex();

    DBGLOG_CLASS_FUNC_LINE_INFO << "INCOMING RECOVERY REQUEST FOR : " << token
                                << " LIVE ORDERS : " << live_orders.size()
                                << " LIVE SIZE : " << live_orders.size() * sizeof(NSE_MDS::NSETBTDataCommonStruct)
                                << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    if (live_orders.size() == 0) return false;

    // Check whether the buffer storage can hold enough elements
    if ((int32_t)(4 + (live_orders.size() * sizeof(NSE_MDS::NSETBTDataCommonStruct))) > (int32_t)MAX_BUFFER_SIZE)
      return false;

    int32_t data_size = live_orders.size() * sizeof(NSE_MDS::NSETBTDataCommonStruct);

    // Insert length to buffer
    memcpy((void*)data_packet, (void*)&data_size, sizeof(int32_t));

    DBGLOG_CLASS_FUNC_LINE_INFO << "4 BYTES OF DATA : " << *((int32_t*)((char*)(data_packet))) << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    data_packet += 4;

    int32_t max_seq = 0;

    double best_bid_price = 0;
    double best_ask_price = 5000000000;
    uint64_t best_bid_order_id = 0;
    uint64_t best_ask_order_id = 0;

    // Fill up the buffer with data
    for (auto& itr : live_orders) {
      max_seq = std::max(max_seq, (itr.second).msg_seq);
      memcpy((void*)data_packet, (void*)&(itr.second), sizeof(NSE_MDS::NSETBTDataCommonStruct));
      data_packet += sizeof(NSE_MDS::NSETBTDataCommonStruct);

      if (HFSAT::kTradeTypeBuy == itr.second.data.nse_order.buysell &&
          best_bid_price < itr.second.data.nse_order.order_price) {
        best_bid_price = itr.second.data.nse_order.order_price;
        best_bid_order_id = itr.second.data.nse_order.order_id;
      }

      if (HFSAT::kTradeTypeSell == itr.second.data.nse_order.buysell &&
          best_ask_price > itr.second.data.nse_order.order_price) {
        best_ask_price = itr.second.data.nse_order.order_price;
        best_ask_order_id = itr.second.data.nse_order.order_id;
      }

      if (0.0 != best_bid_price && 0.0 != best_ask_price && best_bid_price >= best_ask_price) {
        DBGLOG_CLASS_FUNC_LINE_INFO << " BOOK HAS CROSSED AT THIS POINT WITH BEST BID PROCE : " << best_bid_price
                                    << " ORDER ID : " << best_bid_order_id << " ASK PRICE :" << best_ask_price
                                    << " ORDER ID : " << best_ask_order_id << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
      }
    }

    DBGLOG_CLASS_FUNC_LINE_INFO << "SENDING MAX SEQUENCE IN RECOVERY AS :" << max_seq << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    // Request was handled successfully
    return true;
  }
};

class NSETBTDataProcessor : public HFSAT::LiveProductsManagerListener {
 private:
  void ProcessEventsForLoggerMode(NSE_MDS::NSETBTDataCommonStruct* nse_tbt_data_common_struct) {
    data_logger_thread_->log(*nse_tbt_data_common_struct);
  }

  void ProcessEventsForComShmMode(NSE_MDS::NSETBTDataCommonStruct* nse_tbt_data_common_struct) {
    if (tokens_requested_.find(nse_tbt_data_common_struct->token) != tokens_requested_.end()) {
      mds_shm_interface_->WriteGenericStruct(generic_mds_message_, false);
    }
//     if (is_local_data_) {
//      HFSAT::CpucycleProfiler::GetUniqueInstance().End(4);
//    }

    if (true == is_logging_enabled_) {
      generic_logger_->Log(*generic_mds_message_);
    }
  }

  void ProcessEvetnsForDataMode(NSE_MDS::NSETBTDataCommonStruct* nse_tbt_data_common_struct) {
    multicast_sender_socket_->WriteN(sizeof(NSE_MDS::NSETBTDataCommonStruct), (void*)nse_tbt_data_common_struct);
  }

 public:
  bool IsTokenAvailableForRecovery(int32_t token, char segment) {
    return (segment_to_history_manager_[segment]->IsTokenAvailableForRecovery(token));
  }

  bool FetchProcessedLiveOrdersMap(int32_t token, char* data_packet, int32_t const& MAX_BUFFER_SIZE, char segment) {
    return (segment_to_history_manager_[segment]->FetchProcessedLiveOrdersMap(token, data_packet, MAX_BUFFER_SIZE));
  }

 private:
#define NSE_SOURCES_PATH "/spare/local/files/NSE/nse_recovery_datasources.txt"

  void ApplyAddTSFilterForProcessingData() {
    ExchangeSymbolManager::SetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());
    HFSAT::SecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal())
        .LoadNSESecurityDefinitions();

    // Load Addts File and Only add tokens for those products for recovery management

    std::ifstream addtsfs;
    addtsfs.open(NSE_SOURCES_PATH);

    if (false == addtsfs.is_open()) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "UNABLE TO OPEN NSE RECOVERY SOURCES FILE : " << NSE_SOURCES_PATH
                                   << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      return;
    }

    char buffer[1024];
    while (addtsfs.good()) {
      addtsfs.getline(buffer, 1024);
      std::string line = buffer;

      if (line.length() == 0) continue;
      if (std::string::npos != line.find("#")) continue;

      std::string shortcode = line;

      if (false ==
          HFSAT::SecurityDefinitions::CheckIfContractSpecExists(shortcode, HFSAT::DateTime::GetCurrentIsoDateLocal()))
        continue;

      std::string exchange_symbol = GetExchangeSymbol(shortcode);
      std::string internal_symbol = NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(exchange_symbol);

      if (internal_symbol == std::string("INVALID")) {
        std::cerr << "ApplyAddTSFilterForProcessingData Error: Invalid Internal Symbol for " << exchange_symbol
                  << std::endl;
        continue;
      }

      char segment = NSESecurityDefinitions::GetSegmentTypeFromShortCode(shortcode);
      int32_t token = nse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(internal_symbol.c_str(), segment);

      if (-1 == token) {
        std::cerr << "ApplyAddTSFilterForProcessingData Error: Invalid Token : " << token << " For Symbol "
                  << exchange_symbol << std::endl;
        continue;
      }

      processing_tokens_.insert(token);
    }

    addtsfs.close();
  }

  void ProcessEventsForRecoveryHostMode(NSE_MDS::NSETBTDataCommonStruct* nse_tbt_data_common_struct) {
    // DBGLOG_CLASS_FUNC_LINE_FATAL << "New Packet arrived..." << DBGLOG_ENDL_NOFLUSH;
    // DBGLOG_DUMP;
    static bool are_we_processing_all = (0 == processing_tokens_.size());

    if (false == are_we_processing_all) {
      if (processing_tokens_.end() == processing_tokens_.find(nse_tbt_data_common_struct->token)) return;
    }

    if (nse_tbt_data_common_struct->msg_type == NSE_MDS::MsgType::kNSETradeExecutionRange) return;

    // Only process delta events, trades don't need to be re-sent / recovered
    if (ORDER_MESSAGE_TYPE_NEW != nse_tbt_data_common_struct->activity_type &&
        ORDER_MESSAGE_TYPE_MODIFY != nse_tbt_data_common_struct->activity_type &&
        ORDER_MESSAGE_TYPE_DELETE != nse_tbt_data_common_struct->activity_type &&
        TRADE_MESSAGE_TYPE_TRADE != nse_tbt_data_common_struct->activity_type)
      return;

    segment_to_history_manager_[nse_tbt_data_common_struct->segment_type]->UpdateProcessedLiveOrdersMap(
        nse_tbt_data_common_struct->token, *nse_tbt_data_common_struct);
  }

  void OnLiveProductChange(LiveProductsChangeAction action, std::string exchange_symbol, std::string shortcode,
                           ExchSource_t exchange) {
    std::cout << "ReqCW: NSETBTHandler: " << action << " " << shortcode << std::endl;
    if (daemon_mode_ == HFSAT::kComShm && (exchange == kExchSourceNSE || exchange == kExchSourceNSE_FO ||
                                           exchange == kExchSourceNSE_CD || exchange == kExchSourceNSE_EQ)) {
      std::string exchange_symbol = GetExchangeSymbol(shortcode);
      std::string internal_symbol = NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(exchange_symbol);
      if (internal_symbol == std::string("INVALID")) {
        std::cerr << "OnLiveProductsChange Error: Invalid Internal Symbol for " << exchange_symbol << std::endl;
        return;
      }

      char segment = NSESecurityDefinitions::GetSegmentTypeFromShortCode(shortcode);
      int32_t token = nse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(internal_symbol.c_str(), segment);

      if (token != -1 /*Dummy Token*/) {
        if (action == LiveProductsChangeAction::kAdd) {
          std::cout << "Token " << token << " added for " << shortcode << std::endl;
          tokens_requested_.insert(token);
        } else if (action == LiveProductsChangeAction::kRemove) {
          if (tokens_requested_.find(token) != tokens_requested_.end()) {
            std::cout << "Token " << token << " erased for " << shortcode << std::endl;
            tokens_requested_.erase(token);
          }
        }
      } else {
        std::cerr << " Invalid Token for shortcode " << shortcode << std::endl;
      }
    }
  }

  const char* GetExchangeSymbol(const std::string& shortcode) {
    const char* exchange_symbol = nullptr;
    try {
      exchange_symbol = ExchangeSymbolManager::GetUniqueInstance().GetExchSymbol(shortcode);

    } catch (...) {
      std::cerr << " ReqCW: NSETBTDataProcessor: Error in ExchangeSymbol Conversion for Shortcode : " << shortcode
                << std::endl;
    }

    return exchange_symbol;
  }

 private:
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::FastMdConsumerMode_t daemon_mode_;
  std::unordered_set<int32_t> processing_tokens_;

  // For Logger Mode//
  MDSLogger<NSE_MDS::NSETBTDataCommonStruct>* data_logger_thread_;

  // For ComShm Mode
  HFSAT::Utils::MDSShmInterface* mds_shm_interface_;
  HFSAT::MDS_MSG::GenericMDSMessage* generic_mds_message_;
  bool is_logging_enabled_;

  // For DataMode
  HFSAT::MulticastSenderSocket* multicast_sender_socket_;

  // For RecoveryMode
  std::map<char, NSESegmentWiseHistoryManager*> segment_to_history_manager_;

  HFSAT::Utils::NSEDailyTokenSymbolHandler& nse_daily_token_symbol_handler_;

  NSE_MDS::NSETBTDataCommonStruct dummy_struct;

  typedef void (NSETBTDataProcessor::*ProcessEventForModes)(
      NSE_MDS::NSETBTDataCommonStruct* nse_tbt_data_common_struct);
  ProcessEventForModes process_event_for_modes_;

  std::unordered_set<int32_t> tokens_requested_;

  HFSAT::Utils::CombinedSourceGenericLogger* generic_logger_;

  bool is_local_data_;

  NSETBTDataProcessor(HFSAT::DebugLogger& dbglogger, HFSAT::FastMdConsumerMode_t const& daemon_mode,
                      HFSAT::CombinedControlMessageLiveSource* p_ccm_livesource)
      : dbglogger_(dbglogger),
        daemon_mode_(daemon_mode),
        processing_tokens_(),
        data_logger_thread_(NULL),
        mds_shm_interface_(nullptr),
        generic_mds_message_(NULL),
        is_logging_enabled_(true),
        multicast_sender_socket_(NULL),
        segment_to_history_manager_(),
        nse_daily_token_symbol_handler_(
            HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal())),
        generic_logger_(nullptr) {
    is_local_data_ = TradingLocationUtils::GetTradingLocationExch(kExchSourceNSE) ==
                     TradingLocationUtils::GetTradingLocationFromHostname();

    switch (daemon_mode_) {
      case HFSAT::kLogger: {
        data_logger_thread_ = new MDSLogger<NSE_MDS::NSETBTDataCommonStruct>("NSETBT");
        process_event_for_modes_ = &NSETBTDataProcessor::ProcessEventsForLoggerMode;
      } break;

      case HFSAT::kMcast: {
        HFSAT::NetworkAccountInfoManager network_account_info_manager;
        HFSAT::DataInfo data_info = network_account_info_manager.GetSrcDataInfo(HFSAT::kExchSourceNSE, "dummy_0");
        multicast_sender_socket_ = new HFSAT::MulticastSenderSocket(
            data_info.bcast_ip_, data_info.bcast_port_, HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(
                                                            HFSAT::kExchSourceNSE, HFSAT::k_MktDataLive));

        process_event_for_modes_ = &NSETBTDataProcessor::ProcessEvetnsForDataMode;
      } break;

      case HFSAT::kComShm: {
        generic_logger_ = &(HFSAT::Utils::CombinedSourceGenericLogger::GetUniqueInstance());
        generic_mds_message_ = new HFSAT::MDS_MSG::GenericMDSMessage();
        generic_mds_message_->mds_msg_exch_ = HFSAT::MDS_MSG::NSE;
        process_event_for_modes_ = &NSETBTDataProcessor::ProcessEventsForComShmMode;
        HFSAT::LiveProductsManager::GetUniqueInstance().AddListener(this);
        mds_shm_interface_ = &HFSAT::Utils::MDSShmInterface::GetUniqueInstance();
        is_logging_enabled_ = generic_logger_->IsLoggingEnabled();

      } break;

      case HFSAT::kRecoveryHost: {
        process_event_for_modes_ = &NSETBTDataProcessor::ProcessEventsForRecoveryHostMode;
        segment_to_history_manager_[NSE_EQ_SEGMENT_MARKING] = new NSESegmentWiseHistoryManager(dbglogger_);
        segment_to_history_manager_[NSE_CD_SEGMENT_MARKING] = new NSESegmentWiseHistoryManager(dbglogger_);
        segment_to_history_manager_[NSE_FO_SEGMENT_MARKING] = new NSESegmentWiseHistoryManager(dbglogger_);

        ApplyAddTSFilterForProcessingData();

      } break;

      default: {
        process_event_for_modes_ = NULL;

        DBGLOG_CLASS_FUNC_LINE_FATAL << "OOPS !!, THIS MODE IS NOT YET SUPPORTED BY THE PROCESSOR, MODE VALUE : "
                                     << (int32_t)daemon_mode << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;

        exit(-1);

      } break;
    }
  }

  ~NSETBTDataProcessor() {}

  NSETBTDataProcessor(NSETBTDataProcessor const& disabled_copy_constructor);

 public:
  static NSETBTDataProcessor& GetUniqueInstance(HFSAT::DebugLogger& dbglogger,
                                                HFSAT::FastMdConsumerMode_t const& daemon_mode,
                                                HFSAT::CombinedControlMessageLiveSource* p_ccm_livesource) {
    static NSETBTDataProcessor unique_instance(dbglogger, daemon_mode, p_ccm_livesource);
    return unique_instance;
  }

  bool IsProcessingSecurityForComShm(int32_t& token) {
    if (false == is_logging_enabled_) {
      if (tokens_requested_.find(token) == tokens_requested_.end()) return false;
    }
    return true;
  }

  NSE_MDS::NSETBTDataCommonStruct* GetNSECommonStructFromGenericPacket() {
    return &((generic_mds_message_->generic_data_).nse_data_);
  }

  void OnMarketEvent(NSE_MDS::NSETBTDataCommonStruct* nse_tbt_data_common_struct) {
    CALL_MEMBER_FN (*this, process_event_for_modes_)(nse_tbt_data_common_struct);
  }
};
}
}
