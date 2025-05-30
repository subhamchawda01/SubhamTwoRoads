/**
    \file ExecLogic/regime_trading.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include "dvctrade/ExecLogic/base_trading.hpp"
#include "dvctrade/ExecLogic/curve_trading_manager.hpp"
#include "dvctrade/ExecLogic/directional_aggressive_trading.hpp"
#include "dvctrade/ExecLogic/price_based_aggressive_pro_rata_trading.hpp"
#include "dvctrade/ExecLogic/price_based_aggressive_trading.hpp"
#include "dvctrade/ExecLogic/price_based_vol_trading.hpp"

namespace HFSAT {

/// Class to trade with a target price that is very dependant on top level nonself marketprice
///
/// the base price moves sharply with market price and so does the target price
/// the targetprice is thus compared to bid price and ask price ( for getting an opinion to place trades )
/// and not midprice or a price that is much more stable than market weighted price
class StructuredGeneralTrading : public PriceBasedAggressiveTrading,
                                 public DirectionalAggressiveTrading,
                                 public PriceBasedVolTrading,
                                 public PriceBasedAggressiveProRataTrading,
                                 public CurveTradingManagerListener {
 public:
  StructuredGeneralTrading(DebugLogger &_dbglogger_, const Watch &_watch_, const SecurityMarketView &_dep_market_view_,
                           SmartOrderManager &_order_manager_, const std::string &_paramfilename_,
                           const bool _livetrading_, MulticastSenderSocket *_p_strategy_param_sender_socket_,
                           EconomicEventsManager &t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                           const int t_trading_end_utc_mfm_, const int t_runtime_id_,
                           const std::vector<std::string> _this_model_source_shortcode_vec_,
                           std::string _strategy_name_, CurveTradingManager *trading_manager_);

  ~StructuredGeneralTrading(){};  ///< destructor not made virtual ... please do so when making child classes

  ///< used in trade_init to check which strategy is to be initialized
  static std::string StrategyName() { return "StructuredGeneralTrading"; }

  static bool IsStructuredGeneralTrading(const std::string &strategy_name) {
    // Expecting LFITradingManager-StructuredGeneralTrading-PriceBasedAggressiveTrading
    auto start_id = strategy_name.find("-", 0);
    auto end_id = strategy_name.find("-", start_id + 1);
    if (strategy_name.substr(start_id + 1, end_id - start_id - 1) == StrategyName()) {
      return true;
    }
    return false;
  }

  static std::string GetTradingManager(const std::string &strategy_name) {
    auto id = strategy_name.find("-", 0);
    std::string t_trading_manager_ = "";
    if (strategy_name.find("-", 0) != std::string::npos) t_trading_manager_ = strategy_name.substr(0, id);
    return t_trading_manager_;
  }

  static bool IsRegimeStrategy(std::string _strategy_name_);
  int execlogic_to_use_;
  int position_to_close_;
  bool getting_flat_;

  bool disallow_position_in_bid_;
  bool disallow_position_in_ask_;
  CurveTradingManager *trading_manager_;
  bool trade_varset_logic_from_structured_;

  inline void OnStdevUpdate(const unsigned int security_id, const double &new_stdev_val) {
    // implementing it here as since PriceBasedVolTrading exteneds BaseTrading virtually and then implements
    // OnStdevUpdate,
    // SlowStdevCalculator calls OnStdevUpdate of PbVol irrespective of what exec-logics are used here
    stdev_ = new_stdev_val;
    double t_new_stdev_val_ = new_stdev_val;

    if (param_set_.use_sqrt_stdev_) {
      // to slowdown the impact of stdev on threshholds
      // Optimizing here , actual formula was tick_value * sqrt ( stdev/min_price )
      t_new_stdev_val_ = (sqrt(dep_market_view_.min_price_increment() * new_stdev_val));
    }

    // stdev_scaled_capped_in_ticks_ and stdev_scaled_capped_in_ticks_, used for mainly scaling thresholds
    stdev_scaled_capped_in_ticks_ = std::min(
        param_set_.stdev_overall_cap_,
        std::min(param_set_.stdev_cap_, param_set_.stdev_fact_ticks_ * t_new_stdev_val_) * last_day_vol_ratio_);
  }

  inline void OnControlUpdate(const ControlMessage &_control_message_, const char *symbol, int trader_id) {
    BaseTrading::OnControlUpdate(_control_message_, symbol, trader_id);
  }

  inline void UpdateRiskPosition(int _new_inflrisk_position_, int _new_minrisk_position_,
                                 bool _cancel_bid_orders_ = false, bool _cancel_ask_orders_ = false) override {
    minimal_risk_position_ = _new_minrisk_position_;
    my_risk_ = _new_inflrisk_position_;
    trade_varset_logic_from_structured_ = true;
    TradeVarSetLogic(my_risk_);
    trade_varset_logic_from_structured_ = false;
    if (_cancel_bid_orders_) {
      current_tradevarset_.l1bid_trade_size_ = 0;
    }
    if (_cancel_ask_orders_) {
      current_tradevarset_.l1ask_trade_size_ = 0;
    }
    if (is_ready_ && !external_freeze_trading_ && !should_be_getting_flat_ && dep_market_view_.is_ready() &&
        (throttle_manager_->allowed_through_throttle(watch_.msecs_from_midnight()))) {
      TradingLogic();
    }
  }
  inline int PnlAtClose() override { return pnl_at_close_; };
  inline int PositionAtClose() override { return position_at_close_; };
  void OnPositionChange(int t_new_position_, int position_diff_, const unsigned int _security_id_);

  SmartOrderManager &order_manager() override { return order_manager_; }
  const SecurityMarketView *smv() override { return &dep_market_view_; }
  const ParamSet *paramset() override { return &param_set_; }
  void SetExecLogic(const std::string &_strategy_name_);

  void OnTimePeriodUpdate(const int num_pages_to_add_) override;
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_) override;

  inline void set_zero_bid_di_trade() override {
    current_tradevarset_.l1bid_trade_size_ = 0;
    // order_manager_.CancelBidsEqAboveIntPrice(dep_market_view_.bestbid_int_price());
    // last_ors_exec_ask_cancel_msecs_ = watch_.msecs_from_midnight();
  }
  inline void set_zero_ask_di_trade() override {
    current_tradevarset_.l1ask_trade_size_ = 0;
    // order_manager_.CancelAsksEqAboveIntPrice(dep_market_view_.bestask_int_price());
    // last_ors_exec_ask_cancel_msecs_ = watch_.msecs_from_midnight();
  }

  void set_getflat_due_to_max_position(bool flag) override { getflat_due_to_max_position_ = flag; }
  bool getflat_due_to_max_position() override { return getflat_due_to_max_position_; }

  void set_getflat_due_to_non_standard_market_conditions(bool flag) override {
    getflat_due_to_non_standard_market_conditions_ = flag;
  }
  bool getflat_due_to_non_standard_market_conditions() override {
    return getflat_due_to_non_standard_market_conditions_;
  }

  void Refresh() override { ProcessGetFlat(); }
  void DisAllowOneSideTrade(TradeType_t buysell) override;
  void AllowTradesForSide(TradeType_t buysell) override;
  inline bool IsDisallowed() override { return disallow_position_in_bid_ || disallow_position_in_ask_; };
  int runtime_id() override { return runtime_id_; }

  void TradeVarSetLogic(int t_position) override;

 protected:
  void OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_, const double _price_,
              const int r_int_price_, const int _security_id_);
  void OnCancelReject(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                      const int _security_id_);

  void TradingLogic();  ///< All the strategy based trade execution is written here

  int GetPositionToClose() override { return position_to_close_; }
  void CallPlaceCancelNonBestLevels();
  void PlaceCancelNonBestLevels();
  void PrintFullStatus();
};
}
