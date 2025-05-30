/**
 \file InitCommon/paramset.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 353, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551

 */
#ifndef BASE_INITCOMMON_PARAMSET_H
#define BASE_INITCOMMON_PARAMSET_H

#include "dvccode/CDef/control_messages.hpp"
#include "dvccode/CommonTradeUtils/economic_events_manager.hpp"
#include "dvccode/CommonTradeUtils/sample_data_util.hpp"
#include "dvctrade/Indicators/positioning.hpp"
#include "dvctrade/Indicators/recent_simple_volume_measure.hpp"
#include "dvctrade/Indicators/stdev_ratio_normalised.hpp"

#define EZONE_MAXLEN 5

namespace HFSAT {

struct ParamSet {
  int tradingdate_;

  int worst_case_position_;  ///< the worst case max position ... neever have orders that can let you excceed this
  /// position
  int worst_case_unit_ratio_;  ///< either this or worst_case_position can be used to specifiy worst_case_postion
  int max_position_;           ///< working max position... used to check position, not orders
  int max_position_original_;
  double max_unit_ratio_;  ///< either this or max_position can be used to specifiy max_position // Fractional for
  int max_total_unit_ratio_to_place_;
  int max_total_size_to_place_;
  /// pro-rata products
  int unit_trade_size_;              ///< in general we should placing and canceling orders of this size
  int min_allowed_unit_trade_size_;  // for TOD, TOM etc in MICEX
  int max_global_position_;  // For BAX , LFL , LFI , DI , GE this would be the max aggregated position across all
  // expries.
  int max_security_position_;  // for PBSAT

  int explicit_max_short_position_;           ///< to prevent intervention loss
  int explicit_max_short_unit_ratio_;         ///< to prevent intervention loss
  int explicit_worst_case_short_position_;    ///< to prevent intervention loss
  int explicit_worst_case_short_unit_ratio_;  ///< to prevent intervention loss

  int explicit_max_long_position_;           ///< to prevent intervention loss
  int explicit_max_long_unit_ratio_;         ///< to prevent intervention loss
  int explicit_worst_case_long_position_;    ///< to prevent intervention loss
  int explicit_worst_case_long_unit_ratio_;  ///< to prevent intervention loss

  double highpos_limits_;  ///< above this position ( as a multiple of unit_trade_size_ ), the thresholds have to be
  /// adjusted further
  double highpos_limits_unit_ratio_;
  double highpos_thresh_factor_;
  double highpos_thresh_decrease_;
  double highpos_size_factor_;

  // two ways of specifying params
  // one each ofthem
  double increase_place_;
  double increase_keep_;
  double zeropos_limits_;
  double zeropos_limits_unit_ratio_;
  double zeropos_place_;
  double zeropos_keep_;
  double decrease_place_;
  double decrease_keep_;
  // two first three principal coefficients :)
  double place_keep_diff_;
  double increase_zeropos_diff_;
  double zeropos_decrease_diff_;
  double spread_increase_;
  double max_min_diff_;

  double l1bias_model_stdev_;
  double l1bias_weight_cap_;
  int l1bias_model_stdev_duration_;
  double max_min_diff_ratio_;
  int bucket_size_;                     // volume over which positioning is calculated
  double positioning_thresh_decrease_;  // magnitude by which the thresholds should be decreased if my_position_ is
  // against positioning indicator
  double positioning_threshold_;  // minimum value of positioning indicator beyond which it should affect exec logic

  double max_min_diff_order_;
  double volume_history_secs_;
  int base_mur_;
  double volume_lower_bound_ratio_;
  double volume_upper_bound_ratio_;
  // -- end

  double thresh_increase_;
  double thresh_decrease_;
  double thresh_place_;
  double thresh_place_keep_diff_;

  int original_unit_trade_size_;
  double high_uts_factor_;
  double max_high_uts_l1_factor_;

  int safe_distance_;

  double high_spread_allowance_;  ///< only for DAT

  bool allowed_to_improve_;
  bool allowed_to_aggress_;
  bool use_new_aggress_logic_;
  double improve_;
  int improve_ticks_;
  double aggressive_;

  double curve_adjusted_thresh_;
  double curve_adjusted_msecs_;

