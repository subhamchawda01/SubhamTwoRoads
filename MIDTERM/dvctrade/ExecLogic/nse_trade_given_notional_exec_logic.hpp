#pragma once
#include "dvctrade/ExecLogic/nse_simple_exec_logic.hpp"
// This extends simple execution logic.
// Receives orders from RV strategy and executes them based on mkt participation logic of simpleExecLogic
// We are now constrained by total orders size from strategy  instead of total_notional or max_lots as specified in
// param file.
namespace NSE_SIMPLEEXEC {
class NseTradeGivenNotionalExecLogic : public SimpleNseExecLogic {
 public:
  NseTradeGivenNotionalExecLogic(HFSAT::SecurityMarketView& this_smv_t, HFSAT::BaseTrader* p_base_trader_t,
                                 HFSAT::SmartOrderManager* p_smart_order_manager_t, HFSAT::DebugLogger& dbglogger_t,
                                 HFSAT::Watch& watch_t, ParamSet* t_param_, bool isLive_t_);
  ~NseTradeGivenNotionalExecLogic() {}
  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) override;
  int get_remaining_order_lots_to_be_executed(double t_price_) override;
  bool CheckStabilityConstraints() override;
  void TradingLogic();
};
}
