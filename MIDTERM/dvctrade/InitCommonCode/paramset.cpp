/**
 \file InitCommonCode/paramset.cpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 353, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551
 */
#include <numeric>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "dvccode/CDef/bse_security_definition.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvctrade/InitCommon/paramset.hpp"
#include "dvctrade/InitCommon/strategy_desc.hpp"

namespace HFSAT {

ParamSet::ParamSet(const std::string &_paramfilename_, const int r_tradingdate_, std::string dep12_short_code)
    : tradingdate_(r_tradingdate_),

      worst_case_position_(1),
      worst_case_unit_ratio_(1),
      max_position_(1),
      max_position_original_(1),
      max_unit_ratio_(1.0),
      max_total_unit_ratio_to_place_(1.0),
      max_total_size_to_place_(1.0),
      unit_trade_size_(1),
      min_allowed_unit_trade_size_(1),
      max_global_position_(0),
      max_security_position_(0),

      explicit_max_short_position_(0),
      explicit_max_short_unit_ratio_(0),
      explicit_worst_case_short_position_(0),
      explicit_worst_case_short_unit_ratio_(0),

      explicit_max_long_position_(0),
      explicit_max_long_unit_ratio_(0),
      explicit_worst_case_long_position_(0),
      explicit_worst_case_long_unit_ratio_(0),

      highpos_limits_(3),
      highpos_limits_unit_ratio_(3),
      highpos_thresh_factor_(0),
      highpos_thresh_decrease_(0),
      highpos_size_factor_(0.0),

      increase_place_(102),
      increase_keep_(101),
      zeropos_limits_(4),
      zeropos_limits_unit_ratio_(4),
      zeropos_place_(101),
      zeropos_keep_(100),
      decrease_place_(100),
      decrease_keep_(99),
      place_keep_diff_(1),
      increase_zeropos_diff_(1),
      zeropos_decrease_diff_(1),
      spread_increase_(0.0),
      max_min_diff_(0.0),
      l1bias_model_stdev_(0.0),
      l1bias_weight_cap_(0.0),
      l1bias_model_stdev_duration_(300),
      bucket_size_(1),
      positioning_thresh_decrease_(0.0),
      positioning_threshold_(0.0),
      max_min_diff_order_(0.0),
      volume_history_secs_(60.0),
      base_mur_(0),
      volume_lower_bound_ratio_(0.75),
      volume_upper_bound_ratio_(1.25),
      thresh_increase_(0.0),
      thresh_decrease_(0.0),
      thresh_place_(0.0),
      thresh_place_keep_diff_(0.0),

      original_unit_trade_size_(1),
      high_uts_factor_(0.0),
      max_high_uts_l1_factor_(1.0),

      safe_distance_(900000),

      high_spread_allowance_(0.25),

      allowed_to_improve_(false),
      allowed_to_aggress_(false),
      use_new_aggress_logic_(false),
      improve_(100),
      improve_ticks_(100),
      aggressive_(100),
      max_self_ratio_at_level_(
          1.00),  // for CGB 0.50 means size 1 > 0.50 * mktsize then cancel, or if mktsize < 2 then cancel
      longevity_support_(0),
      dat_weightage_fraction_(0),
      read_dat_weightage_fraction_(false),
      longevity_version_(0),
      longevity_min_stdev_ratio_(0.70),
      longevity_stdev_dur_(300),

      max_position_to_lift_(0),
      max_position_to_bidimprove_(0),
      max_position_to_cancel_on_lift_(0),
      max_size_to_aggress_(1000),
      min_position_to_hit_(0),
      min_position_to_askimprove_(0),
      min_position_to_cancel_on_hit_(0),

      max_int_spread_to_place_(1),
      max_int_level_diff_to_place_(1),
      max_int_spread_to_cross_(1),
      min_int_spread_to_improve_(2),
      num_non_best_bid_levels_monitored_(0),
      num_non_best_ask_levels_monitored_(0),
      min_distance_for_non_best_(1),
      max_distance_for_non_best_(10),
      max_bid_ask_order_diff_(100),
      min_size_to_quote_(0),
      maturity_(0),
      min_quote_distance_from_best_(1),
      stdev_quote_factor_(1.0),
      min_size_ahead_for_non_best_(0),
      ignore_max_pos_check_for_non_best_(false),

      max_loss_(3000),
      max_pnl_(900000),
      short_term_global_max_loss_(500000),                        // default value high
      msecs_for_short_term_global_max_loss_(30 * 60 * 1000),      // default 30 minutes
      short_term_global_max_loss_getflat_msecs_(30 * 60 * 1000),  // default 30 minutes
      global_max_loss_(500000),                                   // increased global max loss to $ 500 k
      max_opentrade_loss_(1000),
      max_opentrade_loss_per_uts_(1000),
      max_drawdown_(3000),
      max_short_term_loss_(1000),

      place_cancel_cooloff_(0),
      cooloff_interval_(0),
      agg_cooloff_interval_(10000),     // by default 10 seconds
      improve_cooloff_interval_(1000),  // by default 1 second
      // highpos_aversion_msecs_ ( 0 ), // inactive by default

      stdev_fact_(0),
      stdev_cap_(1.00),
      stdev_duration_(100),
      increase_thresholds_symm_(false),
      increase_thresholds_continuous_(false),
      should_stdev_suppress_non_best_level_(false),
      stdev_suppress_non_best_level_threshold_(10),
      stdev_suppress_non_best_level_duration_(900),

      px_band_(1),
      low_stdev_lvl_(0),
      min_size_to_join_(0),
      use_sqrt_stdev_(false),
      spread_add_(0),
      severity_to_getflat_on_(1.00),
      ezone_traded_(EZ_MAX),

      max_severity_to_keep_suporders_on_(2.9),
      agg_closeout_utc_hhmm_(-1),
      agg_closeout_max_size_(-1),

      break_msecs_on_max_opentrade_loss_(15 * 60 * 1000),
      indep_safe_ticks_(1.0),
      indep_safe_size_(1),
      indep_safe_ticks_low_pos_(1.0),
      indep_safe_size_low_pos_(1),
      max_unit_ratio_pp_(0),
      max_position_pp_(0),
      indep_safe_price_diff_(0.5),
      mini_keep_thresh_(0.0),
      mini_agg_thresh_(0.0),
      projected_price_duration_(300.0),
      pp_place_keep_diff_(0.0),
      enforce_min_cooloff_(false),
      pclose_factor_(0),

      moderate_time_limit_(10000000),
      high_time_limit_(10000000),
      safe_cancel_size_(100000),

      place_on_trade_update_implied_quote_(false),

      num_increase_ticks_to_keep_(0),
      num_decrease_ticks_to_keep_(0),

      is_liquid_(true),

      stir_cancel_on_exec_cooloff_msecs_(0),
      stir_cancel_on_level_change_cooloff_msecs_(0),

      pair_exec_cancel_cooloff_msecs_(0),

      max_global_risk_(0),
      max_global_risk_ratio_(1),

      use_stable_bidask_levels_(false),
      online_stats_history_secs_(300),
      online_stats_avg_dep_bidask_spread_(true),

      place_multiple_orders_(false),
      max_unit_size_at_level_(0),
      size_diff_between_orders_(0),

      online_model_stdev_(false),
      min_model_scale_fact_(100),
      max_model_scale_fact_(100),
      offline_model_stdev_(0.000001),
      model_stdev_duration_(900),

      pair_unit_trade_size_factor_(1),
      pair_place_diff_(0),
      pair_keep_diff_(0),
      pair_aggressive_(0.0),
      pair_improve_(0.0),
      pair_zeropos_keep_(0.0),
      pair_zeropos_decrease_diff_(0.0),
      pair_increase_zeropos_diff_(0.0),
      pair_place_keep_diff_(0.0),
      abs_max_pos_factor_(1.0),
      own_base_px_(""),
      price_for_max_unit_ratio_(1),
      use_stable_book_(false),
      spread_factor_(0.0),
      spread_quote_factor_(0.0),
      is_common_param_(false),
      volume_ratio_stop_trading_lower_threshold_(0.1),
      volume_ratio_stop_trading_upper_threshold_(4.0),

      max_global_beta_adjusted_notional_risk_(10000000000),
      self_pos_projection_factor_(1),
      compute_notional_risk_(false),
      use_mean_reversion_(true),
      skip_mean_reversion_on_indep_(false),
      mean_reversion_skip_factor_(2.0),

      moving_bidask_spread_duration_(300),
      size_disclosed_factor_(1.0),

      flatfok_book_depth_(1),
      get_flat_by_fok_mode_(false),

      read_get_flat_by_fok_mode_(false),
      sgx_market_making_(false),

      implied_cancel_threshold_(0.0),
      implied_price_based_cancellation_(false),
      implied_place_threshold_(0.0),
      implied_price_based_place_(false),
      implied_agg_threshold_(0.0),
      implied_price_based_agg_(false),
      implied_improve_threshold_(0.0),
      implied_price_based_improve_(false),

      implied_mkt_cancel_threshold_(0.0),
      implied_mkt_price_based_cancellation_(false),
      implied_mkt_place_threshold_(0.0),
      implied_mkt_price_based_place_(false),

      implied_price_calculation_(0),

      improve_cancel_(false),

      read_worst_case_position_(false),
      read_worst_case_unit_ratio_(false),
      read_max_position_(false),
      read_max_unit_ratio_(false),
      read_max_total_unit_ratio_to_place_(false),
      read_max_total_size_to_place_(false),
      read_unit_trade_size_(false),
      read_min_allowed_unit_trade_size_(false),
      read_max_global_position_(false),
      read_max_security_position_(false),

      read_explicit_max_short_position_(false),
      read_explicit_max_short_unit_ratio_(false),
      read_explicit_worst_case_short_position_(false),
      read_explicit_worst_case_short_unit_ratio_(false),

      read_explicit_max_long_position_(false),
      read_explicit_max_long_unit_ratio_(false),
      read_explicit_worst_case_long_position_(false),
      read_explicit_worst_case_long_unit_ratio_(false),

      non_standard_market_condition_check_short_msecs_(60000),
      non_standard_market_condition_check_long_msecs_(15 * 60 * 1000),
      non_standard_market_condition_min_best_level_size_(0),
      non_standard_market_condition_min_best_level_order_count_(0),
      non_standard_market_condition_max_avg_order_size_(100000),
      non_standard_market_condition_min_counter_order_size_(1000),
      non_standard_market_condition_max_spread_(50),
      non_standard_market_condition_max_position_(5),

      read_non_standard_market_condition_check_short_msecs_(false),
      read_non_standard_market_condition_check_long_msecs_(false),
      read_non_standard_market_condition_min_best_level_size_(false),
      read_non_standard_market_condition_min_best_level_order_count_(false),
      read_non_standard_market_condition_max_avg_order_size_(false),
      read_non_standard_market_condition_min_counter_order_size_(false),
      read_non_standard_market_condition_max_spread_(false),
      read_non_standard_market_condition_max_position_(false),

      read_highpos_limits_(false),
      read_highpos_limits_unit_ratio_(false),
      read_highpos_thresh_factor_(false),
      read_highpos_thresh_decrease_(false),
      read_highpos_size_factor_(false),
      read_increase_place_(false),
      read_increase_keep_(false),
      read_zeropos_limits_(false),
      read_zeropos_limits_unit_ratio_(false),
      read_zeropos_place_(false),
      read_zeropos_keep_(false),
      read_decrease_place_(false),
      read_decrease_keep_(false),
      read_place_keep_diff_(false),
      read_increase_zeropos_diff_(false),
      read_zeropos_decrease_diff_(false),
      read_spread_increase_(false),
      read_max_min_diff_(false),
      read_max_min_diff_ratio_(false),
      read_bucket_size_(false),
      read_positioning_thresh_decrease_(false),
      read_positioning_threshold_(false),
      read_max_min_diff_order_(false),
      read_scale_max_pos_(false),
      read_high_uts_factor_(false),
      read_max_high_uts_l1_factor_(false),

      read_thresh_increase_(false),
      read_thresh_decrease_(false),
      read_thresh_place_(false),
      read_thresh_place_keep_diff_(false),
      read_safe_distance_(false),
      read_high_spread_allowance_(false),
      read_allowed_to_improve_(false),
      read_allowed_to_aggress_(false),
      read_use_new_aggress_logic_(false),
      read_improve_(false),
      read_aggressive_(false),
      read_max_self_ratio_at_level_(false),
      read_longevity_support_(false),
      read_max_position_to_lift_(false),
      read_max_position_to_bidimprove_(false),
      read_max_position_to_cancel_on_lift_(false),
      read_max_size_to_aggress_(false),
      read_min_position_to_hit_(false),
      read_min_position_to_askimprove_(false),
      read_min_position_to_cancel_on_hit_(false),
      read_max_int_spread_to_place_(false),
      read_max_int_level_diff_to_place_(false),
      read_max_int_spread_to_cross_(false),
      read_min_int_spread_to_improve_(false),
      read_num_non_best_bid_levels_monitored_(false),
      read_num_non_best_ask_levels_monitored_(false),
      read_min_distance_for_non_best_(false),
      read_max_distance_for_non_best_(false),
      read_max_bid_ask_order_diff_(false),
      read_min_size_to_quote_(false),
      read_maturity_(false),
      read_min_quote_distance_from_best_(false),
      read_stdev_quote_factor_(false),
      read_min_size_ahead_for_non_best_(false),
      read_ignore_max_pos_check_for_non_best_(false),
      read_max_loss_(false),
      read_max_pnl_(false),
      read_short_term_global_max_loss_(false),
      read_global_max_loss_(false),
      read_max_opentrade_loss_(false),
      read_max_opentrade_loss_per_uts_(false),
      read_max_drawdown_(false),
      read_max_short_term_loss_(false),
      read_place_cancel_cooloff_(false),
      read_cooloff_interval_(false),
      read_agg_cooloff_interval_(false),
      read_improve_cooloff_interval_(false),
      // read_highpos_aversion_msecs_ ( false ),
      read_stdev_fact_(false),
      read_stdev_cap_(false),
      read_stdev_duration_(false),
      read_px_band_(false),
      read_low_stdev_lvl_(false),
      read_min_size_to_join_(false),
      read_spread_add_(false),
      read_severity_to_getflat_on_(false),
      read_ezone_traded_(false),

      read_max_severity_to_keep_suporders_on_(false),
      read_agg_closeout_utc_hhmm_(false),
      read_agg_closeout_max_size_(false),

      read_break_msecs_on_max_opentrade_loss_(false),
      read_indep_safe_ticks_(false),
      read_indep_safe_size_(false),
      read_indep_safe_price_diff_(false),
      read_indep_safe_ticks_low_pos_(false),
      read_indep_safe_size_low_pos_(false),
      read_max_unit_ratio_pp_(false),
      read_mini_keep_thresh_(false),
      read_mini_agg_thresh_(false),
      read_projected_price_duration_(false),
      read_pp_place_keep_diff_(false),
      read_enforce_min_cooloff_(false),
      use_throttle_manager_(false),
      throttle_message_limit_(135),
      paramfilename_(StrategyDesc::GetRollParam(_paramfilename_, r_tradingdate_)),
      read_pclose_factor_(false),
      read_moderate_time_limit_(false),
      read_high_time_limit_(false),
      read_safe_cancel_size_(false),

      read_place_on_trade_update_implied_quote_(false),

      read_num_increase_ticks_to_keep_(false),
      read_num_decrease_ticks_to_keep_(false),

      read_is_liquid_(false),
      read_stir_cancel_on_exec_cooloff_msecs_(false),
      read_stir_cancel_on_level_change_cooloff_msecs_(false),
      read_pair_exec_cancel_cooloff_msecs_(false),

      read_max_global_risk_(false),
      read_max_global_risk_ratio_(false),

