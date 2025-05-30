#include "dvctrade/ExecLogic/nse_rv_strategy_exec_logic.hpp"

namespace NSE_SIMPLEEXEC {

NseRVStrategyExecLogic::NseRVStrategyExecLogic(HFSAT::SecurityMarketView& this_smv_t,
                                               HFSAT::BaseTrader* p_base_trader_t,
                                               HFSAT::SmartOrderManager* p_smart_order_manager_t,
                                               HFSAT::DebugLogger& dbglogger_t, HFSAT::Watch& watch_t,
                                               ParamSet* t_param_, bool isLive_t_)
    : SimpleNseExecLogic(this_smv_t, p_base_trader_t, p_smart_order_manager_t, dbglogger_t, watch_t, t_param_,
                         isLive_t_),
      live_order_id_info_map_(),
      strat_orders_to_be_netted_(),
      trades_file_(),
      snapshot_file_(),
      commission_per_lot(HFSAT::BaseCommish::GetCommishPerContract(param_->instrument_, param_->yyyymmdd_)) {
  // dump saci information here for risk_notifier
  int q_id = 567821;
  dbglogger_ << "SACI: " << server_assigned_client_id_ << " QUERYID: " << q_id << " "
             << "(logging for risk monitor) \n";

  param_->max_lots_ = -1;                // not constrained by this
  param_->max_notional_ = 0;             // doesn't matter
  param_->start_time_msecs_ = 13500000;  // ensure we run throughout the day
  param_->end_time_msecs_ = 36000000;
  trade_over_ = true;  // don't calculate market metrices unless some order is received from strat
  std::ostringstream t_temp_oss;
  t_temp_oss << "/spare/local/logs/alllogs/nse_simple_execlogic_trades_" << param_->yyyymmdd_ << ".dat";
  trades_file_.open(t_temp_oss.str().c_str(), std::ofstream::app);
  if (!trades_file_.is_open()) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "UNABLE TO OPEN THE TRADES FILE TO DUMP EXECUTIONS.."
                                 << "/spare/local/logs/alllogs/nse_simple_execlogic_trades" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }
  t_temp_oss.str("");
  t_temp_oss.clear();
  t_temp_oss << "/spare/local/logs/alllogs/nse_simple_execlogic_snapshot_" << param_->yyyymmdd_;
  snapshot_file_.open(t_temp_oss.str().c_str(), std::ofstream::app);
  if (!snapshot_file_.is_open()) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "UNABLE TO OPEN THE SNAPSHOT FILE TO DUMP snapshot.." << t_temp_oss.str()
                                 << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }
}

void NseRVStrategyExecLogic::OnMarketUpdate(const unsigned int _security_id_,
                                            const HFSAT::MarketUpdateInfo& _market_update_info_) {
  if ((watch_.msecs_from_midnight() >= param_->start_time_msecs_) &&
      (watch_.msecs_from_midnight() <= param_->end_time_msecs_)) {
    if (!l1_changed_since_last_agg_) {
      if (trade_side_ == HFSAT::kTradeTypeBuy && (this_smv_.bestask_int_price() != int_price_when_agg_was_placed_ ||
                                                  this_smv_.bestask_size() != size_when_agg_was_placed_)) {
        l1_changed_since_last_agg_ = true;
      }

      if (trade_side_ == HFSAT::kTradeTypeSell && (this_smv_.bestbid_int_price() != int_price_when_agg_was_placed_ ||
                                                   this_smv_.bestbid_size() != size_when_agg_was_placed_)) {
        l1_changed_since_last_agg_ = true;
      }
    }
    // dump snapshot every 60 seconds
    if (watch_.msecs_from_midnight() - msec_at_last_snapshot_ >= 60000) {
      DumpSnapshot();
      msec_at_last_snapshot_ = watch_.msecs_from_midnight();
    }
    // get all market updates between start_time_ and end_time
    TradingLogic();
    if (curr_prg_state_ != kNoop) {
      GetExistingOrderDetails();
      PlaceOrder();
    } else {
      if (GetExistingOrderDetails()) {
        dbglogger_ << " Cancel all Orders at " << watch_.tv() << " since prg state is NoOp "
                   << " for product: " << param_->instrument_ << "\n";
        p_smart_order_manager_->CancelAllOrders();
      }
    }
  } else if (watch_.msecs_from_midnight() > param_->end_time_msecs_)
  // simulation complete. Proceed to calculate metrices
  {
    // cancel any open orders
    if (GetExistingOrderDetails()) {
      dbglogger_ << "Cancel all Orders at " << watch_.tv() << " since time exceeds endtime "
                 << " for product: " << param_->instrument_ << "\n";
      p_smart_order_manager_->CancelAllOrders();
    }
    // if time is over, and no further orders are active, print statistics
    else if (!isMetricCalculated) {
      CalculateVwapMetrices();
      isMetricCalculated = true;
    }
  }
}