  double max_self_ratio_at_level_;
  double longevity_support_;  ///< longevity_support_ = 1.00 means that if size_behind ~= best_bid_size + best_ask_size,
  /// then this adds almost 1 tick to the expected profit from the trade
  double dat_weightage_fraction_;
  bool read_dat_weightage_fraction_;

  int longevity_version_;
  double longevity_min_stdev_ratio_;
  double longevity_stdev_dur_;

  // settings for aggressive and improve sanity checks
  int max_position_to_lift_;
  int max_position_to_bidimprove_;
  int max_position_to_cancel_on_lift_;
  int max_size_to_aggress_;  // Maximum size of the side on which we are trying to aggress against
  int min_position_to_hit_;
  int min_position_to_askimprove_;
  int min_position_to_cancel_on_hit_;

  int max_int_spread_to_place_;      ///< if the spread is greater than this then do not place an order at inside market
  int max_int_level_diff_to_place_;  ///< if ( L1 - L2 ) is greater than this then do not place an order at inside
  /// market
  int max_int_spread_to_cross_;
  int min_int_spread_to_improve_;
  unsigned int num_non_best_bid_levels_monitored_;
  unsigned int num_non_best_ask_levels_monitored_;
  unsigned int min_distance_for_non_best_;
  unsigned int max_distance_for_non_best_;
  unsigned int max_bid_ask_order_diff_;
  unsigned int min_size_to_quote_;
  unsigned int maturity_;
  unsigned int min_quote_distance_from_best_;
  double stdev_quote_factor_;

  unsigned int min_size_ahead_for_non_best_;
  bool ignore_max_pos_check_for_non_best_;

  int max_loss_;
  int max_pnl_;
  int short_term_global_max_loss_;
  int msecs_for_short_term_global_max_loss_;
  int short_term_global_max_loss_getflat_msecs_;
  int global_max_loss_;
  int max_opentrade_loss_;
  int max_opentrade_loss_per_uts_;
  int max_drawdown_;
  int max_short_term_loss_;
  int place_cancel_cooloff_;

  int cooloff_interval_;
  int agg_cooloff_interval_;
  int improve_cooloff_interval_;
  // int highpos_aversion_msecs_ ; ///< if we are at maxpos and this much time has passed then we should be adding a
  // bias of -1 to signal

  double stdev_fact_;
  double stdev_cap_;
  unsigned int stdev_duration_;
  bool increase_thresholds_symm_;
  bool increase_thresholds_continuous_;

  bool should_stdev_suppress_non_best_level_;
  double stdev_suppress_non_best_level_threshold_;
  double stdev_suppress_non_best_level_duration_;

  int px_band_;
  double low_stdev_lvl_;  ///< below this spread checks can be ignored
  int min_size_to_join_;  ///< the minimum size at a level to place a passive order on
  bool use_sqrt_stdev_;   ///< if set (1) then use sqrt(stdev) instead of stdev in execlogic
  double spread_add_;     ///< a factor decreased from passive thresholds and increased in agg thresholds when spread is
  /// more than 1
  double severity_to_getflat_on_;
  EconomicZone_t ezone_traded_;

  // logic to place orders during non normal trading time
  double max_severity_to_keep_suporders_on_;
  int agg_closeout_utc_hhmm_;  ///< if after this time then place an aggressive order to close out if spread is 1tick
  int agg_closeout_max_size_;  ///< if > 0 and if less than this size on other side then aggress and close out

  int break_msecs_on_max_opentrade_loss_;  ///< only if overriding default value

  double indep_safe_ticks_;  ///< the minimum price difference to bid at in related product
  int indep_safe_size_;      ///< only consider indep_safe_ticks_ if size is more than this
  double indep_safe_ticks_low_pos_;
  int indep_safe_size_low_pos_;
  int max_unit_ratio_pp_;
  int max_position_pp_;
  double indep_safe_price_diff_;  // Minimum difference of mkt_price from bid/ask
  double mini_keep_thresh_;
  double mini_agg_thresh_;
  double projected_price_duration_;
  double pp_place_keep_diff_;
  /// force min cooloff of 10msec .. currently just for cme
  bool enforce_min_cooloff_;

  double pclose_factor_;

  int moderate_time_limit_;
  int high_time_limit_;
  int safe_cancel_size_;

