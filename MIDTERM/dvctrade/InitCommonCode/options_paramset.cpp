/**
 \file InitCommonCode/paramset.cpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 353, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551
 */
#include <vector>
#include <stdlib.h>
#include <string.h>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvctrade/InitCommon/strategy_desc.hpp"
#include "dvctrade/InitCommon/options_paramset.hpp"

namespace HFSAT {

// we are starting with thresholds defined in terms of online futures stdev
// we are then looking into further scale across strikes using options stdev/bidaskspread
// how can we decide if bidask spread is better or stdev ?
// NOTE: we are not going to multiple by min price increment, because we are saying expected premium is varying.

// p = premium
// r = risk
// e = execution_support

OptionsParamSet::OptionsParamSet(const std::string &_paramfilename_, const int r_tradingdate_,
                                 std::string dep12_short_code)
    : tradingdate_(r_tradingdate_),
      // premium variables
      worst_case_unit_ratio_(0),
      max_unit_ratio_(0),
      unit_trade_size_(0),
      highpos_limits_unit_ratio_(0),
      highpos_thresh_factor_(0),
      highpos_thresh_decrease_(0),
      highpos_size_factor_(0),
      zeropos_limits_unit_ratio_(0),
      increase_place_(100),
      increase_keep_(100),
      zeropos_place_(100),
      zeropos_keep_(100),
      decrease_place_(100),
      decrease_keep_(100),

      place_keep_diff_(0),
      read_place_keep_diff_(false),
      increase_zeropos_diff_(100),
      read_increase_zeropos_diff_(false),
      zeropos_decrease_diff_(100),
      read_zeropos_decrease_diff_(false),

      improve_(100),
      aggressive_(100),
      fut_stdev_duration_(300),
      scale_by_fut_stdev_(false),
      opt_stdev_duration_(300),
      scale_by_opt_stdev_(false),
      opt_bidaskspread_duration_(300),
      scale_by_opt_bidaskspread_(false),

      fut_place_(100),
      fut_keep_(100),
      fut_place_keep_diff_(0),
      read_fut_place_keep_diff_(false),

      // risk variables
      max_loss_(0),
      global_max_loss_(0),
      max_opentrade_loss_(0),
      break_msecs_on_max_opentrade_loss_(15 * 60 * 1000),
      max_drawdown_(0),
      max_global_delta_(0),
      max_global_gamma_(0),
      max_global_vega_(0),
      max_global_theta_(0),
      delta_hedge_lower_threshold_(0),
      delta_hedge_upper_threshold_(100),
      fractional_second_implied_vol_(300),
      distribute_risk_stats_(0.0),
      delta_hedge_logic_(1),

