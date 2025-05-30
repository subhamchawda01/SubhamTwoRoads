// =====================================================================================
//
//       Filename:  indicator_helper.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  12/21/2015 04:34:43 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvctrade/ExecLogic/base_trading_defines.hpp"
#include "dvctrade/ExecLogic/exec_logic_utils.hpp"
#include "dvctrade/ExecLogic/instrument_info.hpp"
#include "dvctrade/Indicators/indicator_list.hpp"
#include "dvctrade/InitCommon/paramset.hpp"

namespace HFSAT {
// Using this kind of structure would add one more level of call back in base trading
// Though since this is required in very a few cases and many of them are not being used, we should be fine.
// Not sure if there's better way to it

class IndicatorHelperListener {
 public:
  virtual void ProcessIndicatorHelperUpdate() = 0;
  virtual ~IndicatorHelperListener() {}
};

// Call to keep track for all the indicator
class IndicatorHelper : public IndicatorListener, public TimePeriodListener {
 public:
  IndicatorHelper(DebugLogger& t_dbglogger, const Watch& r_watch, InstrumentInfo* prod, SecurityMarketView* smv,
                  SmartOrderManager* order_manager, int trading_start_mfm, int trading_end_mfm, int runtime_id,
                  bool livetrading);
  ~IndicatorHelper();

  inline void AddIndicatorHelperListener(IndicatorHelperListener* listener) { listener_ = listener; }
  void NotifyIndicatorHelperListener();

  inline CommonIndicator* GetIndicatorFromTokens(DebugLogger& dbglogger, const Watch& watch,
                                                 const std::vector<const char*>& tokens,
                                                 PriceType_t dep_base_pricetype) {
    return (GetUniqueInstanceFunc(tokens[2]))(dbglogger, watch, tokens, dep_base_pricetype);
  }