  bool place_on_trade_update_implied_quote_;

  int num_increase_ticks_to_keep_;  // Keep an order on at a level this many ticks away ( on the side which increases
  // position )
  int num_decrease_ticks_to_keep_;  // Keep an order on at a level this many ticks away ( on the side which decreases
  // position )

  bool is_liquid_;

  unsigned int stir_cancel_on_exec_cooloff_msecs_;
  unsigned int stir_cancel_on_level_change_cooloff_msecs_;
  unsigned int pair_exec_cancel_cooloff_msecs_;

  int max_global_risk_;
  double max_global_risk_ratio_;

  bool use_stable_bidask_levels_;

  int online_stats_history_secs_;
  bool online_stats_avg_dep_bidask_spread_;
  bool place_multiple_orders_;
  int max_unit_size_at_level_;
  int size_diff_between_orders_;

  bool online_model_stdev_;
  double min_model_scale_fact_;
  double max_model_scale_fact_;
  double offline_model_stdev_;
  unsigned int model_stdev_duration_;

  int pair_unit_trade_size_factor_;
  double pair_place_diff_;
  double pair_keep_diff_;
  double pair_aggressive_;
  double pair_improve_;
  double pair_zeropos_keep_;
  double pair_zeropos_decrease_diff_;
  double pair_increase_zeropos_diff_;
  double pair_place_keep_diff_;
  double abs_max_pos_factor_;
  std::string own_base_px_;
  int price_for_max_unit_ratio_;
  bool use_stable_book_;
  double spread_factor_;
  double spread_quote_factor_;

  bool is_common_param_;
  double volume_ratio_stop_trading_lower_threshold_;
  double volume_ratio_stop_trading_upper_threshold_;
  //
  double max_global_beta_adjusted_notional_risk_;
  double self_pos_projection_factor_;
  bool compute_notional_risk_;
  bool use_mean_reversion_;
  bool skip_mean_reversion_on_indep_;
  double mean_reversion_skip_factor_;

  double moving_bidask_spread_duration_;

  double size_disclosed_factor_;

  int flatfok_book_depth_;
  bool get_flat_by_fok_mode_;
  bool read_get_flat_by_fok_mode_;

  bool sgx_market_making_;

  double implied_cancel_threshold_;
  bool implied_price_based_cancellation_;
  double implied_place_threshold_;
  bool implied_price_based_place_;
  double implied_agg_threshold_;
  bool implied_price_based_agg_;
  double implied_improve_threshold_;
  bool implied_price_based_improve_;

  double implied_mkt_cancel_threshold_;
  bool implied_mkt_price_based_cancellation_;
  double implied_mkt_place_threshold_;
  bool implied_mkt_price_based_place_;

  unsigned int implied_price_calculation_;

  bool improve_cancel_;

  // flags
  bool read_worst_case_position_;
  bool read_worst_case_unit_ratio_;
  bool read_max_position_;
  bool read_max_unit_ratio_;
  bool read_max_total_unit_ratio_to_place_;
  bool read_max_total_size_to_place_;
  bool read_unit_trade_size_;
  bool read_min_allowed_unit_trade_size_;  // for TOD, TOM etc in MICEX
  bool read_max_global_position_;
  bool read_max_security_position_;

  bool read_explicit_max_short_position_;           ///< to prevent intervention loss
  bool read_explicit_max_short_unit_ratio_;         ///< to prevent intervention loss
  bool read_explicit_worst_case_short_position_;    ///< to prevent intervention loss
  bool read_explicit_worst_case_short_unit_ratio_;  ///< to prevent intervention loss

  bool read_curve_adjusted_thresh_;
  bool read_curve_adjusted_msecs_;

  bool read_explicit_max_long_position_;           ///< to prevent intervention loss
  bool read_explicit_max_long_unit_ratio_;         ///< to prevent intervention loss
  bool read_explicit_worst_case_long_position_;    ///< to prevent intervention loss
  bool read_explicit_worst_case_long_unit_ratio_;  ///< to prevent intervention loss

  int non_standard_market_condition_check_short_msecs_;
  int non_standard_market_condition_check_long_msecs_;
  int non_standard_market_condition_min_best_level_size_;
  int non_standard_market_condition_min_best_level_order_count_;
  int non_standard_market_condition_max_avg_order_size_;
  int non_standard_market_condition_min_counter_order_size_;
  int non_standard_market_condition_max_spread_;
  int non_standard_market_condition_max_position_;