bool NseRVStrategyExecLogic::CheckStabilityConstraints() {
  // if there exists some size to execute, we proceed to decide next order quantity
  if (!live_order_id_info_map_.empty()) {
    // if there exists some order to be netted, dont place any new order until we net all those
    if (!strat_orders_to_be_netted_.empty()) {
      CheckAndNetOrder();
      dbglogger_ << "NoOp set. First net all orders at " << watch_.tv() << " for product: " << param_->instrument_
                 << "\n";
      return false;
    } else {  // good to decide and place new order, set side in the param field
      dbglogger_ << "Constraint check: good to decide orders " << watch_.tv() << " for product: " << param_->instrument_
                 << "\n";
      trade_side_ = (live_order_id_info_map_.begin()->second.first > 0) ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;
      return true;
    }
  } else {
    return false;
  }
}
// received a new order request from strategy
// if this order is of same side as that in live_orders_id_map, add this to map
// else look for any live orders, if exists, cancel this order, store this strat order
// to be netted later when all orders are cancelled
void NseRVStrategyExecLogic::OnNewOrderFromStrategy(std::string order_id_, int order_lots_, double ref_px_) {
  dbglogger_ << "New order from strat at " << watch_.tv() << "  " << order_id_ << " " << order_lots_ << " " << ref_px_
             << " for product: " << param_->instrument_ << "\n";
  int order_size = order_lots_ * this_smv_.min_order_size_;

  dbglogger_ << watch_.tv() << " SLACK "
             << "Received: \t " << order_id_ << " \t" << param_->instrument_
             << (order_lots_ < 0 ? " \tSELL\t " : " \tBUY\t ") << order_size << "(" << order_lots_ << ") \t" << ref_px_
             << "\n";

  if (live_order_id_info_map_.empty()) {  // no live order id present
    live_order_id_info_map_.insert(std::make_pair(order_id_, std::make_pair(order_size, ref_px_)));
    trade_over_ = false;  //  start calculating market participation metrices
    dbglogger_ << "First live order at " << watch_.tv() << " for " << param_->instrument_ << "\n";
    return;
  } else {  // one or more live order ids exist, all correspond to same side
    if (order_size > 0 &&
        live_order_id_info_map_.begin()->second.first > 0) {  // this order is also buy,add it to live order id map
      live_order_id_info_map_.insert(std::make_pair(order_id_, std::make_pair(order_size, ref_px_)));
      dbglogger_ << "Netting Orders at " << watch_.tv() << " Adding to live order id map " << order_id_
                 << "Buy of size: " << order_size << " for product: " << param_->instrument_ << "\n";
    } else if (order_size < 0 &&
               live_order_id_info_map_.begin()->second.first < 0) {  // sell order,add to live order id map
      live_order_id_info_map_.insert(std::make_pair(order_id_, std::make_pair(order_size, ref_px_)));
      dbglogger_ << "Netting Orders at " << watch_.tv() << " Adding to live order id map " << order_id_
                 << "Sell of size: " << order_size << " for product: " << param_->instrument_ << "\n";
    } else {  // this strat order is of different side than other live order ids present
      if (GetExistingOrderDetails()) {
        dbglogger_ << " Netting Orders: OrdersCancel at " << watch_.tv()
                   << " received opposing side order from strat for product: " << param_->instrument_ << "\n";
        p_smart_order_manager_->CancelAllOrders();
      }
      dbglogger_ << "Netting Orders at " << watch_.tv() << " Adding to to_be_netted_order map " << order_id_
                 << " size: " << order_size << " for product: " << param_->instrument_ << "\n";
      strat_orders_to_be_netted_.insert(std::make_pair(order_id_, std::make_pair(order_size, ref_px_)));
    }
  }
  dbglogger_.DumpCurrentBuffer();
}

