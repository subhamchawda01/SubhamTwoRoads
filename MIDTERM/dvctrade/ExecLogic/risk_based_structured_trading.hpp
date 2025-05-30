/**
   \file ExecLogic/price_based_aggressive_trading.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_EXECLOGIC_RISK_BASED_STRUCTURED_TRADING_H
#define BASE_EXECLOGIC_RISK_BASED_STRUCTURED_TRADING_H

#include "dvctrade/ExecLogic/base_trading.hpp"
#include "dvctrade/ExecLogic/curve_trading_manager.hpp"

namespace HFSAT {

/// Class to trade with a target price that is very dependant on top level nonself marketprice
///
/// the base price moves sharply with market price and so does the target price
/// the targetprice is thus compared to bid price and ask price ( for getting an opinion to place trades )
/// and not midprice or a price that is much more stable than market weighted price
class RiskBasedStructuredTrading : public virtual BaseTrading, public CurveTradingManagerListener {
 protected:
  CurveTradingManager &lfi_trading_manager_;
  int max_global_risk_;

 public:
  RiskBasedStructuredTrading(DebugLogger &_dbglogger_, const Watch &_watch_,
                             const SecurityMarketView &_dep_market_view_, SmartOrderManager &_order_manager_,
                             const std::string &_paramfilename_, const bool _livetrading_,
                             MulticastSenderSocket *_p_strategy_param_sender_socket_,
                             EconomicEventsManager &t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                             const int t_trading_end_utc_mfm_, const int t_runtime_id_,
                             const std::vector<std::string> _this_model_source_shortcode_vec_,
                             CurveTradingManager &_lfi_trading_manager_);

  ~RiskBasedStructuredTrading(){};  ///< destructor not made virtual ... please do so when making child classes

  unsigned GetSecId() { return dep_market_view_.security_id(); }

  SmartOrderManager &order_manager() { return order_manager_; }

  inline void UpdateRiskPosition(int _new_inflrisk_position_, int _new_minrisk_position_,
                                 bool _reset_bid_trade_size_ = true, bool _reset_ask_trade_size_ = true) {
    minimal_risk_position_ = _new_minrisk_position_;
    my_risk_ = _new_inflrisk_position_;
    TradeVarSetLogic(my_risk_);
    UpdateTarget(target_price_, targetbias_numbers_);  // can be avoided but in case risk is updated from other
                                                       // securities, it is better to be called
  }

  inline void OnControlUpdate(const ControlMessage &_control_message_, const char *symbol, int trader_id) {
    BaseTrading::OnControlUpdate(_control_message_, symbol, trader_id);
  }

  void OnTimePeriodUpdate(const int num_pages_to_add_);

  static std::string StrategyName() {
    return "RiskBasedStructuredTrading";
  }  ///< used in trade_init to check which strategy is to be initialized
  void OnPositionChange(int t_new_position_, int position_diff_, const unsigned int _security_id_);
  void Refresh() override {}
  int runtime_id() override { return runtime_id_; }

 protected:
  void TradingLogic();  ///< All the strategy based trade execution is written here
  void PrintFullStatus();
  void TradeVarSetLogic(int t_position);
  void GetFlatTradingLogic();
  bool ShouldBeGettingFlat();
  void BuildPositionTradeVarSetMap(int _index_);
};
}
#endif  // BASE_EXECLOGIC_RISK_BASED_STRUCTURED_TRADING_H
