// =====================================================================================
//
//       Filename:  live_trader.hpp
//
//    Description:
//
//        Version:  1.0
//        Cre  03/18/2016 10:31:11 AM
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
#include "midterm/MidTerm/base_trader.hpp"
#include "dvccode/Utils/tcp_client_socket.hpp"
namespace MIDTERM {

class LiveTrader : public BaseTrader {
public:
  HFSAT::TCPClientSocket *tcp_client_socket_;

public:
  LiveTrader(HFSAT::DebugLogger &dbglogger, HFSAT::Watch &watch,
             HFSAT::TCPClientSocket *tcp_client_socket)
      : BaseTrader(dbglogger, watch), tcp_client_socket_(tcp_client_socket) {}

  LiveTrader(BaseTrader const &disabled_copy_constructor) = delete;

  static LiveTrader &
  GetUniqueInstance(HFSAT::DebugLogger &dbglogger, HFSAT::Watch &_watch_,
                    HFSAT::TCPClientSocket *tcp_client_socket) {
    static LiveTrader unique_instance(dbglogger, _watch_, tcp_client_socket);
    return unique_instance;
  }

public:
  void SendTrade(OrderRequest);
};
}
