// =====================================================================================
//
//       Filename:  simple_trend_execution_for_basket.hpp
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
#include "dvctrade/Indicators/simple_trend.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"

namespace MIDTERM {

// Contains info about the trend of the product(leg)
// Also contains info regarding execution status in market
struct TrendInfo {
  string shortcode;
  double ema;
  int ema_interval;
  double stdev;
  int stdev_interval;
  bool is_executed;
  Order_Status status;
  int order_id;
  double remaining_quantity;
  double passive_best_level;
  double threshold;
  int order_time;
  int entry_time;

  TrendInfo() {
    ema = 0;
    ema_interval = 300;
    stdev = 0;
    stdev_interval = 300;
    is_executed = false;
    status = Order_Status::kNSEOrderCancelled;
    order_id = -1;
    remaining_quantity = -1;
    threshold = -1;
    order_time = -1;
  }
};

// Contains info regarding the two legs of the synthetic product
// x quantity of a synthetic product implies x quantity of each of the two legs
// This algo supports only one threshold
struct SimpleTrendExecutionInfo {
  int quantity;
  string to_aggress_shortcode;
  TrendInfo first_leg;
  TrendInfo second_leg;
};

class SimpleTrendExecutionForBasket : public ExecutionLogic,
                                      public HFSAT::IndicatorListener {
public:
  string first_leg_shortcode_;
  string second_leg_shortcode_;
  string shortcode_string_;
  HFSAT::SecurityNameIndexer &sec_name_indexer_;
  int first_leg_sec_id_;
  int second_leg_sec_id_;
  double weak_passive_threshold_;
  double strong_passive_threshold_;
  double first_leg_ema_trend_;
  double second_leg_ema_trend_;
  double first_leg_stdev;
  double second_leg_stdev;
  HFSAT::SecurityMarketViewPtrVec &sid_to_smv_ptr_map_;
  //  HFSAT::SimpleTrend *price_change_indicator_first_;           // Subscribed
  //  to first leg	-> Index == 0
  //  HFSAT::SimpleTrend *price_change_indicator_second_;          // Subscribed
  //  to second leg	-> Index == 1
  //  HFSAT::SlowStdevCalculator *stdev_change_indicator_first_;   // Subscribed
  //  to first leg	-> Index == 2
  //  HFSAT::SlowStdevCalculator *stdev_change_indicator_second_;  // Subscribed
  //  to second leg	-> Index == 3
  int max_time_to_execute_; // We want to execute within this time in any
                            // scenario
  int min_size_to_execute_;
  int execution_interval_;
  double first_leg_open_slippage_threshold_;
  double second_leg_open_slippage_threshold_;
  map<int, SimpleTrendExecutionInfo>
      simple_trend_map; // Map from timestamp to SimpleTrendExecutionInfo
  std::vector<std::pair<int, double>> first_leg_executions_;
  std::vector<std::pair<int, double>> second_leg_executions_;

public:
  SimpleTrendExecutionForBasket(
      HFSAT::SecurityMarketViewPtrVec &sid_to_smv_ptr_map,
      HFSAT::DebugLogger &dbglogger, HFSAT::Watch &watch,
      std::string shortcode_string, std::vector<std::string> shortcodes,
      int lotsize, HFSAT::PriceType_t price_type, int max_time_to_execute,
      int min_size_to_execute, int execution_interval, BaseTrader &trader,
      BaseAlgoManager &algo_manager, Mode mode)
      : ExecutionLogic(dbglogger, watch, shortcodes[0], lotsize, trader,
                       algo_manager, mode),
        first_leg_shortcode_(shortcodes[0]),
        second_leg_shortcode_(shortcodes[1]),
        shortcode_string_(shortcode_string),
        sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()),
        first_leg_sec_id_(
            sec_name_indexer_.GetIdFromString(first_leg_shortcode_)),
        second_leg_sec_id_(
            sec_name_indexer_.GetIdFromString(second_leg_shortcode_)),
        weak_passive_threshold_(-1), strong_passive_threshold_(-1),
        first_leg_ema_trend_(-1), second_leg_ema_trend_(-1),
        first_leg_stdev(-1), second_leg_stdev(-1),
        // HFSAT::IndicatorListener(),
        sid_to_smv_ptr_map_(sid_to_smv_ptr_map),
        //        price_change_indicator_first_(HFSAT::SimpleTrend::GetUniqueInstance(
        //            dbglogger, watch, shortcodes[0], 300, price_type)),
        //        price_change_indicator_second_(HFSAT::SimpleTrend::GetUniqueInstance(
        //            dbglogger, watch, shortcodes[1], 300, price_type)),
        //        stdev_change_indicator_first_(HFSAT::SlowStdevCalculator::GetUniqueInstance(
        //            dbglogger, watch, shortcodes[0], 300, price_type)),
        //        stdev_change_indicator_second_(HFSAT::SlowStdevCalculator::GetUniqueInstance(
        //            dbglogger, watch, shortcodes[1], 300, price_type)),
        max_time_to_execute_(max_time_to_execute),
        min_size_to_execute_(min_size_to_execute),
        execution_interval_(execution_interval),
        first_leg_open_slippage_threshold_(1000),
        second_leg_open_slippage_threshold_(1000) {
    //    price_change_indicator_first_->add_unweighted_indicator_listener(0,
    //    this);
    //    price_change_indicator_second_->add_unweighted_indicator_listener(1,
    //    this);
    //    stdev_change_indicator_first_->add_unweighted_indicator_listener(2,
    //    this);
    //    stdev_change_indicator_second_->add_unweighted_indicator_listener(3,
    //    this);
    dbglogger_ << "We have been subscribed to SMV\n";
  }

  TrendInfo UpdateTrendInfoForFirstLeg(int quantity);
  TrendInfo UpdateTrendInfoForSecondLeg(int quantity);
  void AddOrder(int order_quantity);
  void OnTimePeriodUpdate(int x);
  void OnIndicatorUpdate(const unsigned int &_indicator_index_,
                         const double &_new_value_);
  void OnIndicatorUpdate(const unsigned int &_indicator_index_,
                         const double &_new_value_decrease_,
                         const double &_new_value_nochange_,
                         const double &_new_value_increase_) {}
  // void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double&
  // _new_value_decrease_,
  //                                 const double& _new_value_nochange_, const
  //                                 double& _new_value_increase_) {}
  void HandleAggressively();
  void AggressOnLeg(TrendInfo &, int);
  void PassiveOnLeg(TrendInfo &, int, double price = -1);
  void PlaceCancel(TrendInfo &);
  void HandleSemiPassively(bool);
  void UseTrend();
  bool CheckOpenSlippage(TrendInfo &, int, int);
  void RefreshOrders(TrendInfo &, int, string, bool);

  void OnOrderExecuted(OrderResponse response_);
  void OnOrderConfirmed(OrderResponse response_);
  void OnOrderCancelled(OrderResponse response_);
};
}
