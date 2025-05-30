// =====================================================================================
//
//       Filename:  PVOL.hpp
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

#include "midterm/MidTerm/execution_logic.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

namespace MIDTERM {

class PVOL : public ExecutionLogic {
public:
  double participation_rate_;
  int execution_interval_;

public:
  PVOL(HFSAT::DebugLogger &dbglogger, HFSAT::Watch &watch,
       std::string shortcode, int lotsize, double participation_rate,
       int execution_interval, BaseTrader &trader,
       BaseAlgoManager &algo_manager, Mode mode)
      : ExecutionLogic(dbglogger, watch, shortcode, lotsize, trader,
                       algo_manager, mode),
        participation_rate_(participation_rate),
        execution_interval_(execution_interval) {}

  // mostly a wrapper around CreateScheduleOfOrders
  // order_quantity could be +ve/-ve
  void AddOrder(int order_quantity) {
    int total_quantity = ExecutionLogic::GetTotalQuantity();
    char old_side = side_;
    if (old_side == 'B')
      total_quantity += order_quantity;
    else if (old_side == 'S')
      total_quantity = order_quantity - total_quantity;

    if (total_quantity >= 0)
      side_ = 'B';
    else
      side_ = 'S';
    schedule_.clear();
    CreateScheduleOfOrders(std::abs(total_quantity), participation_rate_,
                           execution_interval_, shortcode_);
  }
  void OnTimePeriodUpdate(int x) {}

  void OnOrderReceived(OrderResponse response_) {}

private:
  // max_time_to_execute and total quantity to execute
  // execution_interval could be set to 30 sec default
  // always execute atleast min_size_to_execute
  // assume max_time_to_execute % execution_interval == 0
  // all sizes in no of lots
  void CreateScheduleOfOrders(int total_quantity, double participation_rate,
                              int execution_interval, std::string shortcode) {
    // start with opening_time
    std::time_t t =
        watch_.tv().tv_sec - (watch_.tv().tv_sec % execution_interval);
    std::map<int, int> vol_map_ = GetAverageVolumeProfile(shortcode);

    int vol_executed = 0;
    int time_key;
    int vol_this_interval;

    while (vol_executed < total_quantity) {
      time_key = t - (t % (VOLUME_PROFILE_GRANULARITY *
                           60)); // get this to one of the keys of the map
      vol_this_interval = std::round(double(vol_map_[time_key]) *
                                     (double(execution_interval) /
                                      double(VOLUME_PROFILE_GRANULARITY * 60)) *
                                     participation_rate);
      vol_executed += vol_this_interval;
      if (vol_executed <= total_quantity)
        schedule_.push_back(std::make_pair(t, vol_this_interval));
      else
        schedule_.push_back(std::make_pair(
            t, (total_quantity - (vol_executed - vol_this_interval))));
      t += execution_interval;
    }
  }
};
}
