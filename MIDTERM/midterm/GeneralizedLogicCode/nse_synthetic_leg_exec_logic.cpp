
#include "midterm/GeneralizedLogic/nse_synthetic_leg_exec_logic.hpp"


namespace NSE_SIMPLEEXEC {

NseSyntheticLegExecLogic::NseSyntheticLegExecLogic(
    HFSAT::SecurityMarketView &this_smv_t, HFSAT::BaseTrader *p_base_trader_t,
    HFSAT::SmartOrderManager *p_smart_order_manager_t,
	BaseModifyExecLogic &modify_exec_logic_t,
    HFSAT::DebugLogger &dbglogger_t, HFSAT::Watch &watch_t, ParamSet *t_param_,
    bool isLive_t_, std::string shortcode_t, HFSAT::PriceType_t price_type_t,
    std::string strategy_type, int qid)
    : SimpleNseExecLogic(this_smv_t, p_base_trader_t, p_smart_order_manager_t, modify_exec_logic_t,
                         dbglogger_t, watch_t, t_param_, isLive_t_,
                         shortcode_t),
      live_order_id_info_map_(), strat_orders_to_be_netted_(),
      strat_orders_to_be_removed_(), trades_file_(), snapshot_file_(),
      commission_per_lot(HFSAT::BaseCommish::GetCommishPerContract(
          shortcode_t, param_->yyyymmdd_)),
      last_traded_px_(-1), price_change_indicator_(nullptr),
      stdev_change_indicator_(nullptr), simple_trend_(-1), stdev_(-1),
      is_passive_(true), strat_type_(strategy_type) {
  // dump saci information here for risk_notifier
  dbglogger_ << "SACI: " << server_assigned_client_id_ << " QUERYID: " << qid
             << " "
             << "(logging for risk monitor) \n";

  param_->max_lots_ = -1;               // not constrained by this
  param_->max_notional_ = 0;            // doesn't matter
  param_->start_time_msecs_ = 13500000; // ensure we run throughout the day
  param_->end_time_msecs_ = 36000000;
  trade_over_ = true; // don't calculate market metrices unless some order is
                      // received from strat

  //Finding the exchange symbol for this shortcode
  int date = watch_t.YYYYMMDD();
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(date);
  exchange_symbol = std::string(HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_t));

  std::string trade_filename = LOG_DIR + strategy_type + TRADEFILE_PATH;

  std::ostringstream t_temp_oss;
  t_temp_oss << trade_filename << param_->yyyymmdd_ << ".dat";
  trades_file_.open(t_temp_oss.str().c_str(), std::ofstream::app);
  if (!trades_file_.is_open()) {
    DBGLOG_CLASS_FUNC_LINE_FATAL
        << "UNABLE TO OPEN THE TRADES FILE TO DUMP EXECUTIONS.."
        << trade_filename << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }
  t_temp_oss.str("");
  t_temp_oss.clear();
  t_temp_oss << LOG_DIR + strategy_type + SNAPSHOT_PATH << param_->yyyymmdd_;
  snapshot_file_.open(t_temp_oss.str().c_str(), std::ofstream::app);
  if (!snapshot_file_.is_open()) {
    DBGLOG_CLASS_FUNC_LINE_FATAL
        << "UNABLE TO OPEN THE SNAPSHOT FILE TO DUMP snapshot.."
        << t_temp_oss.str() << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }
  // Keeping this so that we can add support for them later if we require
  /*
  if (param_->ema_interval_ <= 0) {
    price_change_indicator_ =
        HFSAT::SimpleTrend::GetUniqueInstance(dbglogger_t, watch_t, this_smv_t,
  param_->ema_interval_, price_type_t);
    price_change_indicator_->add_unweighted_indicator_listener(0, this);
    dbglogger_ << "Added Px Change Listener for shortcode -> " << shortcode_ <<
  DBGLOG_ENDL_FLUSH;
  }
  if (param_->stdev_change_interval_ <= 0) {
    stdev_change_indicator_ = HFSAT::SlowStdevCalculator::GetUniqueInstance(
        dbglogger_t, watch_t, this_smv_t, param_->stdev_change_interval_,
  price_type_t);
    stdev_change_indicator_->add_unweighted_indicator_listener(1, this);
    dbglogger_ << "Added Stdev Listener for shortcode -> " << shortcode_ <<
  DBGLOG_ENDL_FLUSH;
  }
  */
}

