/**
   \file MDSMessages/combined_mds_messages_ose_itch_of_processor.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#pragma once

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "dvccode/CDef/ose_itch_mds_defines.hpp"
#include "dvccode/CDef/assumptions.hpp"
#include "dvccode/Utils/recovery_manager_config_parser.hpp"

namespace HFSAT {

class CombinedMDSMessagesOSEItchOFProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  OrderFeedGlobalListener* order_feed_global_listener_;  ///< Listeners of LIFFE messages in LiveTrading of all types
  ExternalTimeListener* p_time_keeper_;

  // Indication of whether the recovery has been complete or not and what was the last sequenced applied
  // can be switched to bool later
  int32_t security_id_to_recovery_complete_[DEF_MAX_SEC_ID];

  // This contains recovery packets received from RecoveryHost
  char* recovery_buffer;

  // client side recovery purpose
  HFSAT::RecoveryManagerConfigParser recovery_manager_config_parser_;

 public:
  CombinedMDSMessagesOSEItchOFProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        order_feed_global_listener_(NULL),
        p_time_keeper_(NULL),
        recovery_buffer(new char[MAX_RECOVERY_PACKET_SIZE]),
        recovery_manager_config_parser_() {
    memset((void*)security_id_to_recovery_complete_, -1, DEF_MAX_SEC_ID);
    // Reset the buffer
    memset((void*)recovery_buffer, 0, MAX_RECOVERY_PACKET_SIZE);
  }

  ~CombinedMDSMessagesOSEItchOFProcessor() {}

  inline void SetOrderLevelGlobalListener(OrderFeedGlobalListener* p_new_listener_) {
    order_feed_global_listener_ = p_new_listener_;
  }
  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  // Interface to recover live orders for the given security, Applies packets upto given sequence number
  bool RecoverLiveOrders(int security_id, const char* exchange_symbol, int32_t current_seq_num) {
    recovery_manager_config_parser_.Initialise();
    int32_t int_exchange_src = HFSAT::exchange_name_str_to_int_mapping["OSE_ITCH"];
    std::string recovery_ip = recovery_manager_config_parser_.GetRecoveryHostIP(int_exchange_src);
    int recovery_port = recovery_manager_config_parser_.GetRecoveryHostClientPort();

    DBGLOG_CLASS_FUNC_LINE_INFO << "TRYING TO CONNECT TO RECOVERY HOST AT " << recovery_ip << " X " << recovery_port
                                << "..." << DBGLOG_ENDL_NOFLUSH;
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
    size_t copy_length = std::min((size_t)RECOVERY_REQUEST_BODY_LENGTH, std::strlen(exchange_symbol));
    memcpy((void*)(tmp_writer_ptr), exchange_symbol, copy_length);

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

    uint32_t max_seq_number = 0;
    char* msg_ptr = recovery_buffer + 4;

    // Packets are not in order of sequence number, hence let's first check what's the max sequence number
    for (uint32_t event_counter = 0; event_counter < data_length / sizeof(OSE_ITCH_MDS::OSERecoveryOrderInfo);
         event_counter++) {
      max_seq_number = std::max(max_seq_number, ((OSE_ITCH_MDS::OSERecoveryOrderInfo*)(msg_ptr))->msg_seq_number);
      msg_ptr += sizeof(OSE_ITCH_MDS::OSERecoveryOrderInfo);
    }

    // Well, recovery has failed, It's possible we just missed it by a seq or two but we'll need to go for another round
    // for this
    // security
    DBGLOG_CLASS_FUNC_LINE_FATAL << "Max_seq_obtained_from_recovery:" << max_seq_number
                                 << ", Current_seq_number: " << current_seq_num << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;
    if ((int32_t)max_seq_number < current_seq_num) {
      tcp_client_socket.Close();
      return false;
    }

    msg_ptr = recovery_buffer + 4;

    // Timestamp is not marked for recovery packets via combined source, It's fine to have same timestamp of all packets
    // I think ?
    struct timeval event_time;
    gettimeofday(&event_time, NULL);

    DBGLOG_CLASS_FUNC_LINE_INFO << "RECOVERY SUCCESSFUL..., TOTAL PACKETS RECEIVED : "
                                << data_length / sizeof(OSE_ITCH_MDS::OSERecoveryOrderInfo)
                                << " SYNCED UPTO SEQ : " << max_seq_number
                                << " APPLYING PACKETS TO BOOK NOW UPTO REQUESTED SEQ.. " << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    for (uint32_t event_counter = 0; event_counter < data_length / sizeof(OSE_ITCH_MDS::OSERecoveryOrderInfo);
         event_counter++) {
      OSE_ITCH_MDS::OSERecoveryOrderInfo* event = (OSE_ITCH_MDS::OSERecoveryOrderInfo*)(msg_ptr);

      // We are done here with recovery
      //      if (event->msg_seq >= current_seq_num) break;
      //      if (event->msg_seq_number >= current_seq_num)
      //        continue;  // Since We are getting buffer of recovery packets which is sorted based on OrderIds and not
      //        seq

      //      DBGLOG_CLASS_FUNC_LINE_INFO << " RECOVERY PACKET DATA : " << event->ToString() << DBGLOG_ENDL_NOFLUSH;
      p_time_keeper_->OnTimeReceived(event_time);

      //      DBGLOG_CLASS_FUNC_LINE_INFO << " RECOVERY PACKET, SEC ID :" << sec_id << " PRICE : "
      //                                  << (event->data).nse_order.order_price /
      //                                  (double)nse_ref_data[token].price_multiplier
      //                                  << " QTY : " << (event->data).nse_order.order_qty << DBGLOG_ENDL_NOFLUSH;
      //      DBGLOG_DUMP;

      // All orders will be passed as OrderAdd only since they are coming from recovery as live orders
      order_feed_global_listener_->OnOrderAdd(security_id, event->order_id, event->side, event->price, event->size,
                                              event->priority, false);

      msg_ptr += sizeof(OSE_ITCH_MDS::OSERecoveryOrderInfo);
    }

    // Recovery Done For This Security Id
    security_id_to_recovery_complete_[security_id] = max_seq_number;
    DBGLOG_CLASS_FUNC_LINE_INFO << " RECOVERY COMPLETE, CLOSING SOCKET SEC ID :" << security_id << " "
                                << tcp_client_socket.socket_file_descriptor() << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    tcp_client_socket.Close();

    return true;
  }

  uint8_t GetTradeSideOpposite(uint8_t side) {
    switch (side) {
      case 'B': {
        return 'S';
        break;
      }
      case 'S': {
        return 'B';
        break;
      }
      default: {
        return '-';
        break;
      }
    }
  }

  inline void ProcessOSEItchOFEvent(OSE_ITCH_MDS::OSECommonStruct* next_event) {
    int security_id = sec_name_indexer_.GetIdFromSecname(next_event->getContract());
    if (security_id < 0) return;

    // handle recovery case

    // Check if we have ever gone for recovery, If not we'll assume we have missed things and now we'll want to recover
    // all live orders
    if (-1 == security_id_to_recovery_complete_[security_id]) {
      struct timeval current_time;
      gettimeofday(&current_time, NULL);

      DBGLOG_CLASS_FUNC_LINE_INFO << "SECURITY : " << security_id << " EXCHANGE SYMBOL : " << next_event->getContract()
                                  << " IS GOING FOR RECOVERY NOW... CURRENT SEQUENCE : " << next_event->msg_seq_number
                                  << " @ " << current_time.tv_sec << "." << current_time.tv_usec << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      // It's possible that the recovery host is not yet up to date, We might have to call it sometime later
      if (RecoverLiveOrders(security_id, next_event->getContract(), next_event->msg_seq_number)) {
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

        return;  // We'll wait for next turn
      }
    }

    // checking if we have already received this packet from recovery manager
    //      DBGLOG_CLASS_FUNC_LINE_ERROR << "NOT GOING FOR RECOVERY AT ALL....." << DBGLOG_ENDL_NOFLUSH;
    //      DBGLOG_DUMP;
    if (security_id_to_recovery_complete_[security_id] >= (int32_t)next_event->msg_seq_number) {
      return;
    }

    p_time_keeper_->OnTimeReceived(next_event->time_);

    switch (next_event->msg_type) {
      // Order Add
      case OSE_ITCH_MDS::OSEMsgType::kOSEAdd: {
        order_feed_global_listener_->OnOrderAdd(security_id, next_event->data.add.order_id, next_event->data.add.side,
                                                next_event->data.add.price, next_event->data.add.size,
                                                next_event->data.add.priority, next_event->intermediate);
      } break;

      // There is no order modify for OSE_ITCH

      // Order delete
      case OSE_ITCH_MDS::OSEMsgType::kOSEDelete: {
        order_feed_global_listener_->OnOrderDelete(security_id, next_event->data.del.order_id,
                                                   next_event->data.del.side, next_event->intermediate);
      } break;

      // Order Exec
      case OSE_ITCH_MDS::OSEMsgType::kOSEExec: {
        order_feed_global_listener_->OnOrderExec(security_id, next_event->data.exec.order_id,
                                                 GetTradeSideOpposite(next_event->data.exec.side), 0,
                                                 next_event->data.exec.size_exec, next_event->intermediate);
      } break;

      // Order Exec with trade info
      case OSE_ITCH_MDS::OSEMsgType::kOSEExecWithTrade: {
        order_feed_global_listener_->OnOrderExecWithTradeInfo(
            security_id, next_event->data.exec_info.order_id, GetTradeSideOpposite(next_event->data.exec_info.side),
            next_event->data.exec_info.price, next_event->data.exec_info.size_exec, next_event->intermediate);
      } break;

      // Trading Status
      case OSE_ITCH_MDS::OSEMsgType::kOSETradingStatus: {
        order_feed_global_listener_->OnTradingStatus(security_id, std::string(next_event->data.status.state));
      } break;
      case OSE_ITCH_MDS::OSEMsgType::kOSEResetBegin: {
        order_feed_global_listener_->OnOrderResetBegin(security_id);
      } break;
      case OSE_ITCH_MDS::OSEMsgType::kOSEResetEnd: {
        order_feed_global_listener_->OnOrderResetEnd(security_id);
      } break;
      default: {
        //        std::cerr << " \n";
      } break;
    }
  }
};
}
