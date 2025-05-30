/**
    \file ExecLogic/structured_price_based_aggressive_trading.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#ifndef BASE_EXECLOGIC_STRUCTURED_PRICE_BASED_AGGRESSIVE_TRADING_H
#define BASE_EXECLOGIC_STRUCTURED_PRICE_BASED_AGGRESSIVE_TRADING_H

#include "dvctrade/ExecLogic/base_trading.hpp"
#include "dvctrade/ExecLogic/structured_trading_manager.hpp"

namespace HFSAT {

/// Class to trade with a target price that is very dependant on top level nonself marketprice
///
/// the base price moves sharply with market price and so does the target price
/// the targetprice is thus compared to bid price and ask price ( for getting an opinion to place trades )
/// and not midprice or a price that is much more stable than market weighted price
class StructuredPriceBasedAggressiveTrading : public virtual BaseTrading, public StructuredTradingManagerListener {
 protected:
  StructuredTradingManager* stm_;
  int my_margin_risk_;
  int my_max_global_risk_;
  int max_global_risk_;
  bool getflat_due_to_max_global_risk_;
  bool should_be_getting_intraday_flat_;

 public:
  StructuredPriceBasedAggressiveTrading(DebugLogger& _dbglogger_, const Watch& _watch_,
                                        const SecurityMarketView& _dep_market_view_, SmartOrderManager& _order_manager_,
                                        const std::string& _paramfilename_, const bool _livetrading_,
                                        MulticastSenderSocket* _p_strategy_param_sender_socket_,
                                        EconomicEventsManager& t_economic_events_manager_,
                                        const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_,
                                        const int t_runtime_id_,
                                        const std::vector<std::string> _this_model_source_shortcode_vec_,
                                        StructuredTradingManager* structured_trading_manager_);

  ~StructuredPriceBasedAggressiveTrading(){};  ///< destructor not made virtual ... please do so when making child
  /// classes

  static std::string StrategyName() {
    return "StructuredPriceBasedAggressiveTrading";
  }  ///< used in trade_init to check which strategy is to be initialized

  void UpdateRisk(int t_risk_, int t_margin_risk_);

  inline void UpdateMaxGlobalRisk(int _new_max_global_risk_) {
    max_global_risk_ = _new_max_global_risk_;
    BuildTradeVarSets();
  }

  void OnTimePeriodUpdate(const int num_pages_to_add_);

  inline void OnControlUpdate(const HFSAT::ControlMessage& _control_message_, const char* symbol, int trader_id) {
    BaseTrading::OnControlUpdate(_control_message_, symbol, trader_id);
  }

  inline bool SetPositionOffset(int t_position_offset_) {
    DBGLOG_TIME_CLASS_FUNC_LINE << "StructuredPriceBasedAggressiveTrading" << DBGLOG_ENDL_FLUSH;
    return BaseTrading::SetPositionOffset(t_position_offset_);
  }
  void OnPositionChange(int t_new_position_, int position_diff_, const unsigned int _security_id_);

 protected:
  void TradingLogic();  ///< All the strategy based trade execution is written here

  ///< Function to map position to map_index and then the map_index is used to load the
  void TradeVarSetLogic(int t_position);
  /// current_tradevarset_

  void GetFlatMaxPositionTradingLogic();  ///< We expect this to get triggered quite often during the day so

  void BuildPositionTradeVarSetMap(int _index_);

  bool ShouldBeGettingIntradayFlat();

  void ProcessIntradayGetFlat();

  bool ShouldBeGettingFlat();

  void IntradayGetFlatTradingLogic();

  void GetFlatTradingLogic();  ///< Trade Selection and execution during getflat mode

  inline SmartOrderManager& order_manager() { return order_manager_; }

  void PrintFullStatus();
};
}
#endif  // BASE_EXECLOGIC_STRUCTURED_PRICE_BASED_AGGRESSIVE_TRADING_H
