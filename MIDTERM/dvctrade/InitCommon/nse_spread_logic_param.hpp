#ifndef MT_PARAM_HPP
#define MT_PARAM_HPP

#include <string>
#include <fstream>
#include <strings.h>
#include <string.h>
#include <sstream>

namespace MT_SPRD {

struct ParamSet {
 public:
  double NAV_;  // this is strategy NAV

  double
      risk_factor_;  // the fraction of NAV being allocated to margin. Used to tune strategy volatility to desired level

  std::string instrument_1_;  // first instrument of pair
  std::string instrument_2_;  // second instrument of pair

  double hedge_factor_;  // #units of instrument_2 to be sold(bought) when 1 unit of instrument_1 is bought(sold)

  double margin_1_;  // margin for instrument 1, eg 0.15 means 15% margin needed.
  double margin_2_;  // margin for instrument 2

  uint32_t spread_hist_length_;  // spread history vector length - for support current spread computations

  uint32_t trade_cooloff_interval_;  // time between two successive trades of same side - in minutes

  int max_unit_ratio_;          // number of steps of legging in/legging out
  int unit_trade_size_1_;       // number of LOTS of instrument1 bought at one time - derived
  int unit_trade_size_2_;       // number of LOTS of instrument2 bought at one time - derived
  int orig_unit_trade_size_1_;  // defined since adf/hlife/ret modulate unit_trade_size which needs handling in case
                                // they are externally set
  int orig_unit_trade_size_2_;
  bool uts_set_externally_;
  int lotsize_1_;  // number of contracts per lot for instrument 1 - derived
  int lotsize_2_;  // number of contracts per lot for instrument 2 - derived
  bool lotsize_set_externally_;

  // THRESHOLDS for algos
  // initial values are sored - for case in which threshold is in terms of zscores
  double entry_spread_threshold_;  // spread value at which a new position is opened - in bps
  double initial_entry_spread_threshold_;
  double roll_reentry_threshold_;  // ZS threshold for taking position in FUT1 on day of roll
  double initial_roll_reentry_threshold_;
  double incremental_entry_threshold_;  // additional threshold demanded for legging in steps, eg we take
  // 2*unit_trade_size at spread threshold = entry_spread_threshold_ + incremental_entry_threshold
  double minimum_absolute_threshold_;  // absolute value of threshold

  double initial_incremental_entry_threshold_;
  double entry_exit_differential_;  // profit margin for closing spread position - in bps
  double initial_entry_exit_differential_;

  // StopLoss and StopGain numbers
  double stop_loss_;  // drawdown in bps at which open trade is closed
  double stop_gain_;  // profit in bps at which open trade is closed

  // vector length for stdev computation for zscore and regression parameters
  uint32_t zscore_vec_len_;

  // 0 is unsupported ; 1 is linear reg with no constant term; 2 is kalman
  int spread_comp_mode_;

  // 0 is price ; 1 is logprice
  int asset_comp_mode_;

  // parameters to be used by spread_exec_logic
  bool is_inst1_pass_;
  // control how frequently does spread_exec_logic handle marketupdate calls
  double px_thresh_;
  int time_thresh_;
  // control how frequently are orders modified
  int pass_ord_mod_time_threshold_;
  int agg_ord_mod_time_threshold_;
  double pass_ord_mod_std_threshold_;
  double agg_ord_mod_std_threshold_;

  // target vol level - of underlying spread in annualized terms
  double target_vol_;

  // secs from midnight to initiate roll
  unsigned int midnight_secs_to_roll_;

  // unique identifier of this set of parameters
  std::string param_id;

  // mabar dispatcher ids of the instruments
  int inst1_id;
  int inst2_id;

  // overnight position override in case execution fails
  int overnight_position_1;
  int overnight_position_2;
  bool override_on_pos;

  // flat flag which closes all positions in the pair
  bool get_flat;

  // variables to modulate UTS based on hlife
  double lower_hlife_thresh_;
  double upper_hlife_thresh_;

  // variables to modulate UTS based on adf
  double lower_adf_thresh_;
  double upper_adf_thresh_;

  // variables to modulate UTS based on trailing return divergence
  int ret_compute_duration_mins_;
  double ret_thresh_;

  // variables to set getflat based on earnings
  bool earnings_getflat_;  // should we be flat because of some trailing/future earnings; - doesn't need to be
                           // serialized as long as functions called on every start

  // variables to set getflat based on high persistent z-scores
  double zscore_ema_mult_factor_;  // mult factor applied per minute
  double zscore_getflat_threshold_;  // thresh for zscore getflat
  bool zscore_getflat_;  // getflat due to high zscore

