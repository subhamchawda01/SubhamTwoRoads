#include "dvctrade/ExecLogic/nse_trade_given_notional_exec_logic.hpp"

namespace NSE_SIMPLEEXEC {

NseTradeGivenNotionalExecLogic::NseTradeGivenNotionalExecLogic(HFSAT::SecurityMarketView& this_smv_t,
                                                               HFSAT::BaseTrader* p_base_trader_t,
                                                               HFSAT::SmartOrderManager* p_smart_order_manager_t,
                                                               HFSAT::DebugLogger& dbglogger_t, HFSAT::Watch& watch_t,
                                                               ParamSet* t_param_, bool isLive_t_)
    : SimpleNseExecLogic(this_smv_t, p_base_trader_t, p_smart_order_manager_t, dbglogger_t, watch_t, t_param_,
                         isLive_t_) {}

void NseTradeGivenNotionalExecLogic::OnMarketUpdate(const unsigned int _security_id_,
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

bool NseTradeGivenNotionalExecLogic::CheckStabilityConstraints() {
  // we are running between some interval t1-t2
  // if either we've exceeded tolerance levels or max notional allocated, we wait/stop.
  if (total_notional_traded_ / market_notional_ >
      param_->market_participation_factor_ + param_->market_participation_factor_tolerance_) {
    dbglogger_ << "NoOp set due to high partcipation factor for product: " << param_->instrument_ << "\n";
    return false;
  }

  // cannot buy/sell more lots than occupied during entry or stated in param otherwise
  if (param_->max_lots_ > 0 &&
      ((total_volume_traded_ + this_smv_.min_order_size_) > (param_->max_lots_ * this_smv_.min_order_size_))) {
    trade_over_ = true;
    return false;
  }
  // if max_lots provided in param, it overrides max_notional constraints
  if (param_->max_lots_ < 0 &&
      total_notional_traded_ + this_smv_.min_order_size_ * this_smv_.bestbid_price() > param_->max_notional_) {
    trade_over_ = true;
    return false;
  }
  return true;
}

int NseTradeGivenNotionalExecLogic::get_remaining_order_lots_to_be_executed(double t_price_) {
  // Never overshoot max_lots/ max_notional check
  int min_lotsize_ = this_smv_.min_order_size_;
  int t_rem_lots_ = param_->max_lots_ > 0
                        ? (param_->max_lots_ - total_volume_traded_ / min_lotsize_)
                        : (int)(param_->max_notional_ - total_notional_traded_) / (t_price_ * min_lotsize_);
  return t_rem_lots_;
}

void NseTradeGivenNotionalExecLogic::TradingLogic() {
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
}
