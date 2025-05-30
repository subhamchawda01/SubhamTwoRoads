#include "dvctrade/ExecLogic/nse_simple_exec_logic.hpp"

namespace NSE_SIMPLEEXEC {
#define SMALL_EXEC_DELAY 500
// SimpleNseExecLogic: Class functions definitions
SimpleNseExecLogic::SimpleNseExecLogic(HFSAT::SecurityMarketView& this_smv_t, HFSAT::BaseTrader* p_base_trader_t,
                                       HFSAT::SmartOrderManager* p_smart_order_manager_t,
                                       HFSAT::DebugLogger& dbglogger_t, HFSAT::Watch& watch_t, ParamSet* t_param_set,
                                       bool isLive_t)
    : this_smv_(this_smv_t),
      p_base_trader_(p_base_trader_t),
      p_smart_order_manager_(p_smart_order_manager_t),
      dbglogger_(dbglogger_t),
      watch_(watch_t),
      msecs_at_last_aggress_(0),
      size_to_execute_(0),
      server_assigned_client_id_(p_base_trader_t->GetClientId()),
      total_notional_traded_(0),
      total_volume_traded_(0),
      market_notional_(0),
      market_volume_(0),
      isMetricCalculated(false),
      curr_prg_state_(kNoop),
      existing_order_(NULL),
      unseq_order_present_(false),
      int_price_when_agg_was_placed_(0),
      size_when_agg_was_placed_(0),
      l1_changed_since_last_agg_(false),
      param_(t_param_set),
      trade_over_(false),
      msec_at_last_snapshot_(0),
      trade_side_(param_->buysell_),
      isLive_(isLive_t) {}

// Sanity checks for making sure only one order exists - exits if more than one order exists.
// Returns true without setting order if unseq order exists.
// Returns true after setting order if conf order exists
// Returns false otherwise.
bool SimpleNseExecLogic::GetExistingOrderDetails() {
  existing_order_ = NULL;
  unseq_order_present_ = false;

  bool t_retval_ = false;
  // case of buy handling
  if (trade_side_ == HFSAT::kTradeTypeBuy) {
    if (p_smart_order_manager_->GetUnSequencedBids().size() > 0) {
      unseq_order_present_ = true;
      if (p_smart_order_manager_->GetUnSequencedBids().size() > 1) {
        dbglogger_ << "Fatal error - 2 unseq orders present "
                   << " for product: " << param_->instrument_ << "\n";
        std::cerr << "Fatal error - 2 unseq orders present for product: " << param_->instrument_ << "\n";
        dbglogger_.DumpCurrentBuffer();
        exit(-1);
      }
      return true;
    }
    int t_order_vec_top_bid_index_ = p_smart_order_manager_->GetOrderVecTopBidIndex();
    int t_order_vec_bottom_bid_index_ = p_smart_order_manager_->GetOrderVecBottomBidIndex();
    if (t_order_vec_top_bid_index_ != t_order_vec_bottom_bid_index_) {
      dbglogger_ << "More than one bid order present for product: " << param_->instrument_ << "\n";
      std::cerr << "More than one bid order present for product: " << param_->instrument_ << "\n";
      dbglogger_.DumpCurrentBuffer();
      exit(-1);
    }
    if (t_order_vec_top_bid_index_ != -1) {
      int existing_bid_int_price_ = p_smart_order_manager_->GetBidIntPrice(t_order_vec_top_bid_index_);
      existing_order_ = p_smart_order_manager_->GetBottomBidOrderAtIntPx(existing_bid_int_price_);
      t_retval_ = true;
    }
  }
  // case of sell handling
  if (trade_side_ == HFSAT::kTradeTypeSell) {
    if (p_smart_order_manager_->GetUnSequencedAsks().size() > 0) {
      unseq_order_present_ = true;
      if (p_smart_order_manager_->GetUnSequencedAsks().size() > 1) {
        dbglogger_ << "Fatal error - 2 unseq orders present for product: " << param_->instrument_ << "\n";
        std::cerr << "Fatal error - 2 unseq orders present for product: " << param_->instrument_ << "\n";
        dbglogger_.DumpCurrentBuffer();
        exit(-1);
      }
      return true;
    }
    int t_order_vec_top_ask_index_ = p_smart_order_manager_->GetOrderVecTopAskIndex();
    int t_order_vec_bottom_ask_index_ = p_smart_order_manager_->GetOrderVecBottomAskIndex();
    if (t_order_vec_top_ask_index_ != t_order_vec_bottom_ask_index_) {
      dbglogger_ << "More than one sell order present for product: " << param_->instrument_ << "\n";
      std::cerr << "More than one sel order present for product: " << param_->instrument_ << "\n";
      dbglogger_.DumpCurrentBuffer();
      exit(-1);
    }
    if (t_order_vec_top_ask_index_ != -1) {
      int existing_ask_int_price_ = p_smart_order_manager_->GetAskIntPrice(t_order_vec_top_ask_index_);
      existing_order_ = p_smart_order_manager_->GetBottomAskOrderAtIntPx(existing_ask_int_price_);
      t_retval_ = true;
    }
  }
  return t_retval_;
}

// maintain simulation vwap price variables
void SimpleNseExecLogic::OnExec(const int _new_position_, const int _exec_quantity_, const HFSAT::TradeType_t _buysell_,
                                const double _price_, const int r_int_price_, const int _security_id_) {
  dbglogger_ << "Exec at time " << watch_.tv() << " of size " << _exec_quantity_ << " at intpx " << r_int_price_
             << " of type " << (_buysell_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL")
             << " for product: " << param_->instrument_ << "\n";
  total_notional_traded_ += (_exec_quantity_ * _price_);
  total_volume_traded_ += _exec_quantity_;

  RecordOrderExecsForLiveOrderIds(_buysell_, _exec_quantity_, _price_);
}

void SimpleNseExecLogic::OrderRejected(const int t_server_assigned_client_id_,
                                       const int _client_assigned_order_sequence_, const unsigned int _security_id_,
                                       const double _price_, const HFSAT::TradeType_t r_buysell_,
                                       const int _size_remaining_, const int _rejection_reason_, const int r_int_price_,
                                       const uint64_t exchange_order_id, const HFSAT::ttime_t time_set_by_server) {
  if (t_server_assigned_client_id_ == server_assigned_client_id_) {  // the rejected order was sent by me :(
    dbglogger_ << "Rej at time " << watch_.tv() << "for saci: " << t_server_assigned_client_id_ << " of casi "
               << _client_assigned_order_sequence_ << " at intpx " << r_int_price_ << " reason: " << _rejection_reason_
               << " of type " << (r_buysell_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL")
               << " for product: " << param_->instrument_ << "\n";
  }
}
// maintain market vwap price variables
void SimpleNseExecLogic::OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                                      const HFSAT::MarketUpdateInfo& _market_update_info_) {}
void SimpleNseExecLogic::OnRawTradePrint(const unsigned int _security_id_,
                                         const HFSAT::TradePrintInfo& _trade_print_info_,
                                         const HFSAT::MarketUpdateInfo& _market_update_info_) {
  if ((watch_.msecs_from_midnight() >= param_->start_time_msecs_) &&
      (watch_.msecs_from_midnight() <= param_->end_time_msecs_) && (!trade_over_)) {
    market_volume_ += _trade_print_info_.size_traded_;
    market_notional_ += (_trade_print_info_.trade_price_ * _trade_print_info_.size_traded_);
  }
}
int SimpleNseExecLogic::get_max_order_lots_based_on_mkt_participation(double t_price_) {
  return (
      int)((market_notional_ * (param_->market_participation_factor_ + param_->market_participation_factor_tolerance_) -
            total_notional_traded_) /
           (this_smv_.min_order_size_ * t_price_));
}

void SimpleNseExecLogic::ResetParticipationMetrices() {  // reset market metrices
  market_notional_ = 0.0;
  market_volume_ = 0;
  total_notional_traded_ = 0.0;
  total_volume_traded_ = 0;
}
void SimpleNseExecLogic::CalculateVwapMetrices() {
  double market_vwap = (market_notional_ / ((double)market_volume_));
  double simulation_vwap = (total_notional_traded_ / ((double)total_volume_traded_));
  double diff_vwap = 0;
  int total_lots_traded = total_volume_traded_ / this_smv_.min_order_size_;
  if (trade_side_ == HFSAT::kTradeTypeBuy)
    diff_vwap = (1.0 - simulation_vwap / market_vwap) * 10000.0;
  else
    diff_vwap = (simulation_vwap / market_vwap - 1.0) * 10000.0;

  dbglogger_ << "Statistics at: " << watch_.tv() << "\n Instrument Name: " << param_->instrument_ << '\n'
             << "Market VWap: " << market_vwap << '\n' << "Sim VWap: " << simulation_vwap << '\n'
             << "Diff_VWAP: " << diff_vwap << '\n' << "Notional Traded: " << total_notional_traded_ << '\n'
             << "Percent Agg: " << p_smart_order_manager_->AggressiveOrderFilledPercent() << '\n';
  dbglogger_.DumpCurrentBuffer();
  std::cout << "Statistics at: " << watch_.tv() << "\nInstrument Name: " << param_->instrument_ << '\n'
            << "Market VWap: " << market_vwap << '\n' << "Sim VWap: " << simulation_vwap << '\n'
            << "Diff_VWAP: " << diff_vwap << '\n' << "Notional Traded: " << total_notional_traded_ << '\n'
            << "Total_lots: " << total_lots_traded << '\n'
            << "Percent Agg: " << p_smart_order_manager_->AggressiveOrderFilledPercent() << '\n'
            << "Tot_Not: " << total_notional_traded_ << " Mkt_Not: " << market_notional_
            << " Mkt_Vol: " << market_volume_ << "\n\n ";
}

void SimpleNseExecLogic::LogAndSendOrder(int int_px_, int size_, HFSAT::TradeType_t bs_, bool is_agg) {
  if (0 == size_) return;

  if (this_smv_.is_ready_) {
    dbglogger_ << "Send Order at time " << watch_.tv() << " of size " << size_ << " at px " << int_px_ << " of type "

               << (bs_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL") << " " << (is_agg ? "Agg" : "Pass")
               << " for product: " << param_->instrument_ << '\n';
    dbglogger_ << "Mkt_Share " << total_notional_traded_ / market_notional_ * 100.0
               << " for product: " << param_->instrument_ << "\n";
    p_smart_order_manager_->SendTradeIntPx(int_px_, size_, bs_, (is_agg ? 'A' : 'B'));
  } else
    dbglogger_ << "Smv not ready yet" << watch_.tv() << " for product: " << param_->instrument_ << "\n";
}

void SimpleNseExecLogic::PlaceOrder() {
  // in case order in unsequenced we simply return
  if (unseq_order_present_) {
    return;
  }
  // if state is aggressive ( a )  place agg order if none exists; ( b ) cancel any agg order
  // which becomes pass and remains in order book.
  if (curr_prg_state_ == kAggressive) {
    // if no order exists we place new order
    if (existing_order_ == NULL) {
      if (trade_side_ == HFSAT::kTradeTypeBuy) {
        LogAndSendOrder(this_smv_.bestask_int_price(), size_to_execute_, HFSAT::kTradeTypeBuy, true);
        msecs_at_last_aggress_ = watch_.msecs_from_midnight();
        int_price_when_agg_was_placed_ = this_smv_.bestask_int_price();
        size_when_agg_was_placed_ = this_smv_.bestask_size();
        l1_changed_since_last_agg_ = false;
      } else {
        LogAndSendOrder(this_smv_.bestbid_int_price(), size_to_execute_, HFSAT::kTradeTypeSell, true);
        msecs_at_last_aggress_ = watch_.msecs_from_midnight();
        int_price_when_agg_was_placed_ = this_smv_.bestbid_int_price();
        size_when_agg_was_placed_ = this_smv_.bestbid_size();
        l1_changed_since_last_agg_ = false;
      }
      return;
    }

    // if order exists, cancel order - for SIM to maintain accurate agg count
    // TODO - convert to Modify for live mode
    if (existing_order_ != NULL) {
      if (watch_.msecs_from_midnight() - msecs_at_last_aggress_ > SMALL_EXEC_DELAY) {
        dbglogger_ << " Cancel existing order at " << watch_.tv() << " since agg order became pass"
                   << " for product: " << param_->instrument_ << "\n";
        p_smart_order_manager_->CancelAllOrders();
      }
    }
  }
  // else state is Passive - place passive order if none exists; modify order to passive if
  // existing order is at sub-best
  else {
    // if no order exists we place new order
    if (existing_order_ == NULL) {
      if (trade_side_ == HFSAT::kTradeTypeBuy) {
        LogAndSendOrder(this_smv_.bestbid_int_price(), size_to_execute_, HFSAT::kTradeTypeBuy, false);
      } else {
        LogAndSendOrder(this_smv_.bestask_int_price(), size_to_execute_, HFSAT::kTradeTypeSell, false);
      }
      return;
    } else  // order exists .. modify it if it is sub-best
    {
      if ((existing_order_->buysell_ == HFSAT::kTradeTypeBuy &&
           existing_order_->int_price_ < this_smv_.bestbid_int_price()) ||
          (existing_order_->buysell_ == HFSAT::kTradeTypeSell &&
           existing_order_->int_price_ > this_smv_.bestask_int_price())) {
        int t_int_px_to_place_at_ =
            (trade_side_ == HFSAT::kTradeTypeBuy ? this_smv_.bestbid_int_price() : this_smv_.bestask_int_price());
        dbglogger_ << "Modify order at time " << watch_.tv() << " to price " << t_int_px_to_place_at_ << " and size "
                   << size_to_execute_ << " for product: " << param_->instrument_ << '\n';
        p_smart_order_manager_->ModifyOrderAndLog(existing_order_, this_smv_.GetDoublePx(t_int_px_to_place_at_),
                                                  t_int_px_to_place_at_, size_to_execute_);
      }
    }
  }
}
}
