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
#pragma once
#include "midterm/MidTerm/base_trader.hpp"
namespace MIDTERM {

// This class simply sends order to the market maker
class SimTrader : public BaseTrader {
public:
  SimTrader(HFSAT::DebugLogger &dbglogger, HFSAT::Watch &watch)
      : BaseTrader(dbglogger, watch) {}

  SimTrader(BaseTrader const &disabled_copy_constructor) = delete;

  static SimTrader &GetUniqueInstance(HFSAT::DebugLogger &dbglogger,
                                      HFSAT::Watch &_watch_) {
    static SimTrader unique_instance(dbglogger, _watch_);
    return unique_instance;
  }

public:
  void SendTrade(OrderRequest);
};
}
