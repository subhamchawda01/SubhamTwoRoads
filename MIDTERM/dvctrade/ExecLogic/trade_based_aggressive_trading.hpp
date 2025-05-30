/**
    \file ExecLogic/trade_based_aggressive_trading.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_EXECLOGIC_TRADE_BASED_AGGRESSIVE_TRADING_H
#define BASE_EXECLOGIC_TRADE_BASED_AGGRESSIVE_TRADING_H

#include "dvctrade/ExecLogic/base_trading.hpp"

namespace HFSAT {

/// Class to trade with a target price that is very dependant on the trade-price of the dependant
///
class TradeBasedAggressiveTrading : public virtual BaseTrading {
 protected:
 public:
  TradeBasedAggressiveTrading(DebugLogger& _dbglogger_, const Watch& _watch_,
                              const SecurityMarketView& _dep_market_view_, SmartOrderManager& _order_manager_,
                              const std::string& _paramfilename_, const bool _livetrading_,
                              MulticastSenderSocket* _p_strategy_param_sender_socket_,
                              EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                              const int t_trading_end_utc_mfm_, const int t_runtime_id_,
                              const std::vector<std::string> _this_model_source_shortcode_vec_);

  ~TradeBasedAggressiveTrading(){};  ///< destructor not made virtual ... please do so when making child classes

  static std::string StrategyName() {
    return "TradeBasedAggressiveTrading";
  }  ///< used in trade_init to check which strategy is to be initialized

 protected:
  void TradingLogic();  ///< All the strategy based trade execution is written here

  void PrintFullStatus();
};
}
#endif  // BASE_EXECLOGIC_TRADE_BASED_AGGRESSIVE_TRADING_H
