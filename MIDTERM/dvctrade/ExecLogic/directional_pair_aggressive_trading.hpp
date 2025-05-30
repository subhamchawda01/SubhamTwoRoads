/**
    \file ExecLogic/directional_pair_agrressive_trading.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_EXECLOGIC_DIRECTIONAL_PAIR_AGGRESSIVE_TRADING_H
#define BASE_EXECLOGIC_DIRECTIONAL_PAIR_AGGRESSIVE_TRADING_H

#include "dvctrade/ExecLogic/base_trading.hpp"

namespace HFSAT {

/// @brief Class to trade with a target price that is not dependant on top level sizes
///
/// Here we are trying to predit direction of trading. And compare the target price against the midprice.
class DirectionalPairAggressiveTrading : public virtual BaseTrading {
 protected:
 public:
  DirectionalPairAggressiveTrading(DebugLogger& _dbglogger_, const Watch& _watch_,
                                   const SecurityMarketView& _dep_market_view_,
                                   const SecurityMarketView& _indep_market_view_, SmartOrderManager& _order_manager_,
                                   const std::string& _paramfilename_, const bool _livetrading_,
                                   MulticastSenderSocket* _p_strategy_param_sender_socket_,
                                   EconomicEventsManager& t_economic_events_manager_,
                                   const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_,
                                   const int _runtime_id_,
                                   const std::vector<std::string> _this_model_source_shortcode_vec_);

  /// destructor not made virtual ... please do so when making child classes
  ~DirectionalPairAggressiveTrading(){};

  static std::string StrategyName() {
    return "DirectionalPairAggressiveTrading";
  }  ///< used in trade_init to check which strategy is to be initialized

 protected:
  const SecurityMarketView& indep_market_view_;

  void TradingLogic();  ///< All the strategy based trade execution is written here

  void PrintFullStatus();
};
}
#endif  // BASE_EXECLOGIC_DIRECTIONAL_PAIR_AGGRESSIVE_TRADING_H