      retail_size_factor_to_offer_(0.5),
      retail_size_factor_to_place_(0.5),
      retail_max_level_to_offer_(3),
      use_retail_trading_flatlogic_(true),
      aggress_above_max_pos_(false),
      retail_position_discount_factor_(2.0),
      retail_size_tolerance_(0.05),
      retail_price_threshold_tolerance_(0.05),
      retail_max_position_step_size_(100),
      retail_max_global_position_step_size_(200),
      retail_place_fok_(false),
      retail_max_pc1_risk_(0),
      retail_max_ord_size_(0),
      owp_retail_offer_thresh_(0.25),
      read_owp_retail_offer_thresh_(false),
      retail_offer_fra_(false),

      read_use_stable_bidask_levels_(false),

      read_dpt_range_(false),
      dpt_range_(2.0),
      desired_position_leeway_(1),
      desired_position_large_difference_(2),
      read_desired_position_leeway_(false),
      read_desired_position_difference_(false),
      read_place_or_cancel_(false),
      place_or_cancel_(false),
      read_position_change_compensation_(false),
      position_change_compensation_(0.0),
      read_liquidity_factor_(false),
      liquidity_factor_(0.0),
      read_online_stats_history_secs_(300),
      read_online_stats_avg_dep_bidask_spread_(false),
      read_place_multiple_orders_(false),
      read_max_unit_size_at_level_(false),
      read_size_diff_between_orders_(false),
      read_size_disclosed_factor_(false),
      use_online_uts_(false),
      uts_per_L1Size(0),
      lookback_dynamic_uts(1),

      use_notional_uts_(false),
      notional_uts_(0.0),
      trade_indicator_string_(" "),
      factor_trade_bias_(0.0),
      cancel_indicator_string_(" "),
      factor_cancel_bias_(0.0),