      // exec variables
      cooloff_interval_(100),
      agg_cooloff_interval_(100),
      aggflat_cooloff_interval_(100),
      improve_cooloff_interval_(100),
      allowed_to_improve_(false),
      allowed_to_aggress_(false),
      max_position_to_aggress_unit_ratio_(0),  // depends on max_position
      max_position_to_aggress_(0),
      max_position_to_improve_unit_ratio_(0),
      max_position_to_improve_(0),
      max_position_to_cancel_unit_ratio_(0),
      max_position_to_cancel_(0),
      max_int_spread_to_place_(1),
      max_int_spread_to_cross_(1),  // depends on bas
      num_non_best_levels_monitored_(0),
      min_size_to_join_(0),
      min_int_price_to_place_(1),
      use_throttle_manager_(false),
      throttle_message_limit_(1000),
      paramfilename_(_paramfilename_),
      staggered_getflat_msecs_vec_(),
      aggressive_getflat_msecs_(-1)

{
  LoadParamSet(dep12_short_code);
}

void OptionsParamSet::LoadParamSet(std::string dep_short_code) {
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

      if ((strcmp(tokens_[0], "PARAMVALUE") == 0) && (tokens_.size() >= 3)) {
        if (strcmp(tokens_[1], "WORST_CASE_UNIT_RATIO") == 0) {
          worst_case_unit_ratio_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "MAX_UNIT_RATIO") == 0) {
          max_unit_ratio_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "UNIT_TRADE_SIZE") == 0) {
          unit_trade_size_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "HIGHPOS_LIMITS_UNIT_RATIO") == 0) {
          highpos_limits_unit_ratio_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "HIGHPOS_THRESH_FACTOR") == 0) {
          highpos_thresh_factor_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "HIGHPOS_THRESH_DECREASE") == 0) {
          highpos_thresh_decrease_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "HIGHPOS_SIZE_FACTOR") == 0) {
          highpos_size_factor_ = std::max(0.0, std::min(1.0, atof(tokens_[2])));
        } else if (strcmp(tokens_[1], "ZEROPOS_LIMITS_UNIT_RATIO") == 0) {
          zeropos_limits_unit_ratio_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "INCREASE_PLACE") == 0) {
          increase_place_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "INCREASE_KEEP") == 0) {
          increase_keep_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "ZEROPOS_PLACE") == 0) {
          zeropos_place_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "ZEROPOS_KEEP") == 0) {
          zeropos_keep_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "DECREASE_PLACE") == 0) {
          decrease_place_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "DECREASE_KEEP") == 0) {
          decrease_keep_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "PLACE_KEEP_DIFF") == 0) {
          place_keep_diff_ = std::max(0.0, atof(tokens_[2]));
          read_place_keep_diff_ = true;
        } else if (strcmp(tokens_[1], "INCREASE_ZEROPOS_DIFF") == 0) {
          increase_zeropos_diff_ = std::max(0.0, atof(tokens_[2]));
          read_increase_zeropos_diff_ = true;
        } else if (strcmp(tokens_[1], "ZEROPOS_DECREASE_DIFF") == 0) {
          zeropos_decrease_diff_ = std::max(0.0, atof(tokens_[2]));
          read_zeropos_decrease_diff_ = true;
        } else if (strcmp(tokens_[1], "MAX_LOSS") == 0) {
          max_loss_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "GLOBAL_MAX_LOSS") == 0) {
          global_max_loss_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "MAX_OPENTRADE_LOSS") == 0) {
          max_opentrade_loss_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "MAX_DRAWDOWN") == 0) {
          max_drawdown_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "COOLOFF_INTERVAL") == 0) {
          cooloff_interval_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "AGG_COOLOFF_INTERVAL") == 0) {
          agg_cooloff_interval_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "AGGFLAT_COOLOFF_INTERVAL") == 0) {
          aggflat_cooloff_interval_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "IMPROVE_COOLOFF_INTERVAL") == 0) {
          improve_cooloff_interval_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "ALLOWED_TO_IMPROVE") == 0) {
          allowed_to_improve_ = (atoi(tokens_[2]) != 0);
        } else if (strcmp(tokens_[1], "ALLOWED_TO_AGGRESS") == 0) {
          allowed_to_aggress_ = (atoi(tokens_[2]) != 0);
        } else if (strcmp(tokens_[1], "IMPROVE") == 0) {
          improve_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "AGGRESSIVE") == 0) {
          aggressive_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "MAX_POSITION_TO_AGGRESS_UNIT_RATIO") == 0) {
          max_position_to_aggress_unit_ratio_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "MAX_POSITION_TO_IMPROVE_UNIT_RATIO") == 0) {
          max_position_to_improve_unit_ratio_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "MAX_INT_SPREAD_TO_PLACE") == 0) {
          max_int_spread_to_place_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "MAX_INT_SPREAD_TO_CROSS") == 0) {
          max_int_spread_to_cross_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "MIN_INT_SPREAD_TO_IMPROVE") == 0) {
          min_int_spread_to_improve_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "NUM_NON_BEST_LEVELS_MONITORED") == 0) {
          num_non_best_levels_monitored_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "MIN_SIZE_TO_JOIN") == 0) {
          min_size_to_join_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "BREAK_MSECS_ON_OPENTRADE_LOSS") == 0) {
          break_msecs_on_max_opentrade_loss_ = std::max(60 * 1000, atoi(tokens_[2]));
        } else if (strcmp(tokens_[1], "MAX_GLOBAL_DELTA") == 0) {
          max_global_delta_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "MAX_GLOBAL_GAMMA") == 0) {
          max_global_gamma_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "MAX_GLOBAL_THETA") == 0) {
          max_global_theta_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "MAX_GLOBAL_VEGA") == 0) {
          max_global_vega_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "DELTA_HEDGE_LOWER_THRESHOLD") == 0) {
          delta_hedge_lower_threshold_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "DELTA_HEDGE_UPPER_THRESHOLD") == 0) {
          delta_hedge_upper_threshold_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "FRACTIONAL_SECOND_IMPLIED_VOL") == 0) {
          fractional_second_implied_vol_ = atof(tokens_[2]);
        } else if (strcmp(tokens_[1], "THROTTLE_MSGS_PER_SEC") == 0) {
          throttle_message_limit_ = atoi(tokens_[2]);
          use_throttle_manager_ = true;
        } else if (strcmp(tokens_[1], "FUT_STDEV_DURATION") == 0) {
          fut_stdev_duration_ = atoi(tokens_[2]);
          scale_by_fut_stdev_ = true;
        } else if (strcmp(tokens_[1], "OPT_STDEV_DURATION") == 0) {
          opt_stdev_duration_ = atoi(tokens_[2]);
          scale_by_opt_stdev_ = true;
        } else if (strcmp(tokens_[1], "OPT_BIDASKSPREAD_DURATION") == 0) {
          opt_bidaskspread_duration_ = atoi(tokens_[2]);
          scale_by_opt_bidaskspread_ = true;
        } else if (strcmp(tokens_[1], "DISTRIBUTE_RISK") == 0) {
          distribute_risk_stats_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "MIN_INT_PRICE_TO_PLACE") == 0) {
          min_int_price_to_place_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "STAGGERED_GETFLAT_TIME") == 0) {
          for (unsigned i = 2; i < tokens_.size(); i++) {
            staggered_getflat_msecs_vec_.push_back(
                HFSAT::DateTime::GetUTCSecondsFromMidnightFromTZHHMM(tradingdate_, atoi(tokens_[i] + 4), tokens_[i]) *
                1000);
          }
        } else if (strcmp(tokens_[1], "AGGRESSIVE_GETFLAT_MSECS") == 0) {
          aggressive_getflat_msecs_ =
              HFSAT::DateTime::GetUTCSecondsFromMidnightFromTZHHMM(tradingdate_, atoi(tokens_[2] + 4), tokens_[2]) *
              1000;
        } else if (strcmp(tokens_[1], "DELTA_HEDGE_LOGIC") == 0) {
          delta_hedge_logic_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "FUT_PLACE") == 0) {
          fut_place_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "FUT_KEEP") == 0) {
          fut_keep_ = atoi(tokens_[2]);
        } else if (strcmp(tokens_[1], "FUT_KEEP") == 0) {
          fut_place_keep_diff_ = atoi(tokens_[2]);
        }
      }
    }
    paramfile_.close();
  }
  ReconcileParams(dep_short_code);
  if (!VerifySanctity()) {
    ExitVerbose(kExitErrorCodeGeneral, "Params Sanctity Check Failed !!");
  }
}

