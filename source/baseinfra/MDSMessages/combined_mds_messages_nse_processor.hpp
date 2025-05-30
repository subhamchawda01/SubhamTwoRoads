// =====================================================================================
//
//       Filename:  combined_mds_messages_nse_source.cpp
//
//    Description:  A data receiving interface which then processes the data and passes events to books
//
//        Version:  1.0
//        Created:  01/30/2014 01:06:07 PM
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
#include "dvccode/CDef/assumptions.hpp"  //Only include this file on classes which actually holds assumptions, this simplifies the search at any point where one needs to knwo what assumptions have been made
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/Utils/nse_daily_token_symbol_handler.hpp"
#include "dvccode/Utils/bse_daily_token_symbol_handler.hpp"
#include "dvccode/Utils/nse_refdata_loader.hpp"
#include "dvccode/Utils/tcp_client_socket.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/CommonTradeUtils/global_sim_data_manager.hpp"
#include "dvccode/Utils/recovery_manager_config_parser.hpp"
#include "dvccode/CommonTradeUtils/trade_time_manager.hpp"

//#include "dvccode/Profiler/cpucycle_profiler.hpp"

namespace HFSAT {

// Storage class of old size and price
struct Data {
  int32_t price;
  int32_t size;
};

// Simple interface to maintain per security based historical size and price
class HistoryTracker {
 public:
  HistoryTracker() {}
  std::map<uint64_t, Data> order_id_data_;
};

class CombinedMDSMessagesNSEProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  // Indication of whether the recovery has been complete or not and what was the last sequenced applied
  // can be switched to bool later
  int32_t security_id_to_recovery_complete_[DEF_MAX_SEC_ID];

  OrderGlobalListenerNSE* p_order_global_listener_nse_;  ///< Listeners of NSE messages in LiveTrading of all types
  ExternalTimeListener* p_time_keeper_;

  // This classes are useful to access per security based refdata
  HFSAT::Utils::NSEDailyTokenSymbolHandler& nse_daily_token_symbol_handler_;
  HFSAT::Utils::NSERefDataLoader& nse_refdata_loader_;

  // This contains recovery packets received from RecoveryHost
  char* recovery_buffer;

  // It's goot to load and store this once as it's not going to be changed
  std::map<char, std::map<int32_t, NSE_UDP_MDS::NSERefData>*> segment_to_nse_ref_data_;
  TradeTimeManager& trade_time_manager_;

  // client side recovery purpose
  HFSAT::RecoveryManagerConfigParser recovery_manager_config_parser_;

 public:
  CombinedMDSMessagesNSEProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        p_time_keeper_(NULL),
        nse_daily_token_symbol_handler_(HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(
            HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate())),
        nse_refdata_loader_(HFSAT::Utils::NSERefDataLoader::GetUniqueInstance(
            HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate())),
        recovery_buffer(new char[MAX_RECOVERY_PACKET_SIZE]),
        segment_to_nse_ref_data_(),
        trade_time_manager_(TradeTimeManager::GetUniqueInstance(
            _sec_name_indexer_, HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate())),
        recovery_manager_config_parser_() {
    // Note, We are here taking advantage of the fact that -1 will be all FF... so we are not setting it per byte basis
    // but rather
    // total number of elements only

    memset((void*)security_id_to_recovery_complete_, -1, DEF_MAX_SEC_ID*sizeof(int32_t));

    timeval tv;
    gettimeofday(&tv, NULL);
    int curr_hhmm = DateTime::GetUTCHHMMFromTime(tv.tv_sec);
    for (unsigned int sec_id = 0; sec_id < _sec_name_indexer_.NumSecurityId(); sec_id++) {
      // If current time is less than the start time for this sec_id, then don't go for its recovery
      int start_time = trade_time_manager_.GetStartTime(sec_id);
      int start_hhmm = (start_time / 3600) * 100 + (start_time % 3600) / 60;
      if (curr_hhmm < start_hhmm) {
        security_id_to_recovery_complete_[sec_id] = 0;
      }
    }

    if (IsItSimulationServer()) {
      memset((void*)security_id_to_recovery_complete_, 0, DEF_MAX_SEC_ID*sizeof(int32_t));
    }

    // Reset the buffer
    memset((void*)recovery_buffer, 0, MAX_RECOVERY_PACKET_SIZE);

    // Initialize
    segment_to_nse_ref_data_[NSE_EQ_SEGMENT_MARKING] = new std::map<int32_t, NSE_UDP_MDS::NSERefData>();
    segment_to_nse_ref_data_[NSE_FO_SEGMENT_MARKING] = new std::map<int32_t, NSE_UDP_MDS::NSERefData>();
    segment_to_nse_ref_data_[NSE_CD_SEGMENT_MARKING] = new std::map<int32_t, NSE_UDP_MDS::NSERefData>();

    std::map<int32_t, NSE_UDP_MDS::NSERefData>& eq_nse_ref_data = *(segment_to_nse_ref_data_[NSE_EQ_SEGMENT_MARKING]);
    std::map<int32_t, NSE_UDP_MDS::NSERefData>& fo_nse_ref_data = *(segment_to_nse_ref_data_[NSE_FO_SEGMENT_MARKING]);
    std::map<int32_t, NSE_UDP_MDS::NSERefData>& cd_nse_ref_data = *(segment_to_nse_ref_data_[NSE_CD_SEGMENT_MARKING]);

    for (auto& itr : nse_refdata_loader_.GetNSERefData(NSE_EQ_SEGMENT_MARKING)) {
      eq_nse_ref_data[itr.first] = itr.second;
    }

    for (auto& itr : nse_refdata_loader_.GetNSERefData(NSE_CD_SEGMENT_MARKING)) {
      cd_nse_ref_data[itr.first] = itr.second;
    }
    for (auto& itr : nse_refdata_loader_.GetNSERefData(NSE_FO_SEGMENT_MARKING)) {
      fo_nse_ref_data[itr.first] = itr.second;
    }
    /*HFSAT::CpucycleProfiler::SetUniqueInstance(4);
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(1, "OnOrderAdd");
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(2, "OnOrderModify");
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(3, "OnOrderDelete");
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(4, "OnTrade");*/
  }

