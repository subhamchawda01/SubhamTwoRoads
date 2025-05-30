#include <math.h>
#include <stdlib.h>
#include <cmath> // needed for std::round
#include "dvctrade/InitCommon/nse_spread_logic_param.hpp"

#define MIN_NSE_LOTVALUE 500000

namespace MT_SPRD {
ParamSet::ParamSet(const std::string& _paramfilename_)
    : NAV_(10000000),
      risk_factor_(0.15),
      instrument_1_(""),
      instrument_2_(""),
      hedge_factor_(1.0),
      margin_1_(1.0),
      margin_2_(1.0),
      spread_hist_length_(450),
      trade_cooloff_interval_(10),
      max_unit_ratio_(1),
      unit_trade_size_1_(0),
      unit_trade_size_2_(0),
      orig_unit_trade_size_1_(0),
      orig_unit_trade_size_2_(0),
      lotsize_1_(0),
      lotsize_2_(0),
      lotsize_set_externally_(false),
      entry_spread_threshold_(75),  // eg 0.0075 means 75 bps
      initial_entry_spread_threshold_(75),
      roll_reentry_threshold_(75),
      initial_roll_reentry_threshold_(75),
      incremental_entry_threshold_(0),
      minimum_absolute_threshold_(0),
      initial_incremental_entry_threshold_(0),
      entry_exit_differential_(75),
      initial_entry_exit_differential_(75),
      stop_loss_(1000),
      stop_gain_(1000),
      zscore_vec_len_(8000),
      spread_comp_mode_(0),
      asset_comp_mode_(0),
      is_inst1_pass_(false),
      px_thresh_(0.01),
      time_thresh_(10000),  // 10 secs
      pass_ord_mod_time_threshold_(60000),  // 60 secs
      agg_ord_mod_time_threshold_(20000),  // 20 secs
      pass_ord_mod_std_threshold_(0.05),
      agg_ord_mod_std_threshold_(0.05),
      target_vol_(0.15),
      midnight_secs_to_roll_(24300),
      param_id(""),
      inst1_id(-1),
      inst2_id(-1),
      overnight_position_1(0),
      overnight_position_2(0),
      override_on_pos(false),
      get_flat(false),
      lower_hlife_thresh_(15.0),
      upper_hlife_thresh_(35.0),
      lower_adf_thresh_(-3.2),
      upper_adf_thresh_(-2.5),
      ret_compute_duration_mins_(180),
      ret_thresh_(0),
      earnings_getflat_(false),
      zscore_ema_mult_factor_(-1),  // default value zscore getflat check inapplicable
      zscore_getflat_threshold_(100),
      zscore_getflat_(false)

{
  ParseParamFile(_paramfilename_);
}

void ParamSet::ParseParamFile(const std::string& _paramfilename_) {
  std::ifstream paramfile_;
  bool found_roll_reentry_threshold = false;
  paramfile_.open(_paramfilename_.c_str(), std::ifstream::in);
  if (paramfile_.is_open()) {
    const int kParamFileLineBufferLen = 1024;
    char readline_buffer_[kParamFileLineBufferLen];
    bzero(readline_buffer_, kParamFileLineBufferLen);

    while (paramfile_.good()) {
      bzero(readline_buffer_, kParamFileLineBufferLen);
      paramfile_.getline(readline_buffer_, kParamFileLineBufferLen);
      char* param_name_ = strtok(readline_buffer_, " \t\n");
      if (param_name_ == NULL) continue;
      char* param_value_ = strtok(NULL, " \t\n");
      if (strcmp(param_name_, "NAV") == 0) {
        NAV_ = atof(param_value_);
      } else if (strcmp(param_name_, "RISK_FACTOR") == 0) {
        risk_factor_ = atof(param_value_);
      } else if (strcmp(param_name_, "INSTRUMENT_1") == 0) {
        instrument_1_ = param_value_;
      } else if (strcmp(param_name_, "INSTRUMENT_2") == 0) {
        instrument_2_ = param_value_;
      } else if (strcmp(param_name_, "HEDGE_FACTOR") == 0) {
        hedge_factor_ = atof(param_value_);
      } else if (strcmp(param_name_, "MARGIN_1") == 0) {
        margin_1_ = atof(param_value_);
      } else if (strcmp(param_name_, "MARGIN_2") == 0) {
        margin_2_ = atof(param_value_);
      } else if (strcmp(param_name_, "SPREAD_HIST_LENGTH") == 0) {
        spread_hist_length_ = atoi(param_value_);
      } else if (strcmp(param_name_, "TRADE_COOLOFF_INTERVAL") == 0) {
        trade_cooloff_interval_ = atoi(param_value_);
      } else if (strcmp(param_name_, "MAX_UNIT_RATIO") == 0) {
        max_unit_ratio_ = atoi(param_value_);
      } else if (strcmp(param_name_, "ENTRY_SPREAD_THRESHOLD") == 0) {
        entry_spread_threshold_ = atof(param_value_);
        initial_entry_spread_threshold_ = entry_spread_threshold_;
      } else if (strcmp(param_name_, "ROLL_REENTRY_THRESHOLD") == 0) {
        found_roll_reentry_threshold = true;
        roll_reentry_threshold_ = atof(param_value_);
        initial_roll_reentry_threshold_ = roll_reentry_threshold_;
      } else if (strcmp(param_name_, "INCREMENTAL_ENTRY_THRESHOLD") == 0) {
        incremental_entry_threshold_ = atof(param_value_);
        initial_incremental_entry_threshold_ = incremental_entry_threshold_;
      } else if (strcmp(param_name_, "ENTRY_EXIT_DIFFERENTIAL") == 0) {
        entry_exit_differential_ = atof(param_value_);
        initial_entry_exit_differential_ = entry_exit_differential_;
      } else if (strcmp(param_name_, "MINIMUM_ABSOLUTE_THRESHOLD") == 0) {
        minimum_absolute_threshold_ = atof(param_value_);
      } else if (strcmp(param_name_, "STOP_LOSS") == 0) {
        stop_loss_ = atof(param_value_);
      } else if (strcmp(param_name_, "STOP_GAIN") == 0) {
        stop_gain_ = atof(param_value_);
      } else if (strcmp(param_name_, "ZSCORE_VEC_LEN") == 0) {
        zscore_vec_len_ = atoi(param_value_);
      } else if (strcmp(param_name_, "SPREAD_COMP_MODE") == 0) {
        spread_comp_mode_ = atoi(param_value_);
      } else if (strcmp(param_name_, "ASSET_COMP_MODE") == 0) {
        asset_comp_mode_ = atoi(param_value_);
      } else if (strcmp(param_name_, "IS_INST1_PASS") == 0) {
        is_inst1_pass_ = (atoi(param_value_) > 0 ? true : false);
      } else if (strcmp(param_name_, "PX_THRESH") == 0) {
        px_thresh_ = atof(param_value_);
      } else if (strcmp(param_name_, "TIME_THRESH") == 0) {
        time_thresh_ = atoi(param_value_);
      } else if (strcmp(param_name_, "PASS_ORD_MOD_TIME_THRESHOLD") == 0) {
        pass_ord_mod_time_threshold_ = atoi(param_value_);
      } else if (strcmp(param_name_, "AGG_ORD_MOD_TIME_THRESHOLD") == 0) {
        agg_ord_mod_time_threshold_ = atoi(param_value_);
      } else if (strcmp(param_name_, "PASS_ORD_MOD_STD_THRESHOLD") == 0) {
        pass_ord_mod_std_threshold_ = atof(param_value_);
      } else if (strcmp(param_name_, "AGG_ORD_MOD_STD_THRESHOLD") == 0) {
        agg_ord_mod_std_threshold_ = atoi(param_value_);
      } else if (strcmp(param_name_, "TARGET_VOL") == 0) {
        target_vol_ = atof(param_value_);
      } else if (strcmp(param_name_, "MIDNIGHT_SECS_TO_ROLL") == 0) {
        midnight_secs_to_roll_ = atoi(param_value_);
      } else if (strcmp(param_name_, "PARAM_ID") == 0) {
        param_id = param_value_;
      } else if (strcmp(param_name_, "UTS_1") == 0) {
        unit_trade_size_1_ = atoi(param_value_);
        orig_unit_trade_size_1_ = unit_trade_size_1_;
        if (unit_trade_size_1_ > 0 && unit_trade_size_2_ > 0) uts_set_externally_ = true;
      } else if (strcmp(param_name_, "UTS_2") == 0) {
        unit_trade_size_2_ = atoi(param_value_);
        orig_unit_trade_size_2_ = unit_trade_size_2_;
        if (unit_trade_size_1_ > 0 && unit_trade_size_2_ > 0) uts_set_externally_ = true;
      } else if (strcmp(param_name_, "OVERNIGHT_POS1") == 0) {
        overnight_position_1 = atoi(param_value_);
      } else if (strcmp(param_name_, "OVERNIGHT_POS2") == 0) {
        overnight_position_2 = atoi(param_value_);
      } else if (strcmp(param_name_, "OVERRIDE_ON_POS") == 0) {
        override_on_pos = (atoi(param_value_) > 0 ? true : false);
      } else if (strcmp(param_name_, "FLAT") == 0) {
        get_flat = (atoi(param_value_) > 0 ? true : false);
      } else if (strcmp(param_name_, "LOWER_HLIFE_THRESH") == 0) {
        lower_hlife_thresh_ = atof(param_value_);
      } else if (strcmp(param_name_, "UPPER_HLIFE_THRESH") == 0) {
        upper_hlife_thresh_ = atof(param_value_);
      } else if (strcmp(param_name_, "LOWER_ADF_THRESH") == 0) {
        lower_adf_thresh_ = atof(param_value_);
      } else if (strcmp(param_name_, "UPPER_ADF_THRESH") == 0) {
        upper_adf_thresh_ = atof(param_value_);
      } else if (strcmp(param_name_, "RET_COMPUTE_DURATION_MINS") == 0) {
        ret_compute_duration_mins_ = atoi(param_value_);
      } else if (strcmp(param_name_, "RET_THRESH") == 0) {
        ret_thresh_ = atof(param_value_);
      } else if (strcmp(param_name_, "ZSCORE_EMA_MULT_FACTOR") == 0) {
        zscore_ema_mult_factor_ = atof(param_value_);
      } else if (strcmp(param_name_, "ZSCORE_GETFLAT_THRESHOLD") == 0) {
        zscore_getflat_threshold_ = atof(param_value_);
      }
    }
  }
  if (!found_roll_reentry_threshold) {
    roll_reentry_threshold_ = entry_spread_threshold_;
    initial_roll_reentry_threshold_ = initial_entry_spread_threshold_;
  }
}

void ParamSet::MultiplyThreshBy(double fact_) {
  entry_spread_threshold_ = std::max(minimum_absolute_threshold_, initial_entry_spread_threshold_ * fact_);
  roll_reentry_threshold_ = std::max(minimum_absolute_threshold_, initial_roll_reentry_threshold_ * fact_);
  incremental_entry_threshold_ = initial_incremental_entry_threshold_ * fact_;
  entry_exit_differential_ = std::max(minimum_absolute_threshold_, initial_entry_exit_differential_ * fact_);
}

void ParamSet::RecomputeLotSize(double price_1_, double price_2_, double beta_, double voltarget_factor_,
                                double hlife_val_, double adf_val_, double abs_ret_) {
  // Step 1. First get raw uts values without any filters
  RecomputeLotSize(price_1_, price_2_, beta_, voltarget_factor_);
#define NSE_SPARAM_MAX_MULT_FACTOR 1.5
#define NSE_SPARAM_MIN_MULT_FACTOR 0.5
  double t_mult_factor_ = 1.0;
  // Process halflife constraints
  if (hlife_val_ < lower_hlife_thresh_ && hlife_val_ > 1e-5) { 
    t_mult_factor_ *= NSE_SPARAM_MAX_MULT_FACTOR;
  }
  if (hlife_val_ > upper_hlife_thresh_) {
    t_mult_factor_ *= NSE_SPARAM_MIN_MULT_FACTOR;
  }
  // Process adf stat constraints
  if (adf_val_ < lower_adf_thresh_) {
    t_mult_factor_ *= NSE_SPARAM_MAX_MULT_FACTOR;
  }
  if (adf_val_ > upper_adf_thresh_) {
    t_mult_factor_ *= NSE_SPARAM_MIN_MULT_FACTOR;
  }
  // Process ret constraints
  if (abs_ret_ < ret_thresh_) {
    t_mult_factor_ *= NSE_SPARAM_MIN_MULT_FACTOR;
  }

  // floor and cap mult factors
  t_mult_factor_ = std::min(NSE_SPARAM_MAX_MULT_FACTOR, std::max(NSE_SPARAM_MIN_MULT_FACTOR, t_mult_factor_));
#undef NSE_SPARAM_MIN_MULT_FACTOR
#undef NSE_SPARAM_MAX_MULT_FACTOR
  if (!uts_set_externally_) {
    unit_trade_size_1_ = std::round(t_mult_factor_ * unit_trade_size_1_);
    unit_trade_size_2_ = std::round(t_mult_factor_ * unit_trade_size_2_);
  } else {
    unit_trade_size_1_ = std::round(t_mult_factor_ * orig_unit_trade_size_1_);
    unit_trade_size_2_ = std::round(t_mult_factor_ * orig_unit_trade_size_2_);
  }
  //  std::cout << "ADF " << adf_val_ << " HLIFE " << hlife_val_ << " RET " << abs_ret_ << " MF " << t_mult_factor_ << "
  //  UTS1 " << unit_trade_size_1_ << " UTS2 " << unit_trade_size_2_ << '\n';
}

void ParamSet::RecomputeLotSize(double price_1_, double price_2_, double beta_, double voltarget_factor_) {
  //   double t_total_margin_available_ = NAV_*risk_factor_*std::min(1.0,voltarget_factor_);
  double t_total_margin_available_ = NAV_ * risk_factor_;
  if (!lotsize_set_externally_) {
    lotsize_1_ = (int)(MIN_NSE_LOTVALUE / price_1_);
    lotsize_2_ = (int)(MIN_NSE_LOTVALUE / price_2_);
  }
  // uts can be set externally if specified in the param file
  if (!uts_set_externally_) {
    // for spread compmode 0 we use provided hedge factor
    if (spread_comp_mode_ == 0) {
      double t_instrument_1_notional_ = round(t_total_margin_available_ / (margin_1_ + hedge_factor_ * margin_2_));
      unit_trade_size_1_ = (int)(round(t_instrument_1_notional_ / (lotsize_1_ * price_1_ * max_unit_ratio_)));

      double t_instrument_2_notional_ = round(t_instrument_1_notional_ * hedge_factor_);
      unit_trade_size_2_ = (int)(round(t_instrument_2_notional_ / (lotsize_2_ * price_2_ * max_unit_ratio_)));
    }
    // for price stationarity beta is ratio of #shares
    else if (asset_comp_mode_ == 0) {
      double t_instrument_1_shares_ =
          round(t_total_margin_available_ / (max_unit_ratio_ * (margin_1_ * price_1_ + beta_ * margin_2_ * price_2_)));
      unit_trade_size_1_ = (int)(round(t_instrument_1_shares_ / lotsize_1_));

      double t_instrument_2_shares_ = beta_ * t_instrument_1_shares_;
      unit_trade_size_2_ = (int)(round(t_instrument_2_shares_ / lotsize_2_));
    }
    // for logprice stationarity beta is ratio of notional
    else {
      double t_instrument_1_notional_ = round(t_total_margin_available_ / (margin_1_ + beta_ * margin_2_));
      unit_trade_size_1_ = (int)(round(t_instrument_1_notional_ / (lotsize_1_ * price_1_ * max_unit_ratio_)));

      double t_instrument_2_notional_ = round(t_instrument_1_notional_ * beta_);
      unit_trade_size_2_ = (int)(round(t_instrument_2_notional_ / (lotsize_2_ * price_2_ * max_unit_ratio_)));
    }
  }
}
}