void OptionsParamSet::WriteSendStruct(ParamSetSendStruct &retval) const {
  retval.worst_case_position_ = worst_case_unit_ratio_ * unit_trade_size_;
  retval.max_position_ = max_unit_ratio_ * unit_trade_size_;
  retval.unit_trade_size_ = unit_trade_size_;

  retval.highpos_limits_ = highpos_limits_unit_ratio_ * unit_trade_size_;
  retval.highpos_thresh_factor_ = highpos_thresh_factor_;
  retval.highpos_thresh_decrease_ = highpos_thresh_decrease_;
  retval.highpos_size_factor_ = highpos_size_factor_;
  retval.increase_place_ = increase_place_;
  retval.increase_keep_ = increase_keep_;
  retval.zeropos_limits_ = zeropos_limits_unit_ratio_ * unit_trade_size_;
  retval.zeropos_place_ = zeropos_place_;
  retval.zeropos_keep_ = zeropos_keep_;
  retval.decrease_place_ = decrease_place_;
  retval.decrease_keep_ = decrease_keep_;

  retval.place_keep_diff_ = place_keep_diff_;
  retval.increase_zeropos_diff_ = increase_zeropos_diff_;
  retval.zeropos_decrease_diff_ = zeropos_decrease_diff_;

  retval.allowed_to_improve_ = allowed_to_improve_;
  retval.allowed_to_aggress_ = allowed_to_aggress_;
  retval.improve_ = improve_;
  retval.aggressive_ = aggressive_;

  retval.max_loss_ = max_loss_;
  retval.global_max_loss_ = global_max_loss_;
  retval.max_opentrade_loss_ = max_opentrade_loss_;
  retval.max_drawdown_ = max_drawdown_;

  retval.cooloff_interval_ = cooloff_interval_;
  retval.agg_cooloff_interval_ = agg_cooloff_interval_;

  retval.use_throttle_manager_ = use_throttle_manager_;
  retval.throttle_message_limit_ = throttle_message_limit_;
}