      regime_indicator_string_vec_(),
      regimes_to_trade_(),
      read_pair_unit_trade_size_factor_(false),
      read_pair_place_diff_(false),
      read_pair_keep_diff_(false),
      read_pair_aggressive_(false),
      read_pair_improve_(false),
      read_pair_zeropos_keep_(false),
      read_pair_zeropos_decrease_diff_(false),
      read_pair_increase_zeropos_diff_(false),
      read_pair_place_keep_diff_(false),
      read_abs_max_pos_factor_(false),
      read_own_base_px_(false),
      read_use_stable_book_(false),
      read_spread_factor_(false),
      read_spread_quote_factor_(false),
      read_volume_ratio_stop_trading_lower_threshold_(false),
      read_volume_ratio_stop_trading_upper_threshold_(false),
      read_moving_bidask_spread_duration_(false),
      read_max_global_beta_adjusted_notional_risk_(false),
      read_self_pos_projection_factor_(false),
      read_place_only_after_cancel_(false),
      read_allow_modify_orders_(false),
      stdev_fact_ticks_(1),
      volume_lower_bound_(0.0),
      volume_upper_bound_(0.0),
      volume_norm_factor_(0.0),
      source_for_dmur_(""),
      read_source_for_dmur_(false),
      max_unit_ratio_structured(100),
      positioning_indicator_(nullptr),
      volume_ratio_indicator_(nullptr),
      self_volume_ratio_indicator_(nullptr),
      stdev_ratio_normalised_indicator_(nullptr),
      self_volume_expected_(0.0),
      self_volume_ratio_threshold_(1.0),
      self_stdev_ratio_threshold_(1.0),
      base_shortcode_structured(dep12_short_code),
      combined_get_flat_model("NoModel"),
      should_get_combined_flat(false),
      number_strats(1),
      mini_top_keep_logic_(false),
      mini_agg_logic_(false),
      mini_agg_ticks_(1),
      mini_safe_indep_agg_ticks_(2),
      mini_safe_indep_agg_spread_scale_factor(0.3),
      mini_improve_logic_(false),
      indep_safe_improve_ticks_(1),
      indep_safe_improve_size_(1),
      mkt_price_improve_bias_(0.5),
      mini_agg_sweep_margin_(10000),
      mini_sweep_cancel_cooloff_(0),
      spread_factor_agg_(0),
      spread_factor_place_(0),
      fixed_factor_agg_(1000000),
      fixed_factor_place_(100000),
      price_type_("MktSizeWPrice"),
      read_spread_factor_agg_(false),
      use_pre_getflat_(false),
      pre_getflat_msecs_(300 * 1000),
      pre_getflat_multiplier_(0),
      allow_to_aggress_on_getflat_(false),
      getflat_aggress_(1),
      max_spread_getflat_aggress_(1),
      use_min_portfolio_risk_(false),
      source_stdev_risk_scaling_(""),
      use_min_risk_(0),
      af_event_id_(0),
      af_event_halflife_secs_(-1),
      af_event_drawdown_secs_(-1),
      af_event_pxch_act_(1000),
      af_event_pxch_getflat_(1000),
      af_event_max_uts_pxch_(0),
      read_af_event_pxch_act_(false),
      read_af_event_pxch_getflat_(false),
      read_af_event_max_uts_pxch_(false),
      af_source_shc_(""),
      af_use_source_trade_(false),
      af_dd_maxpxch_factor_(-1),
      af_dd_predpxch_factor_(-1),
      af_use_mult_levels_(1),
      af_scale_beta_(""),
      read_af_scale_beta_(false),
      af_risk_scaling_(0),
      arb_place_thresh_(100),
      num_lvl_order_keep_(5),
      arb_best_shc_(""),
      place_only_after_cancel_(false),
      allow_modify_orders_(false),
      max_fraction_uts_size_to_modify_(0.3),
      read_bigtrades_cancel_(false),
      bigtrades_cancel_threshold_(100),
      bigtrades_window_msecs_(0),
      bigtrades_cooloff_interval_(0),
      bigtrades_decay_factor_(1),
      bigtrades_source_id_(""),
      read_bigtrades_place_(false),
      bigtrades_place_threshold_(100),
      bigtrades_place_window_msecs_(0),
      bigtrades_place_cooloff_interval_(0),
      bigtrades_place_decay_factor_(1),
      bigtrades_place_source_id_(""),
      read_bigtrades_aggress_(false),
      bigtrades_aggress_threshold_(100),
      bigtrades_aggress_window_msecs_(0),
      bigtrades_aggress_cooloff_interval_(0),
      bigtrades_aggress_decay_factor_(1),
      bigtrades_aggress_source_id_(""),
      read_l1_trade_volume_bias_(false),
      l1_trade_volume_bias_(10000000000),
      l1_bid_ask_flow_num_events_(0),
      read_l1_bid_ask_flow_cancel_thresh_(false),
      l1_bid_ask_flow_cancel_thresh_(0.0),
      l1_bid_ask_flow_cancel_keep_thresh_diff_(0.0),
      min_size_to_flow_based_cancel_(-1),
      l1_bias_cancel_thresh_(0.0),
      read_cancel_on_market_tilt_source_(false),
      cancel_on_market_tilt_source_(""),
      cancel_on_market_tilt_thresh_(0.0),
      read_use_notional_uts_(false),
      read_notional_uts_(false),
      read_regime_indicator_string_(false),
      read_regimes_to_trade_(false),
      read_trade_indicator_string_(false),
      read_trade_bias_(false),
      read_cancel_indicator_string_(false),
      read_cancel_bias_(false),
      implied_mkt_indicator_string_(" "),
      read_implied_mkt_indicator_string_(false),
      read_implied_mkt_(false),
      implied_mkt_thres_factor_(-100),
      max_size_to_cancel_(2000),
      read_dynamic_zeropos_keep_(false),
      synth_agg_exec_(false),
      delta_hedge_lower_threshold_(0.0),
      delta_hedge_upper_threshold_(0.0),
      fractional_second_implied_vol_(300),
      spread_to_target_(0),
      read_src_based_exec_changes_(false),
      source_for_mkt_condition_(""),
      src_trend_percentile_(0.8),
      src_trend_factor_(1.0),
      src_trend_keep_factor_(0.4),
      sumvars_scaling_factor_(1.0),
      last_day_vol_factor_(1.0),
      read_last_day_vol_(false),
      stdev_overall_cap_(1.0),
      read_stdev_overall_cap_(false),
      max_trade_size_(1),
      read_max_trade_size_(false),
      read_use_cancellation_model_(false),
      read_canc_prob_(false),
      canc_prob_(1.0),
      cancel_prob_thresh_(0.5),
      interday_scaling_factor_(1.0),
      read_interday_scaling_factor_(false),
      max_distance_to_keep_from_band_price_(1),
      mur_reset_time_(0),
      mur_reset_value_(0),
      min_order_size_(0),
      check_mur_reset_time_(false),
      release_core_premature_(true),
      is_mur_low_(false),
      recompute_signal_(true),
      feature_modelfile_(""),
      read_feature_modelfile_(false),
      feature_threshold_(0.0),
      read_feature_threshold_(false),
      feature_more_(1.0),
      not_to_trade_(false),
      project_to_illiquid_(false) {
  for (size_t i = 0; i < EZONE_MAXLEN; i++) {
    ezone_vec_[i] = EZ_MAX;  // undef
  }
  LoadParamSet(dep12_short_code);
}

void ParamSet::LoadParamSet(std::string dep_short_code) {
  std::ifstream paramfile_;
  paramfile_.open(paramfilename_.c_str(), std::ifstream::in);
  if (paramfile_.is_open()) {
    const int kParamFileLineBufferLen = 1024;
    char readline_buffer_[kParamFileLineBufferLen];
    bzero(readline_buffer_, kParamFileLineBufferLen);

    while (paramfile_.good()) {
      bzero(readline_buffer_, kParamFileLineBufferLen);
      paramfile_.getline(readline_buffer_, kParamFileLineBufferLen);
      std::string param_line(readline_buffer_);
      PerishableStringTokenizer st_(readline_buffer_, kParamFileLineBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() < 1) continue;

      // look at the second token and depending on string fill in the appropriate variable from the third token
      // example :
      // PARAMVALUE WORST_CASE_POSITION 60 # comments ...
      // PARAMVALUE MAX_POSITION 30 # comments ...
      // PARAMVALUE UNIT_TRADE_SIZE 5 # comments ...
      if ((strcmp(tokens_[0], "PARAMVALUE") == 0) && (tokens_.size() >= 3)) {
        if (strcmp(tokens_[1], "WORST_CASE_POSITION") == 0) {
          worst_case_position_ = atoi(tokens_[2]);
          read_worst_case_position_ = true;
        } else if (strcmp(tokens_[1], "WORST_CASE_UNIT_RATIO") == 0) {
          worst_case_unit_ratio_ = atoi(tokens_[2]);
          read_worst_case_unit_ratio_ = true;
        } else if (strcmp(tokens_[1], "MAX_POSITION") == 0) {
          max_position_ = atoi(tokens_[2]);
          max_position_original_ = max_position_;
          read_max_position_ = true;
        } else if (strcmp(tokens_[1], "MAX_GLOBAL_POSITION") == 0) {
          max_global_position_ = atoi(tokens_[2]);
          read_max_global_position_ = true;
        } else if (strcmp(tokens_[1], "MAX_SECURITY_POSITION") == 0) {
          max_security_position_ = atoi(tokens_[2]);
          read_max_security_position_ = true;
        } else if (strcmp(tokens_[1], "MAX_UNIT_RATIO") == 0) {
          max_unit_ratio_ = atof(tokens_[2]);
          read_max_unit_ratio_ = true;
        } else if (strcmp(tokens_[1], "MAX_TOTAL_UNIT_RATIO_TO_PLACE") == 0) {
          max_total_unit_ratio_to_place_ = atof(tokens_[2]);
          read_max_total_unit_ratio_to_place_ = true;
        } else if (strcmp(tokens_[1], "MAX_TOTAL_SIZE_TO_PLACE") == 0) {
          max_total_size_to_place_ = atof(tokens_[2]);
          read_max_total_size_to_place_ = true;
        } else if (strcmp(tokens_[1], "EXPLICIT_MAX_SHORT_POSITION") == 0) {
          explicit_max_short_position_ = atoi(tokens_[2]);
          read_explicit_max_short_position_ = true;
        } else if (strcmp(tokens_[1], "EXPLICIT_MAX_SHORT_UNIT_RATIO") == 0) {
          explicit_max_short_unit_ratio_ = atoi(tokens_[2]);
          read_explicit_max_short_unit_ratio_ = true;
        } else if (strcmp(tokens_[1], "EXPLICIT_WORST_CASE_SHORT_POSITION") == 0) {
          explicit_worst_case_short_position_ = atoi(tokens_[2]);
          read_explicit_worst_case_short_position_ = true;
        } else if (strcmp(tokens_[1], "EXPLICIT_WORST_CASE_SHORT_UNIT_RATIO") == 0) {
          explicit_worst_case_short_unit_ratio_ = atoi(tokens_[2]);
          read_explicit_worst_case_short_unit_ratio_ = true;
        } else if (strcmp(tokens_[1], "EXPLICIT_MAX_LONG_POSITION") == 0) {
          explicit_max_long_position_ = atoi(tokens_[2]);
          read_explicit_max_long_position_ = true;
        } else if (strcmp(tokens_[1], "EXPLICIT_MAX_LONG_UNIT_RATIO") == 0) {
          explicit_max_long_unit_ratio_ = atoi(tokens_[2]);
          read_explicit_max_long_unit_ratio_ = true;
        } else if (strcmp(tokens_[1], "EXPLICIT_WORST_CASE_LONG_POSITION") == 0) {
          explicit_worst_case_long_position_ = atoi(tokens_[2]);
          read_explicit_worst_case_long_position_ = true;
        } else if (strcmp(tokens_[1], "EXPLICIT_WORST_CASE_LONG_UNIT_RATIO") == 0) {
          explicit_worst_case_long_unit_ratio_ = atoi(tokens_[2]);
          read_explicit_worst_case_long_unit_ratio_ = true;
        } else if (strcmp(tokens_[1], "NON_STANDARD_MARKET_CONDITION_CHECK_SHORT_MSECS") == 0) {
          non_standard_market_condition_check_short_msecs_ = atoi(tokens_[2]);
          read_non_standard_market_condition_check_short_msecs_ = true;
        } else if (strcmp(tokens_[1], "NON_STANDARD_MARKET_CONDITION_CHECK_LONG_MSECS") == 0) {
          non_standard_market_condition_check_long_msecs_ = atoi(tokens_[2]);
          read_non_standard_market_condition_check_long_msecs_ = true;
        } else if (strcmp(tokens_[1], "NON_STANDARD_MARKET_CONDITION_MIN_BEST_LEVEL_SIZE") == 0) {
          non_standard_market_condition_min_best_level_size_ = atoi(tokens_[2]);
          read_non_standard_market_condition_min_best_level_size_ = true;
        } else if (strcmp(tokens_[1], "NON_STANDARD_MARKET_CONDITION_MIN_BEST_LEVEL_ORDER_COUNT") == 0) {
          non_standard_market_condition_min_best_level_order_count_ = atoi(tokens_[2]);
          read_non_standard_market_condition_min_best_level_order_count_ = true;
        } else if (strcmp(tokens_[1], "NON_STANDARD_MARKET_CONDITION_MAX_AVG_ORDER_SIZE") == 0) {
          non_standard_market_condition_max_avg_order_size_ = atoi(tokens_[2]);
          read_non_standard_market_condition_max_avg_order_size_ = true;
        } else if (strcmp(tokens_[1], "NON_STANDARD_MARKET_CONDITION_MIN_COUNTER_ORDER_SIZE") == 0) {
          non_standard_market_condition_min_counter_order_size_ = atoi(tokens_[2]);
          read_non_standard_market_condition_min_counter_order_size_ = true;
        } else if (strcmp(tokens_[1], "NON_STANDARD_MARKET_CONDITION_MAX_SPREAD") == 0) {
          non_standard_market_condition_max_spread_ = atoi(tokens_[2]);
          read_non_standard_market_condition_max_spread_ = true;
        } else if (strcmp(tokens_[1], "NON_STANDARD_MARKET_CONDITION_MAX_POSITION") == 0) {
          non_standard_market_condition_max_position_ = atoi(tokens_[2]);
          read_non_standard_market_condition_max_position_ = true;
        } else if (strcmp(tokens_[1], "UNIT_TRADE_SIZE") == 0) {
          unit_trade_size_ = atoi(tokens_[2]);
          read_unit_trade_size_ = true;
          desired_position_leeway_ = unit_trade_size_;                // default values
          desired_position_large_difference_ = 2 * unit_trade_size_;  // default values
        } else if (strcmp(tokens_[1], "MIN_ALLOWED_UNIT_TRADE_SIZE") == 0) {
          min_allowed_unit_trade_size_ = atoi(tokens_[2]);
          read_min_allowed_unit_trade_size_ = true;
        } else if (strcmp(tokens_[1], "HIGHPOS_LIMITS") == 0) {
          highpos_limits_ = atof(tokens_[2]);
          read_highpos_limits_ = true;
        } else if (strcmp(tokens_[1], "HIGHPOS_LIMITS_UNIT_RATIO") == 0) {
          highpos_limits_unit_ratio_ = atof(tokens_[2]);
          read_highpos_limits_unit_ratio_ = true;
        } else if (strcmp(tokens_[1], "HIGHPOS_THRESH_FACTOR") == 0) {
          highpos_thresh_factor_ = atof(tokens_[2]);
          read_highpos_thresh_factor_ = true;
        } else if (strcmp(tokens_[1], "HIGHPOS_THRESH_DECREASE") == 0) {
          highpos_thresh_decrease_ = atof(tokens_[2]);
          read_highpos_thresh_decrease_ = true;
        } else if (strcmp(tokens_[1], "HIGHPOS_SIZE_FACTOR") == 0) {
          highpos_size_factor_ = std::max(0.0, std::min(1.0, atof(tokens_[2])));
          read_highpos_size_factor_ = true;
        } else if (strcmp(tokens_[1], "INCREASE_PLACE") == 0) {
          increase_place_ = atof(tokens_[2]);
          read_increase_place_ = true;
        } else if (strcmp(tokens_[1], "INCREASE_KEEP") == 0) {
          increase_keep_ = atof(tokens_[2]);
          read_increase_keep_ = true;
        } else if (strcmp(tokens_[1], "ZEROPOS_LIMITS") == 0) {
          zeropos_limits_ = atof(tokens_[2]);
          read_zeropos_limits_ = true;
        } else if (strcmp(tokens_[1], "ZEROPOS_LIMITS_UNIT_RATIO") == 0) {
          zeropos_limits_unit_ratio_ = atof(tokens_[2]);
          read_zeropos_limits_unit_ratio_ = true;
        } else if (strcmp(tokens_[1], "ZEROPOS_PLACE") == 0) {
          zeropos_place_ = atof(tokens_[2]);
          read_zeropos_place_ = true;
        } else if (strcmp(tokens_[1], "ZEROPOS_KEEP") == 0) {
          zeropos_keep_ = atof(tokens_[2]);
          read_zeropos_keep_ = true;
        } else if (strcmp(tokens_[1], "DECREASE_PLACE") == 0) {
          decrease_place_ = atof(tokens_[2]);
          read_decrease_place_ = true;
        } else if (strcmp(tokens_[1], "DECREASE_KEEP") == 0) {
          decrease_keep_ = atof(tokens_[2]);
          read_decrease_keep_ = true;
        } else if (strcmp(tokens_[1], "SAFE_DISTANCE") == 0) {
          safe_distance_ = std::max(0.0, atof(tokens_[2]));
          read_safe_distance_ = true;
        } else if (strcmp(tokens_[1], "SPREAD_INCREASE") == 0) {
          spread_increase_ = std::max(0.0, atof(tokens_[2]));
          read_spread_increase_ = true;
        } else if (strcmp(tokens_[1], "MAX_MIN_DIFF") == 0) {
          max_min_diff_ = std::max(0.0, atof(tokens_[2]));
          read_max_min_diff_ = true;
        } else if (strcmp(tokens_[1], "L1BIAS_MODEL_STDEV") == 0) {
          l1bias_model_stdev_ = std::max(0.0, atof(tokens_[2]));
        } else if (strcmp(tokens_[1], "L1BIAS_WEIGHT_CAP") == 0) {
          l1bias_weight_cap_ = std::max(0.0, atof(tokens_[2]));
        } else if (strcmp(tokens_[1], "L1BIAS_MODEL_STDEV_DURATION") == 0) {
          l1bias_model_stdev_duration_ = std::max(0, atoi(tokens_[2]));
        } else if (strcmp(tokens_[1], "DAT_WEIGHTAGE_FRACTION") == 0) {
          dat_weightage_fraction_ = std::max(0.0, std::min(1.0, atof(tokens_[2])));
          read_dat_weightage_fraction_ = true;
        } else if (strcmp(tokens_[1], "BUCKET_SIZE") == 0) {
          bucket_size_ = atoi(tokens_[2]);
          read_bucket_size_ = true;
        } else if (strcmp(tokens_[1], "POSITIONING_THRESH_DECREASE") == 0) {
          positioning_thresh_decrease_ = std::max(0.0, atof(tokens_[2]));
          read_positioning_thresh_decrease_ = true;
        } else if (strcmp(tokens_[1], "POSITIONING_THRESHOLD") == 0) {
          positioning_threshold_ = std::max(0.0, atof(tokens_[2]));
          read_positioning_threshold_ = true;
        } else if (strcmp(tokens_[1], "MAX_MIN_DIFF_ORDER") == 0) {
          max_min_diff_order_ = std::max(0.0, atof(tokens_[2]));
          read_max_min_diff_order_ = true;
        } else if (strcmp(tokens_[1], "SCALE_MAX_POS") == 0) {
          read_scale_max_pos_ = true;
          volume_history_secs_ = std::max(60.0, atof(tokens_[2]));
        } else if (strcmp(tokens_[1], "BASE_MUR") == 0) {
          base_mur_ = std::max(0, atoi(tokens_[2]));
        } else if (strcmp(tokens_[1], "VOL_UPPER_BOUND_RATIO") == 0) {
          volume_upper_bound_ratio_ = std::max(0.0, atof(tokens_[2]));
        } else if (strcmp(tokens_[1], "VOL_LOWER_BOUND_RATIO") == 0) {
          volume_lower_bound_ratio_ = std::max(0.0, atof(tokens_[2]));
        } else if (strcmp(tokens_[1], "SELF_VOL_RATIO_THRESHOLD") == 0) {
          self_volume_ratio_threshold_ = std::max(0.0, atof(tokens_[2]));
        } else if (strcmp(tokens_[1], "SELF_STDEV_RATIO_THRESHOLD") == 0) {
          self_stdev_ratio_threshold_ = std::max(0.0, atof(tokens_[2]));
        } else if (strcmp(tokens_[1], "SOURCE_FOR_DMUR") == 0) {
          read_source_for_dmur_ = true;
          source_for_dmur_ = tokens_[2];
        } else if (strcmp(tokens_[1], "HIGH_UTS_FACTOR") == 0) {
          read_high_uts_factor_ = true;
          high_uts_factor_ = std::max(0.0, atof(tokens_[2]));
        } else if (strcmp(tokens_[1], "HIGH_UTS_L1_FACTOR") == 0) {
          max_high_uts_l1_factor_ = std::max(0.0, atof(tokens_[2]));
          read_max_high_uts_l1_factor_ = true;
        } else if (strcmp(tokens_[1], "IS_LIQUID") == 0) {
          if (atoi(tokens_[2]) == 0) {
            is_liquid_ = false;
          }
          read_is_liquid_ = true;
        } else if (strcmp(tokens_[1], "PROJECT_TO_ILLIQUID") == 0) {
          project_to_illiquid_ = (atoi(tokens_[2]) != 0);
        } else if (strcmp(tokens_[1], "STIR_CANCEL_ON_EXEC_COOLOFF_MSECS") == 0) {
          stir_cancel_on_exec_cooloff_msecs_ = atoi(tokens_[2]);
          read_stir_cancel_on_exec_cooloff_msecs_ = true;
        } else if (strcmp(tokens_[1], "STIR_CANCEL_ON_LEVEL_CHANGE_COOLOFF_MSECS") == 0) {
          stir_cancel_on_level_change_cooloff_msecs_ = atoi(tokens_[2]);
          read_stir_cancel_on_level_change_cooloff_msecs_ = true;
        } else if (strcmp(tokens_[1], "PAIR_EXEC_CANCEL_COOLOFF_MSECS") == 0) {
          pair_exec_cancel_cooloff_msecs_ = atoi(tokens_[2]);
          read_pair_exec_cancel_cooloff_msecs_ = true;
        } else if (strcmp(tokens_[1], "THRESH_INCREASE") == 0) {
          thresh_increase_ = std::max(0.0, atof(tokens_[2]));
          read_thresh_increase_ = true;
        } else if (strcmp(tokens_[1], "THRESH_DECREASE") == 0) {
          thresh_decrease_ = std::max(0.0, atof(tokens_[2]));
          read_thresh_decrease_ = true;
        } else if (strcmp(tokens_[1], "THRESH_PLACE") == 0) {
          thresh_place_ = std::max(0.0, atof(tokens_[2]));
          read_thresh_place_ = true;
        } else if (strcmp(tokens_[1], "THRESH_PLACE_KEEP_DIFF") == 0) {
          thresh_place_keep_diff_ = std::max(0.0, atof(tokens_[2]));
          read_thresh_place_keep_diff_ = true;
        } else if (strcmp(tokens_[1], "PLACE_KEEP_DIFF") == 0) {
          place_keep_diff_ = std::max(0.0, atof(tokens_[2]));
          read_place_keep_diff_ = true;
        } else if (strcmp(tokens_[1], "INCREASE_ZEROPOS_DIFF") == 0) {
          increase_zeropos_diff_ = atof(tokens_[2]);
          read_increase_zeropos_diff_ = true;
        } else if (strcmp(tokens_[1], "ZEROPOS_DECREASE_DIFF") == 0) {
          zeropos_decrease_diff_ = atof(tokens_[2]);
          read_zeropos_decrease_diff_ = true;
        } else if (strcmp(tokens_[1], "HIGH_SPREAD_ALLOWANCE") == 0) {
          high_spread_allowance_ = atof(tokens_[2]);
          read_high_spread_allowance_ = true;
        } else if (strcmp(tokens_[1], "ALLOWED_TO_IMPROVE") == 0) {
          allowed_to_improve_ = (atoi(tokens_[2]) != 0);
          read_allowed_to_improve_ = true;
        } else if (strcmp(tokens_[1], "ALLOWED_TO_AGGRESS") == 0) {
          allowed_to_aggress_ = (atoi(tokens_[2]) != 0);
          read_allowed_to_aggress_ = true;
        } else if (strcmp(tokens_[1], "USE_NEW_AGGRESS_LOGIC") == 0) {
          use_new_aggress_logic_ = (atoi(tokens_[2]) != 0);
          read_use_new_aggress_logic_ = true;
        } else if (strcmp(tokens_[1], "IMPROVE") == 0) {
          improve_ = atof(tokens_[2]);
          read_improve_ = true;
        } else if (strcmp(tokens_[1], "SUMVAR_SCALING_FACTOR") == 0) {
          if (atof(tokens_[2]) > 0.01) {
            sumvars_scaling_factor_ = atof(tokens_[2]);
          } else {
            sumvars_scaling_factor_ = 1.0;
          }
        } else if (strcmp(tokens_[1], "AGGRESSIVE") == 0) {
          aggressive_ = atof(tokens_[2]);
          read_aggressive_ = true;
        } else if (strcmp(tokens_[1], "MAX_SELF_RATIO_AT_LEVEL") == 0) {
          max_self_ratio_at_level_ = std::max(0.01, std::min(1.00, atof(tokens_[2])));
          read_max_self_ratio_at_level_ = true;
        } else if (strcmp(tokens_[1], "QUEUE_LONGEVITY_SUPPORT") == 0) {
          longevity_support_ = atof(tokens_[2]);
          read_longevity_support_ = true;
        } else if (strcmp(tokens_[1], "LONGEVITY_VERSION") == 0) {
          longevity_version_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "LONGEVITY_MIN_STDEV_RATIO") == 0) {
          longevity_min_stdev_ratio_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "LONGEVITY_STDEV_DUR") == 0) {
          longevity_stdev_dur_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "MAX_POSITION_TO_AGGRESS_UNIT_RATIO") == 0) {
          if (read_unit_trade_size_) {
            max_position_to_lift_ = atoi(tokens_[2]) * unit_trade_size_;
            min_position_to_hit_ = -max_position_to_lift_;
            read_max_position_to_lift_ = true;
            read_min_position_to_hit_ = true;
          }
        } else if (strcmp(tokens_[1], "MAX_POSITION_TO_AGGRESS") == 0) {
          max_position_to_lift_ = atoi(tokens_[2]);
          min_position_to_hit_ = -max_position_to_lift_;
          read_max_position_to_lift_ = true;
          read_min_position_to_hit_ = true;
        } else if (strcmp(tokens_[1], "MAX_POSITION_TO_IMPROVE_UNIT_RATIO") == 0) {
          if (read_unit_trade_size_) {
            max_position_to_bidimprove_ = atoi(tokens_[2]) * unit_trade_size_;
            min_position_to_askimprove_ = -max_position_to_bidimprove_;
            read_min_position_to_askimprove_ = true;
            read_max_position_to_bidimprove_ = true;
          }
        } else if (strcmp(tokens_[1], "MAX_POSITION_TO_IMPROVE") == 0) {
          max_position_to_bidimprove_ = atoi(tokens_[2]);
          min_position_to_askimprove_ = -max_position_to_bidimprove_;
          read_min_position_to_askimprove_ = true;
          read_max_position_to_bidimprove_ = true;
        } else if (strcmp(tokens_[1], "MAX_POSITION_TO_LIFT") == 0) {
          max_position_to_lift_ = atoi(tokens_[2]);
          read_max_position_to_lift_ = true;
        } else if (strcmp(tokens_[1], "MAX_POSITION_TO_BIDIMPROVE") == 0) {
          max_position_to_bidimprove_ = atoi(tokens_[2]);
          read_max_position_to_bidimprove_ = true;
        } else if (strcmp(tokens_[1], "MAX_POSITION_TO_CANCEL_ON_AGGRESS_UNIT_RATIO") == 0) {
          if (read_unit_trade_size_) {
            max_position_to_cancel_on_lift_ = atoi(tokens_[2]) * unit_trade_size_;
            min_position_to_cancel_on_hit_ = -max_position_to_cancel_on_lift_;
            read_min_position_to_cancel_on_hit_ = true;
            read_max_position_to_cancel_on_lift_ = true;
          }
        } else if (strcmp(tokens_[1], "MAX_POSITION_TO_CANCEL_ON_AGGRESS") == 0) {
          max_position_to_cancel_on_lift_ = atoi(tokens_[2]);
          min_position_to_cancel_on_hit_ = -max_position_to_cancel_on_lift_;
          read_min_position_to_cancel_on_hit_ = true;
          read_max_position_to_cancel_on_lift_ = true;
        } else if (strcmp(tokens_[1], "MAX_POSITION_TO_CANCEL_ON_LIFT") == 0) {
          max_position_to_cancel_on_lift_ = atoi(tokens_[2]);
          read_max_position_to_cancel_on_lift_ = true;
        } else if (strcmp(tokens_[1], "MAX_SIZE_TO_AGGRESS") == 0) {
          max_size_to_aggress_ = atoi(tokens_[2]);
          read_max_size_to_aggress_ = true;
        } else if (strcmp(tokens_[1], "MIN_POSITION_TO_HIT") == 0) {
          min_position_to_hit_ = atoi(tokens_[2]);
          read_min_position_to_hit_ = true;
        } else if (strcmp(tokens_[1], "MIN_POSITION_TO_ASKIMPROVE") == 0) {
          min_position_to_askimprove_ = atoi(tokens_[2]);
          read_min_position_to_askimprove_ = true;
        } else if (strcmp(tokens_[1], "MIN_POSITION_TO_CANCEL_ON_HIT") == 0) {
          min_position_to_cancel_on_hit_ = atoi(tokens_[2]);
          read_min_position_to_cancel_on_hit_ = true;
        } else if (strcmp(tokens_[1], "MAX_INT_SPREAD_TO_PLACE") == 0) {
          max_int_spread_to_place_ = atoi(tokens_[2]);
          read_max_int_spread_to_place_ = true;
        } else if (strcmp(tokens_[1], "MAX_INT_LEVEL_DIFF_TO_PLACE") == 0) {
          max_int_level_diff_to_place_ = atoi(tokens_[2]);
          read_max_int_level_diff_to_place_ = true;
        } else if (strcmp(tokens_[1], "MAX_INT_SPREAD_TO_CROSS") == 0) {
          max_int_spread_to_cross_ = atoi(tokens_[2]);
          read_max_int_spread_to_cross_ = true;
        } else if (strcmp(tokens_[1], "MIN_INT_SPREAD_TO_IMPROVE") == 0) {
          min_int_spread_to_improve_ = atoi(tokens_[2]);
          read_min_int_spread_to_improve_ = true;
        } else if (strcmp(tokens_[1], "NUM_NON_BEST_LEVELS_MONITORED") == 0) {
          unsigned int num_non_best_levels_monitored_ = atoi(tokens_[2]);

          if (!read_num_non_best_bid_levels_monitored_) {
            num_non_best_bid_levels_monitored_ = num_non_best_levels_monitored_;
            read_num_non_best_bid_levels_monitored_ = true;
          }
          if (!read_num_non_best_ask_levels_monitored_) {
            num_non_best_ask_levels_monitored_ = num_non_best_levels_monitored_;
            read_num_non_best_ask_levels_monitored_ = true;
          }
        } else if (strcmp(tokens_[1], "NUM_NON_BEST_BID_LEVELS_MONITORED") == 0) {
          num_non_best_bid_levels_monitored_ = atoi(tokens_[2]);
          read_num_non_best_bid_levels_monitored_ = true;
        } else if (strcmp(tokens_[1], "SPREAD_FACTOR_AGG") == 0) {
          spread_factor_agg_ = atof(tokens_[2]);
          read_spread_factor_agg_ = true;
        } else if (strcmp(tokens_[1], "SPREAD_FACTOR_PLACE") == 0) {
          spread_factor_place_ = atof(tokens_[2]);
          read_spread_factor_agg_ = true;
        } else if (strcmp(tokens_[1], "FIXED_FACTOR_AGG") == 0) {
          fixed_factor_agg_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "FIXED_FACTOR_PLACE") == 0) {
          fixed_factor_place_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "NUM_NON_BEST_ASK_LEVELS_MONITORED") == 0) {
          num_non_best_ask_levels_monitored_ = atoi(tokens_[2]);
          read_num_non_best_ask_levels_monitored_ = true;
        } else if (strcmp(tokens_[1], "MIN_DISTANCE_FOR_NON_BEST") == 0) {
          min_distance_for_non_best_ = atoi(tokens_[2]);
          read_min_distance_for_non_best_ = true;
          if (!read_max_distance_for_non_best_) {
            max_distance_for_non_best_ = min_distance_for_non_best_ + 4;
          }
        } else if (strcmp(tokens_[1], "MAX_DISTANCE_FOR_NON_BEST") == 0) {
          max_distance_for_non_best_ = atoi(tokens_[2]);
          read_max_distance_for_non_best_ = true;
        } else if (strcmp(tokens_[1], "MAX_BID_ASK_ORDER_DIFF") == 0) {
          max_bid_ask_order_diff_ = atoi(tokens_[2]);
          read_max_bid_ask_order_diff_ = true;
        } else if (strcmp(tokens_[1], "MIN_SIZE_TO_QUOTE") == 0) {
          min_size_to_quote_ = atoi(tokens_[2]);
          read_min_size_to_quote_ = true;
        } else if (strcmp(tokens_[1], "MATURITY") == 0) {
          maturity_ = atoi(tokens_[2]);
          read_maturity_ = true;
        } else if (strcmp(tokens_[1], "MIN_QUOTE_DISTANCE_FROM_BEST") == 0) {
          min_quote_distance_from_best_ = atoi(tokens_[2]);
          read_min_quote_distance_from_best_ = true;
        } else if (strcmp(tokens_[1], "STDEV_QUOTE_FACTOR") == 0) {
          stdev_quote_factor_ = atof(tokens_[2]);
          read_stdev_quote_factor_ = true;
        } else if (strcmp(tokens_[1], "MIN_SIZE_AHEAD_FOR_NON_BEST") == 0) {
          min_size_ahead_for_non_best_ = atoi(tokens_[2]);
          read_min_size_ahead_for_non_best_ = true;
        } else if (strcmp(tokens_[1], "IGNORE_MAX_POS_CHECK_FOR_NON_BEST") == 0) {
          ignore_max_pos_check_for_non_best_ = (atoi(tokens_[2]) > 0 ? true : false);
          read_ignore_max_pos_check_for_non_best_ = true;
        } else if (strcmp(tokens_[1], "MAX_LOSS") == 0) {
          max_loss_ = atoi(tokens_[2]);
          read_max_loss_ = true;
          if (!read_max_opentrade_loss_) {
            max_opentrade_loss_ = max_loss_;
            if (!read_max_short_term_loss_) {  // for current params ... just assume SHORT_TERM = 1.5 * OPENTRADE
              max_short_term_loss_ = 1.5 * max_opentrade_loss_;
            }
            // if ( ! read_max_pertrade_loss_ )
            // 	{ // assume PERTRADE = OPENTRADE
            // 	  max_pertrade_loss_ = max_opentrade_loss_;
            // 	}
          }
        } else if (strcmp(tokens_[1], "MAX_PNL") == 0) {
          max_pnl_ = atoi(tokens_[2]);
          read_max_pnl_ = true;
        } else if (strcmp(tokens_[1], "GLOBAL_MAX_LOSS") == 0) {
          global_max_loss_ = atoi(tokens_[2]);
          read_global_max_loss_ = true;
        } else if (strcmp(tokens_[1], "SHORT_TERM_GLOBAL_MAX_LOSS") == 0) {
          short_term_global_max_loss_ = atoi(tokens_[2]);
          read_short_term_global_max_loss_ = true;
        } else if (strcmp(tokens_[1], "MAX_OPENTRADE_LOSS") == 0) {
          if (!read_max_opentrade_loss_per_uts_) {
            max_opentrade_loss_ = atoi(tokens_[2]);
          } else {
            max_opentrade_loss_ = max_opentrade_loss_per_uts_ * unit_trade_size_;
          }
          read_max_opentrade_loss_ = true;
          if (!read_max_short_term_loss_) {  // for current params ... just assume SHORT_TERM = 1.5 * OPENTRADE
            max_short_term_loss_ = 1.5 * max_opentrade_loss_;
          }
          // if ( ! read_max_pertrade_loss_ )
          //   { // assume PERTRADE = OPENTRADE
          //     max_pertrade_loss_ = max_opentrade_loss_;
          //   }
        } else if (strcmp(tokens_[1], "MAX_OPENTRADE_LOSS_PER_UTS") == 0) {
          read_max_opentrade_loss_per_uts_ = true;
          max_opentrade_loss_per_uts_ = atoi(tokens_[2]);
          max_opentrade_loss_ = max_opentrade_loss_per_uts_ * unit_trade_size_;
        }

        else if (strcmp(tokens_[1], "MAX_SHORT_TERM_LOSS") == 0) {
          max_short_term_loss_ = atoi(tokens_[2]);
          read_max_short_term_loss_ = true;
          // if ( ! read_max_pertrade_loss_ )
          //   { // assume PERTRADE = SHORT_TERM
          //     max_pertrade_loss_ = max_short_term_loss_;
          //   }
          // } else if ( strcmp ( tokens_ [ 1 ] , "MAX_PERTRADE_LOSS" ) == 0 ) {
          //   max_pertrade_loss_ = atoi ( tokens_ [ 2 ] );
          //   read_max_pertrade_loss_ = true;
        } else if (strcmp(tokens_[1], "PLACE_CANCEL_COOLOFF") == 0) {
          place_cancel_cooloff_ = atoi(tokens_[2]);
          read_place_cancel_cooloff_ = true;
        } else if (strcmp(tokens_[1], "MAX_DRAWDOWN") == 0) {
          max_drawdown_ = atoi(tokens_[2]);
          read_max_drawdown_ = true;
          // if ( ! read_max_pertrade_loss_ )
          //   { // assume PERTRADE = SHORT_TERM
          //     max_pertrade_loss_ = max_short_term_loss_;
          //   }
          // } else if ( strcmp ( tokens_ [ 1 ] , "MAX_PERTRADE_LOSS" ) == 0 ) {
          //   max_pertrade_loss_ = atoi ( tokens_ [ 2 ] );
          //   read_max_pertrade_loss_ = true;
        } else if (strcmp(tokens_[1], "COOLOFF_INTERVAL") == 0) {
          cooloff_interval_ = atoi(tokens_[2]);
          read_cooloff_interval_ = true;
        } else if (strcmp(tokens_[1], "AGG_COOLOFF_INTERVAL") == 0) {
          agg_cooloff_interval_ = atoi(tokens_[2]);
          read_agg_cooloff_interval_ = true;
          // } else if ( strcmp ( tokens_[1], "HIGHPOS_AVERSION_MSECS" ) == 0 ) {
          //   highpos_aversion_msecs_ = atoi ( tokens_[2] );
          //   read_highpos_aversion_msecs_ = true;
        } else if (strcmp(tokens_[1], "IMPROVE_COOLOFF_INTERVAL") == 0) {
          improve_cooloff_interval_ = atoi(tokens_[2]);
          read_improve_cooloff_interval_ = true;
        } else if (strcmp(tokens_[1], "STDEV_FACT") == 0) {
          stdev_fact_ = std::max(0.0, atof(tokens_[2]));
          read_stdev_fact_ = true;
        } else if (strcmp(tokens_[1], "STDEV_CAP") == 0) {
          stdev_cap_ = std::max(0.0, atof(tokens_[2]));
          read_stdev_cap_ = true;
        } else if (strcmp(tokens_[1], "STDEV_DURATION") == 0) {
          stdev_duration_ = std::max(1, atoi(tokens_[2]));
          read_stdev_duration_ = true;
        } else if (strcmp(tokens_[1], "STDEV_SUPPRESS_NON_BEST_LVL_THRESHOLD") == 0) {
          stdev_suppress_non_best_level_threshold_ = atof(tokens_[2]);
          should_stdev_suppress_non_best_level_ = true;
        } else if (strcmp(tokens_[1], "STDEV_SUPPRESS_NON_BEST_LVL_DURATION") == 0) {
          stdev_suppress_non_best_level_duration_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "INCREASE_THRESHOLDS_SYMM") == 0) {
          increase_thresholds_symm_ = atoi(tokens_[2]) > 0;
        } else if (strcmp(tokens_[1], "INCREASE_THRESHOLDS_CONTINUOUS") == 0) {
          increase_thresholds_continuous_ = atoi(tokens_[2]) > 0;
        } else if (strcmp(tokens_[1], "PX_BAND") == 0) {
          px_band_ = atoi(tokens_[2]);
          read_px_band_ = true;
        } else if (strcmp(tokens_[1], "LOW_STDEV_LVL") == 0) {
          low_stdev_lvl_ = atof(tokens_[2]);
          read_low_stdev_lvl_ = true;
        } else if (strcmp(tokens_[1], "MIN_SIZE_TO_JOIN") == 0) {
          min_size_to_join_ = atoi(tokens_[2]);
          read_min_size_to_join_ = true;
        } else if (strcmp(tokens_[1], "USE_SQRT_STDEV") == 0) {
          use_sqrt_stdev_ = atoi(tokens_[2]) > 0;
        } else if (strcmp(tokens_[1], "THROTTLE_MSGS_PER_SEC") == 0) {
          throttle_message_limit_ = atoi(tokens_[2]);
          use_throttle_manager_ = true;
        } else if (strcmp(tokens_[1], "EZONE_ADD") == 0) {
          for (unsigned int ez_idx = 2; ez_idx < tokens_.size(); ez_idx++) {
            EconomicZone_t this_ez = GetEZFromStr(tokens_[ez_idx]);
            if (this_ez < EZ_MAX) {
              for (size_t i = 0; i < EZONE_MAXLEN; i++) {
                if (ezone_vec_[i] == EZ_MAX) {
                  ezone_vec_[i] = this_ez;
                  break;
                }
                if (ezone_vec_[i] == this_ez) {
                  break;
                }
              }
            }
          }
        } else if (strcmp(tokens_[1], "SPREAD_ADD") == 0) {
          spread_add_ = atof(tokens_[2]);
          read_spread_add_ = true;
        } else if (strcmp(tokens_[1], "SEVERITY_TO_GETFLAT_ON") == 0) {
          severity_to_getflat_on_ = atof(tokens_[2]);
          read_severity_to_getflat_on_ = true;
        } else if (strcmp(tokens_[1], "EZONE_TRADED") == 0) {
          ezone_traded_ = GetEZFromStr(tokens_[2]);
          read_ezone_traded_ = true;
        } else if (strcmp(tokens_[1], "MAX_SEVERITY_TO_KEEP_SUPORDERS_ON") == 0) {
          max_severity_to_keep_suporders_on_ = atof(tokens_[2]);
          read_max_severity_to_keep_suporders_on_ = true;
        } else if (strcmp(tokens_[1], "AGG_CLOSEOUT_TIME") == 0) {
          if ((strncmp(tokens_[2], "EST_", 4) == 0) || (strncmp(tokens_[2], "CST_", 4) == 0) ||
              (strncmp(tokens_[2], "CET_", 4) == 0) || (strncmp(tokens_[2], "BRT_", 4) == 0) ||
              (strncmp(tokens_[2], "UTC_", 4) == 0) || (strncmp(tokens_[2], "KST_", 4) == 0) ||
              (strncmp(tokens_[2], "HKT_", 4) == 0) || (strncmp(tokens_[2], "MSK_", 4) == 0) ||
              (strncmp(tokens_[2], "IST_", 4) == 0) || (strncmp(tokens_[2], "JST_", 4) == 0) ||
              (strncmp(tokens_[2], "BST_", 4) == 0)) {
            agg_closeout_utc_hhmm_ =
                HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(tokens_[2] + 4), tokens_[2]);
          } else {
            agg_closeout_utc_hhmm_ = atoi(tokens_[2]);
          }

          read_agg_closeout_utc_hhmm_ = true;
        } else if (strcmp(tokens_[1], "ALLOW_AGG_CLOSEOUT_ON_MKT_TILT") == 0) {
          agg_closeout_max_size_ = atoi(tokens_[2]);
          read_agg_closeout_max_size_ = true;
        } else if (strcmp(tokens_[1], "BREAK_MSECS_ON_OPENTRADE_LOSS") == 0) {
          break_msecs_on_max_opentrade_loss_ = std::max(60 * 1000, atoi(tokens_[2]));
          read_break_msecs_on_max_opentrade_loss_ = true;
        } else if (strcmp(tokens_[1], "INDEP_SAFE_TICKS") == 0) {
          indep_safe_ticks_ = atof(tokens_[2]);
          read_indep_safe_ticks_ = true;
        } else if (strcmp(tokens_[1], "INDEP_SAFE_SIZE") == 0) {
          indep_safe_size_ = std::max(1, atoi(tokens_[2]));
          read_indep_safe_size_ = true;
        } else if (strcmp(tokens_[1], "INDEP_SAFE_TICKS_LOW_POS") == 0) {
          indep_safe_ticks_low_pos_ = atof(tokens_[2]);
          read_indep_safe_ticks_low_pos_ = true;
        } else if (strcmp(tokens_[1], "INDEP_SAFE_SIZE_LOW_POS") == 0) {
          indep_safe_size_low_pos_ = std::max(1, atoi(tokens_[2]));
          read_indep_safe_size_low_pos_ = true;
        } else if (strcmp(tokens_[1], "MAX_UNIT_RATIO_PP") == 0) {
          max_unit_ratio_pp_ = std::max(0, atoi(tokens_[2]));
          read_max_unit_ratio_pp_ = true;
        } else if (strcmp(tokens_[1], "INDEP_SAFE_PRICE_DIFF") == 0) {
          indep_safe_price_diff_ = atof(tokens_[2]);
          indep_safe_price_diff_ = std::max(0.0, std::min(1.0, indep_safe_price_diff_));
          read_indep_safe_price_diff_ = true;
        } else if (strcmp(tokens_[1], "MINI_KEEP_THRESH") == 0) {
          mini_keep_thresh_ = std::max(0.0, atof(tokens_[2]));
          read_mini_keep_thresh_ = true;
        } else if (strcmp(tokens_[1], "MINI_AGG_THRESH") == 0) {
          mini_agg_thresh_ = std::max(0.0, atof(tokens_[2]));
          read_mini_agg_thresh_ = true;
        } else if (strcmp(tokens_[1], "PROJECTED_PRICE_DURATION") == 0) {
          projected_price_duration_ = std::max(0.0, atof(tokens_[2]));
          read_projected_price_duration_ = true;
        } else if (strcmp(tokens_[1], "PP_PLACE_KEEP_DIFF") == 0) {
          pp_place_keep_diff_ = std::max(0.0, atof(tokens_[2]));
          read_place_keep_diff_ = true;
        } else if (strcmp(tokens_[1], "ENFORCE_MIN_COOLOFF") == 0) {
          enforce_min_cooloff_ = true;
          read_enforce_min_cooloff_ = true;
        } else if (strcmp(tokens_[1], "PCLOSE_FACTOR") == 0) {
          pclose_factor_ = atof(tokens_[2]);
          read_pclose_factor_ = true;
        } else if (strcmp(tokens_[1], "MODERATE_TIME_LIMIT") == 0) {
          moderate_time_limit_ = atoi(tokens_[2]);
          read_moderate_time_limit_ = true;
        } else if (strcmp(tokens_[1], "HIGH_TIME_LIMIT") == 0) {
          high_time_limit_ = atoi(tokens_[2]);
          read_high_time_limit_ = true;
        } else if (strcmp(tokens_[1], "SAFE_CANCEL_SIZE") == 0) {
          safe_cancel_size_ = atoi(tokens_[2]);
          read_safe_cancel_size_ = true;
        } else if (strcmp(tokens_[1], "PLACE_ON_TRADE_UPDATE_IMPLIED_QUOTE") == 0) {
          place_on_trade_update_implied_quote_ = bool(atoi(tokens_[2]));
          read_place_on_trade_update_implied_quote_ = true;
        } else if (strcmp(tokens_[1], "NUM_INCREASE_TICKS_TO_KEEP") == 0) {
          num_increase_ticks_to_keep_ = atoi(tokens_[2]);
          read_num_increase_ticks_to_keep_ = true;
        } else if (strcmp(tokens_[1], "NUM_DECREASE_TICKS_TO_KEEP") == 0) {
          num_decrease_ticks_to_keep_ = atoi(tokens_[2]);
          read_num_decrease_ticks_to_keep_ = true;
        } else if (strcmp(tokens_[1], "RETAIL_SIZE_FACTOR_TO_OFFER") == 0) {
          retail_size_factor_to_offer_ = std::max(0.0, std::min(5.0, atof(tokens_[2])));
        } else if (strcmp(tokens_[1], "RETAIL_SIZE_FACTOR_TO_PLACE") == 0) {
          retail_size_factor_to_place_ = std::max(0.0, std::min(1.0, atof(tokens_[2])));
        } else if (strcmp(tokens_[1], "USE_RETAIL_TRADING_FLATLOGIC") == 0) {
          use_retail_trading_flatlogic_ = atoi(tokens_[2]) > 0;
        } else if (strcmp(tokens_[1], "RETAIL_MAX_LEVEL_TO_OFFER") == 0) {
          retail_max_level_to_offer_ = std::max(0, atoi(tokens_[2]));
        } else if (strcmp(tokens_[1], "RETAIL_SIZE_TO_OFFER_VEC") == 0) {
          for (unsigned int i = 2; i < tokens_.size(); i++) {
            VectorUtils::UniqueVectorAdd(retail_size_to_offer_vec_, std::max(0, atoi(tokens_[i])));
          }
          std::sort(retail_size_to_offer_vec_.begin(), retail_size_to_offer_vec_.end());
        } else if (strcmp(tokens_[1], "RETAIL_POSITION_DISCOUNT_FACTOR") == 0) {
          retail_position_discount_factor_ = std::max(1.0, atof(tokens_[2]));
        } else if (strcmp(tokens_[1], "RETAIL_SIZE_TOLERANCE") == 0) {
          retail_size_tolerance_ = std::max(0.0, std::min(0.4, atof(tokens_[2])));
        } else if (strcmp(tokens_[1], "RETAIL_PRICE_THRESHOLD_TOLERANCE") == 0) {
          retail_price_threshold_tolerance_ = std::max(0.0, std::min(0.4, atof(tokens_[2])));
        } else if (strcmp(tokens_[1], "RETAIL_MAX_POSITION_STEP_SIZE") == 0) {
          retail_max_position_step_size_ = std::max(1, atoi(tokens_[2]));
        } else if (strcmp(tokens_[1], "RETAIL_MAX_GLOBAL_POSITION_STEP_SIZE") == 0) {
          retail_max_global_position_step_size_ = std::max(1, atoi(tokens_[2]));
        } else if (strcmp(tokens_[1], "RETAIL_PLACE_FOK") == 0) {
          retail_place_fok_ = atoi(tokens_[2]) > 0;
        } else if (strcmp(tokens_[2], "OWP_RETAIL_OFFER_THRESH") == 0) {
          owp_retail_offer_thresh_ = atof(tokens_[2]);
          read_owp_retail_offer_thresh_ = true;
        } else if (strcmp(tokens_[1], "RETAIL_OFFER_FRA") == 0) {
          retail_offer_fra_ = atoi(tokens_[2]) > 0;
        } else if (strcmp(tokens_[1], "AGGRESS_ABOVE_MAX_POS") == 0) {
          aggress_above_max_pos_ = atoi(tokens_[2]) > 0;
        } else if (strcmp(tokens_[1], "RETAIL_MAX_PC1_RISK") == 0) {
          retail_max_pc1_risk_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "RETAIL_MAX_ORDER_SIZE") == 0) {
          retail_max_ord_size_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "USE_STABLE_BIDASK_LEVELS") == 0) {
          use_stable_bidask_levels_ = (atoi(tokens_[2]) != 0);
          read_use_stable_bidask_levels_ = true;
        } else if (strcmp(tokens_[1], "MAX_GLOBAL_RISK") == 0) {
          max_global_risk_ = atoi(tokens_[2]);
          read_max_global_risk_ = true;
        } else if (strcmp(tokens_[1], "MAX_GLOBAL_RISK_RATIO") == 0) {
          max_global_risk_ratio_ = atof(tokens_[2]);
          read_max_global_risk_ratio_ = true;
        } else if (strcmp(tokens_[1], "DPT_RANGE") == 0) {
          dpt_range_ = (atof(tokens_[2]));
          read_dpt_range_ = true;
        } else if (strcmp(tokens_[1], "DESIRED_POSITION_LEEWAY") == 0) {
          desired_position_leeway_ = (atoi(tokens_[2]));
          read_desired_position_leeway_ = true;
        } else if (strcmp(tokens_[1], "DESIRED_POSITION_LARGE_DIFFERENCE") == 0) {
          desired_position_large_difference_ = (atoi(tokens_[2]));
          read_desired_position_difference_ = true;
        } else if (strcmp(tokens_[1], "PLACE_OR_CANCEL") == 0) {
          place_or_cancel_ = (atoi(tokens_[2]) != 0);
          read_place_or_cancel_ = true;
        } else if (strcmp(tokens_[1], "POSITION_CHANGE_COMPENSATION") == 0) {
          position_change_compensation_ = atof(tokens_[2]);
          read_position_change_compensation_ = true;
        } else if (strcmp(tokens_[1], "LIQUIDITY_FACTOR") == 0) {
          liquidity_factor_ = (atof(tokens_[2]));
          read_liquidity_factor_ = true;
        } else if (strcmp(tokens_[1], "ONLINE_STATS_HISTORY_SECS") == 0) {
          online_stats_history_secs_ = atoi(tokens_[2]);
          read_online_stats_history_secs_ = true;
        } else if (strcmp(tokens_[1], "ONLINE_STATS_AVG_DEP_BIDASK_SPREAD") == 0) {
          online_stats_avg_dep_bidask_spread_ = (atoi(tokens_[2]) != 0);
          read_online_stats_avg_dep_bidask_spread_ = true;
        } else if (strcmp(tokens_[1], "ONLINE_MODEL_STDEV") == 0) {
          online_model_stdev_ = (atoi(tokens_[2]) != 0);
        } else if (strcmp(tokens_[1], "MIN_MODEL_SCALE_FACT") == 0) {
          min_model_scale_fact_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "MAX_MODEL_SCALE_FACT") == 0) {
          max_model_scale_fact_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "OFFLINE_MODEL_STDEV") == 0) {
          offline_model_stdev_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "MODEL_STDEV_DURATION") == 0) {
          model_stdev_duration_ = std::max(1, atoi(tokens_[2]));
        } else if (strcmp(tokens_[1], "PLACE_MULTIPLE_ORDERS") == 0) {
          place_multiple_orders_ = (atoi(tokens_[2]) != 0);
          read_place_multiple_orders_ = true;
        } else if (strcmp(tokens_[1], "MAX_UNIT_SIZE_AT_LEVEL") == 0) {
          max_unit_size_at_level_ = atoi(tokens_[2]);
          read_max_unit_size_at_level_ = true;
        } else if (strcmp(tokens_[1], "SIZE_DIFF_BETWEEN_ORDERS") == 0) {
          size_diff_between_orders_ = atoi(tokens_[2]);
          read_size_diff_between_orders_ = true;
        } else if (strcmp(tokens_[1], "PAIR_UNIT_TRADE_SIZE_FACTOR") == 0) {
          pair_unit_trade_size_factor_ = atoi(tokens_[2]);
          read_pair_unit_trade_size_factor_ = true;
        } else if (strcmp(tokens_[1], "PAIR_PLACE_DIFF") == 0) {
          pair_place_diff_ = atof(tokens_[2]);
          read_pair_place_diff_ = true;
        } else if (strcmp(tokens_[1], "PAIR_KEEP_DIFF") == 0) {
          pair_keep_diff_ = atof(tokens_[2]);
          read_pair_keep_diff_ = true;
        } else if (strcmp(tokens_[1], "PAIR_AGGRESSIVE") == 0) {
          pair_aggressive_ = atof(tokens_[2]);
          read_pair_aggressive_ = true;
        } else if (strcmp(tokens_[1], "PAIR_IMPROVE") == 0) {
          pair_improve_ = atof(tokens_[2]);
          read_pair_improve_ = true;
        } else if (strcmp(tokens_[1], "PAIR_ZEROPOS_KEEP") == 0) {
          pair_zeropos_keep_ = atof(tokens_[2]);
          read_pair_zeropos_keep_ = true;
        } else if (strcmp(tokens_[1], "PAIR_ZEROPOS_DECREASE_DIFF") == 0) {
          pair_zeropos_decrease_diff_ = atof(tokens_[2]);
          read_pair_zeropos_decrease_diff_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "PAIR_INCREASE_ZEROPOS_DIFF") == 0) {
          pair_increase_zeropos_diff_ = atof(tokens_[2]);
          read_pair_increase_zeropos_diff_ = true;
        } else if (strcmp(tokens_[1], "PAIR_PLACE_KEEP_DIFF") == 0) {
          pair_place_keep_diff_ = atof(tokens_[2]);
          read_pair_place_keep_diff_ = true;
        } else if (strcmp(tokens_[1], "ABS_MAX_POS_FACTOR") == 0) {
          abs_max_pos_factor_ = atof(tokens_[2]);
          read_abs_max_pos_factor_ = true;
        } else if (strcmp(tokens_[1], "OWN_BASE_PX") == 0) {
          own_base_px_ = std::string(tokens_[2]);
          read_own_base_px_ = true;
        } else if (strcmp(tokens_[1], "GET_FLAT_BY_FOK_MODE") == 0) {
          get_flat_by_fok_mode_ = bool(atoi(tokens_[2]));
          read_get_flat_by_fok_mode_ = true;
        } else if (strcmp(tokens_[1], "FLATFOK_BOOK_DEPTH") == 0) {
          flatfok_book_depth_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "USE_ONLINE_UTS") == 0) {
          use_online_uts_ = bool(atoi(tokens_[2]));
        } else if (strcmp(tokens_[1], "LOOKBACK_DYNAMIC_UTS") == 0) {
          lookback_dynamic_uts = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "UTS_PER_L1SIZE") == 0) {
          uts_per_L1Size = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "COMBINED_GET_FLAT_MODEL") == 0) {
          combined_get_flat_model = tokens_[2];
        } else if (strcmp(tokens_[1], "SHOUD_GET_COMBINE_GET_FLAT") == 0) {
          should_get_combined_flat = bool(atoi(tokens_[2]));
        } else if (strcmp(tokens_[1], "NUMBER_OF_STRATS") == 0) {
          number_strats = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "MAX_UNIT_RATIO_STRUCTURED") == 0) {
          max_unit_ratio_structured = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "BASE_SHORTCODE_STRUCTURED") == 0) {
          base_shortcode_structured = tokens_[2];
        } else if (strcmp(tokens_[1], "MINI_TOP_KEEP_LOGIC") == 0) {
          mini_top_keep_logic_ = (atoi(tokens_[2]) != 0);
        } else if (strcmp(tokens_[1], "MINI_AGG_LOGIC") == 0) {
          mini_agg_logic_ = (atoi(tokens_[2]) != 0);
        } else if (strcmp(tokens_[1], "MINI_IMPROVE_LOGIC") == 0) {
          mini_improve_logic_ = (atoi(tokens_[2]) != 0);
        } else if (strcmp(tokens_[1], "INDEP_SAFE_IMPROVE_TICKS") == 0) {
          indep_safe_improve_ticks_ = std::max(0, atoi(tokens_[2]));
        } else if (strcmp(tokens_[1], "INDEP_SAFE_IMPROVE_SIZE") == 0) {
          indep_safe_improve_size_ = std::max(0, atoi(tokens_[2]));
        } else if (strcmp(tokens_[1], "MKT_PRICE_IMPROVE_BIAS") == 0) {
          mkt_price_improve_bias_ = std::max(0.25, atof(tokens_[2]));
        } else if (strcmp(tokens_[1], "MINI_AGG_SWEEP_MARGIN") == 0) {
          mini_agg_sweep_margin_ = std::max(0, atoi(tokens_[2]));
        } else if (strcmp(tokens_[1], "MINI_SWEEP_CANCEL_COOLOFF") == 0) {
          mini_sweep_cancel_cooloff_ = std::max(0, atoi(tokens_[2]));
        } else if (strcmp(tokens_[1], "MINI_AGG_TICKS") == 0) {
          mini_agg_ticks_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "MINI_SAFE_INDEP_AGG_TICKS") == 0) {
          mini_safe_indep_agg_ticks_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "PRICE_FOR_MAX_UNIT_RATIO") == 0) {
          price_for_max_unit_ratio_ = atoi(tokens_[2]);
          read_price_for_max_unit_ratio_ = true;
        } else if (strcmp(tokens_[1], "USE_STABLE_BOOK") == 0) {
          use_stable_book_ = (atoi(tokens_[2]) != 0);
          read_use_stable_book_ = true;
        } else if (strcmp(tokens_[1], "SPREAD_FACTOR") == 0) {
          spread_factor_ = atof(tokens_[2]);
          read_spread_factor_ = true;
        } else if (strcmp(tokens_[1], "SPREAD_QUOTE_FACTOR") == 0) {
          spread_quote_factor_ = atof(tokens_[2]);
          read_spread_quote_factor_ = true;
        } else if (strcmp(tokens_[1], "COMMON_PARAM") == 0) {
          is_common_param_ = (atoi(tokens_[2]) != 0);
        } else if (strcmp(tokens_[1], "MAX_GLOBAL_BETA_ADJUSTED_NOTIONAL_RISK") == 0) {
          max_global_beta_adjusted_notional_risk_ = atof(tokens_[2]);
          read_max_global_beta_adjusted_notional_risk_ = true;
        } else if (strcmp(tokens_[1], "SELF_POS_PROJECTION_FACTOR") == 0) {
          self_pos_projection_factor_ = atof(tokens_[2]);
          read_self_pos_projection_factor_ = true;
        } else if (strcmp(tokens_[1], "COMPUTE_NOTIONAL_RISK") == 0) {
          compute_notional_risk_ = atoi(tokens_[1]) != 0;
        } else if (strcmp(tokens_[1], "VOLUME_RATIO_STOP_TRADING_LOWER_THREHOLD") == 0) {
          volume_ratio_stop_trading_lower_threshold_ = atof(tokens_[2]);
          read_volume_ratio_stop_trading_lower_threshold_ = true;
        } else if (strcmp(tokens_[1], "VOLUME_RATIO_STOP_TRADING_UPPER_THREHOLD") == 0) {
          volume_ratio_stop_trading_upper_threshold_ = atof(tokens_[2]);
          read_volume_ratio_stop_trading_upper_threshold_ = true;
        } else if (strcmp(tokens_[1], "MOVING_BIDASK_SPREAD_DURATION") == 0) {
          moving_bidask_spread_duration_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "MINI_SAFE_INDEP_AGG_TICKS") == 0) {
          mini_safe_indep_agg_spread_scale_factor = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "PRICE_TYPE") == 0) {
          price_type_ = tokens_[2];
        } else if (strcmp(tokens_[1], "SOURCE_STDEV_RISK_SCALING") == 0) {
          source_stdev_risk_scaling_ = tokens_[2];
        } else if (strcmp(tokens_[1], "MIN_RISK") == 0) {
          use_min_risk_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "AF_EVENT_ID") == 0) {
          af_event_id_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "AF_EVENT_HALFLIFE_SECS") == 0) {
          af_event_halflife_secs_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "AF_EVENT_DRAWDOWN_SECS") == 0) {
          af_event_drawdown_secs_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "AF_EVENT_ACTIVE_PXCH_MARGIN") == 0) {
          af_event_pxch_act_ = atof(tokens_[2]);
          read_af_event_pxch_act_ = true;
        } else if (strcmp(tokens_[1], "AF_EVENT_GETFLAT_PXCH_MARGIN") == 0) {
          af_event_pxch_getflat_ = atof(tokens_[2]);
          read_af_event_pxch_getflat_ = true;
        } else if (strcmp(tokens_[1], "AF_EVENT_MAX_UTS_PXCH") == 0) {
          af_event_max_uts_pxch_ = atof(tokens_[2]);
          read_af_event_max_uts_pxch_ = true;
        } else if (strcmp(tokens_[1], "AF_SOURCE_SHC") == 0) {
          af_source_shc_ = tokens_[2];
        } else if (strcmp(tokens_[1], "AF_SOURCE_TRADE") == 0) {
          if (atoi(tokens_[2]) == 1) {
            af_use_source_trade_ = true;
          }
        } else if (strcmp(tokens_[1], "AF_DD_MAXPXCH_FACTOR") == 0) {
          af_dd_maxpxch_factor_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "AF_DD_PREDPXCH_FACTOR") == 0) {
          af_dd_predpxch_factor_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "AF_MULT_LEVELS") == 0) {
          af_use_mult_levels_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "AF_SCALE_BETA") == 0) {
          af_scale_beta_ = std::string(tokens_[2]);
          if (!af_scale_beta_.empty()) {
            read_af_scale_beta_ = true;
          }
        } else if (strcmp(tokens_[1], "AF_RISK_SCALING") == 0) {
          af_risk_scaling_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "AF_STAGGERED_GETFLAT_MINS") == 0) {
          for (unsigned i = 2; i < tokens_.size(); i++) {
            af_staggered_getflat_mins_.push_back(atoi(tokens_[i]));
          }
        } else if (strcmp(tokens_[1], "USE_PRE_GETFLAT") == 0) {
          use_pre_getflat_ = (atoi(tokens_[2]) != 0);
        } else if (strcmp(tokens_[1], "PRE_GETFLAT_SECS") == 0) {
          pre_getflat_msecs_ = atoi(tokens_[2]) * 1000;
        } else if (strcmp(tokens_[1], "PRE_GETFLAT_MULTIPLIER") == 0) {
          pre_getflat_multiplier_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "GETFLAT_AGGRESS") == 0) {
          allow_to_aggress_on_getflat_ = true;
          getflat_aggress_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "MAX_SPREAD_GETFLAT_AGGRESS") == 0) {
          max_spread_getflat_aggress_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "USE_MIN_PORTFOLIO_RISK") == 0) {
          if (atoi(tokens_[2]) == 1) {
            use_min_portfolio_risk_ = true;
          }
        } else if (strcmp(tokens_[1], "SIZE_DISCLOSED_FACTOR") == 0) {
          size_disclosed_factor_ = atof(tokens_[2]);
          read_size_disclosed_factor_ = true;
        } else if (strcmp(tokens_[1], "ARB_PLACE_THRESH") == 0) {
          arb_place_thresh_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "NUM_LVL_ORDER_KEEP") == 0) {
          num_lvl_order_keep_ = std::max(0, atoi(tokens_[2]));
        } else if (strcmp(tokens_[1], "ARB_BEST_SHC") == 0) {
          arb_best_shc_ = tokens_[2];
        } else if (strcmp(tokens_[1], "PLACE_ONLY_AFTER_CANCEL") == 0) {
          place_only_after_cancel_ = (atoi(tokens_[2]) != 0);
        } else if (strcmp(tokens_[1], "ALLOW_MODIFY_ORDERS") == 0) {
          allow_modify_orders_ = (atoi(tokens_[2]) != 0);
        } else if (strcmp(tokens_[1], "MAX_FRACTION_UTS_SIZE_TO_MODIFY") == 0) {
          max_fraction_uts_size_to_modify_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "BIGTRADES_CANCEL_THRESHOLD") == 0) {
          read_bigtrades_cancel_ = true;
          bigtrades_cancel_threshold_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "BIGTRADES_WINDOW_MSECS") == 0) {
          bigtrades_window_msecs_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "BIGTRADES_COOLOFF_INTERVAL") == 0) {
          bigtrades_cooloff_interval_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "BIGTRADES_PRICE_DECAY_FACTOR") == 0) {
          bigtrades_decay_factor_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "USE_BIGTRADES_SOURCE") == 0) {
          bigtrades_source_id_ = tokens_[2];
        } else if (strcmp(tokens_[1], "BIGTRADES_PLACE_THRESHOLD") == 0) {
          read_bigtrades_place_ = true;
          bigtrades_place_threshold_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "BIGTRADES_PLACE_WINDOW_MSECS") == 0) {
          bigtrades_place_window_msecs_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "BIGTRADES_PLACE_COOLOFF_INTERVAL") == 0) {
          bigtrades_place_cooloff_interval_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "BIGTRADES_PLACE_PRICE_DECAY_FACTOR") == 0) {
          bigtrades_place_decay_factor_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "USE_BIGTRADES_PLACE_SOURCE") == 0) {
          bigtrades_place_source_id_ = tokens_[2];
        } else if (strcmp(tokens_[1], "BIGTRADES_AGGRESS_THRESHOLD") == 0) {
          read_bigtrades_aggress_ = true;
          bigtrades_aggress_threshold_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "BIGTRADES_AGGRESS_WINDOW_MSECS") == 0) {
          bigtrades_aggress_window_msecs_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "BIGTRADES_AGGRESS_COOLOFF_INTERVAL") == 0) {
          bigtrades_aggress_cooloff_interval_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "BIGTRADES_AGGRESS_PRICE_DECAY_FACTOR") == 0) {
          bigtrades_aggress_decay_factor_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "USE_BIGTRADES_AGGRESS_SOURCE") == 0) {
          bigtrades_aggress_source_id_ = tokens_[2];
        } else if (strcmp(tokens_[1], "L1_TRADE_VOLUME_BIAS") == 0) {
          read_l1_trade_volume_bias_ = true;
          l1_trade_volume_bias_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "L1_BID_ASK_FLOW_NUM_EVENTS") == 0) {
          l1_bid_ask_flow_num_events_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "L1_BID_ASK_FLOW_CANCEL_THRESH") == 0) {
          l1_bid_ask_flow_cancel_thresh_ = atof(tokens_[2]);
          read_l1_bid_ask_flow_cancel_thresh_ = true;
        } else if (strcmp(tokens_[1], "L1_BID_ASK_FLOW_CANCEL_KEEP_THRESH_DIFF") == 0) {
          l1_bid_ask_flow_cancel_keep_thresh_diff_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "MIN_SIZE_TO_FLOW_BASED_CANCEL") == 0) {
          min_size_to_flow_based_cancel_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "L1_BIAS_CANCEL_THRESH_") == 0) {
          l1_bias_cancel_thresh_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "CANCEL_ON_MARKET_TILT_SOURCE") == 0) {
          read_cancel_on_market_tilt_source_ = true;
          cancel_on_market_tilt_source_ = tokens_[2];
        } else if (strcmp(tokens_[1], "CANCEL_ON_MARKET_TILT_THRESH") == 0) {
          cancel_on_market_tilt_thresh_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "CANCEL_ON_MARKET_TILT_MSECS") == 0) {
          cancel_on_market_tilt_msecs_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "USE_NOTIONAL_UTS") == 0) {
          use_notional_uts_ = (atoi(tokens_[2]) != 0);
          read_use_notional_uts_ = true;
        } else if (strcmp(tokens_[1], "NOTIONAL_UTS") == 0) {
          notional_uts_ = atof(tokens_[2]);
          read_notional_uts_ = true;
        } else if (strcmp(tokens_[1], "DELTA_HEDGE_LOWER_THRESHOLD") == 0) {
          delta_hedge_lower_threshold_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "DELTA_HEDGE_UPPER_THRESHOLD") == 0) {
          delta_hedge_upper_threshold_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "FRACTIONAL_SECOND_IMPLIED_VOL") == 0) {
          fractional_second_implied_vol_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "SPREAD_TO_TARGET") == 0) {
          spread_to_target_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "REGIMEINDICATOR") == 0) {
          std::string ind_str = "";
          for (unsigned i = 2; i < tokens_.size(); i++) {
            if (i > 2) {
              ind_str += " ";
            }
            ind_str += tokens_[i];
          }
          regime_indicator_string_vec_.push_back(ind_str);
          read_regime_indicator_string_ = true;
        } else if (strcmp(tokens_[1], "REGIMES_TO_TRADE") == 0) {
          std::vector<int> vec;
          regimes_to_trade_.push_back(vec);
          for (unsigned i = 2; i < tokens_.size(); i++) {
            regimes_to_trade_[regimes_to_trade_.size() - 1].push_back(atoi(tokens_[i]));
          }
          read_regimes_to_trade_ = true;
        } else if (strcmp(tokens_[1], "TRADEINDICATOR") == 0) {
          std::string ind_str = "";
          for (unsigned i = 2; i < tokens_.size(); i++) {
            if (i > 2) {
              ind_str += " ";
            }
            ind_str += tokens_[i];
          }
          trade_indicator_string_ = ind_str;
          read_trade_indicator_string_ = true;
        } else if (strcmp(tokens_[1], "FACTOR_TRADE_BIAS") == 0) {
          factor_trade_bias_ = atof(tokens_[2]);
          read_trade_bias_ = true;
        } else if (strcmp(tokens_[1], "CANCELINDICATOR") == 0) {
          std::string ind_str = "";
          for (unsigned i = 2; i < tokens_.size(); i++) {
            if (i > 2) {
              ind_str += " ";
            }
            ind_str += tokens_[i];
          }
          cancel_indicator_string_ = ind_str;
          read_cancel_indicator_string_ = true;
        } else if (strcmp(tokens_[1], "FACTOR_CANCEL_BIAS") == 0) {
          factor_cancel_bias_ = atof(tokens_[2]);
          read_cancel_bias_ = true;
        } else if (strcmp(tokens_[1], "IMPLIEDMKTINDICATOR") == 0) {
          std::string ind_str = "";
          for (unsigned i = 2; i < tokens_.size(); i++) {
            if (i > 2) {
              ind_str += " ";
            }
            ind_str += tokens_[i];
          }
          implied_mkt_indicator_string_ = ind_str;
          read_implied_mkt_indicator_string_ = true;
        } else if (strcmp(tokens_[1], "IMPLIED_MKT_THRES_FACTOR") == 0) {
          implied_mkt_thres_factor_ = atof(tokens_[2]);
          read_implied_mkt_ = true;
        } else if (strcmp(tokens_[1], "MAX_SIZE_TO_CANCEL") == 0) {
          max_size_to_cancel_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "DYNAMIC_ZEROPOS_KEEP") == 0) {
          read_dynamic_zeropos_keep_ = true;
          // change_params(param_line);
        } else if (strcmp(tokens_[1], "DYNAMIC_UTS_VOLUME_AR_5") == 0) {
          read_dynamic_zeropos_keep_ = true;
          // change_params(param_line);
        } else if (strcmp(tokens_[1], "DYNAMIC_UTS_L1_SIZE") == 0) {
          // change_params(param_line);
        } else if (strcmp(tokens_[1], "SYNTH_AGG_EXEC") == 0) {
          synth_agg_exec_ = true;
        } else if (strcmp(tokens_[1], "DYNAMIC_OTL_STDEV") == 0) {
          // change_params(param_line);
        } else if (strcmp(tokens_[1], "MAX_MIN_DIFF_RATIO") == 0) {
          max_min_diff_ratio_ = atof(tokens_[2]);
          read_max_min_diff_ratio_ = true;
        } else if (strcmp(tokens_[1], "SOURCE_FOR_MKT_CONDITION") == 0) {
          read_src_based_exec_changes_ = true;
          source_for_mkt_condition_ = tokens_[2];
        } else if (strcmp(tokens_[1], "SOURCE_TREND_PERCENTILE") == 0) {
          src_trend_percentile_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "SOURCE_TREND_FACTOR") == 0) {
          src_trend_factor_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "SOURCE_TREND_KEEP_FACTOR") == 0) {
          src_trend_keep_factor_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "LAST_DAY_VOL_FACTOR") == 0) {
          last_day_vol_factor_ = std::max(1.0, atof(tokens_[2]));
          read_last_day_vol_ = true;
        } else if (strcmp(tokens_[1], "STDEV_OVERALL_CAP") == 0) {
          stdev_overall_cap_ = std::max(1.0, std::min(3.0, atof(tokens_[2])));
          read_stdev_overall_cap_ = true;
        } else if (strcmp(tokens_[1], "MAX_TRADE_SIZE") == 0) {
          max_trade_size_ = std::max(1, atoi(tokens_[2]));
          read_max_trade_size_ = true;
        } else if (strcmp(tokens_[1], "MAX_DISTANCE_TO_KEEP_FROM_BAND_PRICE") == 0) {
          max_distance_to_keep_from_band_price_ = std::max(4, atoi(tokens_[2]));
        } else if (strcmp(tokens_[1], "SGX_MARKET_MAKING") == 0) {
          sgx_market_making_ = true;
        } else if (strcmp(tokens_[1], "USE_CANCELLATION_MODEL") == 0) {
          read_use_cancellation_model_ = atoi(tokens_[2]) == 1 ? true : false;
        } else if (strcmp(tokens_[1], "CANC_PROB") == 0) {
          read_canc_prob_ = true;
          canc_prob_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "CANC_PROB_THRESH") == 0) {
          cancel_prob_thresh_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "IMPLIED_CANCEL_THRESHOLD") == 0) {
          implied_price_based_cancellation_ = true;
          implied_cancel_threshold_ = std::max(0.0, atof(tokens_[2]));
        } else if (strcmp(tokens_[1], "IMPLIED_PLACE_THRESHOLD") == 0) {
          implied_price_based_place_ = true;
          implied_place_threshold_ = std::max(0.0, atof(tokens_[2]));
        } else if (strcmp(tokens_[1], "IMPLIED_AGG_THRESHOLD") == 0) {
          implied_price_based_agg_ = true;
          implied_agg_threshold_ = std::max(0.0, atof(tokens_[2]));
        } else if (strcmp(tokens_[1], "IMPLIED_IMPROVE_THRESHOLD") == 0) {
          implied_price_based_improve_ = true;
          implied_improve_threshold_ = std::max(0.0, atof(tokens_[2]));
        } else if (strcmp(tokens_[1], "IMPLIED_AGG_THRESHOLD") == 0) {
          implied_price_based_agg_ = true;
          implied_agg_threshold_ = std::max(0.0, atof(tokens_[2]));
        } else if (strcmp(tokens_[1], "IMPLIED_MKT_CANCEL_THRESHOLD") == 0) {
          implied_mkt_price_based_cancellation_ = true;
          implied_mkt_cancel_threshold_ = std::max(0.0, atof(tokens_[2]));
        } else if (strcmp(tokens_[1], "IMPLIED_MKT_PLACE_THRESHOLD") == 0) {
          implied_mkt_price_based_place_ = true;
          implied_mkt_place_threshold_ = std::max(0.0, atof(tokens_[2]));
        } else if (strcmp(tokens_[1], "IMPLIED_PRICE_CALCULATION") == 0) {
          implied_price_calculation_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "PORTFOLIO_MEAN_REVERSION") == 0) {
          use_mean_reversion_ = atoi(tokens_[2]) != 0;
        } else if (strcmp(tokens_[1], "PORTFOLIO_CONDITIONAL_MEAN_REVERSION") == 0) {
          skip_mean_reversion_on_indep_ = (atoi(tokens_[2]) != 0);
        } else if (strcmp(tokens_[1], "PORTFOLIO_CONDITIONAL_MEAN_REVERSION_FACTOR") == 0) {
          mean_reversion_skip_factor_ = atof(tokens_[2]);
          use_mean_reversion_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "IMPROVE_CANCEL") == 0) {
          improve_cancel_ = (atoi(tokens_[2]) != 0);
        } else if (strcmp(tokens_[1], "INTERDAY_SCALING") == 0) {
          interday_scaling_factor_ = atof(tokens_[2]);
          read_interday_scaling_factor_ = true;
        } else if (strcmp(tokens_[1], "RESET_MUR_TIME") == 0) {
          check_mur_reset_time_ = true;
          if ((strncmp(tokens_[2], "EST_", 4) == 0) || (strncmp(tokens_[2], "CST_", 4) == 0) ||
              (strncmp(tokens_[2], "CET_", 4) == 0) || (strncmp(tokens_[2], "BRT_", 4) == 0) ||
              (strncmp(tokens_[2], "UTC_", 4) == 0) || (strncmp(tokens_[2], "KST_", 4) == 0) ||
              (strncmp(tokens_[2], "HKT_", 4) == 0) || (strncmp(tokens_[2], "MSK_", 4) == 0) ||
              (strncmp(tokens_[2], "IST_", 4) == 0) || (strncmp(tokens_[2], "JST_", 4) == 0) ||
              (strncmp(tokens_[2], "BST_", 4) == 0)) {
            mur_reset_time_ =
                HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(tradingdate_, atoi(tokens_[2] + 4), tokens_[2]);
          } else {
            mur_reset_time_ =
                HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(tradingdate_, atoi(tokens_[2]), "UTC_");
          }
        } else if (strcmp(tokens_[1], "RESET_MUR_VALUE") == 0) {
          mur_reset_value_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "MIN_ORDR_SIZE") == 0) {
          min_order_size_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "RELEASE_CORE_PREMATURE") == 0) {
          release_core_premature_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "IS_MUR_LOW") == 0) {
          is_mur_low_ = atoi(tokens_[2]) != 0;
        } else if (strcmp(tokens_[1], "RECOMPUTE_SIGNAL") == 0) {
          recompute_signal_ = atoi(tokens_[2]) != 0;
        } else if (strcmp(tokens_[1], "FEATURE_MODELFILE") == 0) {
          feature_modelfile_ = string(tokens_[2]);
          read_feature_modelfile_ = true;
        } else if (strcmp(tokens_[1], "FEATURE_THRESHOLD") == 0) {
          feature_threshold_ = atof(tokens_[2]);
          if (tokens_.size() > 3) feature_more_ = atof(tokens_[3]);
          read_feature_threshold_ = true;
        } else if (strcmp(tokens_[1], "NOT_TO_TRADE") == 0) {
          not_to_trade_ = (atoi(tokens_[2]) != 0);
        }
        /* else if (strcmp(tokens_[1], "SIM_EXEC_THRESHOLD") == 0) {
          sim_exec_threshold_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "SIM_EXEC_COOLOFF") == 0) {
          sim_exec_cooloff_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "SHCS_TO_REACT") == 0) {
          for (unsigned i = 2; i < tokens_.size(); i++) {
            shcs_to_react_on_exec_.push_back((tokens_[i]));
          }
        } else if (strcmp(tokens_[1], "OUTRIGHT_FRAC_CANCEL") == 0) {
          outright_frac_cancel_ = atof(tokens_[2]);
        }*/
      }
    }
    paramfile_.close();
  }

  // there are two ways of specifying params ... building the other one
  ReconcileParams(base_shortcode_structured);
}

