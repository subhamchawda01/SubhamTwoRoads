#pragma once
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "midterm/GeneralizedLogic/nse_simple_exec_param.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/OrderRouting/prom_order_manager.hpp"
#include "baseinfra/SmartOrderRouting/smart_order_manager.hpp"
#include "dvccode/Utils/slack_utils.hpp"
#include "baseinfra/VolatileTradingInfo/base_commish.hpp"
#include "base_modify_exec_logic.hpp"
// It trades a given max_notional between time interval T1-T2 under
// constraints of market participation, aggression spread if placing aggressive
// orders.

namespace NSE_SIMPLEEXEC {

typedef std::pair<std::string, std::string> live_order_id_shc_pair;

typedef struct ORSInfoStruct {
	int SAOS;
	int CAOS;
	int int_px;
	HFSAT::ORRType_t order_status;

	ORSInfoStruct() {
		SAOS = -1;
		CAOS = -1;
		int_px = -1;
		order_status = HFSAT::kORRType_None;
	}

}ORSInfo;

class SimpleNseExecLogic : public HFSAT::SecurityMarketViewChangeListener,
                           public HFSAT::SecurityMarketViewRawTradesListener,
                           public HFSAT::ExecutionListener,
                           public HFSAT::OrderRejectedListener {
protected:
  HFSAT::SecurityMarketView &this_smv_;
  HFSAT::BaseTrader *p_base_trader_;
  HFSAT::SmartOrderManager *p_smart_order_manager_;
  BaseModifyExecLogic &modify_exec_logic_;
  HFSAT::DebugLogger &dbglogger_;
  HFSAT::Watch &watch_;
  int msecs_at_last_aggress_;
  int size_to_execute_;

  //complex order id shc pair to ors details info
  std::map<live_order_id_shc_pair, ORSInfo> order_id_to_ors_info_map;

  // saci for this exec
  const int server_assigned_client_id_;

  // NEEDED FOR CALCULATION OF METRIC //
  double total_notional_traded_;
  int total_volume_traded_;
  double market_notional_;
  int market_volume_;
  bool isMetricCalculated;

  // program state - separation between tradinglogic and order placement.
  ProgramState_t curr_prg_state_;

  // existing order parameters
  HFSAT::BaseOrder *existing_order_;
  bool unseq_order_present_;

  // inbuilt stability check -- for use in hist mode -- requires L1 change prior
  // to successive
  // agg orders being placed
  int int_price_when_agg_was_placed_;
  int size_when_agg_was_placed_;
  bool l1_changed_since_last_agg_;

  ParamSet *param_;

  // trade over -- we've acquired max allowed notional
  bool trade_over_;
  int msec_at_last_snapshot_;

  HFSAT::TradeType_t trade_side_; // order to be placed this side
  bool isLive_;                   // live or sim mode
  std::string shortcode_;

public:
  SimpleNseExecLogic(HFSAT::SecurityMarketView &this_smv_t,
                     HFSAT::BaseTrader *p_base_trader_t,
                     HFSAT::SmartOrderManager *p_smart_order_manager_t,
					 BaseModifyExecLogic &modify_exec_logic_t,
                     HFSAT::DebugLogger &dbglogger_t, HFSAT::Watch &watch_t,
                     ParamSet *t_param_, bool isLive_t,
                     std::string shortcode_t);
  virtual ~SimpleNseExecLogic() {}
  void
  OnMarketUpdate(const unsigned int _security_id_,
                 const HFSAT::MarketUpdateInfo &_market_update_info_) override {
  }
  void
  OnTradePrint(const unsigned int _security_id_,
               const HFSAT::TradePrintInfo &_trade_print_info_,
               const HFSAT::MarketUpdateInfo &_market_update_info_) override;
  void
  OnRawTradePrint(const unsigned int _security_id_,
                  const HFSAT::TradePrintInfo &_trade_print_info_,
                  const HFSAT::MarketUpdateInfo &_market_update_info_) override;
  void OnExec(const int _new_position_, const int _exec_quantity_,
              const HFSAT::TradeType_t _buysell_, const double _price_,
              const int r_int_price_, const int _security_id_) override;
  void OrderRejected(const int t_server_assigned_client_id_,
                     const int _client_assigned_order_sequence_,
                     const unsigned int _security_id_, const double _price_,
                     const HFSAT::TradeType_t r_buysell_,
                     const int _size_remaining_, const int _rejection_reason_,
                     const int r_int_price_, const uint64_t exchange_order_id,
                     const HFSAT::ttime_t time_set_by_server) override;
  int get_max_order_lots_based_on_mkt_participation(double t_price_);

  void PlaceOrder();
  void LogAndSendOrder(int int_px_, int size_, HFSAT::TradeType_t bs_,
                       bool is_agg); // returns .
  bool GetExistingOrderDetails();    // true if an there is an existing order
  void CalculateVwapMetrices();
  void ResetParticipationMetrices();

  // virtual functions
  virtual void
  TradingLogic() = 0; // sets program state, decides size_to_execute
  virtual int get_remaining_order_lots_to_be_executed(double t_price_) {
    return 0;
  }
  virtual bool
  CheckStabilityConstraints() = 0; // check if we are good to place new orders
  virtual bool OnNewOrderFromStrategy(std::string order_id_, int order_lots_) { return false; }
  virtual void CheckAndNetOrder() {}
  virtual void
  RecordOrderExecsForLiveOrderIds(const HFSAT::TradeType_t _buysell_,
                                  int size_exec_, double trade_px_) {}
  virtual int GetTotalSizeForAllLiveOrders() { return 0; }
  virtual void SendSlackNotif(std::string alert_message) {}
  virtual void DumpSnapshot() {}

  void CancelAllOrders() { p_smart_order_manager_->CancelAllOrders(); }

  int GetDate() { return param_->yyyymmdd_; }

  void SetPassiveParam() {
    param_->exec_algo_ = kPassOnly;
    return;
  }

  void SetPassAggParam() {
    param_->exec_algo_ = kPassAndAgg;
    return;
  }

  void SetAggressiveParam() {
    param_->exec_algo_ = kAggOnly;
    return;
  }

  void SetAggSpreadFactor(double x) {
    param_->agg_sprd_factor_ = x;
    return;
  }
};
}
