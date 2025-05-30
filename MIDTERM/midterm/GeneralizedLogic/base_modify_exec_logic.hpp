#pragma once
#include <math.h>

namespace NSE_SIMPLEEXEC {
class BaseModifyExecLogic {
public:
  BaseModifyExecLogic() {}

  virtual ~BaseModifyExecLogic() {}

  virtual void
  ModifyOrder(HFSAT::DebugLogger &dbglogger_, HFSAT::Watch &watch_,
              int size_to_execute_, std::string &shortcode_,
              HFSAT::TradeType_t trade_side_, HFSAT::BaseOrder *existing_order_,
              HFSAT::SecurityMarketView &this_smv_,
              HFSAT::SmartOrderManager *p_smart_order_manager_) = 0;
};
}