void NseSyntheticLegExecLogic::OnMarketUpdate(
    const unsigned int _security_id_,
    const HFSAT::MarketUpdateInfo &_market_update_info_) {



  if (HFSAT::IsItSimulationServer() ||
      ((watch_.msecs_from_midnight() >= param_->start_time_msecs_) &&
       (watch_.msecs_from_midnight() <= param_->end_time_msecs_))) {
    if (!l1_changed_since_last_agg_) {
      if (trade_side_ == HFSAT::kTradeTypeBuy &&
          (this_smv_.bestask_int_price() != int_price_when_agg_was_placed_ ||
           this_smv_.bestask_size() != size_when_agg_was_placed_)) {
        l1_changed_since_last_agg_ = true;
      }

      if (trade_side_ == HFSAT::kTradeTypeSell &&
          (this_smv_.bestbid_int_price() != int_price_when_agg_was_placed_ ||
           this_smv_.bestbid_size() != size_when_agg_was_placed_)) {
        l1_changed_since_last_agg_ = true;
      }
    }
    // dump snapshot every 60 seconds
    if (watch_.msecs_from_midnight() - msec_at_last_snapshot_ >= 10000) {
      DumpSnapshot();
      UpdateOrsInfo();
      msec_at_last_snapshot_ = watch_.msecs_from_midnight();
    }
    // get all market updates between start_time_ and end_time
    TradingLogic();
    if (curr_prg_state_ != kNoop) {
      GetExistingOrderDetails();
      PlaceOrder();
      UpdateOrsInfo();

    } else {
      if (GetExistingOrderDetails()) {
        dbglogger_ << " Cancel all Orders at " << watch_.tv()
                   << " since prg state is NoOp "
                   << " for product: " << shortcode_ << "\n";
        p_smart_order_manager_->CancelAllOrders();
      }
    }
  } else if (watch_.msecs_from_midnight() > param_->end_time_msecs_)
  // simulation complete. Proceed to calculate metrices
  {
    // cancel any open orders
    if (GetExistingOrderDetails()) {
      dbglogger_ << "Cancel all Orders at " << watch_.tv()
                 << " since time exceeds endtime "
                 << " for product: " << shortcode_ << "\n";
      p_smart_order_manager_->CancelAllOrders();
    }
    // if time is over, and no further orders are active, print statistics
    else if (!isMetricCalculated) {
      CalculateVwapMetrices();
      isMetricCalculated = true;
    }
  }
}

void NseSyntheticLegExecLogic::UpdateOrsInfo(){
	std::string order_id;
	if(!live_order_id_info_map_.empty()) {
		order_id = live_order_id_info_map_.begin()->first;
	}
	else {
		//dbglogger_<<"Live order map empty for shortcode : "<<shortcode_<<"\n";
		return;
	}

	//Assumes that there will always be one open order in the market at the best level
	if(trade_side_ == HFSAT::kTradeTypeBuy){
		HFSAT::BaseOrder * order = p_smart_order_manager_->GetTopBidOrder();
		if(order != nullptr) {
			const HFSAT::ORRType_t order_status = order->order_status();
			int saos = order->server_assigned_order_sequence_;
			int caos = order->client_assigned_order_sequence_;
			int int_px = order->int_price_;
			ORSInfo  ors_info;
			ors_info.CAOS = caos;
			ors_info.SAOS = saos;
			ors_info.int_px = int_px;
			double price = order->price_;
			order_id_to_ors_info_map[std::make_pair(order_id,shortcode_)] = ors_info;
			dbglogger_ << "Updating  ORS Info for Order Id: "<<order_id<<" Shortcode : "<<shortcode_<< " SAOS: "<<saos<<" CAOS : "<<caos<<" Int Px : "<<int_px<<" Trade side : "<<trade_side_<<" Price : "<<price<<" Order Status : "<<std::string(HFSAT::ToString(order_status))<<" Exchange Symbol : "<<exchange_symbol<<"\n";
		}
		else {
			dbglogger_ <<"Order empty for shortcode in smart order manager : "<<shortcode_<<"\n";
		}
	}
	else {
		HFSAT::BaseOrder * order = p_smart_order_manager_->GetTopAskOrder();
		if(order != nullptr) {
			const HFSAT::ORRType_t order_status = order->order_status();
			int saos = order->server_assigned_order_sequence_;
			int caos = order->client_assigned_order_sequence_;
			int int_px = order->int_price_;
			ORSInfo  ors_info;
			ors_info.CAOS = caos;
			ors_info.SAOS = saos;
			ors_info.int_px = int_px;
			double price = order->price_;
			order_id_to_ors_info_map[std::make_pair(order_id,shortcode_)] = ors_info;
			dbglogger_ << "Updating  ORS Info for Order Id: "<<order_id<<" Shortcode : "<<shortcode_<< " SAOS: "<<saos<<" CAOS : "<<caos<<" Int Px : "<<int_px<<" Trade side : "<<trade_side_<<" Price : "<<price<<" Order Status : "<<std::string(HFSAT::ToString(order_status))<<" Exchange Symbol : "<<exchange_symbol<<"\n";
		}
		else {
			dbglogger_ <<"Order empty for shortcode in smart order manager : "<<shortcode_<<"\n";
		}

	}
}