  ~CombinedMDSMessagesNSEProcessor() {
    // std::cout << HFSAT::CpucycleProfiler::GetUniqueInstance().GetCpucycleSummaryString();
  }

  inline void SetOrderGlobalListenerNSE(OrderGlobalListenerNSE* p_new_listener_) {
    p_order_global_listener_nse_ = p_new_listener_;
  }

  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  // Interface to recover live orders for the given security, Applies packets upto given sequence number
  bool RecoverLiveOrders(int32_t const& token, uint32_t const& current_seq_num, int32_t const& sec_id, char segment) {
    DBGLOG_CLASS_FUNC_LINE_INFO << "STARTING CONNECTION TO RECOVERY MANAGER..." << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    std::string this_exch_string = "NSE";

    switch (segment) {
      case NSE_FO_SEGMENT_MARKING: {
        this_exch_string = "NSE_FO";
      } break;
      case NSE_EQ_SEGMENT_MARKING: {
        this_exch_string = "NSE_EQ";
      } break;
      case NSE_CD_SEGMENT_MARKING: {
        this_exch_string = "NSE_CD";
      } break;
      default: { this_exch_string = "NSE"; } break;
    }

    if ("NSE" != this_exch_string) {
      recovery_manager_config_parser_.Initialise(this_exch_string);
    } else {
      recovery_manager_config_parser_.Initialise();
    }

    // hard coded the exchange src value
    int32_t int_exchange_src = HFSAT::exchange_name_str_to_int_mapping[this_exch_string];
    std::string recovery_ip = recovery_manager_config_parser_.GetRecoveryHostIP(int_exchange_src);

    // For NSE always recover from IND12 by default
    if (std::string("127.0.0.1") == recovery_ip) {
      recovery_ip = "10.23.115.62";  // IND12
    }

    int recovery_port = recovery_manager_config_parser_.GetRecoveryHostClientPort(int_exchange_src);

    DBGLOG_CLASS_FUNC_LINE_INFO << "TRYING TO CONNECT TO RECOVERY HOST AT " << recovery_ip << " X " << recovery_port
                                << "..." << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    if (segment_to_nse_ref_data_.end() == segment_to_nse_ref_data_.find(segment)) return false;

    std::map<int32_t, NSE_UDP_MDS::NSERefData>& nse_ref_data = *(segment_to_nse_ref_data_[segment]);

    HFSAT::TCPClientSocket tcp_client_socket;
    tcp_client_socket.Connect(recovery_ip, recovery_port);

    if (-1 == tcp_client_socket.socket_file_descriptor()) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO CONNECT TO RECOVERY HOST, SYSTEMERROR : " << strerror(errno)
                                   << " WILL EXIT... " << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      tcp_client_socket.Close();
      exit(-1);
    }

