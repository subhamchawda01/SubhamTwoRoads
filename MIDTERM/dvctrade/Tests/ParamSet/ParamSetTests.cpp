/*
 * ParamSetTests.cpp
 *
 *  Created on: 05-Jul-2017
 *      Author: mehul
 */

#include "dvctrade/Tests/ParamSet/ParamSetTests.hpp"

namespace HFTEST {
using namespace HFSAT;

void ParamSetTests::setUp(void) {
  shortcode_vec_.push_back("USD000UTSTOM");
  tradingdate_ = 20170123;

  common_smv_source_ = new CommonSMVSource(shortcode_vec_, tradingdate_, false);
  common_smv_source_->InitializeVariables();
  watch_ = &common_smv_source_->getWatch();
  dbglogger_ = &common_smv_source_->getLogger();

  auto& sid_to_smv_map = common_smv_source_->getSMVMap();
  smv = sid_to_smv_map[SecurityNameIndexer::GetUniqueInstance().GetIdFromString("USD000UTSTOM")];

  std::string paramfile_path_ = GetTestDataFullPath("param_paramset_test_1", "dvctrade");
  param_1 = new ParamSet(paramfile_path_, tradingdate_, smv->shortcode());

  paramfile_path_ = GetTestDataFullPath("param_paramset_test_2", "dvctrade");
  param_2 = new ParamSet(paramfile_path_, tradingdate_, smv->shortcode());

  paramfile_path_ = GetTestDataFullPath("param_paramset_test_3", "dvctrade");
  param_3 = new ParamSet(paramfile_path_, tradingdate_, smv->shortcode());

  param_1->LoadParamSet(smv->shortcode());
  param_2->LoadParamSet(smv->shortcode());
  param_3->LoadParamSet(smv->shortcode());
}

void ParamSetTests::TestLoadParamSetImportantParams(void) {
  CPPUNIT_ASSERT_DOUBLES_EQUAL(param_1->max_unit_ratio_, 6, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(param_1->highpos_limits_unit_ratio_, 5, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(param_1->zeropos_limits_unit_ratio_, 4, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(param_1->zeropos_keep_, 3.00, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(param_1->place_keep_diff_, 1.00, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(param_1->increase_zeropos_diff_, 1.50, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(param_1->zeropos_decrease_diff_, 1.50, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(param_1->max_loss_, 2000, DOUBLE_ASSERT_PRECISION);

  CPPUNIT_ASSERT(param_1->release_core_premature_ == false);

  CPPUNIT_ASSERT(param_1->read_max_unit_ratio_ == true);
  CPPUNIT_ASSERT(param_1->read_highpos_limits_unit_ratio_ == true);
  CPPUNIT_ASSERT(param_1->read_zeropos_limits_unit_ratio_ == false);
  CPPUNIT_ASSERT(param_1->read_zeropos_keep_ == true);
  CPPUNIT_ASSERT(param_1->read_place_keep_diff_ == true);
  CPPUNIT_ASSERT(param_1->read_increase_zeropos_diff_ == true);
  CPPUNIT_ASSERT(param_1->read_zeropos_decrease_diff_ == true);
  CPPUNIT_ASSERT(param_1->read_max_loss_ == true);
}

void ParamSetTests::TestLoadParamSetAllParams(void) {
  CPPUNIT_ASSERT(param_3->read_dat_weightage_fraction_ == false);
  CPPUNIT_ASSERT(param_3->read_get_flat_by_fok_mode_ == false);
  CPPUNIT_ASSERT(param_3->read_worst_case_position_ == false);
  CPPUNIT_ASSERT(param_3->read_worst_case_unit_ratio_ == false);
  CPPUNIT_ASSERT(param_3->read_max_position_ == false);
  CPPUNIT_ASSERT(param_3->read_max_unit_ratio_ == false);
  CPPUNIT_ASSERT(param_3->read_max_total_unit_ratio_to_place_ == false);
  CPPUNIT_ASSERT(param_3->read_max_total_size_to_place_ == false);
  CPPUNIT_ASSERT(param_3->read_unit_trade_size_ == false);
  CPPUNIT_ASSERT(param_3->read_min_allowed_unit_trade_size_ == false);
  CPPUNIT_ASSERT(param_3->read_max_global_position_ == false);
  CPPUNIT_ASSERT(param_3->read_max_security_position_ == false);
  CPPUNIT_ASSERT(param_3->read_explicit_max_short_position_ == false);
  CPPUNIT_ASSERT(param_3->read_explicit_max_short_unit_ratio_ == false);
  CPPUNIT_ASSERT(param_3->read_explicit_worst_case_short_position_ == false);
  CPPUNIT_ASSERT(param_3->read_explicit_worst_case_short_unit_ratio_ == false);
  CPPUNIT_ASSERT(param_3->read_explicit_max_long_position_ == false);
  CPPUNIT_ASSERT(param_3->read_explicit_max_long_unit_ratio_ == false);
  CPPUNIT_ASSERT(param_3->read_explicit_worst_case_long_position_ == false);
  CPPUNIT_ASSERT(param_3->read_explicit_worst_case_long_unit_ratio_ == false);
  CPPUNIT_ASSERT(param_3->read_non_standard_market_condition_check_short_msecs_ == false);
  CPPUNIT_ASSERT(param_3->read_non_standard_market_condition_check_long_msecs_ == false);
  CPPUNIT_ASSERT(param_3->read_non_standard_market_condition_min_best_level_size_ == false);
  CPPUNIT_ASSERT(param_3->read_non_standard_market_condition_min_best_level_order_count_ == false);
  CPPUNIT_ASSERT(param_3->read_non_standard_market_condition_max_avg_order_size_ == false);
  CPPUNIT_ASSERT(param_3->read_non_standard_market_condition_min_counter_order_size_ == false);
  CPPUNIT_ASSERT(param_3->read_non_standard_market_condition_max_spread_ == false);
  CPPUNIT_ASSERT(param_3->read_non_standard_market_condition_max_position_ == false);
  CPPUNIT_ASSERT(param_3->read_highpos_limits_ == false);
  CPPUNIT_ASSERT(param_3->read_highpos_limits_unit_ratio_ == false);
  CPPUNIT_ASSERT(param_3->read_highpos_thresh_factor_ == false);
  CPPUNIT_ASSERT(param_3->read_highpos_thresh_decrease_ == false);
  CPPUNIT_ASSERT(param_3->read_highpos_size_factor_ == false);
  CPPUNIT_ASSERT(param_3->read_increase_place_ == false);
  CPPUNIT_ASSERT(param_3->read_increase_keep_ == false);
  CPPUNIT_ASSERT(param_3->read_zeropos_limits_ == false);
  CPPUNIT_ASSERT(param_3->read_zeropos_limits_unit_ratio_ == false);
  CPPUNIT_ASSERT(param_3->read_zeropos_place_ == false);
  CPPUNIT_ASSERT(param_3->read_zeropos_keep_ == false);
  CPPUNIT_ASSERT(param_3->read_decrease_place_ == false);
  CPPUNIT_ASSERT(param_3->read_decrease_keep_ == false);
  CPPUNIT_ASSERT(param_3->read_place_keep_diff_ == false);
  CPPUNIT_ASSERT(param_3->read_increase_zeropos_diff_ == false);
  CPPUNIT_ASSERT(param_3->read_zeropos_decrease_diff_ == false);
  CPPUNIT_ASSERT(param_3->read_spread_increase_ == false);
  CPPUNIT_ASSERT(param_3->read_max_min_diff_ == false);
  CPPUNIT_ASSERT(param_3->read_max_min_diff_ratio_ == false);
  CPPUNIT_ASSERT(param_3->read_bucket_size_ == false);
  CPPUNIT_ASSERT(param_3->read_positioning_thresh_decrease_ == false);
  CPPUNIT_ASSERT(param_3->read_positioning_threshold_ == false);
  CPPUNIT_ASSERT(param_3->read_max_min_diff_order_ == false);
  CPPUNIT_ASSERT(param_3->read_scale_max_pos_ == false);
  CPPUNIT_ASSERT(param_3->read_high_uts_factor_ == false);
  CPPUNIT_ASSERT(param_3->read_max_high_uts_l1_factor_ == false);
  CPPUNIT_ASSERT(param_3->read_thresh_increase_ == false);
  CPPUNIT_ASSERT(param_3->read_thresh_decrease_ == false);
  CPPUNIT_ASSERT(param_3->read_thresh_place_ == false);
  CPPUNIT_ASSERT(param_3->read_thresh_place_keep_diff_ == false);
  CPPUNIT_ASSERT(param_3->read_safe_distance_ == false);
  CPPUNIT_ASSERT(param_3->read_high_spread_allowance_ == false);
  CPPUNIT_ASSERT(param_3->read_allowed_to_improve_ == false);
  CPPUNIT_ASSERT(param_3->read_allowed_to_aggress_ == false);
  CPPUNIT_ASSERT(param_3->read_use_new_aggress_logic_ == false);
  CPPUNIT_ASSERT(param_3->read_improve_ == false);
  CPPUNIT_ASSERT(param_3->read_aggressive_ == false);
  CPPUNIT_ASSERT(param_3->read_max_self_ratio_at_level_ == false);
  CPPUNIT_ASSERT(param_3->read_longevity_support_ == false);
  CPPUNIT_ASSERT(param_3->read_max_position_to_lift_ == false);
  CPPUNIT_ASSERT(param_3->read_max_position_to_bidimprove_ == false);
  CPPUNIT_ASSERT(param_3->read_max_position_to_cancel_on_lift_ == false);
  CPPUNIT_ASSERT(param_3->read_max_size_to_aggress_ == false);
  CPPUNIT_ASSERT(param_3->read_min_position_to_hit_ == false);
  CPPUNIT_ASSERT(param_3->read_min_position_to_askimprove_ == false);
  CPPUNIT_ASSERT(param_3->read_min_position_to_cancel_on_hit_ == false);
  CPPUNIT_ASSERT(param_3->read_max_int_spread_to_place_ == false);
  CPPUNIT_ASSERT(param_3->read_max_int_level_diff_to_place_ == false);
  CPPUNIT_ASSERT(param_3->read_max_int_spread_to_cross_ == false);
  CPPUNIT_ASSERT(param_3->read_min_int_spread_to_improve_ == false);
  CPPUNIT_ASSERT(param_3->read_num_non_best_bid_levels_monitored_ == false);
  CPPUNIT_ASSERT(param_3->read_num_non_best_ask_levels_monitored_ == false);
  CPPUNIT_ASSERT(param_3->read_min_distance_for_non_best_ == false);
  CPPUNIT_ASSERT(param_3->read_max_distance_for_non_best_ == false);
  CPPUNIT_ASSERT(param_3->read_max_bid_ask_order_diff_ == false);
  CPPUNIT_ASSERT(param_3->read_min_size_to_quote_ == false);
  CPPUNIT_ASSERT(param_3->read_maturity_ == false);
  CPPUNIT_ASSERT(param_3->read_min_quote_distance_from_best_ == false);
  CPPUNIT_ASSERT(param_3->read_stdev_quote_factor_ == false);
  CPPUNIT_ASSERT(param_3->read_min_size_ahead_for_non_best_ == false);
  CPPUNIT_ASSERT(param_3->read_ignore_max_pos_check_for_non_best_ == false);
  CPPUNIT_ASSERT(param_3->read_max_loss_ == false);
  CPPUNIT_ASSERT(param_3->read_max_pnl_ == false);
  CPPUNIT_ASSERT(param_3->read_short_term_global_max_loss_ == false);
  CPPUNIT_ASSERT(param_3->read_global_max_loss_ == false);
  CPPUNIT_ASSERT(param_3->read_max_opentrade_loss_ == false);
  CPPUNIT_ASSERT(param_3->read_max_opentrade_loss_per_uts_ == false);
  CPPUNIT_ASSERT(param_3->read_max_drawdown_ == false);
  CPPUNIT_ASSERT(param_3->read_max_short_term_loss_ == false);
  CPPUNIT_ASSERT(param_3->read_place_cancel_cooloff_ == false);
  CPPUNIT_ASSERT(param_3->read_cooloff_interval_ == false);
  CPPUNIT_ASSERT(param_3->read_agg_cooloff_interval_ == false);
  CPPUNIT_ASSERT(param_3->read_improve_cooloff_interval_ == false);
  CPPUNIT_ASSERT(param_3->read_stdev_fact_ == false);
  CPPUNIT_ASSERT(param_3->read_stdev_cap_ == false);
  CPPUNIT_ASSERT(param_3->read_stdev_duration_ == false);
  CPPUNIT_ASSERT(param_3->read_px_band_ == false);
  CPPUNIT_ASSERT(param_3->read_low_stdev_lvl_ == false);
  CPPUNIT_ASSERT(param_3->read_min_size_to_join_ == false);
  CPPUNIT_ASSERT(param_3->read_spread_add_ == false);
  CPPUNIT_ASSERT(param_3->read_severity_to_getflat_on_ == false);
  CPPUNIT_ASSERT(param_3->read_ezone_traded_ == false);
  CPPUNIT_ASSERT(param_3->read_max_severity_to_keep_suporders_on_ == false);
  CPPUNIT_ASSERT(param_3->read_agg_closeout_utc_hhmm_ == false);
  CPPUNIT_ASSERT(param_3->read_agg_closeout_max_size_ == false);
  CPPUNIT_ASSERT(param_3->read_break_msecs_on_max_opentrade_loss_ == false);
  CPPUNIT_ASSERT(param_3->read_indep_safe_ticks_ == false);
  CPPUNIT_ASSERT(param_3->read_indep_safe_size_ == false);
  CPPUNIT_ASSERT(param_3->read_indep_safe_price_diff_ == false);
  CPPUNIT_ASSERT(param_3->read_mini_keep_thresh_ == false);
  CPPUNIT_ASSERT(param_3->read_mini_agg_thresh_ == false);
  CPPUNIT_ASSERT(param_3->read_projected_price_duration_ == false);
  CPPUNIT_ASSERT(param_3->read_pp_place_keep_diff_ == false);
  CPPUNIT_ASSERT(param_3->read_enforce_min_cooloff_ == false);
  CPPUNIT_ASSERT(param_3->read_pclose_factor_ == false);
  CPPUNIT_ASSERT(param_3->read_moderate_time_limit_ == false);
  CPPUNIT_ASSERT(param_3->read_high_time_limit_ == false);
  CPPUNIT_ASSERT(param_3->read_safe_cancel_size_ == false);
  CPPUNIT_ASSERT(param_3->read_place_on_trade_update_implied_quote_ == false);
  CPPUNIT_ASSERT(param_3->read_num_increase_ticks_to_keep_ == false);
  CPPUNIT_ASSERT(param_3->read_num_decrease_ticks_to_keep_ == false);
  CPPUNIT_ASSERT(param_3->read_is_liquid_ == false);
  CPPUNIT_ASSERT(param_3->read_stir_cancel_on_exec_cooloff_msecs_ == false);
  CPPUNIT_ASSERT(param_3->read_stir_cancel_on_level_change_cooloff_msecs_ == false);
  CPPUNIT_ASSERT(param_3->read_pair_exec_cancel_cooloff_msecs_ == false);
  CPPUNIT_ASSERT(param_3->read_max_global_risk_ == false);
  CPPUNIT_ASSERT(param_3->read_max_global_risk_ratio_ == false);
  CPPUNIT_ASSERT(param_3->read_owp_retail_offer_thresh_ == false);
  CPPUNIT_ASSERT(param_3->read_use_stable_bidask_levels_ == false);
  CPPUNIT_ASSERT(param_3->read_dpt_range_ == false);
  CPPUNIT_ASSERT(param_3->read_desired_position_leeway_ == false);
  CPPUNIT_ASSERT(param_3->read_desired_position_difference_ == false);
  CPPUNIT_ASSERT(param_3->read_place_or_cancel_ == false);
  CPPUNIT_ASSERT(param_3->read_position_change_compensation_ == false);
  CPPUNIT_ASSERT(param_3->read_liquidity_factor_ == false);
  CPPUNIT_ASSERT(param_3->read_online_stats_avg_dep_bidask_spread_ == false);
  CPPUNIT_ASSERT(param_3->read_place_multiple_orders_ == false);
  CPPUNIT_ASSERT(param_3->read_max_unit_size_at_level_ == false);
  CPPUNIT_ASSERT(param_3->read_size_diff_between_orders_ == false);
  CPPUNIT_ASSERT(param_3->read_size_disclosed_factor_ == false);
  CPPUNIT_ASSERT(param_3->read_pair_unit_trade_size_factor_ == false);
  CPPUNIT_ASSERT(param_3->read_pair_place_diff_ == false);
  CPPUNIT_ASSERT(param_3->read_pair_keep_diff_ == false);
  CPPUNIT_ASSERT(param_3->read_pair_aggressive_ == false);
  CPPUNIT_ASSERT(param_3->read_pair_improve_ == false);
  CPPUNIT_ASSERT(param_3->read_pair_zeropos_keep_ == false);
  CPPUNIT_ASSERT(param_3->read_pair_zeropos_decrease_diff_ == false);
  CPPUNIT_ASSERT(param_3->read_pair_increase_zeropos_diff_ == false);
  CPPUNIT_ASSERT(param_3->read_pair_place_keep_diff_ == false);
  CPPUNIT_ASSERT(param_3->read_abs_max_pos_factor_ == false);
  CPPUNIT_ASSERT(param_3->read_own_base_px_ == false);
  CPPUNIT_ASSERT(param_3->read_use_stable_book_ == false);
  CPPUNIT_ASSERT(param_3->read_spread_factor_ == false);
  CPPUNIT_ASSERT(param_3->read_spread_quote_factor_ == false);
  CPPUNIT_ASSERT(param_3->read_volume_ratio_stop_trading_lower_threshold_ == false);
  CPPUNIT_ASSERT(param_3->read_volume_ratio_stop_trading_upper_threshold_ == false);
  CPPUNIT_ASSERT(param_3->read_moving_bidask_spread_duration_ == false);
  CPPUNIT_ASSERT(param_3->read_max_global_beta_adjusted_notional_risk_ == false);
  CPPUNIT_ASSERT(param_3->read_self_pos_projection_factor_ == false);
  CPPUNIT_ASSERT(param_3->read_place_only_after_cancel_ == false);
  CPPUNIT_ASSERT(param_3->read_allow_modify_orders_ == false);
  CPPUNIT_ASSERT(param_3->read_source_for_dmur_ == false);
  CPPUNIT_ASSERT(param_3->read_spread_factor_agg_ == false);
  CPPUNIT_ASSERT(param_3->read_af_event_pxch_act_ == false);
  CPPUNIT_ASSERT(param_3->read_af_event_pxch_getflat_ == false);
  CPPUNIT_ASSERT(param_3->read_af_event_max_uts_pxch_ == false);
  CPPUNIT_ASSERT(param_3->read_af_scale_beta_ == false);
  CPPUNIT_ASSERT(param_3->read_bigtrades_cancel_ == false);
  CPPUNIT_ASSERT(param_3->read_use_notional_uts_ == false);
  CPPUNIT_ASSERT(param_3->read_notional_uts_ == false);
  CPPUNIT_ASSERT(param_3->read_regime_indicator_string_ == false);
  CPPUNIT_ASSERT(param_3->read_regimes_to_trade_ == false);
  CPPUNIT_ASSERT(param_3->read_trade_indicator_string_ == false);
  CPPUNIT_ASSERT(param_3->read_trade_bias_ == false);
  CPPUNIT_ASSERT(param_3->read_cancel_indicator_string_ == false);
  CPPUNIT_ASSERT(param_3->read_cancel_bias_ == false);
  CPPUNIT_ASSERT(param_3->read_implied_mkt_indicator_string_ == false);
  CPPUNIT_ASSERT(param_3->read_implied_mkt_ == false);
  CPPUNIT_ASSERT(param_3->read_dynamic_zeropos_keep_ == false);
  CPPUNIT_ASSERT(param_3->read_src_based_exec_changes_ == false);
  CPPUNIT_ASSERT(param_3->read_last_day_vol_ == false);
  CPPUNIT_ASSERT(param_3->read_stdev_overall_cap_ == false);
  CPPUNIT_ASSERT(param_3->read_max_trade_size_ == false);
  CPPUNIT_ASSERT(param_3->read_use_cancellation_model_ == false);
  CPPUNIT_ASSERT(param_3->read_canc_prob_ == false);
  CPPUNIT_ASSERT(param_3->read_interday_scaling_factor_ == false);
}

void ParamSetTests::TestLoadParamSetAssignesDefaults(void) {
  CPPUNIT_ASSERT_DOUBLES_EQUAL(param_2->max_unit_ratio_, 1, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(param_2->highpos_limits_unit_ratio_, 3, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(param_2->zeropos_limits_unit_ratio_, 2, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(param_2->zeropos_keep_, 100, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(param_2->place_keep_diff_, 1, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(param_2->increase_zeropos_diff_, 1, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(param_2->zeropos_decrease_diff_, 1, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(param_2->max_loss_, 3000, DOUBLE_ASSERT_PRECISION);

  CPPUNIT_ASSERT(param_2->release_core_premature_ == true);

  CPPUNIT_ASSERT(param_2->read_max_unit_ratio_ == false);
  CPPUNIT_ASSERT(param_2->read_highpos_limits_unit_ratio_ == false);
  CPPUNIT_ASSERT(param_2->read_zeropos_limits_unit_ratio_ == true);
  CPPUNIT_ASSERT(param_2->read_zeropos_keep_ == false);
  CPPUNIT_ASSERT(param_2->read_place_keep_diff_ == false);
  CPPUNIT_ASSERT(param_2->read_increase_zeropos_diff_ == false);
  CPPUNIT_ASSERT(param_2->read_zeropos_decrease_diff_ == false);
  CPPUNIT_ASSERT(param_2->read_max_loss_ == false);
}

void ParamSetTests::TestReconcileParams(void) {
  double max_position_ = (int)(param_1->unit_trade_size_ * param_1->max_unit_ratio_ + 0.5);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(param_1->max_position_, max_position_, DOUBLE_ASSERT_PRECISION);

  double zeropos_place_ = param_1->zeropos_keep_ + param_1->place_keep_diff_;
  CPPUNIT_ASSERT_DOUBLES_EQUAL(param_1->zeropos_place_, zeropos_place_, DOUBLE_ASSERT_PRECISION);

  double increase_keep_ = param_1->zeropos_keep_ + param_1->increase_zeropos_diff_;
  CPPUNIT_ASSERT_DOUBLES_EQUAL(param_1->increase_keep_, increase_keep_, DOUBLE_ASSERT_PRECISION);

  double increase_place_ = param_1->zeropos_keep_ + param_1->increase_zeropos_diff_ +
                           std::max(param_1->place_keep_diff_,
                                    (param_1->place_keep_diff_ * param_1->increase_keep_ / param_1->zeropos_keep_));
  CPPUNIT_ASSERT_DOUBLES_EQUAL(param_1->increase_place_, increase_place_, DOUBLE_ASSERT_PRECISION);

  double decrease_place_ =
      param_1->zeropos_keep_ - param_1->zeropos_decrease_diff_ +
      std::max(0.0, std::min(param_1->place_keep_diff_,
                             (param_1->place_keep_diff_ * param_1->decrease_keep_ / param_1->zeropos_keep_)));
  CPPUNIT_ASSERT_DOUBLES_EQUAL(param_1->decrease_place_, decrease_place_, DOUBLE_ASSERT_PRECISION);
}

void ParamSetTests::tearDown(void) {
  // Remove the unique instances

  HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
  HFSAT::SecurityDefinitions::RemoveUniqueInstance();
  HFSAT::SecurityNameIndexer::GetUniqueInstance().Clear();
  HFSAT::CurrencyConvertor::RemoveInstance();

  // deallocate the heap variables
  delete common_smv_source_;
  //  delete param_;  // No destructor for paramset available
}
}