void NseSyntheticLegExecLogic::OnTradePrint(
    const unsigned int _security_id_,
    const HFSAT::TradePrintInfo &_trade_print_info_,
    const HFSAT::MarketUpdateInfo &_market_update_info_) {
  OnMarketUpdate(_security_id_, _market_update_info_);
  last_traded_px_ = _trade_print_info_.trade_price_;
}

bool NseSyntheticLegExecLogic::CheckStabilityConstraints() {
  // if there exists some size to execute, we proceed to decide next order
  // quantity
  if (!live_order_id_info_map_.empty()) {
    // if there exists some order to be netted, dont place any new order until
    // we net all those
    if (!strat_orders_to_be_netted_.empty() ||
        !strat_orders_to_be_removed_.empty()) {
      CheckAndNetOrder();
      return false;
    } else { // good to decide and place new order, set side in the param field
      trade_side_ = (live_order_id_info_map_.begin()->second.first > 0)
                        ? HFSAT::kTradeTypeBuy
                        : HFSAT::kTradeTypeSell;
      return true;
    }
  } else {
    return true;
  }
}
// received a new order request from strategy
// if this order is of same side as that in live_orders_id_map, add this to map
// else look for any live orders, if exists, cancel this order, store this strat
// order
// to be netted later when all orders are cancelled
bool NseSyntheticLegExecLogic::OnNewOrderFromStrategy(std::string order_id_,
                                                      int order_lots_) {

  // Add risk checks here
  // Risk manager will not let the order be executed if it returns false
  NseRiskManager &risk_manager_ =
      NseRiskManager::GetUniqueInstance(watch_, dbglogger_, strat_type_);
  bool allowed_to_trade_ = risk_manager_.AddEntry(shortcode_, order_lots_);
  if (!allowed_to_trade_) {
    dbglogger_ << "ALERT -> Not allowed to take any more trades on "
               << shortcode_ <<" Order id: "<<order_id_<< '\n';
    return false;
  }

  double ref_px_ = (this_smv_.bestbid_price() + this_smv_.bestask_price()) / 2;

  dbglogger_ << "New order from strat at " << watch_.tv() << "  " << order_id_
             << " " << order_lots_ << " " << ref_px_
             << " for product: " << shortcode_ << "\n";
  int order_size = order_lots_ * this_smv_.min_order_size_;
  if (live_order_id_info_map_.empty()) { // no live order id present
    live_order_id_info_map_.insert(
        std::make_pair(order_id_, std::make_pair(order_size, ref_px_)));
    trade_over_ = false; //  start calculating market participation metrices
    dbglogger_ << "First live order at " << watch_.tv() << " for " << shortcode_
               << "\n";
    return true;
  } else { // one or more live order ids exist, all correspond to same side
    if (order_size > 0 &&
        live_order_id_info_map_.begin()->second.first >
            0) { // this order is also buy,add it to live order id map
      live_order_id_info_map_.insert(
          std::make_pair(order_id_, std::make_pair(order_size, ref_px_)));
      dbglogger_ << "Netting Orders at " << watch_.tv()
                 << " Adding to live order id map " << order_id_
                 << "Buy of size: " << order_size
                 << " for product: " << shortcode_ << "\n";
    } else if (order_size < 0 &&
               live_order_id_info_map_.begin()->second.first <
                   0) { // sell order,add to live order id map
      live_order_id_info_map_.insert(
          std::make_pair(order_id_, std::make_pair(order_size, ref_px_)));

      dbglogger_ << "Netting Orders at " << watch_.tv()
                 << " Adding to live order id map " << order_id_
                 << "Sell of size: " << order_size
                 << " for product: " << shortcode_ << "\n";
    } else { // this strat order is of different side than other live order ids
             // present
      if (GetExistingOrderDetails()) {
        dbglogger_ << " Netting Orders: OrdersCancel at " << watch_.tv()
                   << " received opposing side order from strat for product: "
                   << shortcode_ << "\n";
        p_smart_order_manager_->CancelAllOrders();
      }
      dbglogger_ << "Netting Orders at " << watch_.tv()
                 << " Adding to to_be_netted_order map " << order_id_
                 << " size: " << order_size << " for product: " << shortcode_
                 << "\n";
      strat_orders_to_be_netted_.insert(
          std::make_pair(order_id_, std::make_pair(order_size, ref_px_)));
    }
  }
  return true;
}

