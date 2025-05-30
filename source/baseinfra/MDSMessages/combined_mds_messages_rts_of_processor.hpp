#ifndef _COMBINED_MDS_MESSAGES_RTS_OF_PROCESSOR_HPP_
#define _COMBINED_MDS_MESSAGES_RTS_OF_PROCESSOR_HPP_

// =====================================================================================
//
//       Filename:  combined_mds_messages_rts_source.cpp
//
//    Description:
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
#include "dvccode/CommonTradeUtils/trade_time_manager.hpp"
#include "dvccode/CDef/assumptions.hpp"
#include "dvccode/Utils/recovery_manager_config_parser.hpp"
#include "dvccode/Utils/tcp_client_socket.hpp"
#pragma once

namespace HFSAT {

class CombinedMDSMessagesRTSOFProcessor {
 protected:
  DebugLogger& dbglogger_;
  const SecurityNameIndexer& sec_name_indexer_;

  OrderGlobalListener<RTS_MDS::RTSOFCommonStructv2>* orderfeed_global_listener_;
  ExternalTimeListener* p_time_keeper_;
  HFSAT::TradeTimeManager& trade_time_manager_;

  //============ Members required to manage recovery request to recovery_mgr =========//
  // Indication whether the recovery has been complete or not and what was the last sequenced applied
  int32_t security_id_to_recovery_complete_[DEF_MAX_SEC_ID];

  // This contains recovery packets received from RecoveryHost
  char* recovery_buffer;

  // client side recovery purpose: gets recovery server info from config files
  HFSAT::RecoveryManagerConfigParser recovery_manager_config_parser_;

 public:
  CombinedMDSMessagesRTSOFProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        orderfeed_global_listener_(NULL),
        p_time_keeper_(NULL),
        trade_time_manager_(HFSAT::TradeTimeManager::GetUniqueInstance(sec_name_indexer_)),
        recovery_buffer(new char[MAX_RECOVERY_PACKET_SIZE]),
        recovery_manager_config_parser_() {
    memset((void*)security_id_to_recovery_complete_, -1, DEF_MAX_SEC_ID);
    // Reset the buffer
    memset((void*)recovery_buffer, 0, MAX_RECOVERY_PACKET_SIZE);
  }

  ~CombinedMDSMessagesRTSOFProcessor() {}

  inline void SetOrderfeedListener(OrderGlobalListener<RTS_MDS::RTSOFCommonStructv2>* listener_) {
    orderfeed_global_listener_ = listener_;
  }

  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  // Interface to recover live orders for the given security, Applies packets upto given sequence number
  bool RecoverLiveOrders(int security_id, const char* exchange_symbol, int32_t current_seq_num) {
    if (current_seq_num <= 0) return false;  // if recovery attempt made on RESET_BEGIN/RESET_END msg
    recovery_manager_config_parser_.Initialise();
    int int_exchange_src = HFSAT::exchange_name_str_to_int_mapping["RTS_OF"];  // RTS_OF in defines.hpp
    std::string recovery_ip = recovery_manager_config_parser_.GetRecoveryHostIP(int_exchange_src);
    int recovery_port = recovery_manager_config_parser_.GetRecoveryHostClientPort();

    DBGLOG_CLASS_FUNC_LINE_FATAL << "TRYING TO CONNECT TO RECOVERY HOST AT " << recovery_ip << " X " << recovery_port
                                 << " for " << exchange_symbol << " sec id " << security_id << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    HFSAT::TCPClientSocket tcp_client_socket;
    tcp_client_socket.Connect(recovery_ip, recovery_port);

    if (-1 == tcp_client_socket.socket_file_descriptor()) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO CONNECT TO RECOVERY HOST, SYSTEMERROR : " << strerror(errno)
                                   << " WILL EXIT... " << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      tcp_client_socket.Close();
      raise(SIGINT);
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
    memcpy((void*)(tmp_writer_ptr), exchange_symbol, std::strlen(exchange_symbol));

    int32_t written_length = tcp_client_socket.WriteN(RECOVERY_REQUEST_MESSAGE_LENGTH, request_message);

    if (written_length < (int32_t)RECOVERY_REQUEST_MESSAGE_LENGTH) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO SEND MESSAGE TO RECOVERY HOST, SYSTEMERROR : " << strerror(errno)
                                   << " WILL EXIT... " << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      tcp_client_socket.Close();
      raise(SIGINT);
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
        raise(SIGINT);
      }

      packet_length += read_length;
    }

    int32_t data_length = *((int32_t*)((char*)(recovery_buffer)));

