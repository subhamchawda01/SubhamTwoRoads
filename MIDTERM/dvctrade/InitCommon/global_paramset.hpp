// =====================================================================================
//
//       Filename:  global_paramset.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Monday 30 May 2016 05:02:42  UTC
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

namespace HFSAT {

struct GlobalParamSet {
  int tradingdate_;
  double strat_max_loss_;
  double product_maxloss_;
  int returns_duration_;
  double open_trade_loss_;
  int inst_uts_;
  int inst_maxlots_;
  int inst_worstcaselots_;
  int portfolio_maxlots_;
  std::vector<int> inst_mur_vec_;
  int cooloff_secs_;
  double base_threshold_;
  double place_keep_diff_;
  double change_threshold_;
  double increase_threshold_;
  double decrease_threshold_;
  double increase_threshold_mur_;
  double decrease_threshold_mur_;
  bool use_book_indicator_;
  bool use_trade_indicator_;
  std::string histfile_prefix_;
  unsigned int hist_price_length_;
  unsigned int hist_error_length_;
  double time_hysterisis_factor_;
  double notional_for_unit_lot_;   // notional in ruppes corresponding to 1 lot of param
  bool use_notional_scaling_;      // whether lotsize/maxpos/thresholds need to incorporate notional
  bool use_notional_n2d_scaling_;  // whether lotsize/maxpos/thresholds need to incorporate notional
  int portprice_type_;
  std::vector<double> custom_scaling_vec_;
  bool use_custom_scaling_;
  bool use_dv01_ratios_;
  std::vector<double> base_threshold_vec_;

  bool use_abnormal_volume_getflat_;
  double abnormal_share_set_threshold_;  // the minimum market share that is required for triggering getflat
  double severity_to_getflat_on_;

  int opt_maxlots_;
  int min_bid_int_price_to_trade_;  // for options to avoid trading close to expiry or otherwise
  int min_days_to_expiry_to_trade_;
  std::vector<int> staggered_getflat_msecs_vec_;  // we reduce option_max_position by half for every msec
  int aggressive_getflat_msecs_;                  // just in case
  int rate_in_minutes_to_recompute_delta_;
  int iv_model_setting_;
  int iv_model_aux_token_1_;
  int iv_model_aux_token_2_;
  bool aggressive_;
  int max_int_spread_to_cross_;
  int agg_cooloff_secs_;
  double opt_bid_ask_spread_factor_;
  bool place_supporting_orders_;

  bool read_strat_max_loss_;
  bool read_product_maxloss_;
  bool read_returns_duration_;
  bool read_open_trade_loss_;
  bool read_inst_uts_;
  bool read_inst_maxlots_;
  bool read_portfolio_maxlots_;
  bool read_inst_mur_vec_;
  bool read_cooloff_secs_;
  bool read_base_threshold_;
  bool read_change_threshold_;
  bool read_increase_threshold_;
  bool read_decrease_threshold_;
  bool read_increase_threshold_mur_;
  bool read_decrease_threshold_mur_;
  bool read_use_book_indicator_;
  bool read_use_trade_indicator_;
  bool read_histfile_prefix_;
  bool read_hist_prices_length_;
  bool read_hist_error_length_;
  bool read_time_hysterisis_factor_;
  bool read_notional_for_unit_lot_;
  bool read_use_notional_scaling_;
  bool read_use_notional_n2d_scaling_;

  bool read_use_abnormal_volume_getflat_;
  bool read_abnormal_share_set_threshold_;
  bool read_severity_to_getflat_on_;
  bool read_base_threshold_vec_;

  std::string paramfilename_;

  GlobalParamSet(const std::string& _paramfilename_, const int r_tradingdate_);
  void LoadGolbalParamSet();
  void ReconcileParams();
};
}
