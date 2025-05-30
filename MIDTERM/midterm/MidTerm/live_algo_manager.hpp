// =====================================================================================
//
//       Filename:  trade_bar_generator.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  01/05/2016 12:10:02 PM
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
#include "midterm/MidTerm/base_algo_manager.hpp"

namespace MIDTERM {

class LiveAlgoManager : public BaseAlgoManager, public HFSAT::Thread {
public:
  HFSAT::TCPClientSocket *tcp_client_socket_;

public:
  LiveAlgoManager(Mode mode, HFSAT::DebugLogger &dbglogger, HFSAT::Watch &watch,
                  HFSAT::TCPClientSocket *tcp_client_socket)
      : BaseAlgoManager(Mode::kNSEServerMode, dbglogger, watch),
        tcp_client_socket_(tcp_client_socket) {
    // Simply return when in logger mode
    if (Mode::kNSELoggerMode == mode)
      return;
    dbglogger_ << "Connecting to Final Exec...\n";
    tcp_client_socket_->Connect(NSE_MIDTERM_ORDER_SERVER_IP,
                                NSE_MTERM_ORDER_ROUTING_SERVER_PORT);
    if (-1 == tcp_client_socket_->socket_file_descriptor()) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << " CONNECTION TO FINAL EXEC FAILED.."
                                   << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      exit(-1);
    }
  }

  LiveAlgoManager(LiveAlgoManager const &disabled_copy_constructor) = delete;

  static LiveAlgoManager &
  GetUniqueInstance(Mode mode, HFSAT::DebugLogger &dbglogger,
                    HFSAT::Watch &watch,
                    HFSAT::TCPClientSocket *tcp_client_socket) {
    static LiveAlgoManager unique_instance(mode, dbglogger, watch,
                                           tcp_client_socket);
    return unique_instance;
  }

  void thread_main() {
    dbglogger_ << "Thread main...\n";
    while (true) {
      int32_t response_length = tcp_client_socket_->ReadN(
          MAX_ORDER_RESPONSE_BUFFER_SIZE - read_offset_,
          data_buffer_ + read_offset_);
      if (response_length <= 0) {
        DBGLOG_CLASS_FUNC_LINE_FATAL
            << "-----EXITING AS THE ORDER ROUTING SERVER IS DOWN-----"
            << DBGLOG_ENDL_FLUSH;
        exit(1);
      }
      read_offset_ =
          ProcessOrderResponses(data_buffer_, read_offset_ + response_length);
    }
  }
  int32_t ProcessOrderResponses(char *server_data_packet,
                                int32_t const this_msg_length) {
    int32_t length_to_be_processed = this_msg_length;
    char const *msg_ptr = server_data_packet;

    while (length_to_be_processed > 0) {
      if ((uint32_t)length_to_be_processed < sizeof(OrderResponse)) {
        memcpy((void *)data_buffer_, (void *)msg_ptr, length_to_be_processed);
        return length_to_be_processed;
      }

      memcpy((void *)&order_response_, (void *)msg_ptr, sizeof(OrderResponse));
      msg_ptr += sizeof(OrderResponse);
      length_to_be_processed -= sizeof(OrderResponse);

      // At this point we have a single struct response stored in data member
      mkt_lock_.LockMutex();
      ProcessSingleResponse();
      mkt_lock_.UnlockMutex();
    }
    return 0;
  }
};
}
