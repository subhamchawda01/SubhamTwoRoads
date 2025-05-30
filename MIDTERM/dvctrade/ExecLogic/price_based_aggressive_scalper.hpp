/**
    \file ExecLogic/price_based_aggressive_scalper.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_EXECLOGIC_PRICE_BASED_AGGRESSIVE_SCALPER_H
#define BASE_EXECLOGIC_PRICE_BASED_AGGRESSIVE_SCALPER_H

#include "dvctrade/ExecLogic/base_trading.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator.hpp"
#include "baseinfra/SmartOrderRouting/base_pnl.hpp"
namespace HFSAT {

/// Class to trade with a target price that is very dependant on top level nonself marketprice
///
/// the base price moves sharply with market price and so does the target price
/// the targetprice is thus compared to bid price and ask price ( for getting an opinion to place trades )
/// and not midprice or a price that is much more stable than market weighted price
class PriceBasedAggressiveScalper : public virtual BaseTrading {
 protected:
 public:
  PriceBasedAggressiveScalper(DebugLogger& _dbglogger_, const Watch& _watch_,
                              const SecurityMarketView& _dep_market_view_, SmartOrderManager& _order_manager_,
                              const std::string& _paramfilename_, const bool _livetrading_,
                              MulticastSenderSocket* _p_strategy_param_sender_socket_,
                              EconomicEventsManager& t_economic_events_manager_,
                              MarketUpdateManager& t_market_update_manager_, const int t_trading_start_utc_mfm_,
                              const int t_trading_end_utc_mfm_, const int _runtime_id_,
                              const std::vector<std::string> _this_model_source_shortcode_vec_);

  virtual ~PriceBasedAggressiveScalper() {}

  static std::string StrategyName() {
    return "PriceBasedAggressiveScalper";
  }  ///< used in trade_init to check which strategy is to be initialized

 protected:
  virtual void TradingLogic();  ///< All the strategy based trade execution is written here

  void PrintFullStatus();
};
}
#endif  // BASE_EXECLOGIC_PRICE_BASED_AGGRESSIVE_SCALPER_H
