/*
 * price_based_leg_trading.hpp
 *
 *  Created on: 14-May-2014
 *      Author: archit
 */

#ifndef PRICE_BASED_LEG_TRADING_HPP_
#define PRICE_BASED_LEG_TRADING_HPP_

#include "dvctrade/ExecLogic/base_trading.hpp"
#include "dvctrade/ExecLogic/spread_trading_manager.hpp"

namespace HFSAT {

class PriceBasedLegTrading : public virtual BaseTrading, public SpreadTradingManagerListener {
 protected:
  SpreadTradingManager& spread_trading_manager_;
  int ideal_spread_position_;

 public:
  PriceBasedLegTrading(DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
                       SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
                       MulticastSenderSocket* _p_strategy_param_sender_socket_,
                       EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                       const int t_trading_end_utc_mfm_, const int t_runtime_id_,
                       const std::vector<std::string> _this_model_source_shortcode_vec_,
                       SpreadTradingManager& _spread_trading_manager_);

  ~PriceBasedLegTrading(){};  ///< destructor not made virtual ... please do so when making child classes

  static std::string StrategyName() {
    return "PriceBasedLegTrading";
  }  ///< used in trade_init to check which strategy is to be initialized

  void OnPositionUpdate(int _ideal_spread_position_);

  const SmartOrderManager& order_manager() { return order_manager_; }

  unsigned int GetSecId() { return dep_market_view_.security_id(); }
  void OnPositionChange(int t_new_position_, int position_diff_, const unsigned int _security_id_);

 protected:
  void TradingLogic();  ///< All the strategy based trade execution is written here

  void PrintFullStatus();

  void TradeVarSetLogic(int t_position);

  inline int GetPositionToClose() { return (my_position_ - ideal_spread_position_); }
};

} /* namespace HFSAT */

#endif /* PRICE_BASED_LEG_TRADING_HPP_ */