// Order netting(current side BUY,say):
// case1: id1 B 10 followed by id2 B 5 => id1 B 10 ; id2 B 15 & no order cancellation
// case2: id1 B 10 followed by id2 S -15 =>
// 	1) id1 B 10 executed 2) id2 S 10 executed 3) cancel all orders 4) id2 S 5 to be executed now
// case 3: id1 B 10 followed by id2 S 6 && (live_order_size =5 (> 10-6)) =>
// 1) id1 B 10 executed 2) cancel order 3) id2 B 4 to be executed now
void NseRVStrategyExecLogic::CheckAndNetOrder() {
  if (GetExistingOrderDetails()) {  // if some order exists, cancel it
    dbglogger_ << " CheckAndNetOrder: OrdersCancel at " << watch_.tv()
               << " won't proceed to net without cancelling for product: " << param_->instrument_ << "\n";
    p_smart_order_manager_->CancelAllOrders();
    return;
  }

  for (auto strat_order_itr = strat_orders_to_be_netted_.begin(); strat_order_itr != strat_orders_to_be_netted_.end();
       strat_order_itr++) {
    int order_size = strat_order_itr->second.first;
    int strat_order_size = order_size;
    std::string order_id_ = strat_order_itr->first;
    double strat_order_ref_px = strat_order_itr->second.second;
    bool isEntrydeleted = false;
    for (auto order_itr = live_order_id_info_map_.begin(); order_itr != live_order_id_info_map_.end();) {
      isEntrydeleted = false;
      int live_order_size = order_itr->second.first;
      std::string live_order_id = order_itr->first;
      double live_order_ref_px = order_itr->second.second;
      int r_new_order_size = strat_order_size + live_order_size;
      if (live_order_size > 0) {                   // buy live orders
        if (r_new_order_size > live_order_size) {  // this order is also buy,add it to live order id map
          live_order_id_info_map_.insert(std::make_pair(order_id_, std::make_pair(order_size, strat_order_ref_px)));
          dbglogger_ << "Netting Orders at " << watch_.tv() << " Adding to live order id map " << order_id_
                     << " Buy of size: " << order_size << " for product: " << param_->instrument_ << "\n";
        } else if (r_new_order_size <= 0) {  // side changes after netting this order
          dbglogger_ << "Netting Orders at " << watch_.tv() << " Assume execution Ord_id " << live_order_id
                     << "Buy of size: " << live_order_size << " for product: " << param_->instrument_ << "\n";
          dbglogger_ << "Netting Orders at " << watch_.tv() << " Assume execution Ord_id " << order_id_
                     << "Sell of size: " << (-1 * live_order_size) << " for product: " << param_->instrument_ << "\n";

          dbglogger_ << watch_.tv() << " SLACK "
                     << "Assume Exec: \t" << live_order_id << " \t" << param_->instrument_ << " \tBUY \t"
                     << live_order_size << " \t" << live_order_ref_px << "\n";
          dbglogger_ << watch_.tv() << " SLACK "
                     << "Assume Exec: \t " << order_id_ << " \t" << param_->instrument_ << " \tSELL \t"
                     << (-1 * live_order_size) << " \t" << strat_order_ref_px << "\n";

          trades_file_ << live_order_id << "\t" << param_->instrument_ << "\tABUY\t" << live_order_size << "\t"
                       << live_order_ref_px << " \t" << (commission_per_lot * live_order_size * live_order_ref_px)
                       << std::endl;
          trades_file_ << order_id_ << "\t" << param_->instrument_ << "\tASELL\t" << (-1 * live_order_size) << "\t"
                       << strat_order_ref_px << " \t" << (commission_per_lot * live_order_size * strat_order_ref_px)
                       << std::endl;

          live_order_id_info_map_.erase(order_itr++);  // delete this entry from order map
          isEntrydeleted = true;
          strat_order_size = r_new_order_size;  // adjust the size remaining for this new order after netting
        } else {                                // side will not change after netting this
          dbglogger_ << "Netting Orders at " << watch_.tv() << " Assume execution Ord_id " << live_order_id
                     << "Buy of size: " << order_size << " for product: " << param_->instrument_ << "\n";
          dbglogger_ << "Netting Orders at " << watch_.tv() << " Assume execution Ord_id " << order_id_
                     << "Sell of size: " << order_size << " for product: " << param_->instrument_ << "\n";

          dbglogger_ << watch_.tv() << " SLACK "
                     << "Assume Exec: \t " << live_order_id << " \t" << param_->instrument_ << " \tBUY\t " << order_size
                     << " " << live_order_ref_px << "\n";
          dbglogger_ << watch_.tv() << " SLACK "
                     << "Assume Exec: \t " << order_id_ << " \t" << param_->instrument_ << " \tSELL\t "
                     << (-1 * order_size) << " \t" << strat_order_ref_px << "\n";

          trades_file_ << live_order_id << " \t" << param_->instrument_ << " \tABUY\t " << order_size << " "
                       << live_order_ref_px << " \t" << (commission_per_lot * order_size * live_order_ref_px)
                       << std::endl;
          trades_file_ << order_id_ << " \t" << param_->instrument_ << " \tASELL\t " << (-1 * order_size) << " \t"
                       << strat_order_ref_px << " \t" << (commission_per_lot * order_size * strat_order_ref_px)
                       << std::endl;
          // adjust the order_size remaining for this live order
          order_itr->second.first = r_new_order_size;
          strat_order_size = 0;  // strat_order size is completely netted
        }
      } else {                                     // sell live orders
        if (r_new_order_size < live_order_size) {  // this order is also sell,add it to live order id map
          live_order_id_info_map_.insert(std::make_pair(order_id_, std::make_pair(order_size, strat_order_ref_px)));
          dbglogger_ << "Netting Orders at " << watch_.tv() << " Adding to live order id map " << order_id_
                     << "Sell of size: " << order_size << " for product: " << param_->instrument_ << "\n";
        } else if (r_new_order_size >= 0) {  // side changes after netting this order

          dbglogger_ << "Netting Orders at " << watch_.tv() << " Assume execution Ord_id " << live_order_id
                     << "Sell of size: " << live_order_size << " for product: " << param_->instrument_ << "\n";
          dbglogger_ << "Netting Orders at " << watch_.tv() << " Assume execution Ord_id " << order_id_
                     << "Buy of size: " << (-1 * live_order_size) << " for product: " << param_->instrument_ << "\n";

          dbglogger_ << watch_.tv() << " SLACK "
                     << "Assume Exec: \t " << live_order_id << " \t" << param_->instrument_ << " \tSELL\t "
                     << live_order_size << " \t" << live_order_ref_px << "\n";
          dbglogger_ << watch_.tv() << " SLACK "
                     << "Assume Exec: \t " << order_id_ << " \t" << param_->instrument_ << " \tBUY\t "
                     << (-1 * live_order_size) << " \t" << strat_order_ref_px << "\n";

          trades_file_ << live_order_id << " \t" << param_->instrument_ << " \tASELL\t " << live_order_size << " \t"
                       << live_order_ref_px << " \t" << (commission_per_lot * -1 * live_order_size * live_order_ref_px)
                       << std::endl;
          trades_file_ << order_id_ << " \t" << param_->instrument_ << " \tABUY\t " << (-1 * live_order_size) << " \t"
                       << strat_order_ref_px << " \t"
                       << (commission_per_lot * -1 * live_order_size * strat_order_ref_px) << std::endl;

          live_order_id_info_map_.erase(order_itr++);  // delete this entry from order map
          isEntrydeleted = true;
          strat_order_size = r_new_order_size;  // adjust the size remaining for this new order after netting
        } else {                                // side will not change after netting this
          dbglogger_ << "Netting Orders at " << watch_.tv() << " Assume execution Ord_id " << live_order_id
                     << "Sell of size: " << order_size << " for product: " << param_->instrument_ << "\n";
          dbglogger_ << "Netting Orders at " << watch_.tv() << " Assume execution Ord_id " << order_id_
                     << "Buy of size: " << (-1 * order_size) << " for product: " << param_->instrument_ << "\n";

          dbglogger_ << watch_.tv() << " SLACK "
                     << "Assume Exec: \t " << live_order_id << " \t" << param_->instrument_ << " \tSELL\t "
                     << order_size << " \t" << live_order_ref_px << "\n";
          dbglogger_ << watch_.tv() << " SLACK "
                     << "Assume Exec: \t " << order_id_ << " \t" << param_->instrument_ << " \tBUY\t "
                     << (-1 * order_size) << " \t" << strat_order_ref_px << "\n";

          trades_file_ << live_order_id << " \t" << param_->instrument_ << " \tASELL\t " << order_size << " \t"
                       << live_order_ref_px << " \t" << (commission_per_lot * -1 * order_size * live_order_ref_px)
                       << std::endl;
          trades_file_ << order_id_ << " \t" << param_->instrument_ << " \tABUY\t " << (-1 * order_size) << " \t"
                       << strat_order_ref_px << " \t" << (commission_per_lot * -1 * order_size * live_order_ref_px)
                       << std::endl;

          // adjust the order_size remaining for this live order
          order_itr->second.first = r_new_order_size;
          strat_order_size = 0;  // strat_order size is completely netted
        }
      }
      if (strat_order_size == 0)  // strat_order size is completely netted
        break;                    // we need not net remaining live order ids
      trades_file_.flush();
      if (!isEntrydeleted) ++order_itr;  // when entry deleted from live order map, itr is taken care of
    }
    if (strat_order_size != 0) {
      // insert this strat order remaining size to live order id info map
      live_order_id_info_map_.insert(std::make_pair(order_id_, std::make_pair(strat_order_size, strat_order_ref_px)));
    }
  }
  if (live_order_id_info_map_.empty()) {  // no live order id present, everything is netted
    trade_over_ = true;
    dbglogger_ << "Netted all orders from strategy at : " << watch_.tv() << " for product: " << param_->instrument_
               << " Printing stats: \n";
    CalculateVwapMetrices();       // statistics
    ResetParticipationMetrices();  // reset market metrices
  }
  // all pending strat orders netted, clear the map
  strat_orders_to_be_netted_.clear();
}