void ParamSet::WriteSendStruct(ParamSetSendStruct &retval) const {
  retval.worst_case_position_ = worst_case_position_;
  retval.max_position_ = max_position_;
  retval.unit_trade_size_ = unit_trade_size_;
  retval.max_global_position_ = max_global_position_;
  retval.max_security_position_ = max_security_position_;

  retval.highpos_limits_ = highpos_limits_;
  retval.highpos_thresh_factor_ = highpos_thresh_factor_;
  retval.highpos_thresh_decrease_ = highpos_thresh_decrease_;
  retval.highpos_size_factor_ = highpos_size_factor_;
  retval.increase_place_ = increase_place_;
  retval.increase_keep_ = increase_keep_;
  retval.zeropos_limits_ = zeropos_limits_;
  retval.zeropos_place_ = zeropos_place_;
  retval.zeropos_keep_ = zeropos_keep_;
  retval.decrease_place_ = decrease_place_;
  retval.decrease_keep_ = decrease_keep_;

  retval.place_keep_diff_ = place_keep_diff_;
  retval.increase_zeropos_diff_ = increase_zeropos_diff_;
  retval.zeropos_decrease_diff_ = zeropos_decrease_diff_;

  retval.safe_distance_ = safe_distance_;

  retval.allowed_to_improve_ = allowed_to_improve_;
  retval.allowed_to_aggress_ = allowed_to_aggress_;
  retval.improve_ = improve_;
  retval.aggressive_ = aggressive_;

  retval.max_self_ratio_at_level_ = max_self_ratio_at_level_;
  retval.longevity_support_ = longevity_support_;

  retval.max_position_to_lift_ = max_position_to_lift_;
  retval.max_position_to_bidimprove_ = max_position_to_bidimprove_;
  retval.max_position_to_cancel_on_lift_ = max_position_to_cancel_on_lift_;
  retval.max_size_to_aggress_ = max_size_to_aggress_;
  retval.min_position_to_hit_ = min_position_to_hit_;
  retval.min_position_to_askimprove_ = min_position_to_askimprove_;
  retval.min_position_to_cancel_on_hit_ = min_position_to_cancel_on_hit_;

  retval.max_int_spread_to_place_ = max_int_spread_to_place_;
  retval.max_int_level_diff_to_place_ = max_int_level_diff_to_place_;
  retval.max_int_spread_to_cross_ = max_int_spread_to_cross_;
  retval.min_int_spread_to_improve_ = min_int_spread_to_improve_;
  retval.num_non_best_bid_levels_monitored_ = num_non_best_bid_levels_monitored_;
  retval.num_non_best_ask_levels_monitored_ = num_non_best_ask_levels_monitored_;

  retval.max_loss_ = max_loss_;
  retval.max_pnl_ = max_pnl_;
  retval.global_max_loss_ = global_max_loss_;
  retval.max_opentrade_loss_ = max_opentrade_loss_;
  retval.max_drawdown_ = max_drawdown_;
  retval.max_short_term_loss_ = max_short_term_loss_;

  retval.cooloff_interval_ = cooloff_interval_;
  retval.agg_cooloff_interval_ = agg_cooloff_interval_;
  // retval.highpos_aversion_msecs_ = highpos_aversion_msecs_ ;

  retval.stdev_fact_ = stdev_fact_;
  retval.stdev_cap_ = stdev_cap_;
  retval.px_band_ = px_band_;
  retval.low_stdev_lvl_ = low_stdev_lvl_;
  retval.min_size_to_join_ = min_size_to_join_;
  retval.spread_add_ = spread_add_;

  retval.use_throttle_manager_ = use_throttle_manager_;
  retval.throttle_message_limit_ = throttle_message_limit_;
}