  bool read_non_standard_market_condition_check_short_msecs_;
  bool read_non_standard_market_condition_check_long_msecs_;
  bool read_non_standard_market_condition_min_best_level_size_;
  bool read_non_standard_market_condition_min_best_level_order_count_;
  bool read_non_standard_market_condition_max_avg_order_size_;
  bool read_non_standard_market_condition_min_counter_order_size_;
  bool read_non_standard_market_condition_max_spread_;
  bool read_non_standard_market_condition_max_position_;

  bool read_highpos_limits_;
  bool read_highpos_limits_unit_ratio_;
  bool read_highpos_thresh_factor_;
  bool read_highpos_thresh_decrease_;
  bool read_highpos_size_factor_;
  bool read_increase_place_;
  bool read_increase_keep_;
  bool read_zeropos_limits_;
  bool read_zeropos_limits_unit_ratio_;
  bool read_zeropos_place_;
  bool read_zeropos_keep_;
  bool read_decrease_place_;
  bool read_decrease_keep_;

  bool read_place_keep_diff_;
  bool read_increase_zeropos_diff_;
  bool read_zeropos_decrease_diff_;
  bool read_spread_increase_;
  bool read_max_min_diff_;
  bool read_max_min_diff_ratio_;
  bool read_bucket_size_;
  bool read_positioning_thresh_decrease_;
  bool read_positioning_threshold_;
  bool read_max_min_diff_order_;
  bool read_scale_max_pos_;
  bool read_high_uts_factor_;
  bool read_max_high_uts_l1_factor_;

  bool read_thresh_increase_;
  bool read_thresh_decrease_;
  bool read_thresh_place_;
  bool read_thresh_place_keep_diff_;

  bool read_safe_distance_;

  bool read_high_spread_allowance_;

  bool read_allowed_to_improve_;
  bool read_allowed_to_aggress_;
  bool read_use_new_aggress_logic_;
  bool read_improve_;
  bool read_aggressive_;

  bool read_max_self_ratio_at_level_;
  bool read_longevity_support_;

  bool read_max_position_to_lift_;
  bool read_max_position_to_bidimprove_;
  bool read_max_position_to_cancel_on_lift_;
  bool read_max_size_to_aggress_;
  bool read_min_position_to_hit_;
  bool read_min_position_to_askimprove_;
  bool read_min_position_to_cancel_on_hit_;

  bool read_max_int_spread_to_place_;
  bool read_max_int_level_diff_to_place_;
  bool read_max_int_spread_to_cross_;
  bool read_min_int_spread_to_improve_;

  bool read_num_non_best_bid_levels_monitored_;
  bool read_num_non_best_ask_levels_monitored_;
  bool read_min_distance_for_non_best_;
  bool read_max_distance_for_non_best_;
  bool read_max_bid_ask_order_diff_;
  bool read_min_size_to_quote_;
  bool read_maturity_;
  bool read_min_quote_distance_from_best_;
  bool read_stdev_quote_factor_;

  bool read_min_size_ahead_for_non_best_;
  bool read_ignore_max_pos_check_for_non_best_;

  bool read_max_loss_;
  bool read_max_pnl_;
  bool read_short_term_global_max_loss_;
  bool read_global_max_loss_;
  bool read_max_opentrade_loss_;
  bool read_max_opentrade_loss_per_uts_;
  bool read_max_drawdown_;
  bool read_max_short_term_loss_;
  bool read_place_cancel_cooloff_;
  bool read_cooloff_interval_;
  bool read_agg_cooloff_interval_;
  bool read_improve_cooloff_interval_;
  // bool read_highpos_aversion_msecs_ ;

  bool read_stdev_fact_;
  bool read_stdev_cap_;
  bool read_stdev_duration_;

  bool read_px_band_;
  bool read_low_stdev_lvl_;
  bool read_min_size_to_join_;
  bool read_spread_add_;
  bool read_severity_to_getflat_on_;
  bool read_ezone_traded_;

  bool read_max_severity_to_keep_suporders_on_;
  bool read_agg_closeout_utc_hhmm_;
  bool read_agg_closeout_max_size_;

