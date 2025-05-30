// =====================================================================================
//
//       Filename:  sim_order_manager.hpp
//
//    Description:
//
//        Version:  1.0
//        Cre  03/21/2016 10:31:11 AM
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
#include "midterm/MidTerm/base_order_manager.hpp"

namespace MIDTERM {

// BaseOrderManager in SimMode
class SimOrderManager : public BaseOrderManager {
public:
  SimOrderManager(HFSAT::DebugLogger &dbglogger, HFSAT::Watch &watch,
                  BaseTrader &trader, BaseAlgoManager &algo_manager)
      : BaseOrderManager(Mode::kNSESimMode, dbglogger, watch, trader,
                         algo_manager) {
    dbglogger_ << "SimOrder Manager constructor complete...\n";
  }
  static SimOrderManager &GetUniqueInstance(HFSAT::DebugLogger &dbglogger,
                                            HFSAT::Watch &_watch_,
                                            BaseTrader &trader,
                                            BaseAlgoManager &algo_manager) {
    static SimOrderManager unique_instance(dbglogger, _watch_, trader,
                                           algo_manager);
    return unique_instance;
  }
};
}