void ParamSet::ReconcileParams(std::string dep_short_code) {
  // Return for regime params as no fields will be set.

  if (!read_max_unit_ratio_ && !read_max_position_) {
    return;
  }

  if (read_regime_indicator_string_ || read_regimes_to_trade_) {
    if (!read_regime_indicator_string_ || !read_regimes_to_trade_) {
      ExitVerbose(kExitErrorCodeGeneral,
                  "Either of Regime Indicator String or regimes_to_Trade string are not provided");
    }

    if (regime_indicator_string_vec_.size() != regimes_to_trade_.size()) {
      ExitVerbose(kExitErrorCodeGeneral,
                  "Number of regime indicators provided and number of list of regimes to trade are not same");
    }
  }

  if (use_online_uts_ && (uts_per_L1Size > 0)) {
    std::map<int, double> feature_avg_;
    int new_unit_trade_size_ =
        uts_per_L1Size *
        HFSAT::SampleDataUtil::GetAvgForPeriod(dep_short_code, tradingdate_, min(60, lookback_dynamic_uts), "L1SZ");
    if (new_unit_trade_size_ > 0) {
      max_opentrade_loss_ = max_opentrade_loss_ * new_unit_trade_size_ / unit_trade_size_;
      max_short_term_loss_ = max_short_term_loss_ * new_unit_trade_size_ / unit_trade_size_;
      max_loss_ = max_loss_ * new_unit_trade_size_ / unit_trade_size_;
      max_drawdown_ = max_drawdown_ * new_unit_trade_size_ / unit_trade_size_;
      unit_trade_size_ = new_unit_trade_size_;
    }
  }

  // for NSE & BSE make notional UTS mandatory
  if (strncmp(dep_short_code.c_str(), "NSE_", 4) == 0 || strncmp(dep_short_code.c_str(), "BSE_", 4) == 0) {
    if ((!read_use_notional_uts_ || !read_notional_uts_ || !use_notional_uts_) && !is_common_param_) {
      std::cerr << "NSE & BSE require Notional UTS .. Exiting\n";
      exit(-1);
    } else {
      int t_min_lotsize_ = HFSAT::SecurityDefinitions::GetContractMinOrderSize(dep_short_code, tradingdate_);
      if (strncmp(dep_short_code.c_str(), "NSE_", 4) == 0) {
        double t_last_close_ =
            HFSAT::NSESecurityDefinitions::GetUniqueInstance(tradingdate_).GetLastClose(dep_short_code);
        unit_trade_size_ = (int)(std::ceil(notional_uts_ / (t_last_close_ * t_min_lotsize_ *
                                                            HFSAT::NSESecurityDefinitions::GetUniqueInstance(
                                                                tradingdate_).GetContractMultiplier(dep_short_code))) *
                                 t_min_lotsize_);
      } else if (strncmp(dep_short_code.c_str(), "BSE_", 4) == 0) {
        unit_trade_size_ = 1; /*(int)(std::ceil(notional_uts_ / (t_last_close_ * t_min_lotsize_ *
                                     HFSAT::BSESecurityDefinitions::GetUniqueInstance(
                                         tradingdate_).GetContractMultiplier(dep_short_code))) *
          t_min_lotsize_);*/
      }
      // OBSERVE that there is no support for scaling explcitly provided Max pos, worst case pos, since
      // we have no sematic way of scaling those values. Hence max, worst positions should always
      // be provided as ratios for NSE & BSE products.
      read_unit_trade_size_ = true;
    }
  }

  if (retail_max_ord_size_ <= 0) {
    retail_max_ord_size_ = unit_trade_size_;
  }

  if (!allowed_to_improve_) {
    improve_ = 100;
  }

  if (read_worst_case_unit_ratio_ && (!read_worst_case_position_)) {
    worst_case_position_ = unit_trade_size_ * worst_case_unit_ratio_;
    read_worst_case_position_ = true;
  }

  if (read_max_unit_ratio_ && (!read_max_position_)) {
    max_position_ = (int)(unit_trade_size_ * max_unit_ratio_ + 0.5);
    max_position_original_ = max_position_;
    read_max_position_ = true;
  }

  if ((!read_max_total_size_to_place_) && (!read_max_total_unit_ratio_to_place_)) {
    max_total_unit_ratio_to_place_ = max_unit_ratio_;
    max_total_size_to_place_ = max_position_;
    read_max_total_unit_ratio_to_place_ = true;
    read_max_total_size_to_place_ = true;
  }

  if (read_max_total_unit_ratio_to_place_ && (!read_max_total_size_to_place_)) {
    max_total_size_to_place_ = max_total_unit_ratio_to_place_ * unit_trade_size_;
    read_max_total_size_to_place_ = true;
  }

  if (read_scale_max_pos_) {
    base_mur_ = std::min(base_mur_, (int)(max_position_ / unit_trade_size_));
  }

  if (read_explicit_worst_case_short_unit_ratio_ && (!read_explicit_worst_case_short_position_)) {
    explicit_worst_case_short_position_ = unit_trade_size_ * explicit_worst_case_short_unit_ratio_;
    read_explicit_worst_case_short_position_ = true;
  }
  if (!read_explicit_worst_case_short_position_ && !read_explicit_worst_case_short_unit_ratio_) {  // copy from normal
    explicit_worst_case_short_unit_ratio_ = worst_case_unit_ratio_;
    explicit_worst_case_short_position_ = worst_case_position_;
  }

  if (read_explicit_max_short_unit_ratio_ && (!read_explicit_max_short_position_)) {
    explicit_max_short_position_ = unit_trade_size_ * explicit_max_short_unit_ratio_;
    read_explicit_max_short_position_ = true;
  }
  if (!read_explicit_max_short_position_ && !read_explicit_max_short_unit_ratio_) {  // copy from normal
    explicit_max_short_unit_ratio_ = max_unit_ratio_;
    explicit_max_short_position_ = max_position_;
  }

  if (read_explicit_worst_case_long_unit_ratio_ && (!read_explicit_worst_case_long_position_)) {
    explicit_worst_case_long_position_ = unit_trade_size_ * explicit_worst_case_long_unit_ratio_;
    read_explicit_worst_case_long_position_ = true;
  }
  if (!read_explicit_worst_case_long_position_ && !read_explicit_worst_case_long_unit_ratio_) {  // copy from normal
    explicit_worst_case_long_unit_ratio_ = worst_case_unit_ratio_;
    explicit_worst_case_long_position_ = worst_case_position_;
  }

  if (read_explicit_max_long_unit_ratio_ && (!read_explicit_max_long_position_)) {
    explicit_max_long_position_ = unit_trade_size_ * explicit_max_long_unit_ratio_;
    read_explicit_max_long_position_ = true;
  }
  if (!read_explicit_max_long_position_ && !read_explicit_max_long_unit_ratio_) {  // copy from normal
    explicit_max_long_unit_ratio_ = max_unit_ratio_;
    explicit_max_long_position_ = max_position_;
  }

  // from A to B
  if (read_zeropos_place_ && read_zeropos_keep_ && (!read_place_keep_diff_)) {
    place_keep_diff_ = std::max(0.0, zeropos_place_ - zeropos_keep_);
    read_place_keep_diff_ = true;
  }
  if (read_zeropos_keep_ && read_increase_keep_ && (!read_increase_zeropos_diff_)) {
    increase_zeropos_diff_ = increase_keep_ - zeropos_keep_;
    read_increase_zeropos_diff_ = true;
  }
  if (read_decrease_keep_ && read_zeropos_keep_ && (!read_zeropos_decrease_diff_)) {
    zeropos_decrease_diff_ = zeropos_keep_ - decrease_keep_;
    read_zeropos_decrease_diff_ = true;
  }
  if (read_zeropos_place_ && read_place_keep_diff_ && (!read_zeropos_keep_)) {
    zeropos_keep_ = zeropos_place_ - place_keep_diff_;
    read_zeropos_keep_ = true;
  }

  // from B to A
  if (read_zeropos_keep_ && read_place_keep_diff_ && (!read_zeropos_place_)) {
    zeropos_place_ = zeropos_keep_ + place_keep_diff_;
    read_zeropos_place_ = true;
  }
  if (read_increase_zeropos_diff_ && !read_zeropos_decrease_diff_) {
    zeropos_decrease_diff_ = increase_zeropos_diff_;
    read_zeropos_decrease_diff_ = true;
  }
  if (read_zeropos_decrease_diff_ && !read_increase_zeropos_diff_) {
    increase_zeropos_diff_ = zeropos_decrease_diff_;
    read_increase_zeropos_diff_ = true;
  }
  if (read_zeropos_keep_ && read_increase_zeropos_diff_ && (!read_increase_keep_)) {
    increase_keep_ = zeropos_keep_ + increase_zeropos_diff_;
    read_increase_keep_ = true;
  }
  if (read_zeropos_place_ && read_increase_zeropos_diff_ && (!read_increase_place_)) {
    if (read_zeropos_keep_ && read_increase_zeropos_diff_ &&
        read_place_keep_diff_) {  // scale place_keep_diff_ by ik / zk
      if (((increase_keep_ <= 0) || (zeropos_keep_ <= 0)) &&
          ((increase_keep_ + 0.5 > 0) || (zeropos_keep_ + 0.5 > 0))) {  // maybe in DAT these values are -ve
        increase_place_ =
            zeropos_keep_ + increase_zeropos_diff_ +
            std::max(place_keep_diff_, (place_keep_diff_ * (0.5 + increase_keep_) / (0.5 + zeropos_keep_)));
      } else {
        increase_place_ = zeropos_keep_ + increase_zeropos_diff_ +
                          std::max(place_keep_diff_, (place_keep_diff_ * increase_keep_ / zeropos_keep_));
      }
    } else {  // just take it from place
      increase_place_ = zeropos_place_ + increase_zeropos_diff_;
    }
    read_increase_place_ = true;
  }
  if (read_zeropos_keep_ && read_zeropos_decrease_diff_ && (!read_decrease_keep_)) {
    decrease_keep_ = zeropos_keep_ - zeropos_decrease_diff_;
    read_decrease_keep_ = true;
  }
  if (read_zeropos_place_ && read_zeropos_decrease_diff_ && (!read_decrease_place_)) {
    if (read_zeropos_keep_ && read_zeropos_decrease_diff_ &&
        read_place_keep_diff_) {  // scale place_keep_diff_ by dk / zk
      if (((decrease_keep_ <= 0) || (zeropos_keep_ <= 0)) &&
          ((decrease_keep_ + 0.5 > 0) || (zeropos_keep_ + 0.5 > 0))) {  // maybe in DAT these values are -ve
        decrease_place_ = zeropos_keep_ - zeropos_decrease_diff_ +
                          std::max(0.0, std::min(place_keep_diff_,
                                                 (place_keep_diff_ * (0.5 + decrease_keep_) / (0.5 + zeropos_keep_))));
      } else {
        decrease_place_ =
            zeropos_keep_ - zeropos_decrease_diff_ +
            std::max(0.0, std::min(place_keep_diff_, (place_keep_diff_ * decrease_keep_ / zeropos_keep_)));
      }
    } else {  // just take it from place
      decrease_place_ = zeropos_place_ - zeropos_decrease_diff_;
    }
    read_decrease_place_ = true;
  }
  if (read_high_uts_factor_) {
    original_unit_trade_size_ = unit_trade_size_;
  }
  // initialise max_global_risk_ to max_position_
  if (!read_max_global_risk_) {
    max_global_risk_ = max_position_;
  }

  if (dep_short_code.compare("USD000000TOD") == 0 || dep_short_code.compare("USD000UTSTOM") == 0 ||
      dep_short_code.compare("EUR_RUB__TOD") == 0 || dep_short_code.compare("EUR_RUB__TOM") == 0) {
    if (!read_min_allowed_unit_trade_size_) {
      read_min_allowed_unit_trade_size_ = true;
      min_allowed_unit_trade_size_ = 50;
    }
  }

  if (read_max_min_diff_ratio_) {
    max_min_diff_ = zeropos_place_ * max_min_diff_ratio_;
    read_max_min_diff_ = true;
  }
  if (!read_stdev_overall_cap_) {
    stdev_overall_cap_ = stdev_cap_;
  }

  if (read_max_unit_ratio_pp_) {
    max_position_pp_ = max_unit_ratio_pp_ * unit_trade_size_;
    if (!read_indep_safe_ticks_low_pos_) {
      indep_safe_ticks_low_pos_ = indep_safe_ticks_;
    }
    if (!read_indep_safe_size_low_pos_) {
      indep_safe_size_low_pos_ = indep_safe_size_;
    }
  }
}

