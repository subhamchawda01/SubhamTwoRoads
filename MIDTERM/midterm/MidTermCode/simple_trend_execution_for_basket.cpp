// =====================================================================================
//
//       Filename:  simple_trend_execution_for_basket.cpp
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
#include "midterm/MidTerm/simple_trend_execution_for_basket.hpp"
#include "midterm/MidTerm/sim_order_manager.hpp"
#include "midterm/MidTerm/mid_term_order_manager.hpp"

namespace MIDTERM {

// Simply initializes the TrendInfo for a particular lef
// Quantity can be +ve/-ve depending on BUY/SELL
// Passive best level is used to check the OPEN SLIPPAGE going forward
TrendInfo
SimpleTrendExecutionForBasket::UpdateTrendInfoForFirstLeg(int quantity) {
  TrendInfo trend_ = TrendInfo();
  trend_.ema = first_leg_ema_trend_;
  trend_.shortcode = first_leg_shortcode_;
  trend_.stdev = first_leg_stdev;
  trend_.passive_best_level = (quantity > 0)
                                  ? market_[first_leg_sec_id_].best_bid_price
                                  : market_[first_leg_sec_id_].best_ask_price;
  trend_.remaining_quantity = quantity;
  trend_.order_time = watch_.tv().tv_sec - 40;
  trend_.entry_time = watch_.tv().tv_sec;
  return trend_;
}

// Simply initializes the TrendInfo for a particular leg
// Quantity can be +ve/-ve depending on BUY/SELL
// Passive best level is used to check the OPEN SLIPPAGE going forward
TrendInfo
SimpleTrendExecutionForBasket::UpdateTrendInfoForSecondLeg(int quantity) {
  TrendInfo trend_ = TrendInfo();
  trend_.ema = second_leg_ema_trend_;
  trend_.shortcode = second_leg_shortcode_;
  trend_.stdev = second_leg_stdev;
  trend_.passive_best_level = (quantity > 0)
                                  ? market_[second_leg_sec_id_].best_bid_price
                                  : market_[second_leg_sec_id_].best_ask_price;
  trend_.remaining_quantity = quantity;
  // TODO:: FIX THIS
  trend_.order_time = watch_.tv().tv_sec - 40;
  trend_.entry_time = watch_.tv().tv_sec;
  return trend_;
}

// Called be the base_order_manager class when it wants to inject orders in algo
// Updates the SimpleTrendExecutionInfo map
// We find the shortcode that would be dealt aggressively here itself
// TODO:: Aggressive handling can be made dynamic ?
void SimpleTrendExecutionForBasket::AddOrder(int order_quantity) {
  // dbglogger_ << "Lock acquiring: " << __func__ << ":" << __LINE__ <<
  // DBGLOG_ENDL_FLUSH;
  mkt_lock_.LockMutex();
  // dbglogger_ << "Lock acquired: " << __func__ << ":" << __LINE__ <<
  // DBGLOG_ENDL_FLUSH;

  // Straddle
  total_orders_to_process_ += 2 * abs(order_quantity);

  is_executing_ = true;
  SimpleTrendExecutionInfo info_;
  int timestamp_ = watch_.tv().tv_sec;
  // Algo can only sell 1 lot at once..
  info_.quantity = -1;
  info_.first_leg = UpdateTrendInfoForFirstLeg(-1);
  info_.second_leg = UpdateTrendInfoForSecondLeg(-1);

  // Find the shortcode which would be dealt aggressively
  if (((info_.first_leg.ema > info_.second_leg.ema) && (order_quantity > 0)) ||
      ((info_.first_leg.ema < info_.second_leg.ema) && (order_quantity < 0))) {
    info_.to_aggress_shortcode = first_leg_shortcode_;
  } else {
    info_.to_aggress_shortcode = second_leg_shortcode_;
  }

  // Print all parameters
  dbglogger_
      << "--------------ALGO PARAMETERS---------------------------------\n";
  dbglogger_ << "Quantity: " << order_quantity << '\n';
  dbglogger_ << "Sec ID for: " << first_leg_shortcode_ << "\t"
             << first_leg_sec_id_ << "\n";
  dbglogger_ << "Sec ID for: " << second_leg_shortcode_ << "\t"
             << second_leg_sec_id_ << "\n";
  dbglogger_ << "Trend for: " << first_leg_shortcode_ << "\t"
             << info_.first_leg.ema << "\n";
  dbglogger_ << "Trend for: " << second_leg_shortcode_ << "\t"
             << info_.second_leg.ema << "\n";
  dbglogger_ << "Stdev for: " << first_leg_shortcode_ << "\t"
             << info_.first_leg.stdev << "\n";
  dbglogger_ << "Stdev for: " << second_leg_shortcode_ << "\t"
             << info_.second_leg.stdev << "\n";
  dbglogger_ << "Agress shortcode is: " << info_.to_aggress_shortcode << "\n";
  dbglogger_ << "Bid-Ask on: " << first_leg_shortcode_
             << " is: " << market_[first_leg_sec_id_].best_bid_price << ", "
             << market_[first_leg_sec_id_].best_ask_price << "\n";
  dbglogger_ << "Bid-Ask on: " << second_leg_shortcode_
             << " is: " << market_[second_leg_sec_id_].best_bid_price << ", "
             << market_[second_leg_sec_id_].best_ask_price << "\n";

  // TODO::PARAMETERIZE THIS
  // Intuitively an ATM call/put sells for max 400, and that equates to 8 points
  // which is 400 bps
  first_leg_open_slippage_threshold_ =
      max(10 * info_.first_leg.stdev,
          2.0 * market_[first_leg_sec_id_].best_ask_price / 100.0);
  second_leg_open_slippage_threshold_ =
      max(10 * info_.second_leg.stdev,
          2.0 * market_[second_leg_sec_id_].best_ask_price / 100.0);

  dbglogger_ << "Open-Slippage threshold on: " << first_leg_shortcode_
             << " is: " << first_leg_open_slippage_threshold_ << "\n";
  dbglogger_ << "Open-Slippage threshold on: " << second_leg_shortcode_
             << " is: " << second_leg_open_slippage_threshold_ << "\n";
  dbglogger_
      << "----------------------------------------------------------------\n";

  for (int i = 0; i < abs(order_quantity); i++) {
    // Insert into the queue of orders
    // TODO::Add better handling when multiple orders are added..
    if (simple_trend_map.find(timestamp_) == simple_trend_map.end()) {
      simple_trend_map.insert(make_pair(timestamp_, info_));
    } else {
      dbglogger_ << "ALERT -> Same timestamp exists in the map !!!"
                 << DBGLOG_ENDL_FLUSH;
      int last_timestamp_ = simple_trend_map.rbegin()->first;
      simple_trend_map.insert(make_pair(last_timestamp_ + 1, info_));
      dbglogger_ << "==========================================---"
                 << DBGLOG_ENDL_FLUSH;
    }
  }

  // dbglogger_ << "UnLock acquiring: " << __func__ << ":" << __LINE__ <<
  // DBGLOG_ENDL_FLUSH;
  mkt_lock_.UnlockMutex();
  // dbglogger_ << "UnLock acquired: " << __func__ << ":" << __LINE__ <<
  // DBGLOG_ENDL_FLUSH;
}

// Called on every time update
void SimpleTrendExecutionForBasket::OnTimePeriodUpdate(int x) {
  HandleAggressively();
  // HandleSemiPassively( false );
  // UseTrend();
}

// Place aggressively on the leg which we think would move away
// Place passively on the other leg
// No refresh, we wait for the max_time_to_execute_, by this time the aggress
// leg is most probaably executed
// If the passive leg too is pending, we aggress on it
// If reverse is set to true, then we bet on mean reversion and aggress on the
// non aggress_shortcode, get it ?
void SimpleTrendExecutionForBasket::HandleSemiPassively(bool reverse) {
  typedef map<int, SimpleTrendExecutionInfo>::iterator it_;
  for (it_ iterator = simple_trend_map.begin();
       iterator != simple_trend_map.end(); iterator++) {
    ExecutionLogic::MarketLock();
    int timestamp_ = iterator->first;
    SimpleTrendExecutionInfo &info_ = iterator->second;
    // info_.to_aggress_shortcode

    // Possible that the passive leg has not yet been executed
    // Becomes a FAIL SAFE for the aggress leg as it should be executed by now
    // TODO::PARAMETERIZE 100
    if (watch_.tv().tv_sec > timestamp_ + max_time_to_execute_) {
      if ((watch_.tv().tv_sec > info_.first_leg.order_time + 100) &&
          (info_.first_leg.status == Order_Status::kNSEOrderConfirmed)) {
        PlaceCancel(info_.first_leg);
      }
      if ((watch_.tv().tv_sec > info_.second_leg.order_time + 100) &&
          (info_.second_leg.status == Order_Status::kNSEOrderConfirmed)) {
        PlaceCancel(info_.second_leg);
      }
    }

    if (!reverse) {
      if (first_leg_shortcode_ == info_.to_aggress_shortcode) {
        AggressOnLeg(info_.first_leg, first_leg_sec_id_);
        PassiveOnLeg(info_.second_leg, second_leg_sec_id_);
      } else if (second_leg_shortcode_ == info_.to_aggress_shortcode) {
        AggressOnLeg(info_.second_leg, second_leg_sec_id_);
        PassiveOnLeg(info_.first_leg, first_leg_sec_id_);
      }
    } else {
      if (first_leg_shortcode_ == info_.to_aggress_shortcode) {
        AggressOnLeg(info_.second_leg, second_leg_sec_id_);
        PassiveOnLeg(info_.first_leg, first_leg_sec_id_);
      } else if (second_leg_shortcode_ == info_.to_aggress_shortcode) {
        AggressOnLeg(info_.first_leg, first_leg_sec_id_);
        PassiveOnLeg(info_.second_leg, second_leg_sec_id_);
      }
    }
    ExecutionLogic::MarketUnlock();
  }
}

// Simply go aggressive on both legs simultaneously
// We keep orders_sent/orders_received to make sure that orders sent == orders
// received
// Place cancel after 100 seconds if not executed -> Should be an extremely rare
// condition
void SimpleTrendExecutionForBasket::HandleAggressively() {
  typedef map<int, SimpleTrendExecutionInfo>::iterator it_;
  for (it_ iterator = simple_trend_map.begin();
       iterator != simple_trend_map.end(); iterator++) {
    ExecutionLogic::MarketLock();
    int timestamp_ = iterator->first;
    SimpleTrendExecutionInfo &info_ = iterator->second;
    // Initial status of a leg is CANCELLED

    // This should be an extremely rare condition, more of a FAIL SAFE logic
    // TODO::PARAMETERIZE 100
    if (watch_.tv().tv_sec > timestamp_ + max_time_to_execute_) {
      if ((watch_.tv().tv_sec > info_.first_leg.order_time + 100) &&
          (info_.first_leg.status == Order_Status::kNSEOrderConfirmed)) {
        PlaceCancel(info_.first_leg);
      }
      if ((watch_.tv().tv_sec > info_.second_leg.order_time + 100) &&
          (info_.second_leg.status == Order_Status::kNSEOrderConfirmed)) {
        PlaceCancel(info_.second_leg);
      }
    }
    AggressOnLeg(info_.first_leg, first_leg_sec_id_);
    AggressOnLeg(info_.second_leg, second_leg_sec_id_);
    ExecutionLogic::MarketUnlock();
  }
}

void SimpleTrendExecutionForBasket::UseTrend() {
  typedef map<int, SimpleTrendExecutionInfo>::iterator it_;
  for (it_ iterator = simple_trend_map.begin();
       iterator != simple_trend_map.end(); iterator++) {
    SimpleTrendExecutionInfo &info_ = iterator->second;

    bool first_leg_check_ = false;
    bool second_leg_check_ = false;
    // Check open slippage threshold
    if (watch_.tv().tv_sec - info_.first_leg.entry_time > 30) {
      first_leg_check_ =
          CheckOpenSlippage(info_.first_leg, first_leg_open_slippage_threshold_,
                            first_leg_sec_id_);
    }
    if (watch_.tv().tv_sec - info_.second_leg.entry_time > 30) {
      second_leg_check_ = CheckOpenSlippage(info_.second_leg,
                                            second_leg_open_slippage_threshold_,
                                            second_leg_sec_id_);
    }

    // Check aggress due to time condition
    if (!first_leg_check_) {
      RefreshOrders(info_.first_leg, first_leg_sec_id_,
                    info_.to_aggress_shortcode, true);
    }
    if (!second_leg_check_) {
      RefreshOrders(info_.second_leg, second_leg_sec_id_,
                    info_.to_aggress_shortcode, true);
    }
  }
}

// Check if the open slippage has been breached, and if so place cancel/aggress
// order
// Returns bool so that one knows that the open slippage has been breached and
// was handled here
bool SimpleTrendExecutionForBasket::CheckOpenSlippage(TrendInfo &leg_,
                                                      int threshold_, int id_) {
  double open_slippage =
      (leg_.remaining_quantity > 0)
          ? (market_[id_].best_ask_price - leg_.passive_best_level)
          : (leg_.passive_best_level - market_[id_].best_bid_price);
  if (open_slippage > threshold_) {
    // TODO::PARAMETERIZE 100
    if (watch_.tv().tv_sec - leg_.order_time > 100 &&
        leg_.status == Order_Status::kNSEOrderConfirmed) {
      dbglogger_ << "Market price during open slippage handling: "
                 << watch_.tv().tv_sec << "\t" << leg_.shortcode << "\t"
                 << market_[id_].best_bid_price << "\t"
                 << market_[id_].best_ask_price << "\n";
      PlaceCancel(leg_);
      return true;
    } else if (leg_.status == Order_Status::kNSEOrderCancelled) {
      dbglogger_ << "Market price during open slippage handling: "
                 << watch_.tv().tv_sec << "\t" << leg_.shortcode << "\t"
                 << market_[id_].best_bid_price << "\t"
                 << market_[id_].best_ask_price << "\n";
      AggressOnLeg(leg_, id_);
      return true;
    }
  }
  return false;
}

void SimpleTrendExecutionForBasket::RefreshOrders(TrendInfo &leg_, int id_,
                                                  string agress_shortcode_,
                                                  bool improve_) {
  if (watch_.tv().tv_sec - leg_.order_time > 50 &&
      leg_.status == Order_Status::kNSEOrderConfirmed) {
    dbglogger_ << "Market price: " << watch_.tv().tv_sec << "\t"
               << leg_.shortcode << "\t" << market_[id_].best_bid_price << "\t"
               << market_[id_].best_ask_price << "\n";
    PlaceCancel(leg_);
  } else if (leg_.status == Order_Status::kNSEOrderCancelled) {
    dbglogger_ << "Market price: " << watch_.tv().tv_sec << "\t"
               << leg_.shortcode << "\t" << market_[id_].best_bid_price << "\t"
               << market_[id_].best_ask_price << "\n";
    double passive_price = (leg_.remaining_quantity > 0)
                               ? (market_[id_].best_bid_price)
                               : (market_[id_].best_ask_price);

    if (leg_.shortcode != agress_shortcode_) {
      // Currently placing at 1 stdev away
      // TODO::PARAMETERIZE STDEV STEPS
      if (leg_.remaining_quantity > 0) {
        passive_price -= leg_.stdev;
      } else {
        passive_price += leg_.stdev;
      }
    }

    // TODO::PARAMETERIZE THIS
    if (improve_ &&
        watch_.tv().tv_sec > leg_.entry_time + 0.5 * max_time_to_execute_) {
      dbglogger_ << "Going for improve on order ID: " << order_id_
                 << DBGLOG_ENDL_FLUSH;
      double improve_amt_ =
          (market_[id_].best_ask_price - market_[id_].best_bid_price) / 2.0;
      if (leg_.remaining_quantity > 0) {
        passive_price += improve_amt_;
      } else {
        passive_price -= improve_amt_;
      }
    }

    // Aggress situation
    // TODO::PARAMETERIZE THIS
    if (watch_.tv().tv_sec > leg_.entry_time + 0.9 * max_time_to_execute_) {
      dbglogger_ << "Going for aggression on order ID: " << order_id_
                 << DBGLOG_ENDL_FLUSH;
      passive_price = (leg_.remaining_quantity > 0)
                          ? market_[id_].best_ask_price
                          : market_[id_].best_bid_price;
    }

    PassiveOnLeg(leg_, id_, passive_price);
  }
}

// Utility function to place a cancel order on a leg
void SimpleTrendExecutionForBasket::PlaceCancel(TrendInfo &leg_) {
  leg_.status = Order_Status::kNSEOrderInProcess;
  // Need to subtract this as we are placing a cancel order
  // TODO::Move this to order cancel confirmation as order cancel reject is
  // possible
  orders_sent_--;
  ExecutionLogic::SendOrder(leg_.order_id, leg_.shortcode, 'I', -1, -1,
                            OrderType::kOrderCancel);
}

// Utility function to aggress on a leg
void SimpleTrendExecutionForBasket::AggressOnLeg(TrendInfo &leg_, int id_) {
  if (leg_.status == Order_Status::kNSEOrderCancelled) {
    // Check buy/sell
    char side_ = (leg_.remaining_quantity > 0) ? 'B' : 'S';
    leg_.order_id = order_id_;
    // Change status from cancelled to under process
    leg_.status = Order_Status::kNSEOrderInProcess;
    leg_.order_time = watch_.tv().tv_sec;
    double aggressive_price = (leg_.remaining_quantity > 0)
                                  ? market_[id_].best_ask_price
                                  : market_[id_].best_bid_price;
    ExecutionLogic::SendOrder(
        order_id_, sec_name_indexer_.GetShortcodeFromId(id_), side_,
        abs(leg_.remaining_quantity), aggressive_price, OrderType::kOrderSend);
    order_id_++;
    orders_sent_++;
  }
}

// Utility function to go passive on a leg
void SimpleTrendExecutionForBasket::PassiveOnLeg(TrendInfo &leg_, int id_,
                                                 double price_) {
  if (leg_.status == Order_Status::kNSEOrderCancelled) {
    // Check buy/sell
    char side_ = (leg_.remaining_quantity > 0) ? 'B' : 'S';
    leg_.order_id = order_id_;
    // Change status from cancelled to under process
    leg_.status = Order_Status::kNSEOrderInProcess;
    leg_.order_time = watch_.tv().tv_sec;
    double passive_price;
    if (price_ == -1) {
      passive_price = (leg_.remaining_quantity > 0)
                          ? market_[id_].best_bid_price
                          : market_[id_].best_ask_price;
    } else {
      passive_price = price_;
    }
    ExecutionLogic::SendOrder(order_id_, first_leg_shortcode_, side_,
                              abs(leg_.remaining_quantity), passive_price,
                              OrderType::kOrderSend);
    order_id_++;
    orders_sent_++;
  }
}

// Update indicators on every indicator update
void SimpleTrendExecutionForBasket::OnIndicatorUpdate(
    const unsigned int &_indicator_index_, const double &_new_value_) {
  ExecutionLogic::MarketLock();
  if (_indicator_index_ == 0) {
    first_leg_ema_trend_ = _new_value_;
  } else if (_indicator_index_ == 1) {
    second_leg_ema_trend_ = _new_value_;
  } else if (_indicator_index_ == 2) {
    first_leg_stdev = _new_value_;
  } else if (_indicator_index_ == 3) {
    second_leg_stdev = _new_value_;
  }
  ExecutionLogic::MarketUnlock();
}

void SimpleTrendExecutionForBasket::OnOrderExecuted(OrderResponse response_) {
  // dbglogger_ << "Lock acquiring: " << __func__ << ":" << __LINE__ <<
  // DBGLOG_ENDL_FLUSH;
  ExecutionLogic::MarketLock();
  // dbglogger_ << "Lock acquired: " << __func__ << ":" << __LINE__ <<
  // DBGLOG_ENDL_FLUSH;
  if (response_.size_remaining == 0) {
    if (orders_sent_ != 0) {
      dbglogger_ << "Order Execution for: " << response_.unique_order_id
                 << "\n";
    }
    typedef map<int, SimpleTrendExecutionInfo>::iterator it_;
    for (it_ iterator = simple_trend_map.begin();
         iterator != simple_trend_map.end(); iterator++) {
      int timestamp_ = iterator->first;
      SimpleTrendExecutionInfo info_ = iterator->second;
      if (info_.first_leg.order_id == response_.unique_order_id) {
        dbglogger_ << "Complete execution for Order ID: "
                   << response_.unique_order_id << DBGLOG_ENDL_FLUSH;
        // TODO::PARAMETERIZE LOTSIZE
        first_leg_executions_.push_back(std::make_pair(
            response_.response_size / lotsize_, response_.response_price));
        orders_received_++;
        simple_trend_map[timestamp_].first_leg.status =
            Order_Status::kNSEOrderExecuted;
      }
      if (info_.second_leg.order_id == response_.unique_order_id) {
        dbglogger_ << "Complete execution for Order ID: "
                   << response_.unique_order_id << DBGLOG_ENDL_FLUSH;
        second_leg_executions_.push_back(std::make_pair(
            response_.response_size / lotsize_, response_.response_price));
        orders_received_++;
        simple_trend_map[timestamp_].second_leg.status =
            Order_Status::kNSEOrderExecuted;
      }
    }
    if (orders_sent_ != 0) {
      dbglogger_ << "Orders sent/received: " << orders_sent_ << "\t"
                 << orders_received_ << "\n";
    }
    if (orders_sent_ == orders_received_ && is_executing_ &&
        orders_sent_ != 0 && orders_sent_ == total_orders_to_process_) {

      for (unsigned int i = 0; i < first_leg_executions_.size(); i++) {
        double exec_ =
            first_leg_executions_[i].second + second_leg_executions_[i].second;
        dbglogger_ << "Call-Put execution -> "
                   << first_leg_executions_[i].second + '\t'
                   << second_leg_executions_[i].second + '\n';
        executions.push_back(make_pair(first_leg_executions_[i].first, exec_));
      }
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

      order_manager_->OnEndOfTradeAnalysis(shortcode_string_, executions);
      orders_sent_ = 0;
      orders_received_ = 0;
      total_orders_to_process_ = 0;
      executions.clear();
      first_leg_executions_.clear();
      second_leg_executions_.clear();
      is_executing_ = false;
    }
  }
  // dbglogger_ << "UnLock acquiring: " << __func__ << ":" << __LINE__ <<
  // DBGLOG_ENDL_FLUSH;
  ExecutionLogic::MarketUnlock();
  // dbglogger_ << "UnLock acquired: " << __func__ << ":" << __LINE__ <<
  // DBGLOG_ENDL_FLUSH;
}

void SimpleTrendExecutionForBasket::OnOrderConfirmed(OrderResponse response_) {
  // dbglogger_ << "Lock acquiring: " << __func__ << ":" << __LINE__ <<
  // DBGLOG_ENDL_FLUSH;
  ExecutionLogic::MarketLock();
  // dbglogger_ << "Lock acquired: " << __func__ << ":" << __LINE__ <<
  // DBGLOG_ENDL_FLUSH;
  typedef map<int, SimpleTrendExecutionInfo>::iterator it_;
  for (it_ iterator = simple_trend_map.begin();
       iterator != simple_trend_map.end(); iterator++) {
    int timestamp_ = iterator->first;
    SimpleTrendExecutionInfo info_ = iterator->second;
    if (info_.first_leg.order_id == response_.unique_order_id) {
      dbglogger_ << "Order: " << response_.unique_order_id
                 << " has been confirmed...\n";
      simple_trend_map[timestamp_].first_leg.status =
          Order_Status::kNSEOrderConfirmed;
    }
    if (info_.second_leg.order_id == response_.unique_order_id) {
      dbglogger_ << "Order: " << response_.unique_order_id
                 << " has been confirmed...\n";
      simple_trend_map[timestamp_].second_leg.status =
          Order_Status::kNSEOrderConfirmed;
    }
  }
  // dbglogger_ << "UnLock acquiring: " << __func__ << ":" << __LINE__ <<
  // DBGLOG_ENDL_FLUSH;
  ExecutionLogic::MarketUnlock();
  // dbglogger_ << "UnLock acquired: " << __func__ << ":" << __LINE__ <<
  // DBGLOG_ENDL_FLUSH;
}

void SimpleTrendExecutionForBasket::OnOrderCancelled(OrderResponse response_) {
  // dbglogger_ << "Lock acquiring: " << __func__ << ":" << __LINE__ <<
  // DBGLOG_ENDL_FLUSH;
  ExecutionLogic::MarketLock();
  // dbglogger_ << "Lock acquired: " << __func__ << ":" << __LINE__ <<
  // DBGLOG_ENDL_FLUSH;
  typedef map<int, SimpleTrendExecutionInfo>::iterator it_;
  for (it_ iterator = simple_trend_map.begin();
       iterator != simple_trend_map.end(); iterator++) {
    int timestamp_ = iterator->first;
    SimpleTrendExecutionInfo info_ = iterator->second;
    if (info_.first_leg.order_id == response_.unique_order_id) {
      dbglogger_ << "Order: " << response_.unique_order_id
                 << " has been cancelled...\n";
      simple_trend_map[timestamp_].first_leg.status =
          Order_Status::kNSEOrderCancelled;
    }
    if (info_.second_leg.order_id == response_.unique_order_id) {
      dbglogger_ << "Order: " << response_.unique_order_id
                 << " has been cancelled...\n";
      simple_trend_map[timestamp_].second_leg.status =
          Order_Status::kNSEOrderCancelled;
    }
  }
  // dbglogger_ << "UnLock acquiring: " << __func__ << ":" << __LINE__ <<
  // DBGLOG_ENDL_FLUSH;
  ExecutionLogic::MarketUnlock();
  // dbglogger_ << "UnLock acquired: " << __func__ << ":" << __LINE__ <<
  // DBGLOG_ENDL_FLUSH;
}
}