  template <class Archive>
  void serialize(Archive& ar, const unsigned int /*   file_version */) {
    ar& NAV_& risk_factor_;
    ar& instrument_1_& instrument_2_;
    ar& hedge_factor_& margin_1_& margin_2_;
    ar& spread_hist_length_& trade_cooloff_interval_;
    ar& max_unit_ratio_& unit_trade_size_1_& unit_trade_size_2_& orig_unit_trade_size_1_& orig_unit_trade_size_2_;
    ar& uts_set_externally_& lotsize_1_& lotsize_2_& lotsize_set_externally_;
    ar& entry_spread_threshold_& initial_entry_spread_threshold_& roll_reentry_threshold_&
        initial_roll_reentry_threshold_;
    ar& incremental_entry_threshold_& minimum_absolute_threshold_& initial_incremental_entry_threshold_;
    ar& entry_exit_differential_& initial_entry_exit_differential_;
    ar& stop_loss_& stop_gain_& zscore_vec_len_& spread_comp_mode_& asset_comp_mode_;
    ar& is_inst1_pass_& px_thresh_& time_thresh_;
    ar& pass_ord_mod_time_threshold_& agg_ord_mod_time_threshold_& pass_ord_mod_std_threshold_&
        agg_ord_mod_std_threshold_;
    ar& target_vol_;
    ar& midnight_secs_to_roll_;
    ar& inst1_id& inst2_id;
    ar& param_id;
    ar& lower_hlife_thresh_& upper_hlife_thresh_;
    ar& lower_adf_thresh_& upper_adf_thresh_;
    ar& ret_compute_duration_mins_& ret_thresh_;
    ar& zscore_ema_mult_factor_& zscore_getflat_threshold_& zscore_getflat_;
  }

  ParamSet(){};
  ParamSet(const std::string& _paramfilename_);
  void ParseParamFile(const std::string& _paramfilename_);
  void RecomputeLotSize(double price_1_, double price_2_, double beta_, double voltarget_factor_, double hlife_val_,
                        double adf_val_, double abs_ret_);
  void RecomputeLotSize(double price_1_, double price_2_, double beta_, double voltarget_factor_);

  // set lotsizes - called for runs using HFT data
  // in this case we use provided lot sizes and don't compute them
  void SetContractLotSizes(int t_size_1_, int t_size_2_) {
    lotsize_1_ = t_size_1_;
    lotsize_2_ = t_size_2_;
    lotsize_set_externally_ = true;
  }

  // for setup in which entry/exit params are in terms of multiples of stdev, i.e. zscore values
  void MultiplyThreshBy(double t_factor_);

  std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "risk_factor_: " << risk_factor_ << "\n";
    t_temp_oss << "instrument_1_: " << instrument_1_ << "\n";
    t_temp_oss << "instrument_2_: " << instrument_2_ << "\n";
    t_temp_oss << "hedge_factor_: " << hedge_factor_ << "\n";
    t_temp_oss << "margin_1_: " << margin_1_ << "\n";
    t_temp_oss << "margin_2_: " << margin_2_ << "\n";
    t_temp_oss << "spread_hist_length_: " << spread_hist_length_ << "\n";
    t_temp_oss << "trade_cooloff_interval_: " << trade_cooloff_interval_ << "\n";
    t_temp_oss << "max_unit_ratio_: " << max_unit_ratio_ << "\n";
    t_temp_oss << "unit_trade_size_1_: " << unit_trade_size_1_ << "\n";
    t_temp_oss << "unit_trade_size_2_: " << unit_trade_size_2_ << "\n";
    t_temp_oss << "orig_unit_trade_size_1_: " << orig_unit_trade_size_1_ << "\n";
    t_temp_oss << "orig_unit_trade_size_2_: " << orig_unit_trade_size_2_ << "\n";
    t_temp_oss << "lotsize_1_: " << lotsize_1_ << "\n";
    t_temp_oss << "lotsize_2_: " << lotsize_2_ << "\n";
    t_temp_oss << "entry_spread_threshold_: " << entry_spread_threshold_ << "\n";
    t_temp_oss << "initial_entry_spread_threshold_: " << initial_entry_spread_threshold_ << "\n";
    t_temp_oss << "incremental_entry_threshold_: " << incremental_entry_threshold_ << "\n";
    t_temp_oss << "minimum_absolute_threshold_: " << minimum_absolute_threshold_ << "\n";
    t_temp_oss << "initial_incremental_entry_threshold_: " << initial_incremental_entry_threshold_ << "\n";
    t_temp_oss << "entry_exit_differential_: " << entry_exit_differential_ << "\n";
    t_temp_oss << "initial_entry_exit_differential_: " << initial_entry_exit_differential_ << "\n";
    t_temp_oss << "stop_loss_: " << stop_loss_ << "\n";
    t_temp_oss << "stop_gain_: " << stop_gain_ << "\n";
    t_temp_oss << "zscore_vec_len_: " << zscore_vec_len_ << "\n";
    t_temp_oss << "spread_comp_mode_: " << spread_comp_mode_ << "\n";
    t_temp_oss << "asset_comp_mode_: " << asset_comp_mode_ << "\n";
    t_temp_oss << "target_vol_: " << target_vol_ << "\n";
    t_temp_oss << "param_id: " << param_id << "\n";
    t_temp_oss << "lower_hlife_thresh_: " << lower_hlife_thresh_ << "\n";
    t_temp_oss << "upper_hlife_thresh_: " << upper_hlife_thresh_ << "\n";
    t_temp_oss << "lower_adf_thresh_: " << lower_adf_thresh_ << "\n";
    t_temp_oss << "upper_adf_thresh_: " << upper_adf_thresh_ << "\n";
    t_temp_oss << "ret_compute_duration_mins_: " << ret_compute_duration_mins_ << "\n";
    t_temp_oss << "ret_thresh_: " << ret_thresh_ << "\n";
    t_temp_oss << "zscore_ema_mult_factor_ " << zscore_ema_mult_factor_ << "\n";
    t_temp_oss << "zscore_getflat_threshold_ " << zscore_getflat_threshold_ << "\n";
    return t_temp_oss.str();
  }
};
}
#endif
