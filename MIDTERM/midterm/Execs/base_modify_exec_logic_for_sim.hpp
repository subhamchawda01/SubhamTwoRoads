#pragma once
#include "midterm/GeneralizedLogic/base_modify_exec_logic.hpp"

namespace NSE_SIMPLEEXEC {
class BaseModifyExecLogicForSim : public BaseModifyExecLogic {

protected:
  BaseModifyExecLogicForSim() : BaseModifyExecLogic() {}

public:
  static BaseModifyExecLogicForSim &GetUniqueInstance() {
    static BaseModifyExecLogicForSim uniqueinstance_;
    return uniqueinstance_;
  }

  ~BaseModifyExecLogicForSim() {};

  void ModifyOrder(HFSAT::DebugLogger &dbglogger_, HFSAT::Watch &watch_,
                   int size_to_execute_, std::string &shortcode_,
                   HFSAT::TradeType_t trade_side_,
                   HFSAT::BaseOrder *existing_order_,
                   HFSAT::SecurityMarketView &this_smv_,
                   HFSAT::SmartOrderManager *p_smart_order_manager_) {

    // order exists .. modify it if it is sub-best
    if ((existing_order_->buysell_ == HFSAT::kTradeTypeBuy &&
         existing_order_->int_price_ < this_smv_.bestbid_int_price()) ||
        (existing_order_->buysell_ == HFSAT::kTradeTypeSell &&
         existing_order_->int_price_ > this_smv_.bestask_int_price())) {
      int t_int_px_to_place_at_ =
          (trade_side_ == HFSAT::kTradeTypeBuy ? this_smv_.bestbid_int_price()
                                               : this_smv_.bestask_int_price());
      dbglogger_ << "Modify order at time " << watch_.tv() << " to price "
                 << t_int_px_to_place_at_ << " and size " << size_to_execute_
                 << " for product: " << shortcode_ << '\n';
      p_smart_order_manager_->ModifyOrderAndLog(
          existing_order_, this_smv_.GetDoublePx(t_int_px_to_place_at_),
          t_int_px_to_place_at_, size_to_execute_);
    }
    // order exists.. modify to just a simple improve for buy
    else if (existing_order_->buysell_ == HFSAT::kTradeTypeBuy &&
             existing_order_->int_price_ > this_smv_.bestbid_int_price()) {
      int t_int_px_to_place_at_ = this_smv_.bestbid_int_price() + 1;
      if (t_int_px_to_place_at_ != existing_order_->int_price_) {
        dbglogger_ << "Improve order at time " << watch_.tv() << " to price "
                   << t_int_px_to_place_at_ * 0.05 << " and size "
                   << size_to_execute_ << " for product: " << shortcode_
                   << '\n';
        p_smart_order_manager_->ModifyOrderAndLog(
            existing_order_, this_smv_.GetDoublePx(t_int_px_to_place_at_),
            t_int_px_to_place_at_, size_to_execute_);
      }
    }
    // order exists.. modify to just a simple improve for sell
    else if (existing_order_->buysell_ == HFSAT::kTradeTypeSell &&
             existing_order_->int_price_ < this_smv_.bestask_int_price()) {
      int t_int_px_to_place_at_ = this_smv_.bestask_int_price() - 1;
      if (t_int_px_to_place_at_ != this_smv_.bestask_int_price()) {
        dbglogger_ << "Improve order at time " << watch_.tv() << " to price "
                   << t_int_px_to_place_at_ * 0.05 << " and size "
                   << size_to_execute_ << " for product: " << shortcode_
                   << '\n';
        p_smart_order_manager_->ModifyOrderAndLog(
            existing_order_, this_smv_.GetDoublePx(t_int_px_to_place_at_),
            t_int_px_to_place_at_, size_to_execute_);
      }
    }
  }
};
}