  bool read_break_msecs_on_max_opentrade_loss_;
  bool read_indep_safe_ticks_;
  bool read_indep_safe_size_;
  bool read_indep_safe_price_diff_;
  bool read_indep_safe_ticks_low_pos_;
  bool read_indep_safe_size_low_pos_;
  bool read_max_unit_ratio_pp_;
  bool read_mini_keep_thresh_;
  bool read_mini_agg_thresh_;
  bool read_projected_price_duration_;
  bool read_pp_place_keep_diff_;
  bool read_enforce_min_cooloff_;
  bool read_price_for_max_unit_ratio_;

  EconomicZone_t ezone_vec_[EZONE_MAXLEN];  ///< in case we want to specifiy which economic zones are important

  bool use_throttle_manager_;
  int throttle_message_limit_;

  std::string paramfilename_;

  bool read_pclose_factor_;

  bool read_moderate_time_limit_;
  bool read_high_time_limit_;
  bool read_safe_cancel_size_;

  bool read_place_on_trade_update_implied_quote_;

  bool read_num_increase_ticks_to_keep_;
  bool read_num_decrease_ticks_to_keep_;

  bool read_is_liquid_;

  bool read_stir_cancel_on_exec_cooloff_msecs_;
  bool read_stir_cancel_on_level_change_cooloff_msecs_;
  bool read_pair_exec_cancel_cooloff_msecs_;
  bool read_max_global_risk_;
  bool read_max_global_risk_ratio_;

  double retail_size_factor_to_offer_;
  double retail_size_factor_to_place_;
  unsigned int retail_max_level_to_offer_;
  std::vector<int> retail_size_to_offer_vec_;
  bool use_retail_trading_flatlogic_;
  bool aggress_above_max_pos_;
  double retail_position_discount_factor_;
  double retail_size_tolerance_;
  double retail_price_threshold_tolerance_;
  int retail_max_position_step_size_;
  int retail_max_global_position_step_size_;
  bool retail_place_fok_;
  int retail_max_pc1_risk_;
  int retail_max_ord_size_;
  double owp_retail_offer_thresh_;
  bool read_owp_retail_offer_thresh_;
  bool retail_offer_fra_;

  bool read_use_stable_bidask_levels_;

  bool read_dpt_range_;
  double dpt_range_;
  int desired_position_leeway_;
  int desired_position_large_difference_;
  bool read_desired_position_leeway_;
  bool read_desired_position_difference_;
  bool read_place_or_cancel_;
  bool place_or_cancel_;  // if at any time you want to either place or cancel
  bool read_position_change_compensation_;
  double position_change_compensation_;

  bool read_liquidity_factor_;
  double liquidity_factor_;
  bool read_online_stats_history_secs_;
  bool read_online_stats_avg_dep_bidask_spread_;
  bool read_place_multiple_orders_;
  bool read_max_unit_size_at_level_;
  bool read_size_diff_between_orders_;
  bool read_size_disclosed_factor_;

  bool use_online_uts_;
  double uts_per_L1Size;
  int lookback_dynamic_uts;

  // for NSE & BSE we want to make UTS multiple of time varying lot size. We use notional
  // UTS to get around this problem
  bool use_notional_uts_;
  double notional_uts_;

  std::string trade_indicator_string_;
  double factor_trade_bias_;
  std::string cancel_indicator_string_;
  double factor_cancel_bias_;

  std::vector<std::string> regime_indicator_string_vec_;
  std::vector<std::vector<int> > regimes_to_trade_;

  ParamSet(const std::string& _paramfilename_, const int r_tradingdate_, std::string dep_short_code_);

  void LoadParamSet(std::string dep_short_code);

  bool read_pair_unit_trade_size_factor_;
  bool read_pair_place_diff_;
  bool read_pair_keep_diff_;
  bool read_pair_aggressive_;
  bool read_pair_improve_;
  bool read_pair_zeropos_keep_;
  bool read_pair_zeropos_decrease_diff_;
  bool read_pair_increase_zeropos_diff_;
  bool read_pair_place_keep_diff_;
  bool read_abs_max_pos_factor_;
  bool read_own_base_px_;
  bool read_use_stable_book_;
  bool read_spread_factor_;
  bool read_spread_quote_factor_;

