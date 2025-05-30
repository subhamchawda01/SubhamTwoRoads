#pragma once
#include "dvctrade/ExecLogic/nse_simple_exec_logic.hpp"
// This extends simple execution logic.
// Receives orders from RV strategy and executes them based on mkt participation logic of simpleExecLogic
// We are now constrained by total orders size from strategy  instead of total_notional or max_lots as specified in
// param file.
namespace NSE_SIMPLEEXEC {
typedef std::pair<int, double> size_px_pair;

class NseRVStrategyExecLogic : public SimpleNseExecLogic {
  std::map<std::string, size_px_pair> live_order_id_info_map_;
  std::map<std::string, size_px_pair> strat_orders_to_be_netted_;
  std::ofstream trades_file_;
  std::ofstream snapshot_file_;
  double commission_per_lot;

 public:
  NseRVStrategyExecLogic(HFSAT::SecurityMarketView& this_smv_t, HFSAT::BaseTrader* p_base_trader_t,
                         HFSAT::SmartOrderManager* p_smart_order_manager_t, HFSAT::DebugLogger& dbglogger_t,
                         HFSAT::Watch& watch_t, ParamSet* t_param_, bool isLive_t_);
  ~NseRVStrategyExecLogic() {
    trades_file_.close();
    snapshot_file_.close();
  }
  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) override;
  int get_remaining_order_lots_to_be_executed(double t_price_) override;
  bool CheckStabilityConstraints() override;
  void OnNewOrderFromStrategy(std::string order_id_, int order_lots_, double ref_px_) override;
  void CheckAndNetOrder() override;
  void RecordOrderExecsForLiveOrderIds(const HFSAT::TradeType_t _buysell_, int size_exec_, double trade_px_) override;
  int GetTotalSizeForAllLiveOrders() override;
  void TradingLogic();
  void DumpSnapshot() override;
};
}
