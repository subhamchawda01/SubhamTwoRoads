// =====================================================================================
//
//       Filename:  VWAP.hpp
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

class VWAP : public ExecutionLogic {
public:
  int max_time_to_execute_;
  int execution_interval_;

public:
  VWAP(HFSAT::DebugLogger &dbglogger, HFSAT::Watch &watch,
       std::string shortcode, int lotsize, int max_time_to_execute,
       int execution_interval, BaseTrader &trader,
       BaseAlgoManager &algo_manager, Mode mode)
      : ExecutionLogic(dbglogger, watch, shortcode, lotsize, trader,
                       algo_manager, mode),
        max_time_to_execute_(max_time_to_execute),
        execution_interval_(execution_interval) {}

  // mostly a wrapper around CreaetScheduleOfOrders
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
    CreateScheduleOfOrders(std::abs(total_quantity), max_time_to_execute_,
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
  void CreateScheduleOfOrders(int total_quantity, int max_time_to_execute,
                              int execution_interval, std::string shortcode) {
    // start with opening_time
    std::time_t t =
        watch_.tv().tv_sec - (watch_.tv().tv_sec % execution_interval);
    std::map<int, int> vol_map_ = GetAverageVolumeProfile(shortcode);

    int number_of_intervals = max_time_to_execute / execution_interval;
    int time_key;
    double orders_this_interval;
    std::vector<std::pair<int, double>> temp_;
    double total = 0;
    for (int i = 0; i < number_of_intervals; i++) {
      time_key = t - (t % (VOLUME_PROFILE_GRANULARITY * 60)); // Get this to one
                                                              // of the keys of
                                                              // the map as they
                                                              // differ by half
                                                              // hour

      orders_this_interval = double(vol_map_[time_key]) *
                             (double(execution_interval) /
                              double(VOLUME_PROFILE_GRANULARITY * 60));

      temp_.push_back(std::make_pair(t, orders_this_interval));
      t += execution_interval;
      total += orders_this_interval;
    }

    double vol_this_interval;
    double error = 0.0;
    for (int i = 0; i < number_of_intervals; i++) {
      vol_this_interval = (temp_[i].second / total) * double(total_quantity);
      schedule_.push_back(
          std::make_pair(temp_[i].first, std::floor(vol_this_interval)));
      error += vol_this_interval - std::floor(vol_this_interval);
      if (error >= 1.0) {
        schedule_[i].second += 1;
        error -= 1.0;
      }
    }
  }
};
}
