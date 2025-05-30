/**
    \file ExecLogic/price_based_trading.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_EXECLOGIC_PRICE_BASED_VOL_TRADING_H
#define BASE_EXECLOGIC_PRICE_BASED_VOL_TRADING_H

#include "dvctrade/ExecLogic/base_trading.hpp"
#include "dvccode/CommonTradeUtils/sample_data_util.hpp"

#define ALLOW_MODIFY_ORDER 1
/// meant for volatile stuff ..
/// main differences are
/// a) maintaining strict supporting orders at price band intervals
/// b) top levels adjusted by volatility of product
namespace HFSAT {
class PriceBasedVolTrading : public virtual BaseTrading, public IndicatorListener {
 protected:
 public:
  PriceBasedVolTrading(DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
                       SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
                       MulticastSenderSocket* _p_strategy_param_sender_socket_,
                       EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                       const int t_trading_end_utc_mfm_, const int _runtime_id_,
                       const std::vector<std::string> _this_model_source_shortcode_vec_);

  ~PriceBasedVolTrading() {}

  static std::string StrategyName() {
    return "PriceBasedVolTrading";
  }  ///< used in trade_init to check which strategy is to be initialized

  void OnStdevUpdate(const unsigned int security_id_, const double& _new_stdev_val_) {
    double t_new_stdev_val_ = _new_stdev_val_;
    if (param_set_.use_sqrt_stdev_) {
      // to slowdown the impact of stdev on threshholds
      // Optimizing here , actual formula was tick_value * sqrt ( stdev/min_price )
      t_new_stdev_val_ = (sqrt(dep_market_view_.min_price_increment() * _new_stdev_val_));
    }

    m_stdev_ = std::min(param_set_.stdev_fact_ticks_ * t_new_stdev_val_, param_set_.stdev_cap_);
    m_stdev_ *= dep_market_view_.min_price_increment();

    num_stdev_calls_++;
    sum_stdev_calls_ += _new_stdev_val_;
  }
  virtual void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
    if (_indicator_index_ == 1) moving_avg_dep_bidask_spread_ = _new_value_;
  }

  virtual void OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value_decrease,
                                 const double& new_value_nochange, const double& new_value_increase){};

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
  bool agg_only_no_improve_;
  int last_bid_agg_msecs_, last_ask_agg_msecs_;
  int last_bid_imp_msecs_, last_ask_imp_msecs_;

  int cancel_bids_above_, cancel_asks_above_;

  std::vector<double> px_to_be_placed_at_vec_;
  std::vector<int> int_px_to_be_placed_at_vec_;
  std::vector<int> size_to_be_placed_vec_;
  std::vector<char> order_level_indicator_vec_;
  bool use_stable_book_;
  void TradingLogic();  ///< All the strategy based trade execution is written here
  void CallPlaceCancelNonBestLevels() {}
  void PlaceCancelNonBestLevels() {}

  /// override function of basetrader - unconsitional setting of buy/sell msecs
  void OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_, const double _price_,
              const int r_int_price_, const int _security_id_);
  void OnCancelReject(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                      const int _security_id_);

  void PrintFullStatus();
  char GetOrderLevelIndicator(TradeType_t order_side, int int_order_px);
  void SetCancelThresholds();
  void SetImproveBidAskPrices();
  void SetAggNoImproveBidAskPrices();

  void CheckAggressCoolOff();
  void CheckImproveCoolOff();
  void CheckORSMDSDelay();
  void BidCancelLogic();
  void AskCancelLogic();
};
}
#endif  // BASE_EXECLOGIC_PRICE_BASED_VOL_TRADING_H
