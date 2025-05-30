// =====================================================================================
//
//       Filename:  TWAP.cpp
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
#include "midterm/MidTerm/TWAP.hpp"
#include "midterm/MidTerm/sim_order_manager.hpp"
#include "midterm/MidTerm/mid_term_order_manager.hpp"

namespace MIDTERM {
// mostly a wrapper around CreateScheduleOfOrders
// order_quantity could be +ve/-ve
// Giving default param for shortcode in AddOrder.. incase of TWAP this is not
// used
void TWAP::AddOrder(int order_quantity) {
  int total_quantity = ExecutionLogic::GetTotalQuantity();
  entry_time_ = watch_.tv().tv_sec;
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
  dbglogger_ << "Creating schedule of orders in TWAP...\n";
  CreateScheduleOfOrders(std::abs(total_quantity), max_time_to_execute_,
                         min_size_to_execute_, execution_interval_);
  dbglogger_ << PrintSchedule() << DBGLOG_ENDL_FLUSH;
}

void TWAP::OnTimePeriodUpdate(int x) {
  MarketLock();

  // dbglogger_ << __func__ << " " << watch_.tv().tv_sec << "\n";

  // Case when got something to execute
  if (ExecutionLogic::GetTimeOfFirstOrder() != -1) {
    // Initializing..
    if (!is_executing_) {
      is_executing_ = true;
      orders_sent_ = 0;
      orders_received_ = 0;
    }
    // Normal case when scheduled order should be sent
    if (watch_.tv().tv_sec > ExecutionLogic::GetTimeOfFirstOrder()) {
      dbglogger_ << "Sending orders to market...\n";
      // Make this false when order executed
      double order_price_ = (side_ == 'B')
                                ? (market_[security_id_].best_ask_price)
                                : (market_[security_id_].best_bid_price);
      int order_size_ = ExecutionLogic::GetQuantityOfFirstOrder();
      // Insert order ID to order status map
      OrderStatus status_ = OrderStatus();
      status_.timestamp = watch_.tv().tv_sec;
      status_.quantity = order_size_;
      status_.status = Order_Status::kNSEOrderInProcess;
      status_.side = side_;
      order_status_info_.insert(make_pair(order_id_, status_));
      ExecutionLogic::SendOrder(order_id_, shortcode_, side_, order_size_,
                                order_price_, OrderType::kOrderSend);
      // Delete order from schedule
      ExecutionLogic::DeleteFirstOrder();
      order_id_++;
      orders_sent_++;
    }
  } else if (is_executing_ && (orders_sent_ == orders_received_) &&
             (orders_sent_ != 0)) {
    dbglogger_ << "Sending executions back to order manager for end of trade "
                  "analysis...\n";
    BaseOrderManager *order_manager_;
    if (operating_mode_ == Mode::kNSEServerMode) {
      order_manager_ = &MidTermOrderManager::GetUniqueInstance(
          Mode::kNSEServerMode, dbglogger_, watch_, trader_, algo_manager_);
    } else {
      order_manager_ = &SimOrderManager::GetUniqueInstance(
          dbglogger_, watch_, trader_, algo_manager_);
    }

    for (auto exec_ : executions) {
      dbglogger_ << "Execution for: " << exec_.first << "\t" << exec_.second
                 << " for " << shortcode_ << "\n";
    }

    order_manager_->OnEndOfTradeAnalysis(shortcode_, executions);
    is_executing_ = false;
    orders_sent_ = 0;
    orders_received_ = 0;
    executions.clear();
  }

  // In any case, handle confirmed, cancelled etc. orders
  typedef map<int, OrderStatus>::iterator it_;
  vector<int> remove_order_id_;
  for (it_ iterator = order_status_info_.begin();
       iterator != order_status_info_.end(); iterator++) {
    int order_id = iterator->first;
    Order_Status order_status = iterator->second.status;
    int order_time = iterator->second.timestamp;
    if ((watch_.tv().tv_sec - order_time >= 50) ||
        (watch_.tv().tv_sec > entry_time_ + max_time_to_execute_)) {
      if (order_status == Order_Status::kNSEOrderConfirmed) {
        order_status_info_[order_id].status = Order_Status::kNSEOrderInProcess;
        ExecutionLogic::SendOrder(order_id, shortcode_, 'I', -1, -1,
                                  OrderType::kOrderCancel);
        // orders_sent will decrement by 1 when we get an order cancel
      } else if (order_status == Order_Status::kNSEOrderCancelled ||
                 order_status == Order_Status::kNSEOrderRejected) {
        // Remove order from status map, as we will send new order id
        // order_status_info_[ order_id ].status = "DEAD";
        double order_price_ = (order_status_info_[order_id].side == 'B')
                                  ? (market_[security_id_].best_ask_price)
                                  : (market_[security_id_].best_bid_price);

        OrderStatus status_ = OrderStatus();
        status_.timestamp = watch_.tv().tv_sec;
        status_.quantity = order_status_info_[order_id].quantity;
        status_.side = order_status_info_[order_id].side;
        status_.status = Order_Status::kNSEOrderInProcess;
        remove_order_id_.push_back(order_id);
        order_status_info_.insert(make_pair(order_id_, status_));
        ExecutionLogic::SendOrder(order_id_, shortcode_, status_.side,
                                  status_.quantity, order_price_,
                                  OrderType::kOrderSend);
        order_id_++;
      }
    }
  }
  for (auto x : remove_order_id_) {
    order_status_info_.erase(x);
  }
  remove_order_id_.clear();
  MarketUnlock();
}

// max_time_to_execute and total quantity to execute
// execution_interval could be set to 30 sec default
// always execute atleast min_size_to_execute
// assume max_time_to_execute % execution_interval == 0
// all sizes in no of lots
void TWAP::CreateScheduleOfOrders(int total_quantity, int max_time_to_execute,
                                  int min_size_to_execute,
                                  int execution_interval) {
  double seconds_per_order = double(max_time_to_execute) / total_quantity;
  double orders_per_interval = double(execution_interval) / seconds_per_order;
  // Implies execute an order in interval seconds

  if (orders_per_interval < min_size_to_execute) {
    CreateScheduleOfMinExecutions(total_quantity, min_size_to_execute,
                                  execution_interval);
    return;
  }

  int number_of_intervals = max_time_to_execute / execution_interval;
  std::time_t t = watch_.tv().tv_sec;

  for (int i = 0; i < number_of_intervals; i++) {
    schedule_.push_back(std::make_pair(t, orders_per_interval));
    t += execution_interval;
  }

  double error = 0.0;
  for (uint32_t i = 0; i < schedule_.size(); i++) {
    error += schedule_[i].second - std::floor(schedule_[i].second);
    schedule_[i].second = int(std::floor(schedule_[i].second));
    if (error >= 1.0) {
      schedule_[i].second += 1;
      error -= 1.0;
    }
  }
  if (error > 0)
    schedule_[schedule_.size() - 1].second += int(std::round(error));
}

void TWAP::CreateScheduleOfMinExecutions(int total_quantity,
                                         int min_size_to_execute,
                                         int execution_interval) {
  double number_of_intervals =
      double(total_quantity) / double(min_size_to_execute);
  double error = number_of_intervals - std::floor(number_of_intervals);

  std::time_t t = watch_.tv().tv_sec;
  for (int i = 0; i < int(std::floor(number_of_intervals)); i++) {
    schedule_.push_back(std::make_pair(t, min_size_to_execute));
    t += execution_interval;
  }

  if (error > 0)
    schedule_.push_back(
        std::make_pair(t, int(std::round(error * min_size_to_execute))));
}
}