// action can be BUY/SELL/ABUY/ASELL
// Also inform GeneralizedExec that the order execution has taken place
void NseSyntheticLegExecLogic::WriteToTradesFile(std::string &order_id,
                                                 std::string &shortcode,
                                                 std::string action, int size,
                                                 double price,
                                                 std::string exec_type) {
  std::ostringstream temp_watch_oss_;
  temp_watch_oss_ << std::fixed << std::setprecision(2) << watch_.tv().tv_sec;
  std::string temp_watch_ = temp_watch_oss_.str();

  trades_file_ << order_id << '\t' << shortcode << '\t' << action << '\t'
               << size << '\t' << price << '\t'
               << std::abs(commission_per_lot * size * price) << '\t'
               << temp_watch_ << '\t' << exec_type << std::endl;

  NseExecutionListenerManager::GetUniqueInstance().NotifyNseExecutionListener(
      order_id, 1, price);
}

// Order netting(current side BUY,say):
// case1: id1 B 10 followed by id2 B 5 => id1 B 10 ; id2 B 15 & no order
// cancellation
// case2: id1 B 10 followed by id2 S -15 =>
// 	1) id1 B 10 executed 2) id2 S 10 executed 3) cancel all orders 4) id2 S
// 5 to be executed now
// case 3: id1 B 10 followed by id2 S 6 && (live_order_size =5 (> 10-6)) =>
// 1) id1 B 10 executed 2) cancel order 3) id2 B 4 to be executed now
void NseSyntheticLegExecLogic::CheckAndNetOrder() {
  if (GetExistingOrderDetails()) { // if some order exists, cancel it
    dbglogger_ << " CheckAndNetOrder: OrdersCancel at " << watch_.tv()
               << " won't proceed to net without cancelling for product: "
               << shortcode_ << " SAOS:" << server_assigned_client_id_ << "\n";
    p_smart_order_manager_->CancelAllOrders();
    return;
  }

  std::ostringstream temp_watch_oss_;
  temp_watch_oss_ << std::fixed << std::setprecision(2) << watch_.tv().tv_sec;
  std::string temp_watch_ = temp_watch_oss_.str();

  // Before proceding check if there are any orders to be removed/cancelled
  for (auto i : strat_orders_to_be_removed_) {

    auto j = strat_orders_to_be_netted_.begin();
    while (j != strat_orders_to_be_netted_.end()) {
      if (j->first.find(i) != std::string::npos) {
        strat_orders_to_be_netted_.erase(j++);
      } else {
        j++;
      }
    }

    j = live_order_id_info_map_.begin();
    while (j != live_order_id_info_map_.end()) {
      if (j->first.find(i) != std::string::npos) {
        live_order_id_info_map_.erase(j++);
      } else {
        j++;
      }
    }
  }
  // We have removed the reqd order from whichever map it may be in
  strat_orders_to_be_removed_.clear();

  for (auto strat_order_itr = strat_orders_to_be_netted_.begin();
       strat_order_itr != strat_orders_to_be_netted_.end(); strat_order_itr++) {
    int order_size = strat_order_itr->second.first;
    int strat_order_size = order_size;
    std::string order_id_ = strat_order_itr->first;
    double strat_order_ref_px = strat_order_itr->second.second;
    bool isEntrydeleted = false;
    for (auto order_itr = live_order_id_info_map_.begin();
         order_itr != live_order_id_info_map_.end();) {
      isEntrydeleted = false;
      int live_order_size = order_itr->second.first;
      std::string live_order_id = order_itr->first;
      double live_order_ref_px = order_itr->second.second;
      int r_new_order_size = strat_order_size + live_order_size;
      if (live_order_size > 0) {                  // buy live orders
        if (r_new_order_size > live_order_size) { // this order is also buy,add
                                                  // it to live order id map
          live_order_id_info_map_.insert(std::make_pair(
              order_id_, std::make_pair(order_size, strat_order_ref_px)));

          dbglogger_ << "Netting Orders at " << watch_.tv()
                     << " Adding to live order id map " << order_id_
                     << " Buy of size: " << order_size
                     << " for product: " << shortcode_ << "\n";
        } else if (r_new_order_size <=
                   0) { // side changes after netting this order
          dbglogger_ << "Netting Orders at " << watch_.tv()
                     << " Assume execution Ord_id " << live_order_id
                     << "Buy of size: " << live_order_size
                     << " for product: " << shortcode_ << "\n";
          dbglogger_ << "Netting Orders at " << watch_.tv()
                     << " Assume execution Ord_id " << order_id_
                     << "Sell of size: " << (-1 * live_order_size)
                     << " for product: " << shortcode_ << "\n";
          WriteToTradesFile(live_order_id, shortcode_, "ABUY", live_order_size,
                            live_order_ref_px, "PASS");
          WriteToTradesFile(order_id_, shortcode_, "ASELL",
                            -1 * live_order_size, strat_order_ref_px, "PASS");
          live_order_id_info_map_.erase(
              order_itr++); // delete this entry from order map
          isEntrydeleted = true;
          strat_order_size = r_new_order_size; // adjust the size remaining for
                                               // this new order after netting
        } else { // side will not change after netting this
          dbglogger_ << "Netting Orders at " << watch_.tv()
                     << " Assume execution Ord_id " << live_order_id
                     << "Buy of size: " << order_size
                     << " for product: " << shortcode_ << "\n";
          dbglogger_ << "Netting Orders at " << watch_.tv()
                     << " Assume execution Ord_id " << order_id_
                     << "Sell of size: " << order_size
                     << " for product: " << shortcode_ << "\n";
          WriteToTradesFile(live_order_id, shortcode_, "ABUY", order_size,
                            live_order_ref_px, "PASS");
          WriteToTradesFile(order_id_, shortcode_, "ASELL", -1 * order_size,
                            strat_order_ref_px, "PASS");
          // adjust the order_size remaining for this live order
          order_itr->second.first = r_new_order_size;
          strat_order_size = 0; // strat_order size is completely netted
        }
      } else {                                    // sell live orders
        if (r_new_order_size < live_order_size) { // this order is also sell,add
                                                  // it to live order id map
          live_order_id_info_map_.insert(std::make_pair(
              order_id_, std::make_pair(order_size, strat_order_ref_px)));

          dbglogger_ << "Netting Orders at " << watch_.tv()
                     << " Adding to live order id map " << order_id_
                     << "Sell of size: " << order_size
                     << " for product: " << shortcode_ << "\n";
        } else if (r_new_order_size >=
                   0) { // side changes after netting this order

          dbglogger_ << "Netting Orders at " << watch_.tv()
                     << " Assume execution Ord_id " << live_order_id
                     << "Sell of size: " << live_order_size
                     << " for product: " << shortcode_ << "\n";
          dbglogger_ << "Netting Orders at " << watch_.tv()
                     << " Assume execution Ord_id " << order_id_
                     << "Buy of size: " << (-1 * live_order_size)
                     << " for product: " << shortcode_ << "\n";
          WriteToTradesFile(live_order_id, shortcode_, "ASELL", live_order_size,
                            live_order_ref_px, "PASS");
          WriteToTradesFile(order_id_, shortcode_, "ABUY", -1 * live_order_size,
                            strat_order_ref_px, "PASS");

          live_order_id_info_map_.erase(
              order_itr++); // delete this entry from order map
          isEntrydeleted = true;
          strat_order_size = r_new_order_size; // adjust the size remaining for
                                               // this new order after netting
        } else { // side will not change after netting this
          dbglogger_ << "Netting Orders at " << watch_.tv()
                     << " Assume execution Ord_id " << live_order_id
                     << "Sell of size: " << order_size
                     << " for product: " << shortcode_ << "\n";
          dbglogger_ << "Netting Orders at " << watch_.tv()
                     << " Assume execution Ord_id " << order_id_
                     << "Buy of size: " << (-1 * order_size)
                     << " for product: " << shortcode_ << "\n";
          WriteToTradesFile(live_order_id, shortcode_, "ASELL", order_size,
                            live_order_ref_px, "PASS");
          WriteToTradesFile(order_id_, shortcode_, "ABUY", -1 * order_size,
                            strat_order_ref_px, "PASS");

          // adjust the order_size remaining for this live order
          order_itr->second.first = r_new_order_size;
          strat_order_size = 0; // strat_order size is completely netted
        }
      }
      if (strat_order_size == 0) // strat_order size is completely netted
        break;                   // we need not net remaining live order ids
      trades_file_.flush();
      if (!isEntrydeleted)
        ++order_itr; // when entry deleted from live order map, itr is taken
                     // care of
    }
    if (strat_order_size != 0) {
      // insert this strat order remaining size to live order id info map
      live_order_id_info_map_.insert(std::make_pair(
          order_id_, std::make_pair(strat_order_size, strat_order_ref_px)));
    }
  }
  if (live_order_id_info_map_
          .empty()) { // no live order id present, everything is netted
    trade_over_ = true;
    dbglogger_ << "Netted all orders from strategy at : " << watch_.tv()
               << " for product: " << shortcode_ << " Printing stats: \n";
    CalculateVwapMetrices();      // statistics
    ResetParticipationMetrices(); // reset market metrices
  }
  // all pending strat orders netted, clear the map
  strat_orders_to_be_netted_.clear();
}

