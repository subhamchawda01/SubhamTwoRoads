/**
   \file ExecLogic/price_based_aggressive_trading.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#ifndef BASE_EXECLOGIC_MIN_RISK_PRICE_BASED_AGGRESSIVE_TRADING_H
#define BASE_EXECLOGIC_MIN_RISK_PRICE_BASED_AGGRESSIVE_TRADING_H

#include "dvctrade/ExecLogic/base_trading.hpp"
#include "dvctrade/ExecLogic/minrisk_trading_manager.hpp"
namespace HFSAT {

/// Class to trade with a target price that is very dependant on top level nonself marketprice
///
/// the base price moves sharply with market price and so does the target price
/// the targetprice is thus compared to bid price and ask price ( for getting an opinion to place trades )
/// and not midprice or a price that is much more stable than market weighted price
class MinRiskPriceBasedAggressiveTrading : public virtual BaseTrading, public MinRiskTradingManagerListener {
 protected:
  int min_risk_pos = 0;
  MinRiskTradingManager& mrtm;

 public:
  MinRiskPriceBasedAggressiveTrading(
      DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
      SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
      MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
      const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int t_runtime_id_,
      const std::vector<std::string> _this_model_source_shortcode_vec_, MinRiskTradingManager& mrt);

  ~MinRiskPriceBasedAggressiveTrading(){};  ///< destructor not made virtual ... please do so when making child classes

  static std::string StrategyName() {
    return "MinRiskPriceBasedAggressiveTrading";
  }  ///< used in trade_init to check which strategy is to be initialized
  void UpdateBetaPosition(const unsigned int sec_id_, int _new_position_);

 protected:
  const SmartOrderManager& order_manager() { return order_manager_; }

  void TradingLogic();  ///< All the strategy based trade execution is written here

  void TradeVarSetLogic(int t_position);
  virtual void OnPositionUpdate(int beta_position_, int min_risk_pos);
  int GetSecId();
  std::string GetShortCode();
  void PrintFullStatus();
  void GetFlatTradingLogic();
  void GetFlatFokTradingLogic();
  void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol, const int trader_id);
  bool ShouldBeGettingFlat();
  void OnTimePeriodUpdate(const int num_pages_to_add_);
};
}
#endif  // BASE_EXECLOGIC_PRICE_BASED_AGGRESSIVE_TRADING_H