std::function<double(double, double)> ParamSet::get_dynamic_zero_pos_keep_func_vx_diff(std::string param_line) {
  std::istringstream temp_iss_0(param_line);
  std::string function_name = "";

  temp_iss_0 >> function_name;
  temp_iss_0 >> function_name;
  temp_iss_0 >> function_name;

  if (function_name.compare("LINEAR_CUTOFF") == 0) {
    auto g = [param_line](double val, double old_param_val) {
      std::istringstream temp_iss_1(param_line);
      std::string function_name_1 = "";

      temp_iss_1 >> function_name_1;
      temp_iss_1 >> function_name_1;
      temp_iss_1 >> function_name_1;

      double cutoff;
      double slope;
      temp_iss_1 >> cutoff;
      temp_iss_1 >> slope;

      double thres = old_param_val;
      if (val > cutoff) {
        thres += (val - cutoff) * slope;
      }
      return (thres);

    };
    return (g);
  } else {
    auto g = [param_line](double val, double old_param_val) {
      double thres = old_param_val;
      return (thres);
    };
    return (g);
  }
}

std::function<int(double)> ParamSet::get_dynamic_uts_func_volume(std::string param_line) {
  auto g = [param_line](double volume) {
    std::istringstream temp_iss_1(param_line);
    std::string function_name_1 = "";

    temp_iss_1 >> function_name_1;
    temp_iss_1 >> function_name_1;

    double low_vol;
    double high_vol;
    double low_uts;
    double high_uts;

    temp_iss_1 >> low_vol;
    temp_iss_1 >> high_vol;
    temp_iss_1 >> low_uts;
    temp_iss_1 >> high_uts;

    double slope = (high_uts - low_uts) / (high_vol - low_vol);
    double intercept = low_uts - (slope * low_vol);

    double uts = (volume * slope) + intercept;
    uts = std::max(low_uts, uts);
    uts = std::min(high_uts, uts);
    return ((int)uts);

  };
  return (g);
}

