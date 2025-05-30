/**
    \file ExecLogic/price_based_trading.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_EXECLOGIC_VOL_MM_TRADING_H
#define BASE_EXECLOGIC_VOL_MM_TRADING_H

#include "dvctrade/ExecLogic/base_trading.hpp"

#define ALLOW_MODIFY_ORDER 1
/// meant for volatile stuff ..
/// main differences are
/// a) maintaining supporting orders at strict price band intervals
/// b) top levels adjusted by volatility of product
/// c) Not having race conditions in place/cancel of orders

namespace HFSAT {
class VolMMTrading : public virtual BaseTrading {
 protected:
 public:
  VolMMTrading(DebugLogger& dbglogger, const Watch& watch, const SecurityMarketView& dep_market_view,
               SmartOrderManager& order_manager, const std::string& paramfilename, const bool livetrading,
               MulticastSenderSocket* p_strategy_param_sender_socket, EconomicEventsManager& economic_events_manager,
               const int trading_start_utc_mfm, const int trading_end_utc_mfm, const int runtime_id,
               const std::vector<std::string> this_model_source_shortcode_vec);

  virtual ~VolMMTrading() {}

  static std::string StrategyName() {
    return "VolMMTrading";
  }  ///< used in trade_init to check which strategy is to be initialized

  void OnStdevUpdate(const unsigned int security_id, const double& new_stdev_val) {
    double t_new_stdev_val = new_stdev_val;
    if (param_set_.use_sqrt_stdev_) {
      // to slowdown the impact of stdev on threshholds
      t_new_stdev_val =
          dep_market_view_.min_price_increment() * (sqrt(new_stdev_val / dep_market_view_.min_price_increment()));
    }

    m_stdev_ = std::min(param_set_.stdev_fact_ticks_ * t_new_stdev_val, param_set_.stdev_cap_);
    m_stdev_ *= dep_market_view_.min_price_increment();

    num_stdev_calls_++;
    sum_stdev_calls_ += new_stdev_val;
  }

 protected:
  double m_stdev_, spread_diff_factor_, hist_avg_spread_;
  /// tradeingLogic specific vars
  double base_bid_price_, base_ask_price_;
  double best_bid_place_cxl_px_, best_ask_place_cxl_px_;
  int best_int_bid_place_cxl_px_, best_int_ask_place_cxl_px_;
  int base_iter_bid_px_, base_iter_ask_px_;
  int tot_buy_placed_, tot_sell_placed_;
  int band_level_;
  int low_band_px_, current_px_;
  int current_band_ordered_sz_;
  int current_band_target_sz_;

  int retval_;
  int bid_retval_;
  int ask_retval_;

  /// timestamps to enforce agg/improve cooloff
  int last_bid_agg_msecs_, last_ask_agg_msecs_;
  int last_bid_imp_msecs_, last_ask_imp_msecs_;

  int cancel_bids_above_, cancel_asks_above_;

  std::vector<int> int_px_to_be_placed_at_vec_;
  std::vector<int> size_to_be_placed_vec_;

  bool use_stable_book_;
  void TradingLogic();  ///< All the strategy based trade execution is written here
  void CallPlaceCancelNonBestLevels() {}
  void PlaceCancelNonBestLevels() {}

  /// override function of basetrader - unconsitional setting of buy/sell msecs
  void OnExec(const int new_position, const int exec_quantity, const TradeType_t buysell, const double price,
              const int int_price, const int security_id);
  void OnCancelReject(const TradeType_t buysell, const double price, const int int_price, const int security_id);

  void PrintFullStatus();
  char GetOrderLevelIndicator(TradeType_t order_side, int int_order_px);
  void SetCancelThresholds();
  void SetImproveBidAskPrices();

  void CheckAggressCoolOff();
  void CheckImproveCoolOff();
  void CheckORSMDSDelay();
  void BidCancelLogic();
  void AskCancelLogic();
};
}
#endif  // BASE_EXECLOGIC_PRICE_BASED_VOL_TRADING_H
