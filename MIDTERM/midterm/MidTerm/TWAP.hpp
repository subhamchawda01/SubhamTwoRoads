// =====================================================================================
//
//       Filename:  TWAP.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Friday 22 January 2016 01:54:47  GMT
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

#include "execution_logic.hpp"

namespace MIDTERM {

class TWAP : public ExecutionLogic {
public:
  int max_time_to_execute_;
  int min_size_to_execute_;
  int execution_interval_;
  int entry_time_;

public:
  TWAP(HFSAT::DebugLogger &dbglogger, HFSAT::Watch &watch,
       std::string shortcode, int lotsize, int max_time_to_execute,
       int min_size_to_execute, int execution_interval, BaseTrader &trader,
       BaseAlgoManager &algo_manager, Mode mode)
      : ExecutionLogic(dbglogger, watch, shortcode, lotsize, trader,
                       algo_manager, mode),
        max_time_to_execute_(max_time_to_execute),
        min_size_to_execute_(min_size_to_execute),
        execution_interval_(execution_interval) {
    entry_time_ = watch_.tv().tv_sec;
  }

  // mostly a wrapper around CreateScheduleOfOrders
  // order_quantity could be +ve/-ve
  // Giving default param for shortcode in AddOrder.. incase of TWAP this is not
  // used
  void AddOrder(int order_quantity);
  void OnTimePeriodUpdate(int x);

  // max_time_to_execute and total quantity to execute
  // execution_interval could be set to 30 sec default
  // always execute atleast min_size_to_execute
  // assume max_time_to_execute % execution_interval == 0
  // all sizes in no of lots
  void CreateScheduleOfOrders(int total_quantity, int max_time_to_execute = 60,
                              int min_size_to_execute = 2,
                              int execution_interval = 15);

  void CreateScheduleOfMinExecutions(int total_quantity,
                                     int min_size_to_execute,
                                     int execution_interval);
};
}