// We are limiting the max size_to_execute by sum of sizes for all live order
// ids in TradingLogic()
// On executions, we loop through all the live order ids and mark all those ids
// for which we received executions
void NseSyntheticLegExecLogic::RecordOrderExecsForLiveOrderIds(
    const HFSAT::TradeType_t _buysell_, int size_exec_, double trade_px_) {
  // handling case when OnExec was not recorded but order got deleted from
  // Ordermaps
  // i.e. GetExistingOrderDetails() returned false, example cxlrej received
  // before exec
  // we may have netted the order wrongly in that case
  HFSAT::TradeType_t expected_side =
      (live_order_id_info_map_.begin()->second.first > 0)
          ? HFSAT::kTradeTypeBuy
          : HFSAT::kTradeTypeSell;
  if (expected_side != _buysell_) {
    dbglogger_ << "Fatal Error: OnExec: Received opposite side in exec than "
                  "expected. Some incorrect order netting "
                  "for product: "
               << shortcode_ << "\n";
    dbglogger_.DumpCurrentBuffer();
  }
  if (GetTotalSizeForAllLiveOrders() < size_exec_) {
    dbglogger_ << "Fatal Error: OnExec Received greater order exec size than "
                  "expected for product: "
               << shortcode_ << "\n";
    dbglogger_.DumpCurrentBuffer();
  }

  bool isEntrydeleted = false;

  std::string exec_type_ = (is_passive_) ? "PASS" : "AGG";

  for (auto order_itr = live_order_id_info_map_.begin();
       order_itr != live_order_id_info_map_.end();) {
    std::string order_id_ = order_itr->first;
    int live_order_size = order_itr->second.first;
    isEntrydeleted = false;
    if (live_order_size > 0) {             // buy live orders
      if (size_exec_ >= live_order_size) { // this order id is executed
                                           // completely, delete from map
        dbglogger_ << "Order Executed: " << watch_.tv()
                   << " order_id: " << order_itr->first
                   << " buy size: " << live_order_size
                   << " for product: " << shortcode_ << "\n";

        dbglogger_ << watch_.tv() << " SLACK "
                   << "Execution:\t" << order_itr->first << "\t"
                   << shortcode_ << "\tBUY\t"
                   << std::abs(live_order_size) << "\t" << trade_px_ << '\t'
                   << exec_type_ << "\n";

        WriteToTradesFile(order_id_, shortcode_, "BUY", live_order_size,
                          trade_px_, exec_type_);

        size_exec_ -= live_order_size;
        live_order_id_info_map_.erase(order_itr++);
        isEntrydeleted = true;
        if (size_exec_ == 0)
          break; // complete execution, recorded all orderIDs for this execution
      } else {
        dbglogger_ << "Order Executed: " << watch_.tv()
                   << " order_id: " << order_itr->first
                   << " buy size: " << size_exec_
                   << " for product: " << shortcode_ << "\n";

        dbglogger_ << watch_.tv() << " SLACK "
                   << "Execution:\t" << order_itr->first << "\t"
                   << shortcode_ << "\tBUY\t" << std::abs(size_exec_)
                   << "\t" << trade_px_ << '\t' << exec_type_ << "\n";

        WriteToTradesFile(order_id_, shortcode_, "BUY", size_exec_, trade_px_,
                          exec_type_);

        order_itr->second.first = live_order_size - size_exec_;
        size_exec_ = 0;
        break;
      }
    } else {
      if (size_exec_ >= (-1 * live_order_size)) { // this order id is executed
                                                  // completely, delete from map
        dbglogger_ << "Order Executed: " << watch_.tv()
                   << " order_id: " << order_itr->first
                   << " sell size: " << live_order_size
                   << " for product: " << shortcode_ << "\n";

        dbglogger_ << watch_.tv() << " SLACK "
                   << "Execution:\t" << order_itr->first << "\t"
                   << shortcode_ << "\tSELL\t"
                   << std::abs(live_order_size) << "\t" << trade_px_ << '\t'
                   << exec_type_ << "\n";

        WriteToTradesFile(order_id_, shortcode_, "SELL", live_order_size,
                          trade_px_, exec_type_);

        size_exec_ -= (-1 * live_order_size);
        live_order_id_info_map_.erase(order_itr++);
        isEntrydeleted = true;
        if (size_exec_ == 0)
          break; // complete execution, recorded all orderIDs for this execution
      } else {
        dbglogger_ << "Order Executed: " << watch_.tv()
                   << " order_id: " << order_itr->first
                   << " sell size: " << size_exec_
                   << " for product: " << shortcode_ << "\n";

        dbglogger_ << watch_.tv() << " SLACK "
                   << "Execution:\t" << order_itr->first << "\t"
                   << shortcode_ << "\tSELL\t" << std::abs(size_exec_)
                   << "\t" << trade_px_ << '\t' << exec_type_ << "\n";

        WriteToTradesFile(order_id_, shortcode_, "SELL", size_exec_, trade_px_,
                          exec_type_);

        order_itr->second.first = live_order_size + size_exec_;
        size_exec_ = 0;
        break;
      }
    }
    if (!isEntrydeleted)
      ++order_itr; // when entry deleted from live order map, itr is taken care
                   // of
  }
  // we may have executed all the live order ids, check and mark trade completed
  if (live_order_id_info_map_
          .empty()) { // no live order id present, everything is executed
    trade_over_ = true;
    dbglogger_ << "Executed all orders from strategy at : " << watch_.tv()
               << " Printing stats: for product: " << shortcode_ << "\n";
    CalculateVwapMetrices();      // statistics
    ResetParticipationMetrices(); // reset market metrices
  }
  if (size_exec_ != 0) {
    dbglogger_ << "Fatal Error: RecordOrderExecsForLiveOrderIds: Received "
                  "greater order exec size than expected for product: "
               << shortcode_ << "\n";
    dbglogger_.DumpCurrentBuffer();
  }
  trades_file_.flush();
}
int NseSyntheticLegExecLogic::get_remaining_order_lots_to_be_executed(
    double t_price_) {
  return (live_order_id_info_map_.empty()) ? 0 : GetTotalSizeForAllLiveOrders();
}