// We are limiting the max size_to_execute by sum of sizes for all live order ids in TradingLogic()
// On executions, we loop through all the live order ids and mark all those ids for which we received executions
void NseRVStrategyExecLogic::RecordOrderExecsForLiveOrderIds(const HFSAT::TradeType_t _buysell_, int size_exec_,
                                                             double trade_px_) {
  // handling case when OnExec was not recorded but order got deleted from Ordermaps
  // i.e. GetExistingOrderDetails() returned false, example cxlrej received before exec
  // we may have netted the order wrongly in that case
  HFSAT::TradeType_t expected_side =
      (live_order_id_info_map_.begin()->second.first > 0) ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;
  if (expected_side != _buysell_) {
    dbglogger_ << "Fatal Error: OnExec: Received opposite side in exec than expected. Some incorrect order netting "
                  "for product: "
               << param_->instrument_ << "\n";
    std::cerr << "Fatal Error: OnExec: Received opposite side in exec than expected. Some incorrect order netting "
                 "for product: "
              << param_->instrument_ << "\n";
    dbglogger_.DumpCurrentBuffer();
    // TODO: exit here??
  }
  if (GetTotalSizeForAllLiveOrders() < size_exec_) {
    dbglogger_ << "Fatal Error: OnExec Received greater order exec size than expected for product: "
               << param_->instrument_ << "\n";
    std::cerr << "Fatal Error: OnExec Received greater order exec size than expected for product: "
              << param_->instrument_ << "\n";
    dbglogger_.DumpCurrentBuffer();
    // TODO: exit here??
  }

  bool isEntrydeleted = false;

  for (auto order_itr = live_order_id_info_map_.begin(); order_itr != live_order_id_info_map_.end();) {
    int live_order_size = order_itr->second.first;
    isEntrydeleted = false;
    if (live_order_size > 0) {              // buy live orders
      if (size_exec_ >= live_order_size) {  // this order id is executed completely, delete from map
        dbglogger_ << "Order Executed: " << watch_.tv() << " order_id: " << order_itr->first
                   << " buy size: " << live_order_size << " for product: " << param_->instrument_ << "\n";

        dbglogger_ << watch_.tv() << " SLACK "
                   << "Execution: \t" << order_itr->first << " \t" << param_->instrument_ << " \tBUY\t "
                   << live_order_size << " \t" << trade_px_ << "\n";

        trades_file_ << order_itr->first << "\t" << param_->instrument_ << "\tBUY\t" << live_order_size << "\t"
                     << trade_px_ << " \t" << (commission_per_lot * live_order_size * trade_px_) << std::endl;

        size_exec_ -= live_order_size;
        live_order_id_info_map_.erase(order_itr++);
        isEntrydeleted = true;
        if (size_exec_ == 0) break;  // complete execution, recorded all orderIDs for this execution
      } else {
        dbglogger_ << "Order Executed: " << watch_.tv() << " order_id: " << order_itr->first
                   << " buy size: " << size_exec_ << " for product: " << param_->instrument_ << "\n";

        dbglogger_ << watch_.tv() << " SLACK "
                   << "Execution: \t " << order_itr->first << " \t" << param_->instrument_ << " \tBUY\t " << size_exec_
                   << " \t" << trade_px_ << "\n";

        trades_file_ << order_itr->first << "\t" << param_->instrument_ << "\tBUY\t" << size_exec_ << "\t" << trade_px_
                     << " \t" << (commission_per_lot * size_exec_ * trade_px_) << std::endl;

        order_itr->second.first = live_order_size - size_exec_;
        size_exec_ = 0;
        break;
      }
    } else {
      if (size_exec_ >= (-1 * live_order_size)) {  // this order id is executed completely, delete from map
        dbglogger_ << "Order Executed: " << watch_.tv() << " order_id: " << order_itr->first
                   << " sell size: " << live_order_size << " for product: " << param_->instrument_ << "\n";

        dbglogger_ << watch_.tv() << " SLACK "
                   << "Execution: \t " << order_itr->first << " \t" << param_->instrument_ << " \tSELL\t "
                   << live_order_size << " \t" << trade_px_ << "\n";

        trades_file_ << order_itr->first << "\t" << param_->instrument_ << "\tSELL\t" << live_order_size << "\t"
                     << trade_px_ << " \t" << (commission_per_lot * -1 * live_order_size * trade_px_) << std::endl;

        size_exec_ -= (-1 * live_order_size);
        live_order_id_info_map_.erase(order_itr++);
        isEntrydeleted = true;
        if (size_exec_ == 0) break;  // complete execution, recorded all orderIDs for this execution
      } else {
        dbglogger_ << "Order Executed: " << watch_.tv() << " order_id: " << order_itr->first
                   << " sell size: " << size_exec_ << " for product: " << param_->instrument_ << "\n";

        dbglogger_ << watch_.tv() << " SLACK "
                   << "Execution: \t " << order_itr->first << " \t" << param_->instrument_ << " \tSELL\t " << size_exec_
                   << " \t" << trade_px_ << "\n";
        trades_file_ << order_itr->first << "\t" << param_->instrument_ << "\tSELL\t" << size_exec_ << "\t" << trade_px_
                     << " \t" << (commission_per_lot * -1 * size_exec_ * trade_px_) << std::endl;

        order_itr->second.first = live_order_size + size_exec_;
        size_exec_ = 0;
        break;
      }
    }
    if (!isEntrydeleted) ++order_itr;  // when entry deleted from live order map, itr is taken care of
  }
  // we may have executed all the live order ids, check and mark trade completed
  if (live_order_id_info_map_.empty()) {  // no live order id present, everything is executed
    trade_over_ = true;
    dbglogger_ << "Executed all orders from strategy at : " << watch_.tv()
               << " Printing stats: for product: " << param_->instrument_ << "\n";
    CalculateVwapMetrices();       // statistics
    ResetParticipationMetrices();  // reset market metrices
  }
  if (size_exec_ != 0) {
    dbglogger_
        << "Fatal Error: RecordOrderExecsForLiveOrderIds: Received greater order exec size than expected for product: "
        << param_->instrument_ << "\n";
    std::cerr
        << "Fatal Error: RecordOrderExecsForLiveOrderIds: Received greater order exec size than expected for product: "
        << param_->instrument_ << "\n";
    dbglogger_.DumpCurrentBuffer();
  }
  trades_file_.flush();
}
int NseRVStrategyExecLogic::get_remaining_order_lots_to_be_executed(double t_price_) {
  return (live_order_id_info_map_.empty()) ? 0 : GetTotalSizeForAllLiveOrders();
}

