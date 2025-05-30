#pragma once
#include "midterm/GeneralizedLogic/base_modify_exec_logic.hpp"

namespace NSE_SIMPLEEXEC {
class BaseModifyExecLogicForLive : public BaseModifyExecLogic {

protected:
  BaseModifyExecLogicForLive() : BaseModifyExecLogic() {}

public:
  static BaseModifyExecLogicForLive &GetUniqueInstance() {
    static BaseModifyExecLogicForLive uniqueinstance_;
    return uniqueinstance_;
  }

  ~BaseModifyExecLogicForLive() {};

  // TOOD:: The improve part is done in a hacky way. Once we have a non-self SMV
  // we can do this more gracefully
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
    // Since our own order shows in SMV
    // Check if it is us on the top of the book
    else if (existing_order_->buysell_ == HFSAT::kTradeTypeBuy &&
             existing_order_->int_price_ >= this_smv_.bestbid_int_price() &&
             size_to_execute_ == this_smv_.bestbid_size()) {
      int t_int_px_to_place_at_ = this_smv_.bid_int_price(1) + 1;
      if (t_int_px_to_place_at_ != existing_order_->int_price_) {
        dbglogger_ << "Improve order at time " << watch_.tv() << " to price "
                   << t_int_px_to_place_at_ * 0.05 << " and size "
                   << size_to_execute_ << " for product: " << shortcode_
                   << '\n';
        p_smart_order_manager_->ModifyOrderAndLog(
            existing_order_, this_smv_.GetDoublePx(t_int_px_to_place_at_),
            t_int_px_to_place_at_, size_to_execute_);
      }
    } else if (existing_order_->buysell_ == HFSAT::kTradeTypeSell &&
               existing_order_->int_price_ <= this_smv_.bestask_int_price() &&
               size_to_execute_ == this_smv_.bestask_size()) {
      int t_int_px_to_place_at_ = this_smv_.ask_int_price(1) - 1;
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
  }
};
}
