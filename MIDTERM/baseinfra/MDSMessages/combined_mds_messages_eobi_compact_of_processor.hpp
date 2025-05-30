// =====================================================================================
//
//       Filename:  combined_mds_messages_eobi_compact_of_processor.hpp
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

#pragma once

#include "baseinfra/BaseUtils/eobi_fast_order_manager.hpp"

namespace HFSAT {

class CombinedMDSMessagesEOBICompactOFProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  OrderGlobalListenerEOBI*
      p_eobi_order_level_global_listener_;  // Listeners of EOBIOF Messages in LiveTrading of all types
  ExternalTimeListener* p_time_keeper_;

  // Indication of whether the recovery has been complete or not and what was the last sequenced applied
  // can be switched to bool later
  int32_t security_id_to_recovery_complete_[DEF_MAX_SEC_ID];

  // This contains recovery packets received from RecoveryHost
  char* recovery_buffer_;

 public:
  CombinedMDSMessagesEOBICompactOFProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        p_time_keeper_(NULL),
        security_id_to_recovery_complete_(),
        recovery_buffer_(new char[MAX_RECOVERY_PACKET_SIZE]) {
    // Note, We are here taking advantage of the fact that -1 will be all FF... so we are not setting it per byte basis
    // but rather
    // total number of elements only
    memset((void*)security_id_to_recovery_complete_, -1, DEF_MAX_SEC_ID);

    // Reset the buffer
    memset((void*)recovery_buffer_, 0, MAX_RECOVERY_PACKET_SIZE);
  }

  ~CombinedMDSMessagesEOBICompactOFProcessor() {}

  inline void SetEOBIOrderLevelGlobalListener(OrderGlobalListenerEOBI* p_new_listener) {
    p_eobi_order_level_global_listener_ = p_new_listener;
  }

  inline void SetExternalTimeListener(ExternalTimeListener* new_listener) { p_time_keeper_ = new_listener; }

  // Interface to recover live orders for the given security, Applies packets upto given sequence number
  bool RecoverLiveOrders(std::string const& exchsymbol, int32_t const& current_seq_num, int32_t const& sec_id) {
    HFSAT::TCPClientSocket tcp_client_socket;
    tcp_client_socket.Connect("10.23.102.54", 41041);

    if (-1 == tcp_client_socket.socket_file_descriptor()) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO CONNECT TO RECOVERY HOST, SYSTEMERROR : " << strerror(errno)
                                   << " WILL EXIT... " << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      tcp_client_socket.Close();
      exit(-1);
    }

    char request_message[RECOVERY_HANDSHAKE_LENGTH];
    memset((void*)request_message, 0, RECOVERY_HANDSHAKE_LENGTH);
    memcpy((void*)(request_message), (void*)exchsymbol.c_str(), RECOVERY_HANDSHAKE_LENGTH);

    DBGLOG_CLASS_FUNC_LINE_INFO << " TRYING TO WRITE : " << RECOVERY_HANDSHAKE_LENGTH
                                << " MESSAGE : " << request_message << " SYMBOL : " << exchsymbol
                                << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    int32_t written_length = tcp_client_socket.WriteN(RECOVERY_HANDSHAKE_LENGTH, request_message);

    if (written_length < (int32_t)RECOVERY_HANDSHAKE_LENGTH) {
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
          tcp_client_socket.ReadN(MAX_RECOVERY_PACKET_SIZE - packet_length, recovery_buffer_ + packet_length);
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

    int32_t data_length = *((int32_t*)((char*)(recovery_buffer_)));

    // This means server is not ready for recovery
    if (-2 == data_length) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << " RETURNED WITH-2 " << DBGLOG_ENDL_FLUSH;
      tcp_client_socket.Close();
      return false;
    }