int NseRVStrategyExecLogic::GetTotalSizeForAllLiveOrders() {
  int total_size = 0;
  for (auto order_itr = live_order_id_info_map_.begin(); order_itr != live_order_id_info_map_.end(); ++order_itr) {
    total_size += order_itr->second.first;
  }
  return std::abs(total_size);
}
void NseRVStrategyExecLogic::TradingLogic() {
  curr_prg_state_ = kNoop;

  if (!CheckStabilityConstraints()) return;  // not good to place any order

  // Conditions under which we want to place agg orders - either if (a) algotype is Aggressive; or
  //( b ) algotype is PassAndAgg and our mkt share is below participation_factor_ - tolerance
  if (param_->exec_algo_ == kAggOnly ||
      (param_->exec_algo_ == kPassAndAgg &&
       (total_notional_traded_ / market_notional_ <
        param_->market_participation_factor_ - param_->market_participation_factor_tolerance_))) {
    // don't place a new order immediately after an old one or without seeing an L1 change
    if (GetExistingOrderDetails() == false &&
        (!l1_changed_since_last_agg_ ||
         (watch_.msecs_from_midnight() - msecs_at_last_aggress_ < param_->trade_cooloff_interval_))) {
      dbglogger_ << "NoOp set due to agg cooloff criteria at " << watch_.tv() << " for product: " << param_->instrument_
                 << "\n";
      return;
    }
    // stability checks have passed and vwap is low
    int min_lotsize_ = this_smv_.min_order_size_;

    // ignore diff between bid and ask for this check.
    // if executing an order will not have us cross threshold
    double t_price_ = (trade_side_ == HFSAT::kTradeTypeBuy ? this_smv_.bestask_price() : this_smv_.bestbid_price());
    if ((min_lotsize_ * t_price_ + total_notional_traded_) / market_notional_ <
        param_->market_participation_factor_ + param_->market_participation_factor_tolerance_) {
      if (this_smv_.bestask_int_price() - this_smv_.bestbid_int_price() <
          param_->avg_sprd_ * param_->agg_sprd_factor_) {
        int t_max_lots_based_on_mkt_ptp_ = get_max_order_lots_based_on_mkt_participation(t_price_);
        int t_max_lots_per_order_ = param_->max_lots_per_trade_;
        int remaining_order_lots_to_be_executed = get_remaining_order_lots_to_be_executed(t_price_);
        size_to_execute_ =
            std::min(min_lotsize_ * std::min(t_max_lots_based_on_mkt_ptp_, t_max_lots_per_order_),
                     (trade_side_ == HFSAT::kTradeTypeBuy ? this_smv_.bestask_size() : this_smv_.bestbid_size()));
        size_to_execute_ = std::min(size_to_execute_, min_lotsize_ * remaining_order_lots_to_be_executed);
        curr_prg_state_ = kAggressive;
      } else
        dbglogger_ << "NoOp set due to agg spread factor at " << watch_.tv() << " for product: " << param_->instrument_
                   << "\n";
    }
  }
  // Conditions under which we place a passive order - algotype is PassAndAgg and PassOnly
  else if (param_->exec_algo_ == kPassAndAgg || param_->exec_algo_ == kPassOnly) {
    int min_lotsize_ = this_smv_.min_order_size_;
    double t_price_ = (trade_side_ == HFSAT::kTradeTypeBuy ? this_smv_.bestbid_price() : this_smv_.bestask_price());
    if ((min_lotsize_ * t_price_ + total_notional_traded_) / market_notional_ <
        param_->market_participation_factor_ + param_->market_participation_factor_tolerance_) {
      int t_max_lots_based_on_mkt_ptp_ = get_max_order_lots_based_on_mkt_participation(t_price_);
      int t_max_lots_per_order_ = param_->max_lots_per_trade_;
      int remaining_order_lots_to_be_executed = get_remaining_order_lots_to_be_executed(t_price_);

      size_to_execute_ = min_lotsize_ * std::min(std::min(t_max_lots_based_on_mkt_ptp_, t_max_lots_per_order_),
                                                 remaining_order_lots_to_be_executed);
      curr_prg_state_ = kPassive;
      dbglogger_ << "tradinglogic: decided to place order of size: " << size_to_execute_ << " " << watch_.tv() << " "
                 << " for product: " << param_->instrument_ << "\n";
    } else
      dbglogger_ << "NoOp set due to high potential participation factor "
                 << (min_lotsize_ * t_price_ + total_notional_traded_) / market_notional_ * 100.0 << " at "
                 << watch_.tv() << " for product: " << param_->instrument_ << "\n";
  }
}
void NseRVStrategyExecLogic::DumpSnapshot() {
  snapshot_file_ << "Snapshot at " << watch_.tv() << " for " << param_->instrument_ << std::endl;
  // print live_order_id_info_map_
  snapshot_file_ << "LiveOrders IDs: " << std::endl;
  for (auto& live_order_itr : live_order_id_info_map_) {
    snapshot_file_ << live_order_itr.first << "\t" << live_order_itr.second.first << " for " << param_->instrument_
                   << std::endl;
  }

  snapshot_file_ << "Orders pending netting: " << std::endl;
  for (auto& strat_order_itr : strat_orders_to_be_netted_) {
    snapshot_file_ << strat_order_itr.first << "\t" << strat_order_itr.second.first << " for " << param_->instrument_
                   << std::endl;
  }
  snapshot_file_ << "total_notional_traded_: " << total_notional_traded_ << " for " << param_->instrument_ << std::endl;
  snapshot_file_ << "total_volume_traded_: " << total_volume_traded_ << " for " << param_->instrument_ << std::endl;
  snapshot_file_ << "market_notional_: " << market_notional_ << " for " << param_->instrument_ << std::endl;
  snapshot_file_ << "market_volume_: " << market_volume_ << " for " << param_->instrument_ << std::endl;
  snapshot_file_ << "curr_prg_state_: " << curr_prg_state_ << " for " << param_->instrument_ << std::endl;
  snapshot_file_ << "trading side: " << trade_side_ << " for " << param_->instrument_ << std::endl;
  snapshot_file_ << "sizetoexecute: " << size_to_execute_ << " for " << param_->instrument_ << std::endl;
  snapshot_file_ << "===============================================================================" << std::endl;
}
}