int NseSyntheticLegExecLogic::GetTotalSizeForAllLiveOrders() {
  int total_size = 0;
  for (auto order_itr = live_order_id_info_map_.begin();
       order_itr != live_order_id_info_map_.end(); ++order_itr) {
    total_size += order_itr->second.first;
  }
  return std::abs(total_size);
}
void NseSyntheticLegExecLogic::TradingLogic() {
  curr_prg_state_ = kNoop;

  if (!CheckStabilityConstraints())
    return; // not good to place any order

  bool cond_agg_ =
      (((this_smv_.bestask_price() - this_smv_.bestbid_price()) /
        ((this_smv_.bestask_price() + this_smv_.bestbid_price()) / 2)) *
           10000 <=
       param_->aggress_threshold_)
          ? true
          : false;
  bool min_tick_ =
      ((this_smv_.bestask_price() - this_smv_.bestbid_price()) == 0.05) ? true
                                                                        : false;
  bool is_future_ = false;
  // We never want to aggress on futures..
  if (param_->instrument_.find("FUT") != std::string::npos) {
    is_future_ = true;
  }

  // Conditions under which we want to place agg orders - either if (a) algotype
  // is Aggressive; or
  //( b ) algotype is PassAndAgg and our mkt share is below
  // participation_factor_ - tolerance
  if (param_->exec_algo_ == kAggOnly ||
      (param_->exec_algo_ == kPassAndAgg && !is_future_ &&
       (cond_agg_ || min_tick_))) {
    if (GetExistingOrderDetails() == false) {
      if (!l1_changed_since_last_agg_) {
        //        dbglogger_ << "NoOp set since no L1 change at " << watch_.tv()
        //                   << " for product: " << shortcode_ << "\n";
        return;
      }
      if (watch_.msecs_from_midnight() - msecs_at_last_aggress_ <
          param_->trade_cooloff_interval_) {
        //        dbglogger_ << "NoOp set due to agg cooloff criteria at " <<
        //        watch_.tv()
        //                   << " for product: " << shortcode_ << "\n";
        return;
      }
    }

    // stability checks have passed and vwap is low
    int min_lotsize_ = this_smv_.min_order_size_;

    // ignore diff between bid and ask for this check.
    // if executing an order will not have us cross threshold
    double t_price_ =
        (trade_side_ == HFSAT::kTradeTypeBuy ? this_smv_.bestask_price()
                                             : this_smv_.bestbid_price());
    if ((min_lotsize_ * t_price_ + total_notional_traded_) / market_notional_ <
            param_->market_participation_factor_ +
                param_->market_participation_factor_tolerance_ ||
        market_notional_ == 0) {
      if (cond_agg_ || min_tick_) {
        int t_max_lots_per_order_ = param_->max_lots_per_trade_;
        int remaining_order_lots_to_be_executed =
            get_remaining_order_lots_to_be_executed(t_price_);

        size_to_execute_ =
            min_lotsize_ * std::min(t_max_lots_per_order_,
                                    remaining_order_lots_to_be_executed);
        size_to_execute_ =
            std::min(size_to_execute_,
                     min_lotsize_ * remaining_order_lots_to_be_executed);
        curr_prg_state_ = kAggressive;

        is_passive_ = false;
      } else {
        //        dbglogger_ << "NoOp set due to agg spread factor at " <<
        //        watch_.tv()
        //                   << " for product: " << shortcode_ << ". Bid-Ask
        //                   spread -> "
        //                   << this_smv_.bestbid_price() << " "
        //                   << this_smv_.bestask_price() << "\n";
      }
    }
  }
  // Conditions under which we place a passive order - algotype is PassAndAgg
  // and PassOnly
  else if (param_->exec_algo_ == kPassAndAgg ||
           param_->exec_algo_ == kPassOnly || is_future_) {
    if (param_->exec_algo_ == kPassAndAgg) {
//      dbglogger_ << "Passive due to agg spread factor at " << watch_.tv()
//                 << " for product: " << shortcode_ << ". Bid-Ask spread -> "
//                 << this_smv_.bestbid_price() << " "
//                 << this_smv_.bestask_price() << "\n";
    }

    int min_lotsize_ = this_smv_.min_order_size_;
    double t_price_ =
        (trade_side_ == HFSAT::kTradeTypeBuy ? this_smv_.bestbid_price()
                                             : this_smv_.bestask_price());
    if (((min_lotsize_ * t_price_ + total_notional_traded_) / market_notional_ <
         param_->market_participation_factor_ +
             param_->market_participation_factor_tolerance_) ||
        market_notional_ == 0) {
      int t_max_lots_per_order_ = param_->max_lots_per_trade_;
      int remaining_order_lots_to_be_executed =
          get_remaining_order_lots_to_be_executed(t_price_);

      size_to_execute_ =
          min_lotsize_ *
          std::min(t_max_lots_per_order_, remaining_order_lots_to_be_executed);
      curr_prg_state_ = kPassive;
      is_passive_ = true;
    } else
      dbglogger_ << "NoOp set due to high potential participation factor "
                 << (min_lotsize_ * t_price_ + total_notional_traded_) /
                        market_notional_ * 100.0
                 << " at " << watch_.tv() << " for product: " << shortcode_
                 << "\n";
  }
}
void NseSyntheticLegExecLogic::DumpSnapshot() {

  // print live_order_id_info_map_
  dbglogger_ << "LiveOrders IDs: " << "\n";
  int idx = 0;
  for (auto &live_order_itr : live_order_id_info_map_) {

	  dbglogger_ << "Order Number : "<< idx++ <<" Live Order Complex Code : "<<live_order_itr.first << "\t"
                   << live_order_itr.second.first << " for " << shortcode_
                   << "\n";
  }

  dbglogger_ << "Orders pending netting: " << "\n";
  for (auto &strat_order_itr : strat_orders_to_be_netted_) {
	  dbglogger_ << strat_order_itr.first << "\t"
                   << strat_order_itr.second.first << " for " << shortcode_
                   << "\n";
  }
}
// Update indicators on every indicator update
void NseSyntheticLegExecLogic::OnIndicatorUpdate(
    const unsigned int &_indicator_index_, const double &_new_value_) {
  if (_indicator_index_ == 0) {
    simple_trend_ = _new_value_;
  } else if (_indicator_index_ == 1) {
    stdev_ = _new_value_;
  }
}
}