    // Check if we have read all what was sent and expected
    while (packet_length < data_length + 4) {
      int32_t read_length =
          tcp_client_socket.ReadN(MAX_RECOVERY_PACKET_SIZE - packet_length, recovery_buffer_ + packet_length);

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

    int32_t max_seq_number = 0;
    char* msg_ptr = recovery_buffer_ + 4;

    // Packets are not in order of sequence number, hence let's first check what's the max sequence number
    for (uint32_t event_counter = 0; event_counter < data_length / sizeof(EOBI_MDS::EOBICompactOrder);
         event_counter++) {
      max_seq_number = std::max(max_seq_number, (int32_t)((EOBI_MDS::EOBICompactOrder*)(msg_ptr))->msg_seq_num_);
      msg_ptr += sizeof(EOBI_MDS::EOBICompactOrder);
    }

    // Well, recovery has failed, It's possible we just missed it by a seq or two but we'll need to go for another round
    // for this
    // security
    if (max_seq_number < current_seq_num) {
      tcp_client_socket.Close();
      return false;
    }

    msg_ptr = recovery_buffer_ + 4;

    // Timestamp is not marked for recovery packets via combined source, It's fine to have same timestamp of all packets
    // I think ?
    struct timeval event_time;
    gettimeofday(&event_time, NULL);

    DBGLOG_CLASS_FUNC_LINE_INFO << "RECOVERY SUCCESSFUL..., TOTAL PACKETS RECEIVED : "
                                << data_length / sizeof(EOBI_MDS::EOBICompactOrder)
                                << " SYNCED UPTO SEQ : " << max_seq_number
                                << " APPLYING PACKETS TO BOOK NOW UPTO REQUESTED SEQ.. " << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    for (uint32_t event_counter = 0; event_counter < data_length / sizeof(EOBI_MDS::EOBICompactOrder);
         event_counter++) {
      EOBI_MDS::EOBICompactOrder* event = (EOBI_MDS::EOBICompactOrder*)(msg_ptr);

      // We are done here with recovery
      if ((int32_t)event->msg_seq_num_ >= current_seq_num) break;

      DBGLOG_CLASS_FUNC_LINE_INFO << " RECOVERY PACKET DATA : " << event->ToString() << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      p_time_keeper_->OnTimeReceived(event_time);

      HFSAT::TradeType_t t_buysell = event->side == 'B' ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;

      // All orders will be passed as OrderAdd only since they are coming from recovery as live orders
      p_eobi_order_level_global_listener_->OnOrderAdd(sec_id, t_buysell, event->price, event->size,
                                                      event->intermediate_);

      msg_ptr += sizeof(EOBI_MDS::EOBICompactOrder);
    }

    // Recovery Done For This Security Id
    security_id_to_recovery_complete_[sec_id] = max_seq_number;
    DBGLOG_CLASS_FUNC_LINE_INFO << " RECOVERY COMPLETE, CLOSING SOCKET SEC ID :" << sec_id << " "
                                << tcp_client_socket.socket_file_descriptor() << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    tcp_client_socket.Close();

    return true;
  }

  inline void ProcessEOBICompactOFEvent(EOBI_MDS::EOBICompactOrder* next_event) {
    int32_t security_id = sec_name_indexer_.GetIdFromSecname(next_event->contract_);
    if (security_id < 0) return;

    // Check if we have ever gone for recovery, If not we'll assume we have missed things and now we'll want to recover
    // all live orders
    if (-1 == security_id_to_recovery_complete_[security_id]) {
      DBGLOG_CLASS_FUNC_LINE_INFO << "SECURITY : " << security_id << " SYMBOL : " << next_event->contract_
                                  << " IS GOING FOR RECOVERY NOW... CURRENT SEQUENCE : " << next_event->msg_seq_num_
                                  << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      // It's possible that the recovery host is not yet uptodate, We might have to call it sometime later
      if (true == RecoverLiveOrders(next_event->contract_, next_event->msg_seq_num_, security_id)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "SECURITY : " << security_id << "RECOVERY COMPLETED SUCCESSFULLY... "
                                    << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;

      } else {
        DBGLOG_CLASS_FUNC_LINE_ERROR << "SECURITY : " << security_id
                                     << " FAILED TO RECOVER... WILL ATTEMPT NEXT TIME.. " << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;

        return;  // We'll wait for next turn
      }
    }

    p_time_keeper_->OnTimeReceived(next_event->time_);

    HFSAT::TradeType_t t_buysell = next_event->side == 'B' ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;
    HFSAT::BaseUtils::EOBIFastOrderManager& eobi_fast_order_manager =
        HFSAT::BaseUtils::EOBIFastOrderManager::GetUniqueInstance(security_id);

    switch (next_event->action_) {
      case '0': {
        eobi_fast_order_manager.OnAdd(*next_event);
        p_eobi_order_level_global_listener_->OnOrderAdd(security_id, t_buysell, next_event->price, next_event->size,
                                                        next_event->intermediate_);
      } break;

      case '1': {
        eobi_fast_order_manager.OnModify(*next_event);
        p_eobi_order_level_global_listener_->OnOrderModify(security_id, t_buysell, next_event->price, next_event->size,
                                                           next_event->prev_price, next_event->prev_size);
      } break;

      case '2': {
        eobi_fast_order_manager.OnDelete(*next_event);
        p_eobi_order_level_global_listener_->OnOrderDelete(security_id, t_buysell, next_event->price, next_event->size,
                                                           true, false);
      } break;
      case '3': {
        p_eobi_order_level_global_listener_->OnOrderMassDelete(security_id);
      } break;
      case '4': {
        p_eobi_order_level_global_listener_->OnPartialOrderExecution(security_id, t_buysell, next_event->price,
                                                                     next_event->size);
      } break;
      case '5': {
        eobi_fast_order_manager.OnFullTrade(*next_event);
        p_eobi_order_level_global_listener_->OnFullOrderExecution(security_id, t_buysell, next_event->price,
                                                                  next_event->size);
      } break;
      case '6': {
        eobi_fast_order_manager.OnExecutionSummary(*next_event);
        p_eobi_order_level_global_listener_->OnExecutionSummary(security_id, t_buysell, next_event->price,
                                                                next_event->size);
      } break;
      default: {
        DBGLOG_CLASS_FUNC_LINE_ERROR << "Unexpected Type Of Message In EOBICompact Struct : " << next_event->action_
                                     << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
      } break;
    }
  }
};
}
