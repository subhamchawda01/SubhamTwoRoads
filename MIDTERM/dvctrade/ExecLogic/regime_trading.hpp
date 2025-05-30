/**
    \file ExecLogic/regime_trading.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_EXECLOGIC_REGIME_TRADING_H
#define BASE_EXECLOGIC_REGIME_TRADING_H

#include "dvctrade/ExecLogic/base_trading.hpp"
#include "dvctrade/ExecLogic/price_based_aggressive_trading.hpp"
#include "dvctrade/ExecLogic/directional_aggressive_trading.hpp"
#include "dvctrade/ExecLogic/price_based_vol_trading.hpp"

namespace HFSAT {

/// Class to trade with a target price that is very dependant on top level nonself marketprice
///
/// the base price moves sharply with market price and so does the target price
/// the targetprice is thus compared to bid price and ask price ( for getting an opinion to place trades )
/// and not midprice or a price that is much more stable than market weighted price
class RegimeTrading : public PriceBasedAggressiveTrading,
                      public DirectionalAggressiveTrading,
                      public PriceBasedVolTrading {
 public:
  RegimeTrading(DebugLogger &_dbglogger_, const Watch &_watch_, const SecurityMarketView &_dep_market_view_,
                SmartOrderManager &_order_manager_, const std::string &_paramfilename_, const bool _livetrading_,
                MulticastSenderSocket *_p_strategy_param_sender_socket_,
                EconomicEventsManager &t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                const int t_trading_end_utc_mfm_, const int t_runtime_id_,
                const std::vector<std::string> _this_model_source_shortcode_vec_, std::string _strategy_name_);

  ~RegimeTrading(){};  ///< destructor not made virtual ... please do so when making child classes

  static std::string StrategyName() {
    return "RegimeTrading";
  }  ///< used in trade_init to check which strategy is to be initialized
  static bool IsRegimeStrategy(std::string _strategy_name_);
  std::vector<int> regime_to_execlogic_index_;  // 0 for pbat, 1 for dat 2 for vol

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

 protected:
  void OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_, const double _price_,
              const int r_int_price_, const int _security_id_);
  void OnCancelReject(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                      const int _security_id_);

  void SetExecLogicForRegimes(std::string _strategy_name_);
  void TradingLogic();  ///< All the strategy based trade execution is written here

  void CallPlaceCancelNonBestLevels();
  void PlaceCancelNonBestLevels();
  void PrintFullStatus();
};
}
#endif  // BASE_EXECLOGIC_PRICE_BASED_AGGRESSIVE_TRADING_H
