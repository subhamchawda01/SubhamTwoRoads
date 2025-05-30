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
#include "midterm/MidTerm/live_trader.hpp"
namespace MIDTERM {
void LiveTrader::SendTrade(OrderRequest order_) {

  int32_t written_length = tcp_client_socket_->WriteN(sizeof(order_), &order_);
  // dbglogger_ << order_.ToString() << "\n";
  if (written_length < (int32_t)sizeof(MIDTERM::OrderRequest)) {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "FAILED TO RESPOND TO ORDER ROUTING EXEC : "
                                 << " ERROR : " << strerror(errno)
                                 << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
  }
  dbglogger_ << "WROTE : " << written_length << " ERROR : " << strerror(errno)
             << DBGLOG_ENDL_FLUSH;
}
}