  bool read_volume_ratio_stop_trading_lower_threshold_;
  bool read_volume_ratio_stop_trading_upper_threshold_;
  bool read_moving_bidask_spread_duration_;
  //
  bool read_max_global_beta_adjusted_notional_risk_;
  bool read_self_pos_projection_factor_;
  bool read_place_only_after_cancel_;
  bool read_allow_modify_orders_;

  // some params which are set in basetrading for each paramset
  double stdev_fact_ticks_;
  double volume_lower_bound_;
  double volume_upper_bound_;
  double volume_norm_factor_;
  std::string source_for_dmur_;
  bool read_source_for_dmur_;
  int max_unit_ratio_structured;
  Positioning* positioning_indicator_;
  RecentSimpleVolumeMeasure* volume_ratio_indicator_;
  RecentSimpleVolumeMeasure* self_volume_ratio_indicator_;
  StdevRatioNormalised* stdev_ratio_normalised_indicator_;
  double self_volume_expected_;
  double self_volume_ratio_threshold_;
  double self_stdev_ratio_threshold_;
  std::string base_shortcode_structured;

  std::string combined_get_flat_model = "";
  bool should_get_combined_flat;
  int number_strats;

  bool mini_top_keep_logic_;
  bool mini_agg_logic_;
  int mini_agg_ticks_;
  int mini_safe_indep_agg_ticks_;
  int mini_safe_indep_agg_spread_scale_factor;

  bool mini_improve_logic_;
  int indep_safe_improve_ticks_;
  int indep_safe_improve_size_;
  double mkt_price_improve_bias_;

  int mini_agg_sweep_margin_;
  unsigned int mini_sweep_cancel_cooloff_;

  // these params have been introduced in the modified price_pair_based_aggressive_trading
  double spread_factor_agg_;
  double spread_factor_place_;
  double fixed_factor_agg_;
  double fixed_factor_place_;
  std::string price_type_;

  bool read_spread_factor_agg_;
  bool use_pre_getflat_;
  int pre_getflat_msecs_;
  double pre_getflat_multiplier_;
  bool allow_to_aggress_on_getflat_;
  double getflat_aggress_;
  double max_spread_getflat_aggress_;

  // this is for adjusting risk function in Structured Strategy
  bool use_min_portfolio_risk_;
  bool read_curve_adjusted_indicator_string_;
  std::string curve_adjusted_indicator_string_;
  // these params are used in EBT
  std::string source_stdev_risk_scaling_;
  int use_min_risk_;

  // these params are used in EventBiasAggressiveTrading
  int af_event_id_;
  int af_event_halflife_secs_;
  int af_event_drawdown_secs_;
  int af_event_val_type_;
  double af_event_pxch_act_;
  double af_event_pxch_getflat_;
  double af_event_max_uts_pxch_;
  bool read_af_event_pxch_act_;
  bool read_af_event_pxch_getflat_;
  bool read_af_event_max_uts_pxch_;
  std::string af_source_shc_;  // the source shc whose trend is sued alogn with the alphaflash no.
  bool af_use_source_trade_;
  double af_dd_maxpxch_factor_;
  double af_dd_predpxch_factor_;
  int af_use_mult_levels_;
  std::string af_scale_beta_;
  bool read_af_scale_beta_;
  int af_risk_scaling_;                         // 0: linear, 2: quadratic (default: 0)
  std::vector<int> af_staggered_getflat_mins_;  // AF_STAGGERED_GETFLAT_MINS 2 5 10 (staggered getflat at ,25,10 mins)

  double arb_place_thresh_;
  int num_lvl_order_keep_;
  std::string arb_best_shc_;

  bool place_only_after_cancel_;
  bool allow_modify_orders_;

  double max_fraction_uts_size_to_modify_;

  bool read_bigtrades_cancel_;  // 0 if not using big_trade_cancellation
  double bigtrades_cancel_threshold_;
  int bigtrades_window_msecs_;
  int bigtrades_cooloff_interval_;

  double bigtrades_decay_factor_;
  std::string bigtrades_source_id_;

  bool read_bigtrades_place_;  // 0 if not using big_trade_to_trade_at_best_lvl
  double bigtrades_place_threshold_;
  int bigtrades_place_window_msecs_;
  int bigtrades_place_cooloff_interval_;