  void RecomputeHysterisis();
  void ProcessOnTimePeriodUpdate(int num_pages_to_add, ParamSet* paramset, int position);
  void OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value);
  void OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value_decrease,
                         const double& new_value_nochange, const double& new_value_increase);
  void OnTimePeriodUpdate(const int num_pages_to_add_);

  void ComputeL1TradeVolumeBias(ParamSet* paramset);
  void ComputeL1SizeBias(ParamSet* paramset);
  void ComputeL1OrderBias(ParamSet* paramset);
  void ComputeHighUTSFactor(ParamSet* paramset);
  void ComputeLongevity(ParamSet* paramset);
  void ComputePositioning(ParamSet* paramset);
  void ComputeStdevTriggerForPCNBL(ParamSet* paramset);
  void ComputeBigTrades(ParamSet* paramset);
  void ComputeBigTradesIndep(ParamSet* paramset);
  void ComputeBigTradesPlaceIndep(ParamSet* paramset);
  void ComputeBigTradesAggressIndep(ParamSet* paramset);
  void ComputeL1BidAskSizeFlow(ParamSet* paramset);
  void ComputeScaledMaxPos(ParamSet* paramset);
  void ComputeRegimes(const std::string& regime_indicator_string);
  void ComputeRegimesToTrade(ParamSet* paramset);
  void ComputeMovingBidAskSpread(ParamSet* paramset);
  void ComputeMovingStdev(ParamSet* paramset);
  void ComputeFlagsAsPerMktCondition(ParamSet* paramset);
  void ComputeTradeBias(ParamSet* paramset);
  void ComputeCancelBias(ParamSet* paramset);
  void ComputeImpliedPrice(ParamSet* paramset);
  void InitiateStdevL1Bias(ParamSet* paramset);
  double ComputeScaledBaseMidDiff(double basemid_diff);

  inline double l1_trade_volume_bias() { return l1_trade_volume_bias_; }
  inline double l1_bias() { return l1_bias_; }
  inline double trade_bias() { return trade_bias_; }
  inline double cancel_bias() { return cancel_bias_; }
  inline double implied_price() { return implied_price_; }
  inline double l1_order_bias() { return l1_order_bias_; }
  inline double buy_uts_factor() { return buy_uts_factor_; }
  inline double sell_uts_factor() { return sell_uts_factor_; }
  inline bool updated_factor_this_round() { return updated_factor_this_round_; }

  inline double bestbid_queue_hysterisis() { return bestbid_queue_hysterisis_; }
  inline double bestask_queue_hysterisis() { return bestask_queue_hysterisis_; }
  inline double long_positioning_bias() { return long_positioning_bias_; }
  inline double short_positioning_bias() { return short_positioning_bias_; }
  inline bool place_nonbest() { return stdev_nnbl_switch_; }
  inline int last_bigtrades_ask_cancel_msecs() { return last_bigtrades_ask_cancel_msecs_; }
  inline int last_bigtrades_bid_cancel_msecs() { return last_bigtrades_bid_cancel_msecs_; }
  inline int last_bigtrades_ask_place_msecs() { return last_bigtrades_ask_place_msecs_; }
  inline int last_bigtrades_bid_place_msecs() { return last_bigtrades_bid_place_msecs_; }
  inline int last_bigtrades_ask_aggress_msecs() { return last_bigtrades_ask_aggress_msecs_; }
  inline int last_bigtrades_bid_aggress_msecs() { return last_bigtrades_bid_aggress_msecs_; }
  inline bool called_on_regime_update() { return called_on_regime_update_; }
  inline int regime_param_index() { return regime_index_; }
  inline bool getflat_due_to_regime_indicator() { return getflat_due_to_regime_indicator_; }
  inline double moving_average_spread() { return moving_average_spread_value_; }
  inline double stdev() { return moving_stdev_val_; }
  inline bool avoid_short_market() { return avoid_short_market_; }
  inline bool avoid_short_market_aggressively() { return avoid_short_market_aggressively_; }
  inline bool avoid_long_market() { return avoid_long_market_; }
  inline bool avoid_long_market_aggressively() { return avoid_long_market_aggressively_; }
  inline bool l1_bid_ask_size_flow_cancel_buy() { return l1_bid_ask_size_flow_cancel_buy_; }
  inline bool l1_bid_ask_size_flow_cancel_sell() { return l1_bid_ask_size_flow_cancel_sell_; }

  double volume_adj_max_pos(ParamSet* paramset);

 private:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  InstrumentInfo* product_;

  SecurityMarketView* smv_;
  SmartOrderManager* order_manager_;
  IndicatorHelperListener* listener_;

  int indices_used_so_far_;

  int trading_start_utc_mfm_;
  int trading_end_utc_mfm_;
  int runtime_id_;

  bool livetrading_;

  bool computing_l1_trade_volume_bias_;
  bool computing_l1_bias_;
  bool computing_l1_order_bias_;
  bool computing_high_uts_factor_;
  bool computing_longevity_;
  bool computing_positioning_;
  bool computing_bigtrades_;

  /// l1_size  bias variable variable
  L1SizeTrend* l1_size_indicator_;
  L1OrderTrend* l1_order_indicator_;

  double avg_l1_size_;
  double l1_avg_size_upper_bound_;
  double l1_avg_size_lower_bound_;
  double l1_size_upper_bound_;
  double l1_size_lower_bound_;
  double l1_size_upper_percentile_;
  double l1_size_lower_percentile_;
  bool use_l1_size_static_value_;
  double l1_norm_factor_;
  double l1_factor_;
  double l1_bias_;

  double trade_bias_;
  double cancel_bias_;
  double implied_price_;
  /// l1_order bias variable
  double avg_l1_order_;
  double l1_avg_order_upper_bound_;
  double l1_avg_order_lower_bound_;
  double l1_order_upper_bound_;
  double l1_order_lower_bound_;
  double l1_order_upper_percentile_;
  double l1_order_lower_percentile_;
  bool use_l1_order_static_value_;
  double l1_order_norm_factor_;
  double l1_order_factor_;
  double l1_order_bias_;

  /// high UTS factor variables
  L1SizeTrend* bid_size_indicator_;
  L1SizeTrend* ask_size_indicator_;
  int last_buy_uts_in_high_uts_;
  int last_sell_uts_in_high_uts_;
  double l1_size_upper_bound_high_uts_;
  double l1_size_lower_bound_high_uts_;
  double l1_norm_factor_high_uts_;
  double buy_uts_factor_;
  double sell_uts_factor_;
  bool updated_factor_this_round_;

  /// longevity
  CommonIndicator* stdev_calculator_onoff_;
  int longevity_index_;
  int longevity_version_;
  double longevity_thresh_;
  double longevity_min_stdev_ratio_;
  double offline_stdev_;
  double online_stdev_;
  double bestbid_queue_hysterisis_;
  double bestask_queue_hysterisis_;

  /// Positioning
  double short_positioning_bias_;
  double long_positioning_bias_;

  /// stdev trigger for place cancel non best
  SlowStdevCalculator* stdev_calculator_suppress_non_best_indc_;
  int stdev_calculator_suppress_non_best_index_;
  double stdev_suppress_non_best_level_threshold_;
  bool stdev_nnbl_switch_;

  /// BigTrades
  BigTradeVolumeVersion* l1_trade_volume_bias_indicator_;
  MovingAvgTradeImpact* moving_average_buy_indc_;
  MovingAvgTradeImpact* moving_average_sell_indc_;
  MovingAvgTradeImpact* moving_average_buy_place_indc_;
  MovingAvgTradeImpact* moving_average_sell_place_indc_;
  MovingAvgTradeImpact* moving_average_buy_aggress_indc_;
  MovingAvgTradeImpact* moving_average_sell_aggress_indc_;
  int l1_trade_volume_bias_index_;
  int moving_avg_buy_index_;
  int moving_avg_sell_index_;
  int moving_avg_buy_place_index_;
  int moving_avg_sell_place_index_;
  int moving_avg_buy_aggress_index_;
  int moving_avg_sell_aggress_index_;
  int last_bigtrades_ask_cancel_msecs_;
  int last_bigtrades_bid_cancel_msecs_;
  int last_bigtrades_ask_place_msecs_;
  int last_bigtrades_bid_place_msecs_;
  int last_bigtrades_ask_aggress_msecs_;
  int last_bigtrades_bid_aggress_msecs_;
  double l1_trade_volume_bias_;
  double hist_avg_l1_size_;
  double l1_trade_volume_bias_threshold_;
  double moving_avg_buy_;
  double moving_avg_sell_;
  double moving_avg_buy_place_;
  double moving_avg_sell_place_;
  double moving_avg_buy_aggress_;
  double moving_avg_sell_aggress_;
  double bigtrades_cancel_threshold_;
  double bigtrades_place_threshold_;
  double bigtrades_aggress_threshold_;
  int bigtrades_place_cooloff_interval_;
  int bigtrades_aggress_cooloff_interval_;

  // L1BidAskSizeFlow
  L1BidAskSizeFlow* l1_bid_ask_size_flow_cancel_indc_;
  int l1_bid_ask_size_flow_cancel_index_;
  double l1_bid_ask_size_flow_cancel_;
  double l1_bid_ask_size_flow_cancel_thresh_;
  double l1_bid_ask_size_flow_cancel_keep_thresh_diff_;
  double l1_bid_ask_size_flow_keep_thresh;
  double l1_bias_cancel_thresh_;
  int min_size_to_flow_based_cancel_;

  bool l1_bid_ask_size_flow_cancel_buy_;
  bool l1_bid_ask_size_flow_cancel_sell_;

  /// BigTrades Indep

  /// Regime
  CommonIndicator* regime_indicator_;
  int regime_indicator_index_;
  int regime_index_;
  bool called_on_regime_update_;

  std::string trade_indicator_string_;
  std::string cancel_indicator_string_;
  std::string trade_bias_indicator_string_;
  std::string cancel_bias_indicator_string_;
  double factor_trade_bias_;
  double factor_cancel_bias_;

  std::string implied_mkt_indicator_string_;
  /// GetFlat Regime
  std::vector<CommonIndicator*> regime_indicator_vec_;
  std::vector<bool> trading_vec_;
  std::vector<std::string> regime_indicator_string_vec_;
  std::vector<std::vector<int> > regimes_to_trade_;
  int getflat_regime_indicator_start_index_;
  int getflat_regime_indicator_end_index_;
  bool getflat_due_to_regime_indicator_;

  int trade_bias_indicator_start_index_;
  int trade_bias_indicator_end_index_;
  int cancel_bias_indicator_start_index_;
  int cancel_bias_indicator_end_index_;

  int implied_mkt_indicator_start_index_;
  int implied_mkt_indicator_end_index_;
  /// Moving average spread
  int moving_average_spread_index_;
  double moving_average_spread_value_;
  MovingAvgBidAskSpread* moving_average_spread_;

  /// Stdev
  int moving_stdev_index_;
  double moving_stdev_val_;
  bool use_sqrt_stdev_;
  double stdev_fact_ticks_;
  double stdev_cap_;
  SlowStdevCalculator* slow_stdev_calculator_;

  // Source based exec change
  SimpleTrend* src_trend_indicator_;
  int src_trend_index_;
  SlowStdevCalculator* src_stdev_indicator_;
  int src_stdev_index_;
  double src_trend_thresh_;
  double src_trend_keep_thresh_;
  double src_stdev_thresh_;
  double src_trend_val_;
  double src_stdev_val_;
  bool avoid_short_market_;
  bool avoid_short_market_aggressively_;
  bool avoid_long_market_;
  bool avoid_long_market_aggressively_;
  StdevCalculator* sd_l1bias_;
  double offline_l1bias_stdev_;
  double l1bias_weight_cap_;
  void set_mkt_condition_flags();
};
}
