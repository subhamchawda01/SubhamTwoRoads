#pragma once
#include "midterm/GeneralizedLogic/nse_simple_exec_logic.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "midterm/GeneralizedLogic/paths.hpp"
#include "nse_risk_manager.hpp"
#include "nse_execution_listener_manager.hpp"
// This extends simple execution logic.
// Receives orders from RV strategy and executes them based on mkt participation
// logic of simpleExecLogic
// We are now constrained by total orders size from strategy  instead of
// total_notional or max_lots as specified in
// param file.
namespace NSE_SIMPLEEXEC {
typedef std::pair<int, double> size_px_pair;

class NseSyntheticLegExecLogic : public SimpleNseExecLogic,
                                 public HFSAT::IndicatorListener {
public:
  std::map<std::string, size_px_pair> live_order_id_info_map_;
  std::map<std::string, size_px_pair> strat_orders_to_be_netted_;
  std::vector<std::string> strat_orders_to_be_removed_;
  std::ofstream trades_file_;
  std::ofstream snapshot_file_;
  double commission_per_lot;
  double last_traded_px_;
  HFSAT::SimpleTrend *price_change_indicator_;
  HFSAT::SlowStdevCalculator *stdev_change_indicator_;
  double simple_trend_;
  double stdev_;
  bool is_passive_;
  std::string strat_type_;
  std::string exchange_symbol;

public:
  NseSyntheticLegExecLogic(HFSAT::SecurityMarketView &this_smv_t,
                           HFSAT::BaseTrader *p_base_trader_t,
                           HFSAT::SmartOrderManager *p_smart_order_manager_t,
						   BaseModifyExecLogic &modify_exec_logic_t,
                           HFSAT::DebugLogger &dbglogger_t,
                           HFSAT::Watch &watch_t, ParamSet *t_param_,
                           bool isLive_t_, std::string shortcode_t,
                           HFSAT::PriceType_t price_type,
                           std::string strategy_type, int qid);
  ~NseSyntheticLegExecLogic() {
    trades_file_.close();
    snapshot_file_.close();
  }
  void
  OnMarketUpdate(const unsigned int _security_id_,
                 const HFSAT::MarketUpdateInfo &_market_update_info_) override;
  void
  OnTradePrint(const unsigned int _security_id_,
               const HFSAT::TradePrintInfo &_trade_print_info_,
               const HFSAT::MarketUpdateInfo &_market_update_info_) override;
  int get_remaining_order_lots_to_be_executed(double t_price_) override;
  bool CheckStabilityConstraints() override;
  inline void RequestCancel() { p_smart_order_manager_->CancelAllOrders(); }
  inline void FakeOrsCancel() { p_smart_order_manager_->FakeAllOrsReply(); }
  bool OnNewOrderFromStrategy(std::string order_id_, int order_lots_) override;
  void WriteToTradesFile(std::string &order_id, std::string &shortcode,
                         std::string action, int size, double price,
                         std::string exec_type);
  void CheckAndNetOrder() override;
  void RecordOrderExecsForLiveOrderIds(const HFSAT::TradeType_t _buysell_,
                                       int size_exec_,
                                       double trade_px_) override;
  int GetTotalSizeForAllLiveOrders() override;
  void TradingLogic();

  //Functions to update the complex order_id to ORS info map
  //void InsertInOrsInfoMap(std::string order_id);
  void UpdateOrsInfo();

  void DumpSnapshot() override;
  void OnIndicatorUpdate(const unsigned int &_indicator_index_,
                         const double &_new_value_);
  void OnIndicatorUpdate(const unsigned int &_indicator_index_,
                         const double &_new_value_decrease_,
                         const double &_new_value_nochange_,
                         const double &_new_value_increase_) {}

  inline bool HasOrderID(std::string order_id_) {
    for (auto i : live_order_id_info_map_) {
      if (i.first.find(order_id_) != std::string::npos) {
        return true;
      }
    }
    for (auto i : strat_orders_to_be_netted_) {
      if (i.first.find(order_id_) != std::string::npos) {
        return true;
      }
    }
    return false;
  }
};

}