    char request_message[RECOVERY_REQUEST_MESSAGE_LENGTH];
    memset(request_message, '\0', RECOVERY_REQUEST_MESSAGE_LENGTH);

    char* tmp_writer_ptr = request_message;
    // fill-in the exchange name
    memcpy((void*)(tmp_writer_ptr), (void*)(&int_exchange_src), sizeof(int32_t));
    tmp_writer_ptr += RECOVERY_REQUEST_EXCHANGE_SRC_LENGTH;

    // fill-in the dummy flags (we aren't using these currently)
    tmp_writer_ptr += RECOVERY_REQUEST_DUMMY_FLAGS_LENGTH;

    // fill in the message body
    tmp_writer_ptr[0] = segment;
    memcpy((void*)(tmp_writer_ptr + 1), (void*)&token, sizeof(int32_t));

    int32_t written_length = tcp_client_socket.WriteN(RECOVERY_REQUEST_MESSAGE_LENGTH, request_message);

    if (written_length < (int32_t)RECOVERY_REQUEST_MESSAGE_LENGTH) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO SEND MESSAGE TO RECOVERY HOST, SYSTEMERROR : " << strerror(errno)
                                   << " WILL EXIT... " << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      tcp_client_socket.Close();
      exit(-1);
    }

    int32_t packet_length = 0;

    // First 4 bytes will indicate length of the actual data
    while (packet_length < 4) {
      int32_t read_length =
          tcp_client_socket.ReadN(MAX_RECOVERY_PACKET_SIZE - packet_length, recovery_buffer + packet_length);
      DBGLOG_CLASS_FUNC_LINE_INFO << "READ RETURNED WITH : " << read_length << " PACKET LENGTH : " << packet_length
                                  << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      if (read_length <= 0) {
        DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO READ FROM RECOVERY HOST, WILL EXIT... " << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
        tcp_client_socket.Close();
        exit(-1);
      }

      packet_length += read_length;
    }

    int32_t data_length = *((int32_t*)((char*)(recovery_buffer)));

    // This means server is not ready for recovery
    if (-2 == data_length) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << " RETURNED WITH-2 " << DBGLOG_ENDL_FLUSH;
      tcp_client_socket.Close();
      return false;
    }

    // Check if we have read all what was sent and expected
    while (packet_length < data_length + 4) {
      int32_t read_length =
          tcp_client_socket.ReadN(MAX_RECOVERY_PACKET_SIZE - packet_length, recovery_buffer + packet_length);

      DBGLOG_CLASS_FUNC_LINE_INFO << "RECOVERY READ RETURNED WITH : " << read_length
                                  << " PACKET LENGTH : " << packet_length << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      if (read_length <= 0) {
        DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO READ FROM RECOVERY HOST, WILL EXIT... " << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
        tcp_client_socket.Close();
        exit(-1);
      }

      packet_length += read_length;
    }

    uint32_t max_seq_number = 0;
    char* msg_ptr = recovery_buffer + 4;

    // Packets are not in order of sequence number, hence let's first check what's the max sequence number
    for (uint32_t event_counter = 0; event_counter < data_length / sizeof(NSE_MDS::NSETBTDataCommonStruct);
         event_counter++) {
      max_seq_number = std::max(max_seq_number, ((NSE_MDS::NSETBTDataCommonStruct*)(msg_ptr))->msg_seq);
      msg_ptr += sizeof(NSE_MDS::NSETBTDataCommonStruct);
    }

    // Well, recovery has failed, It's possible we just missed it by a seq or two but we'll need to go for another round
    // for this
    // security, We'll allow a lag upto 50 packets
    if (max_seq_number < current_seq_num - 50) {
      tcp_client_socket.Close();
      return false;
    }

    msg_ptr = recovery_buffer + 4;

    // Timestamp is not marked for recovery packets via combined source, It's fine to have same timestamp of all packets
    // I think ?
    struct timeval event_time;
    gettimeofday(&event_time, NULL);

    DBGLOG_CLASS_FUNC_LINE_INFO << "RECOVERY SUCCESSFUL..., TOTAL PACKETS RECEIVED : "
                                << data_length / sizeof(NSE_MDS::NSETBTDataCommonStruct)
                                << " SYNCED UPTO SEQ : " << max_seq_number
                                << " APPLYING PACKETS TO BOOK NOW UPTO REQUESTED SEQ.. " << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    for (uint32_t event_counter = 0; event_counter < data_length / sizeof(NSE_MDS::NSETBTDataCommonStruct);
         event_counter++) {
      NSE_MDS::NSETBTDataCommonStruct* event = (NSE_MDS::NSETBTDataCommonStruct*)(msg_ptr);

      // We are done here with recovery
      //      if (event->msg_seq >= current_seq_num) break;
      if (event->msg_seq >= current_seq_num)
        continue;  // Since We are getting buffer of recovery packets which is sorted based on OrderIds and not seq

      // DBGLOG_CLASS_FUNC_LINE_INFO << " RECOVERY PACKET DATA : " << event->ToString() << DBGLOG_ENDL_NOFLUSH;
      p_time_keeper_->OnTimeReceived(event_time);

      //      DBGLOG_CLASS_FUNC_LINE_INFO << " RECOVERY PACKET, SEC ID :" << sec_id << " PRICE : "
      //                                  << (event->data).nse_order.order_price /
      //                                  (double)nse_ref_data[token].price_multiplier
      //                                  << " QTY : " << (event->data).nse_order.order_qty << DBGLOG_ENDL_NOFLUSH;
      //      DBGLOG_DUMP;

      // All orders will be passed as OrderAdd only since they are coming from recovery as live orders
      // HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1);
      p_order_global_listener_nse_->OnOrderAdd(
          sec_id, (event->data).nse_order.buysell, (event->data).nse_order.order_id,
          (event->data).nse_order.order_price / (double)nse_ref_data[token].price_multiplier,
          (event->data).nse_order.order_qty, false);

      // HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);

      msg_ptr += sizeof(NSE_MDS::NSETBTDataCommonStruct);
    }

    // Recovery Done For This Security Id
    security_id_to_recovery_complete_[sec_id] = max_seq_number;
    DBGLOG_CLASS_FUNC_LINE_INFO << " RECOVERY COMPLETE, CLOSING SOCKET SEC ID :" << sec_id << " "
                                << tcp_client_socket.socket_file_descriptor() << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    tcp_client_socket.Close();

    return true;
  }

  inline void ProcessNSEEvent(NSE_MDS::NSETBTDataCommonStruct* next_event_) {
    // There will be a few instruments for which datasymbol/exchange won't be defined, TODO : use bool to reduce latency
    std::string internal_symbol =
        nse_daily_token_symbol_handler_.GetInternalSymbolFromToken(next_event_->token, next_event_->segment_type);
    if (std::string("INVALID") == internal_symbol) return;

    std::string exchange_symbol = NSESecurityDefinitions::ConvertDataSourceNametoExchSymbol(internal_symbol);
    if (std::string("INVALID") == exchange_symbol) return;

    int32_t security_id = sec_name_indexer_.GetIdFromSecname(exchange_symbol.c_str());
    if (security_id < 0) return;

    // Check if we have ever gone for recovery, If not we'll assume we have missed things and now we'll want to recover
    // all live orders
    if (-1 == security_id_to_recovery_complete_[security_id]) {
      struct timeval current_time;
      gettimeofday(&current_time, NULL);

      DBGLOG_CLASS_FUNC_LINE_INFO << "SECURITY : " << security_id << " TOKEN : " << next_event_->token
                                  << " SYMBOL : " << exchange_symbol << " DATA SYMBOL : " << internal_symbol
                                  << " IS GOING FOR RECOVERY NOW... CURRENT SEQUENCE : " << next_event_->msg_seq
                                  << " @ " << current_time.tv_sec << "." << current_time.tv_usec << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      // We need to intentionally make client slow if we failed to recover,
      // It's very likely combinedwriter is leading the tbt recovery manager and this sleep is
      // required to make sure we don't push too many TCP connection requests
      HFSAT::usleep(100);

      // It's possible that the recovery host is not yet uptodate, We might have to call it sometime later
      if (true == RecoverLiveOrders(next_event_->token, next_event_->msg_seq, security_id, next_event_->segment_type)) {
        gettimeofday(&current_time, NULL);
        DBGLOG_CLASS_FUNC_LINE_INFO << "SECURITY : " << security_id << "RECOVERY COMPLETED SUCCESSFULLY... "
                                    << " @ " << current_time.tv_sec << "." << current_time.tv_usec
                                    << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;

      } else {
        gettimeofday(&current_time, NULL);
        DBGLOG_CLASS_FUNC_LINE_ERROR << "SECURITY : " << security_id
                                     << " FAILED TO RECOVER... WILL ATTEMPT NEXT TIME.. "
                                     << " @ " << current_time.tv_sec << "." << current_time.tv_usec
                                     << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;

        return;  // We'll wait for next turn
      }
    }

    switch (next_event_->msg_type) {
      case NSE_MDS::MsgType::kNSEOrderDelta: {
        std::map<int32_t, NSE_UDP_MDS::NSERefData>& nse_ref_data =
            *(segment_to_nse_ref_data_[next_event_->segment_type]);

        // Order New
        if ('N' == next_event_->activity_type) {
          // Update watch
          p_time_keeper_->OnTimeReceived(next_event_->source_time);

          // HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1);
          p_order_global_listener_nse_->OnOrderAdd(
              security_id, (next_event_->data).nse_order.buysell, (next_event_->data).nse_order.order_id,
              (next_event_->data).nse_order.order_price / (double)nse_ref_data[next_event_->token].price_multiplier,
              (next_event_->data).nse_order.order_qty, false);
          // HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);
        } else if ('M' == next_event_->activity_type) {  // Order Modify
          // Update watch
          p_time_keeper_->OnTimeReceived(next_event_->source_time);

          // HFSAT::CpucycleProfiler::GetUniqueInstance().Start(2);
          p_order_global_listener_nse_->OnOrderModify(
              security_id, (next_event_->data).nse_order.buysell, (next_event_->data).nse_order.order_id,
              (next_event_->data).nse_order.order_price / (double)nse_ref_data[next_event_->token].price_multiplier,
              (next_event_->data).nse_order.order_qty);

          // HFSAT::CpucycleProfiler::GetUniqueInstance().End(2);
        } else if ('X' == next_event_->activity_type) {  // Order Delete
          // Update watch
          p_time_keeper_->OnTimeReceived(next_event_->source_time);

          // HFSAT::CpucycleProfiler::GetUniqueInstance().Start(3);
          p_order_global_listener_nse_->OnOrderDelete(
              security_id, (next_event_->data).nse_order.buysell, (next_event_->data).nse_order.order_id,
              (next_event_->data).nse_order.order_price / (double)nse_ref_data[next_event_->token].price_multiplier,
              true, false);
          // HFSAT::CpucycleProfiler::GetUniqueInstance().End(3);

        } else {
          DBGLOG_CLASS_FUNC_LINE_ERROR << "UNEXPECTED ACTIVITY TYPE RECEVIED IN DATA : " << next_event_->activity_type
                                       << DBGLOG_ENDL_NOFLUSH;
          DBGLOG_DUMP;
        }

      } break;

      case NSE_MDS::MsgType::kNSETrade: {
        std::map<int32_t, NSE_UDP_MDS::NSERefData>& nse_ref_data =
            *(segment_to_nse_ref_data_[next_event_->segment_type]);
        p_time_keeper_->OnTimeReceived(next_event_->source_time);

        /*int32_t bid_size_remaining = 0;
        int32_t ask_size_remaining = 0;*/

        // HFSAT::CpucycleProfiler::GetUniqueInstance().Start(4);
        p_order_global_listener_nse_->OnTrade(
            security_id,
            (next_event_->data).nse_trade.trade_price / (double)nse_ref_data[next_event_->token].price_multiplier,
            (next_event_->data).nse_trade.trade_qty, (next_event_->data).nse_trade.buy_order_id,
            (next_event_->data).nse_trade.sell_order_id);
        // HFSAT::CpucycleProfiler::GetUniqueInstance().End(4);

      } break;

      case NSE_MDS::MsgType::kNSEOrderSpreadDelta: {
      } break;

      case NSE_MDS::MsgType::kNSESpreadTrade: {
      } break;

      case NSE_MDS::MsgType::kNSETradeExecutionRange: {
      } break;

      default: {
        DBGLOG_CLASS_FUNC_LINE_ERROR << "UNEXPECTED TYPE OF MESSAGE RECEVIED : " << (int32_t)(next_event_->msg_type)
                                     << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
      } break;
    }
  }
};
}
