// =====================================================================================
//
//       Filename:  sim_trader.hpp
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
#include "midterm/MidTerm/sim_trader.hpp"
#include "midterm/MidTerm/sim_market_maker.hpp"
namespace MIDTERM {
void SimTrader::SendTrade(OrderRequest order_) {
  dbglogger_ << "Trader sending order"
             << "\n";
  SimMarketMaker *sim_market_maker_ =
      &SimMarketMaker::GetUniqueInstance(dbglogger_, watch_);
  sim_market_maker_->ReceiveOrder(order_);
}
}