std::function<int(double)> ParamSet::get_dynamic_uts_func_l1_size(std::string param_line) {
  auto g = [param_line](double l1_size) {
    std::istringstream temp_iss_1(param_line);
    std::string function_name_1 = "";

    temp_iss_1 >> function_name_1;
    temp_iss_1 >> function_name_1;

    double low_l1_size;
    double high_l1_size;
    double low_uts;
    double high_uts;

    temp_iss_1 >> low_l1_size;
    temp_iss_1 >> high_l1_size;
    temp_iss_1 >> low_uts;
    temp_iss_1 >> high_uts;

    double slope = (high_uts - low_uts) / (high_l1_size - low_l1_size);
    double intercept = low_uts - (slope * low_l1_size);

    double uts = (l1_size * slope) + intercept;
    uts = std::max(low_uts, uts);
    uts = std::min(high_uts, uts);
    return ((int)uts);

  };
  return (g);
}

std::function<int(double)> ParamSet::get_dynamic_otl_func_stdev(std::string param_line) {
  auto g = [param_line](double stdev) {
    std::istringstream temp_iss_1(param_line);
    std::string function_name_1 = "";

    temp_iss_1 >> function_name_1;
    temp_iss_1 >> function_name_1;

    double low_stdev;
    double high_stdev;
    double low_otl;
    double high_otl;

    temp_iss_1 >> low_stdev;
    temp_iss_1 >> high_stdev;
    temp_iss_1 >> low_otl;
    temp_iss_1 >> high_otl;

    double slope = (high_stdev - low_stdev) / (high_otl - low_otl);
    double intercept = low_otl - (slope * low_stdev);

    double uts = (stdev * slope) + intercept;
    uts = std::max(low_stdev, uts);
    uts = std::min(high_stdev, uts);
    return ((int)uts);

  };
  return (g);
}

