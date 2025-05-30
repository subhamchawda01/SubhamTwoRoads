// =====================================================================================
//
//       Filename:  base_trader.hpp
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
#include "midterm/MidTerm/mid_term_order_routing_defines.hpp"
namespace MIDTERM {

class BaseTrader {
public:
  HFSAT::DebugLogger &dbglogger_;
  HFSAT::Watch &watch_;

public:
  BaseTrader(HFSAT::DebugLogger &dbglogger, HFSAT::Watch &watch)
      : dbglogger_(dbglogger), watch_(watch) {}

  BaseTrader(BaseTrader const &disabled_copy_constructor) = delete;
  virtual ~BaseTrader(){};

public:
  virtual void SendTrade(OrderRequest) = 0;
};
}