    // This means server is not ready for recovery
    if (-2 == data_length) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << " RETURNED WITH -2 " << DBGLOG_ENDL_FLUSH;
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
        raise(SIGINT);
      }

      packet_length += read_length;
    }

    int64_t max_seq_number = 0;
    char* msg_ptr = recovery_buffer + 4;
    uint32_t total_live_orders = data_length / sizeof(RTS_MDS::RTSOFCommonStructv2);
    // Packets are not in order of sequence number, hence let's first check what's the max sequence number
    for (uint32_t event_counter = 0; event_counter < total_live_orders; event_counter++) {
      max_seq_number = std::max(max_seq_number, ((RTS_MDS::RTSOFCommonStructv2*)(msg_ptr))->seq_num);
      msg_ptr += sizeof(RTS_MDS::RTSOFCommonStructv2);
    }

    // Well, recovery has failed, It's possible we just missed it by a seq or two but we'll need to go for another round
    // for this
    // security
    DBGLOG_CLASS_FUNC_LINE_INFO << "Max_seq_obtained_from_recovery:" << max_seq_number
                                << ", Current_seq_number: " << current_seq_num << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;
    if ((int64_t)max_seq_number < current_seq_num) {
      tcp_client_socket.Close();
      return false;
    }

    msg_ptr = recovery_buffer + 4;  // reset the pointer

    DBGLOG_CLASS_FUNC_LINE_INFO << "RECOVERY SUCCESSFUL..., TOTAL PACKETS RECEIVED : " << total_live_orders
                                << " SYNCED UPTO SEQ : " << max_seq_number
                                << " APPLYING PACKETS TO BOOK NOW UPTO REQUESTED SEQ.. " << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;
    struct timeval event_time;
    gettimeofday(&event_time, NULL);

    for (uint32_t event_counter = 0; event_counter < total_live_orders; event_counter++) {
      RTS_MDS::RTSOFCommonStructv2* event = (RTS_MDS::RTSOFCommonStructv2*)(msg_ptr);

      // not notifying until all these bursts of orderAdds are applied to book, mark intermediate
      event->is_intermediate = true;
      event->seq_num = max_seq_number;
      p_time_keeper_->OnTimeReceived(event_time);

      // All orders will be passed as OrderAdd only since they are coming from recovery as live orders
      orderfeed_global_listener_->Process(security_id, event);

      msg_ptr += sizeof(RTS_MDS::RTSOFCommonStructv2);
    }

    // Recovery Done For This Security Id
    security_id_to_recovery_complete_[security_id] = max_seq_number;
    DBGLOG_CLASS_FUNC_LINE_INFO << " RECOVERY COMPLETE, CLOSING SOCKET SEC ID :" << security_id << " "
                                << tcp_client_socket.socket_file_descriptor() << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    tcp_client_socket.Close();

    return true;
  }

  inline bool CheckAndCallRecovery(int security_id, RTS_MDS::RTSOFCommonStructv2* next_event) {
    // Check if we have ever gone for recovery, If not we'll assume we have missed things and now we'll want to recover
    // all live orders
    if (-1 == security_id_to_recovery_complete_[security_id]) {
      struct timeval current_time;
      gettimeofday(&current_time, NULL);

      DBGLOG_CLASS_FUNC_LINE_INFO << "SECURITY : " << security_id << " EXCHANGE SYMBOL : " << next_event->getContract()
                                  << " IS GOING FOR RECOVERY NOW... CURRENT SEQUENCE : " << next_event->seq_num << " @ "
                                  << current_time.tv_sec << "." << current_time.tv_usec << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      // It's possible that the recovery host is not yet up to date, We might have to call it sometime later
      if (RecoverLiveOrders(security_id, next_event->getContract(), next_event->seq_num)) {
        gettimeofday(&current_time, NULL);
        DBGLOG_CLASS_FUNC_LINE_INFO << "SECURITY : " << security_id
                                    << " EXCHANGE SYMBOL : " << next_event->getContract()
                                    << "RECOVERY COMPLETED SUCCESSFULLY... "
                                    << " @ " << current_time.tv_sec << "." << current_time.tv_usec
                                    << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;

      } else {
        gettimeofday(&current_time, NULL);
        DBGLOG_CLASS_FUNC_LINE_ERROR << "SECURITY : " << security_id
                                     << " EXCHANGE SYMBOL : " << next_event->getContract()
                                     << " FAILED TO RECOVER... WILL ATTEMPT NEXT TIME.. "
                                     << " @ " << current_time.tv_sec << "." << current_time.tv_usec
                                     << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;

        // We need to intentionally make client slow if we failed to recover,
        // It's very likely combinedwriter is leading the tbt recovery manager and this sleep is
        // required to make sure we don't push too many TCP connection requests
        HFSAT::usleep(10);

        return false;  // We'll wait for next turn
      }
    }

    // checking if we have already received this packet from recovery manager
    if (security_id_to_recovery_complete_[security_id] >= (int64_t)next_event->seq_num) {
      return false;
    }

    return true;
  }

  inline void ProcessRTSEvent(RTS_MDS::RTSOFCommonStructv2* next_event_) {
    int security_id = sec_name_indexer_.GetIdFromSecname(next_event_->contract);

    if (security_id < 0) return;

    // Handle recovery case
    // Do not process this event if recovery failed(will wait for next turn) or if this event already processed in
    // recovery
    if (!CheckAndCallRecovery(security_id, next_event_)) return;
    p_time_keeper_->OnTimeReceived(next_event_->time_);

    orderfeed_global_listener_->Process(security_id, next_event_);
  }
};
}

#endif
