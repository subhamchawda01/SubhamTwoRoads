/**
    \file ExecLogic/price_based_spread_trading.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_EXECLOGIC_PRICE_BASED_SPREAD_TRADING_H
#define BASE_EXECLOGIC_PRICE_BASED_SPREAD_TRADING_H

#include "dvctrade/ExecLogic/base_trading.hpp"
#include "dvctrade/ExecLogic/spread_trading_manager.hpp"

namespace HFSAT {

/// Class to trade with a target price that is very dependant on top level nonself marketprice
///
/// the base price moves sharply with market price and so does the target price
/// the targetprice is thus compared to bid price and ask price ( for getting an opinion to place trades )
/// and not midprice or a price that is much more stable than market weighted price
class PriceBasedSpreadTrading : public virtual BaseTrading, public SpreadTradingManagerListener {
 protected:
  SpreadTradingManager& spread_trading_manager_;
  const SpreadMarketView& spread_market_view_;

 public:
  PriceBasedSpreadTrading(DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
                          SmartOrderManager& _order_manager_, const std::string& _paramfilename_,
                          const bool _livetrading_, MulticastSenderSocket* _p_strategy_param_sender_socket_,
                          EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                          const int t_trading_end_utc_mfm_, const int t_runtime_id_,
                          const std::vector<std::string> _this_model_source_shortcode_vec_,
                          SpreadTradingManager& _spread_trading_manager_);

  ~PriceBasedSpreadTrading(){};  ///< destructor not made virtual ... please do so when making child classes

  static std::string StrategyName() {
    return "PriceBasedSpreadTrading";
  }  ///< used in trade_init to check which strategy is to be initialized

  inline void OnPositionUpdate(int _ideal_spread_position_) {
    // nothing need to be done
  }

  const SmartOrderManager& order_manager() { return order_manager_; }

  unsigned int GetSecId() { return dep_market_view_.security_id(); }

  bool UpdateTarget(double _new_target_, double _targetbias_numbers_);  ///< Called from BaseModelMath

  void OnPositionChange(int t_new_position_, int position_diff_, const unsigned int _security_id_);

 protected:
  void TradingLogic();  ///< All the strategy based trade execution is written here

  void PrintFullStatus();
};
}
#endif  // BASE_EXECLOGIC_PRICE_BASED_SPREAD_TRADING_H