void ParamSet::change_params(std::string param_line) {
  double stat_val = 0;
  double new_param_val = 0;

  std::istringstream temp_iss_0(param_line);
  std::string param_name = "";
  std::string function_name = "";

  std::string data_file = "/spare/local/tradeinfo/VIX/VX_0_stats";

  temp_iss_0 >> param_name;
  temp_iss_0 >> param_name;

  if (param_name.compare("DYNAMIC_ZEROPOS_KEEP") == 0) {
    auto f = get_dynamic_zero_pos_keep_func_vx_diff(param_line);
    int column_index = 3;
    std::ifstream infile(data_file);

    double value_0 = 0;
    double value_1 = 0;

    std::string line_0;
    std::string line_1;

    std::getline(infile, line_1);

    while (std::getline(infile, line_0)) {
      std::istringstream iss(line_0);

      int temp_date;
      iss >> temp_date;

      if (temp_date == tradingdate_) {
        int temp_ind = column_index;
        std::istringstream temp_iss_1(line_0);
        std::istringstream temp_iss_2(line_1);

        while (temp_ind > 0) {
          temp_iss_1 >> value_0;
          temp_iss_2 >> value_1;
          temp_ind--;
        }
      }
      line_1 = line_0;
    }

    stat_val = std::abs(value_0 - value_1);
    new_param_val = f(stat_val, zeropos_keep_);
    zeropos_keep_ = new_param_val;
  }

  if (param_name.compare("DYNAMIC_UTS_VOLUME_AR_5") == 0) {
    auto f = get_dynamic_uts_func_volume(param_line);
    int column_index = 2;
    std::ifstream infile(data_file);

    int ar = 5;
    double avg_volume = 0;
    double count = 0;

    std::vector<std::string> lines(ar);
    std::vector<double> volumes(ar);
    std::vector<double> weights = {};
    std::string temp_line = "";

    std::string weights_file = "/media/shared/ephemeral2/VIX/VX_0_volume_weights_5";

    std::ifstream ss(weights_file);
    // No loop is necessary, because you can use copy()

    std::string token;

    while (std::getline(ss, token)) {
      weights.push_back(atof(token.c_str()));
    }

    std::reverse(weights.begin(), weights.end());

    for (auto i = 0u; i < lines.size(); i++) {
      std::getline(infile, lines[i]);
    }
    while (std::getline(infile, temp_line)) {
      std::istringstream iss(temp_line);

      int temp_date;
      double temp_volume;

      iss >> temp_date;
      iss >> temp_volume;

      if (temp_date == tradingdate_) {
        int temp_ind = 0;
        for (auto i = 0u; i < lines.size(); i++) {
          temp_ind = column_index;
          std::istringstream temp_iss(lines[i]);
          double value_0 = 0;
          while (temp_ind > 0) {
            temp_iss >> value_0;
            temp_ind--;
          }
          volumes[i] = value_0;
        }
        break;
      }

      for (int i = 0; i < ar - 1; i++) {
        lines[i] = lines[i + 1];
      }

      lines[ar - 1] = temp_line;

      count += 1;
      avg_volume += temp_volume;
    }
    avg_volume /= count;
    double pred_volume = std::inner_product(volumes.begin(), volumes.end(), weights.begin(), 0.0) / avg_volume;
    int new_uts = f(pred_volume);
    unit_trade_size_ = new_uts;
  }

  if (param_name.compare("DYNAMIC_UTS_L1_SIZE") == 0) {
    auto f = get_dynamic_uts_func_l1_size(param_line);
    int column_index = 2;
    std::ifstream infile(data_file);

    int ar = 2;
    double avg_l1_size = 0;
    double count = 0;

    std::vector<std::string> lines(ar);
    std::vector<double> l1_sizes(ar);
    std::vector<double> weights = {};
    std::string temp_line = "";

    std::string weights_file = "/media/shared/ephemeral2/VIX/VX_0_l1_size_weights_2";

    std::ifstream ss(weights_file);
    // No loop is necessary, because you can use copy()

    std::string token;

    while (std::getline(ss, token)) {
      weights.push_back(atof(token.c_str()));
    }

    std::reverse(weights.begin(), weights.end());

    for (auto i = 0u; i < lines.size(); i++) {
      std::getline(infile, lines[i]);
    }
    while (std::getline(infile, temp_line)) {
      std::istringstream iss(temp_line);

      int temp_date;
      double temp_l1_size;

      iss >> temp_date;
      iss >> temp_l1_size;

      if (temp_date == tradingdate_) {
        int temp_ind = 0;
        for (auto i = 0u; i < lines.size(); i++) {
          temp_ind = column_index;
          std::istringstream temp_iss(lines[i]);
          double value_0 = 0;
          while (temp_ind > 0) {
            temp_iss >> value_0;
            temp_ind--;
          }
          l1_sizes[i] = value_0;
        }
        break;
      }

      for (int i = 0; i < ar - 1; i++) {
        lines[i] = lines[i + 1];
      }

      lines[ar - 1] = temp_line;

      count += 1;
      avg_l1_size += temp_l1_size;
    }
    avg_l1_size /= count;
    double pred_l1_size = std::inner_product(l1_sizes.begin(), l1_sizes.end(), weights.begin(), 0.0);
    int new_uts = f(pred_l1_size);
    unit_trade_size_ = new_uts;
  }

  if (param_name.compare("DYNAMIC_OTL_STDEV") == 0) {
    auto f = get_dynamic_otl_func_stdev(param_line);
    int column_index = 7;
    std::ifstream infile(data_file);

    int ar = 3;
    double avg_stdev = 0;
    double count = 0;

    std::vector<std::string> lines(ar);
    std::vector<double> stdev_vec(ar);
    std::vector<double> weights = {};
    std::string temp_line = "";

    std::string weights_file = "/media/shared/ephemeral2/VIX/VX_0_stdev_weights_3";

    std::ifstream ss(weights_file);
    // No loop is necessary, because you can use copy()

    std::string token;

    while (std::getline(ss, token)) {
      weights.push_back(atof(token.c_str()));
    }

    std::reverse(weights.begin(), weights.end());

    for (auto i = 0u; i < lines.size(); i++) {
      std::getline(infile, lines[i]);
    }
    while (std::getline(infile, temp_line)) {
      std::istringstream iss(temp_line);

      int temp_date;
      double temp_stdev;

      iss >> temp_date;
      iss >> temp_stdev;

      if (temp_date == tradingdate_) {
        int temp_ind = 0;
        for (auto i = 0u; i < lines.size(); i++) {
          temp_ind = column_index;
          std::istringstream temp_iss(lines[i]);
          double value_0 = 0;
          while (temp_ind > 0) {
            temp_iss >> value_0;
            temp_ind--;
          }
          stdev_vec[i] = value_0;
        }
        break;
      }

      for (int i = 0; i < ar - 1; i++) {
        lines[i] = lines[i + 1];
      }

      lines[ar - 1] = temp_line;

      count += 1;
      avg_stdev += temp_stdev;
    }
    avg_stdev /= count;
    double pred_stdev = std::inner_product(stdev_vec.begin(), stdev_vec.end(), weights.begin(), 0.0);
    max_opentrade_loss_per_uts_ = f(pred_stdev);
    max_opentrade_loss_ = unit_trade_size_ * max_opentrade_loss_per_uts_;
  }
}
}