bool OptionsParamSet::VerifySanctity() {
  if (unit_trade_size_ > 0 && max_unit_ratio_ >= highpos_limits_unit_ratio_ &&
      highpos_limits_unit_ratio_ >= zeropos_limits_unit_ratio_ && increase_place_ >= increase_keep_ &&
      decrease_place_ >= decrease_keep_ && zeropos_place_ >= zeropos_keep_ && increase_place_ >= zeropos_place_ &&
      zeropos_place_ >= decrease_place_ && max_loss_ > 0 && global_max_loss_ > 0 && max_opentrade_loss_ > 0) {
    return true;
  } else {
    return false;
  }
}

void OptionsParamSet::ReconcileParams(std::string dep_short_code) {
  int t_min_lotsize_ = HFSAT::SecurityDefinitions::GetContractMinOrderSize(dep_short_code, tradingdate_);

  // UTS is in terms of lot numbers
  unit_trade_size_ *= t_min_lotsize_;
  delta_hedge_lower_threshold_ *= unit_trade_size_;
  delta_hedge_upper_threshold_ *= unit_trade_size_;

  worst_case_position_ = unit_trade_size_ * worst_case_unit_ratio_;
  max_position_ = unit_trade_size_ * max_unit_ratio_;
  max_position_to_aggress_ = unit_trade_size_ * max_position_to_aggress_unit_ratio_;
  max_position_to_improve_ = unit_trade_size_ * max_position_to_improve_unit_ratio_;
  max_position_to_cancel_ = unit_trade_size_ * max_position_to_cancel_unit_ratio_;

  // from A to B
  if (read_place_keep_diff_) {
    zeropos_place_ = zeropos_keep_ + place_keep_diff_;
  }

  if (read_fut_place_keep_diff_) {
    fut_place_ = fut_keep_ + fut_place_keep_diff_;
  }

  if (read_increase_zeropos_diff_) {
    increase_keep_ = zeropos_keep_ + increase_zeropos_diff_;
    increase_place_ = zeropos_place_ + increase_zeropos_diff_;
  }

  if (read_zeropos_decrease_diff_) {
    decrease_keep_ = zeropos_keep_ - zeropos_decrease_diff_;
    decrease_place_ = zeropos_place_ - zeropos_decrease_diff_;
  }
}
}