  double bigtrades_place_decay_factor_;
  std::string bigtrades_place_source_id_;

  bool read_bigtrades_aggress_;  // 0 if not using big_trade_to_aggress
  double bigtrades_aggress_threshold_;
  int bigtrades_aggress_window_msecs_;
  int bigtrades_aggress_cooloff_interval_;

  double bigtrades_aggress_decay_factor_;
  std::string bigtrades_aggress_source_id_;

  bool read_l1_trade_volume_bias_;
  double l1_trade_volume_bias_;

  int l1_bid_ask_flow_num_events_;
  bool read_l1_bid_ask_flow_cancel_thresh_;
  double l1_bid_ask_flow_cancel_thresh_;
  double l1_bid_ask_flow_cancel_keep_thresh_diff_;
  int min_size_to_flow_based_cancel_;
  double l1_bias_cancel_thresh_;

  bool read_cancel_on_market_tilt_source_;
  std::string cancel_on_market_tilt_source_;
  double cancel_on_market_tilt_thresh_;
  double cancel_on_market_tilt_msecs_;

  bool read_use_notional_uts_;
  bool read_notional_uts_;

  bool read_regime_indicator_string_;
  bool read_regimes_to_trade_;

  bool read_trade_indicator_string_;
  bool read_trade_bias_;

  bool read_cancel_indicator_string_;
  bool read_cancel_bias_;

  // implied_mkt_info_params
  std::string implied_mkt_indicator_string_;

  bool read_implied_mkt_indicator_string_;
  bool read_implied_mkt_;
  double implied_mkt_thres_factor_;
  double max_size_to_cancel_;

  bool read_dynamic_zeropos_keep_;
  bool synth_agg_exec_;

  double delta_hedge_lower_threshold_;
  double delta_hedge_upper_threshold_;
  int fractional_second_implied_vol_;

  int spread_to_target_;

  bool read_src_based_exec_changes_;
  std::string source_for_mkt_condition_;
  double src_trend_percentile_;
  double src_trend_factor_;
  double src_trend_keep_factor_;
  double sumvars_scaling_factor_;
  double last_day_vol_factor_;
  bool read_last_day_vol_;
  double stdev_overall_cap_;
  bool read_stdev_overall_cap_;

  // For now keeping it as param, later we can replace this with sample data
  int max_trade_size_;
  bool read_max_trade_size_;

  bool read_use_cancellation_model_;
  bool read_canc_prob_;
  double canc_prob_;
  double cancel_prob_thresh_;

  double interday_scaling_factor_;
  bool read_interday_scaling_factor_;

  int max_distance_to_keep_from_band_price_;

  int mur_reset_time_;  ///< if after this time then reset mur value to mur_reset_value
  int mur_reset_value_;
  int min_order_size_;
  bool check_mur_reset_time_;

  bool release_core_premature_;
  bool is_mur_low_;
  bool recompute_signal_;
  std::string feature_modelfile_;
  bool read_feature_modelfile_;
  double feature_threshold_;
  bool read_feature_threshold_;
  double feature_more_;

  bool read_trade_rule_improve_;
  int min_ticks_to_trade_rule_improve_;
  int trade_rule_improve_cooloff_;

  // double sim_exec_threshold_;
  // double sim_exec_cooloff_;

  // std::vector<std::string> shcs_to_react_on_exec_;
  // double outright_frac_cancel;
  bool not_to_trade_;
  bool project_to_illiquid_;
  void LoadParamSet();
  void WriteSendStruct(ParamSetSendStruct& retval) const;

  // there are two ways of specifying params ... building the other one
  void ReconcileParams(std::string dep_short_code_);
  int GetAvgL1Size(std::string dep_short_code, int look_back);

  std::function<double(double, double)> get_dynamic_zero_pos_keep_func_vx_diff(std::string param_line);
  std::function<int(double)> get_dynamic_uts_func_volume(std::string param_line);
  std::function<int(double)> get_dynamic_uts_func_l1_size(std::string param_line);
  std::function<int(double)> get_dynamic_otl_func_stdev(std::string param_line);
  void change_params(std::string param_line);
};
}
#endif  // BASE_INITCOMMON_PARAMSET_H
