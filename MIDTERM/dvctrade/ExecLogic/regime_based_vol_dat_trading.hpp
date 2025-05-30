/**
    \file ExecLogic/price_based_trading.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_EXECLOGIC_REGIME_BASED_VOL_DAT_TRADING_H
#define BASE_EXECLOGIC_REGIME_BASED_VOL_DAT_TRADING_H

#include "dvctrade/ExecLogic/base_trading.hpp"

/// meant for volatile stuff ..
/// main differences are
/// a) maintaining strict supporting orders at price band intervals
/// b) top levels adjusted by volatility of product
namespace HFSAT {
class RegimeBasedVolDatTrading : public virtual BaseTrading {
 protected:
 public:
  RegimeBasedVolDatTrading(DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
                           SmartOrderManager& _order_manager_, const std::string& _paramfilename_,
                           const bool _livetrading_, MulticastSenderSocket* _p_strategy_param_sender_socket_,
                           EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                           const int t_trading_end_utc_mfm_, const int _runtime_id_,
                           const std::vector<std::string> _this_model_source_shortcode_vec_);

  ~RegimeBasedVolDatTrading() {}

  static std::string StrategyName() {
    return "RegimeBasedVolDatTrading";
  }  ///< used in trade_init to check which strategy is to be initialized

  void OnStdevUpdate(const unsigned int security_id_, const double& _new_stdev_val_) {
    m_stdev_ = std::min(param_set_.stdev_fact_ * _new_stdev_val_, param_set_.stdev_cap_);
    num_stdev_calls_++;
    sum_stdev_calls_ += _new_stdev_val_;

    if (stdev_calculator_ != NULL) {
      BaseTrading::OnStdevUpdate(security_id_, _new_stdev_val_);
    }
  }

 protected:
  double m_stdev_;
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

  void TradingLogic();  ///< All the strategy based trade execution is written here
  void VolTradingLogic();
  void DatTradingLogic();

  void CallPlaceCancelNonBestLevels() {
    if (param_index_to_use_ == 1u) {
      BaseTrading::CallPlaceCancelNonBestLevels();
    }
  }
  void PlaceCancelNonBestLevels() {
    if (param_index_to_use_ == 1u) {
      BaseTrading::PlaceCancelNonBestLevels();
    }
  }

  /// override function of basetrader - unconsitional setting of buy/sell msecs
  void OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_, const double _price_,
              const int r_int_price_, const int _security_id_);
  void OnCancelReject(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                      const int _security_id_);

  void PrintFullStatus();
  char GetOrderLevelIndicator(TradeType_t order_side, int int_order_px);

  void SetShouldIncreaseThreshold() {
    if (param_index_to_use_ == 0u) {
      should_increase_thresholds_in_volatile_times_ = false;
    } else if (param_index_to_use_ == 1u) {
      should_increase_thresholds_in_volatile_times_ = true;
    }
  }
};
}
#endif  // BASE_EXECLOGIC_PRICE_BASED_VOL_TRADING_H
