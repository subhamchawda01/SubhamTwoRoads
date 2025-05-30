/**
  \file Tools/print_paramset.cpp

  \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite 217, Level 2, Prestige Omega,
No 104, EPIP Zone, Whitefield,
Bangalore - 560066
India
+91 80 4060 0717
*/
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "dvccode/CDef/math_utils.hpp"
#include "dvccode/CDef/error_utils.hpp"

#include "dvccode/CDef/security_definitions.hpp"
#include "dvctrade/ExecLogic/trade_vars.hpp"

#include "dvctrade/InitCommon/paramset.hpp"

#define MAX_POS_MAP_SIZE 32

void CheckParamSet(HFSAT::ParamSet& r_param_set_);
void BuildConstantTradeVarSets(HFSAT::ParamSet& r_param_set_);
void BuildPositionTradeVarSetMap(HFSAT::ParamSet& r_param_set_);

// mapping position to tradevarset
// 0 position corresponds to index MAX_POS_MAP_SIZE,
int map_pos_increment_;  // = (int)std::max ( 1, ( param_set_.max_position_ / MAX_POS_MAP_SIZE ) ) )
// idx=2*MAX_POS_MAP_SIZE corresponds to position = ( MAX_POS_MAP_SIZE * map_pos_increment_ )
// idx=0 corresponds to position = ( MAX_POS_MAP_SIZE * map_pos_increment_ )
// position P corresponds to idx= MAX_POS_MAP_SIZE + ( P / map_pos_increment_ )
HFSAT::PositionTradeVarSetMap position_tradevarset_map_;
const unsigned int P2TV_zero_idx_ = MAX_POS_MAP_SIZE;

HFSAT::TradeVars_t closeout_zeropos_tradevarset_;
HFSAT::TradeVars_t closeout_long_tradevarset_;
HFSAT::TradeVars_t closeout_short_tradevarset_;

HFSAT::TradeVars_t current_tradevarset_;
unsigned int current_position_tradevarset_map_index_;

double min_price_increment_ = 0.0;
int min_order_size_ = 1;

int main(int argc, char** argv) {
  if (argc < 5) {
    std::cerr << " USAGE: "
              << " $EXEC param_file_ YYYYMMDD min_price_increment_ shortcode_ [min_order_size_=1]" << std::endl;
    return 0;
  }

  std::string param_file_name_ = argv[1];
  unsigned int yyyymmdd_ = atol(argv[2]);
  min_price_increment_ = atof(argv[3]);
  std::string shortcode_ = argv[4];
  if (argc > 5) {
    min_order_size_ = atoi(argv[5]);
  }

  if (strncmp(argv[4], "NSE_", 4) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(yyyymmdd_).LoadNSESecurityDefinitions();
  }

  HFSAT::ParamSet param_set_(param_file_name_, yyyymmdd_, shortcode_);
  param_set_.ReconcileParams(shortcode_);

  CheckParamSet(param_set_);

  BuildConstantTradeVarSets(param_set_);
  BuildPositionTradeVarSetMap(param_set_);

  return 0;
}

void CheckParamSet(HFSAT::ParamSet& param_set_) {
  if (!param_set_.read_worst_case_position_) {
    HFSAT::ExitVerbose(HFSAT::kStrategyDescParamFileIncomplete, "worst_case_position_");
  }
  if (!param_set_.read_max_position_) {
    HFSAT::ExitVerbose(HFSAT::kStrategyDescParamFileIncomplete, "max_position_");
  }
  if (!param_set_.read_unit_trade_size_) {
    HFSAT::ExitVerbose(HFSAT::kStrategyDescParamFileIncomplete, "unit_trade_size_");
  }
  if (!param_set_.read_highpos_limits_ && !param_set_.read_highpos_limits_unit_ratio_) {
    HFSAT::ExitVerbose(HFSAT::kStrategyDescParamFileIncomplete, "highpos_limits_ | highpos_limits_unit_ratio_");
  }
  if (!param_set_.read_highpos_thresh_factor_) {
    HFSAT::ExitVerbose(HFSAT::kStrategyDescParamFileIncomplete, "highpos_thresh_factor_");
  }
  if (!param_set_.read_highpos_size_factor_) {
    HFSAT::ExitVerbose(HFSAT::kStrategyDescParamFileIncomplete, "size_factor_");
  }
  if (!param_set_.read_increase_place_) {
    HFSAT::ExitVerbose(HFSAT::kStrategyDescParamFileIncomplete, "increase_place_");
  }
  if (!param_set_.read_increase_keep_) {
    HFSAT::ExitVerbose(HFSAT::kStrategyDescParamFileIncomplete, "increase_keep_");
  }
  if (!param_set_.read_zeropos_limits_ && !param_set_.read_zeropos_limits_unit_ratio_) {
    HFSAT::ExitVerbose(HFSAT::kStrategyDescParamFileIncomplete, "zeropos_limits_ | zeropos_limits_unit_ratio_");
  }
  if (!param_set_.read_zeropos_place_) {
    HFSAT::ExitVerbose(HFSAT::kStrategyDescParamFileIncomplete, "zeropos_place_");
  }
  if (!param_set_.read_zeropos_keep_) {
    HFSAT::ExitVerbose(HFSAT::kStrategyDescParamFileIncomplete, "zeropos_keep_");
  }
  if (!param_set_.read_decrease_place_) {
    HFSAT::ExitVerbose(HFSAT::kStrategyDescParamFileIncomplete, "decrease_place_");
  }
  if (!param_set_.read_decrease_keep_) {
    HFSAT::ExitVerbose(HFSAT::kStrategyDescParamFileIncomplete, "decrease_keep_");
  }
  if (!param_set_.read_max_loss_) {
    HFSAT::ExitVerbose(HFSAT::kStrategyDescParamFileIncomplete, "max_loss_");
  }
  if (!param_set_.read_max_opentrade_loss_) {
    HFSAT::ExitVerbose(HFSAT::kStrategyDescParamFileIncomplete, "max_opentrade_loss_");
  }
}

/// currently during closeout improve and aggressive are disallowed
void BuildConstantTradeVarSets(HFSAT::ParamSet& param_set_) {
  closeout_zeropos_tradevarset_ =
      HFSAT::TradeVars_t(HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
                         HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
                         HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, 0, 0);

  closeout_long_tradevarset_ =
      HFSAT::TradeVars_t(HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, 0, 0, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
                         HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, 0,
                         HFSAT::MathUtils::GetFlooredMultipleOf(param_set_.unit_trade_size_, min_order_size_));

  closeout_short_tradevarset_ =
      HFSAT::TradeVars_t(0, 0, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
                         HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE, HIGH_THRESHOLD_VALUE,
                         HFSAT::MathUtils::GetFlooredMultipleOf(param_set_.unit_trade_size_, min_order_size_), 0);
}

void BuildPositionTradeVarSetMap(HFSAT::ParamSet& param_set_) {
  const int original_max_position_ = param_set_.max_position_;

  if (param_set_.max_position_ < param_set_.unit_trade_size_) {  // True only for MUR < 1.0
    param_set_.max_position_ = param_set_.unit_trade_size_;      // Build the tradevarset assuming MUR = 1
  }

  // boundary of what is considered zero_position
  double zeropos_limit_position_ = 0;

  if (param_set_.read_zeropos_limits_) {  // zeropos limits were specified as absolute positions
    zeropos_limit_position_ = (param_set_.zeropos_limits_ >= param_set_.unit_trade_size_
                                   ? (param_set_.zeropos_limits_)
                                   : (param_set_.zeropos_limits_ * (double)param_set_.unit_trade_size_));
  } else {  // zeropos limits were specified as a factor of the unit-trade-size
    zeropos_limit_position_ = (param_set_.zeropos_limits_unit_ratio_ * (double)param_set_.unit_trade_size_);
  }

  // boundary of what is considered high_position
  double highpos_limit_position_ = 0;

  if (param_set_.read_highpos_limits_) {  // highpos limits were specified as absolute positions
    highpos_limit_position_ = (param_set_.highpos_limits_ >= param_set_.unit_trade_size_
                                   ? (param_set_.highpos_limits_)
                                   : (param_set_.highpos_limits_ * (double)param_set_.unit_trade_size_));
  } else {  // highpos limits were specified as a factor of the unit-trade-size
    highpos_limit_position_ = (param_set_.highpos_limits_unit_ratio_ * (double)param_set_.unit_trade_size_);
  }

  // keeping increase size unchanged in high_position mode
  // just increasing the decrease trade size to get away from high position faster
  int highpos_adjusted_increase_trade_size_ =
      HFSAT::MathUtils::GetFlooredMultipleOf(param_set_.unit_trade_size_, min_order_size_);
  // int highpos_adjusted_increase_trade_size_ = ( int ) round ( ( double ) param_set_.unit_trade_size_ / ( 1 +
  // param_set_.highpos_size_factor_ ) ) ; // keeping increase size unchanged in high_position mode
  int highpos_adjusted_decrease_trade_size_ = HFSAT::MathUtils::GetFlooredMultipleOf(
      (int)round((double)param_set_.unit_trade_size_ * (1 + param_set_.highpos_size_factor_)), min_order_size_);

  // upto param_set_.max_position_ = MAX_POS_MAP_SIZE , keep a map_pos_increment_ = 1,
  // then onwards increase to keep the maximum map size to ( 2 * MAX_POS_MAP_SIZE + 1 )
  // for _this_position_ position_tradevarset_map_ [ P2TV_zero_idx_ + round ( _this_position_ / map_pos_increment_ ) ]
  // is the applicable threshold set
  map_pos_increment_ = std::max(1, (int)ceil((double)param_set_.max_position_ / (double)MAX_POS_MAP_SIZE));
  position_tradevarset_map_.resize(2 * MAX_POS_MAP_SIZE + 1);

  // for zeropos
  position_tradevarset_map_[P2TV_zero_idx_].Assign(
      param_set_.zeropos_place_, param_set_.zeropos_keep_, param_set_.zeropos_place_, param_set_.zeropos_keep_,
      param_set_.zeropos_place_ + param_set_.improve_, param_set_.zeropos_keep_ + param_set_.improve_,
      param_set_.zeropos_place_ + param_set_.aggressive_, param_set_.zeropos_place_ + param_set_.improve_,
      param_set_.zeropos_keep_ + param_set_.improve_, param_set_.zeropos_place_ + param_set_.aggressive_,
      HFSAT::MathUtils::GetFlooredMultipleOf(
          std::max(0, std::min(param_set_.max_position_, param_set_.unit_trade_size_)), min_order_size_),
      HFSAT::MathUtils::GetFlooredMultipleOf(
          std::max(0, std::min(param_set_.max_position_, param_set_.unit_trade_size_)), min_order_size_));

  position_tradevarset_map_[P2TV_zero_idx_].MultiplyBy(min_price_increment_);
  std::cout << "Position " << 0 << " mapidx " << P2TV_zero_idx_ << ' '
            << HFSAT::ToString(position_tradevarset_map_[P2TV_zero_idx_]).c_str() << std::endl;

  const double very_high_barrier_ = 100;
  const double override_signal_ = 1.0;

  // for positive position values
  for (unsigned int i = (P2TV_zero_idx_ + 1); i < position_tradevarset_map_.size(); i++) {
    int for_position_ = (i - P2TV_zero_idx_) * map_pos_increment_;

    if (for_position_ <= zeropos_limit_position_) {  // [ 0, zeropos ]

      double _zeropos_fraction_ = ((double)for_position_ / (double)zeropos_limit_position_);

      position_tradevarset_map_[i].Assign(
          (param_set_.zeropos_place_ + ((param_set_.increase_place_ - param_set_.zeropos_place_) * _zeropos_fraction_)),
          (param_set_.zeropos_keep_ + ((param_set_.increase_keep_ - param_set_.zeropos_keep_) * _zeropos_fraction_)),
          (param_set_.zeropos_place_ + ((param_set_.decrease_place_ - param_set_.zeropos_place_) * _zeropos_fraction_)),
          (param_set_.zeropos_keep_ + ((param_set_.decrease_keep_ - param_set_.zeropos_keep_) * _zeropos_fraction_)),
          (param_set_.zeropos_place_ +
           ((param_set_.increase_place_ - param_set_.zeropos_place_) * _zeropos_fraction_)) +
              param_set_.improve_,
          (param_set_.zeropos_keep_ + ((param_set_.increase_keep_ - param_set_.zeropos_keep_) * _zeropos_fraction_)) +
              param_set_.improve_,
          (param_set_.zeropos_place_ +
           ((param_set_.increase_place_ - param_set_.zeropos_place_) * _zeropos_fraction_)) +
              param_set_.aggressive_,
          (param_set_.zeropos_place_ +
           ((param_set_.decrease_place_ - param_set_.zeropos_place_) * _zeropos_fraction_)) +
              param_set_.improve_,
          (param_set_.zeropos_keep_ + ((param_set_.decrease_keep_ - param_set_.zeropos_keep_) * _zeropos_fraction_)) +
              param_set_.improve_,
          (param_set_.zeropos_place_ +
           ((param_set_.decrease_place_ - param_set_.zeropos_place_) * _zeropos_fraction_)) +
              param_set_.aggressive_,
          HFSAT::MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param_set_.max_position_ - for_position_), param_set_.unit_trade_size_)),
              min_order_size_),
          HFSAT::MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param_set_.max_position_ + for_position_), param_set_.unit_trade_size_)),
              min_order_size_));

    } else if (for_position_ <= highpos_limit_position_) {  // ( zeropos, highpos ]
      position_tradevarset_map_[i].Assign(
          param_set_.increase_place_, param_set_.increase_keep_, param_set_.decrease_place_, param_set_.decrease_keep_,
          param_set_.increase_place_ + param_set_.improve_, param_set_.increase_keep_ + param_set_.improve_,
          param_set_.increase_place_ + param_set_.aggressive_, param_set_.decrease_place_ + param_set_.improve_,
          param_set_.decrease_keep_ + param_set_.improve_, param_set_.decrease_place_ + param_set_.aggressive_,
          HFSAT::MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param_set_.max_position_ - for_position_), param_set_.unit_trade_size_)),
              min_order_size_),
          HFSAT::MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param_set_.max_position_ + for_position_), param_set_.unit_trade_size_)),
              min_order_size_));

    } else if (for_position_ < param_set_.max_position_) {  // ( highpos, maxpos ]
      double _super_highpos_fraction_ = ((double)for_position_ / (double)highpos_limit_position_);

      // high_position
      double highpos_adjusted_increase_place_ =
          param_set_.increase_place_ +
          (_super_highpos_fraction_ *
           param_set_
               .highpos_thresh_factor_);  // increasing place-threshold of increasing position on high_position mode
      double highpos_adjusted_increase_keep_ =
          param_set_.increase_keep_ +
          (_super_highpos_fraction_ *
           param_set_
               .highpos_thresh_factor_);  // increasing keep-threshold of increasing position on high_position mode
      double highpos_adjusted_decrease_place_ =
          param_set_.decrease_place_ -
          (_super_highpos_fraction_ *
           param_set_
               .highpos_thresh_decrease_);  // decreasing place-threshold of reducing position on high_position mode
      double highpos_adjusted_decrease_keep_ =
          param_set_.decrease_keep_ -
          (_super_highpos_fraction_ *
           param_set_
               .highpos_thresh_decrease_);  // decreasing keep-threshold of reducing position on high_position mode

      position_tradevarset_map_[i].Assign(
          highpos_adjusted_increase_place_, highpos_adjusted_increase_keep_, highpos_adjusted_decrease_place_,
          highpos_adjusted_decrease_keep_, highpos_adjusted_increase_place_ + param_set_.improve_ + very_high_barrier_,
          highpos_adjusted_increase_keep_ + param_set_.improve_ + very_high_barrier_,
          highpos_adjusted_increase_place_ + param_set_.aggressive_ + very_high_barrier_,
          highpos_adjusted_decrease_place_ + param_set_.improve_, highpos_adjusted_decrease_keep_ + param_set_.improve_,
          highpos_adjusted_decrease_place_ + param_set_.aggressive_,
          HFSAT::MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param_set_.max_position_ - for_position_), highpos_adjusted_increase_trade_size_)),
              min_order_size_),
          HFSAT::MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param_set_.max_position_ + for_position_), highpos_adjusted_decrease_trade_size_)),
              min_order_size_));

    } else if (for_position_ == param_set_.max_position_) {  // [ maxpos, maxpos ]
      double _super_highpos_fraction_ = ((double)for_position_ / (double)highpos_limit_position_);

      // high_position
      double highpos_adjusted_increase_place_ =
          param_set_.increase_place_ +
          (_super_highpos_fraction_ *
           param_set_
               .highpos_thresh_factor_);  // increasing place-threshold of increasing position on high_position mode
      double highpos_adjusted_increase_keep_ =
          param_set_.increase_keep_ +
          (_super_highpos_fraction_ *
           param_set_
               .highpos_thresh_factor_);  // increasing keep-threshold of increasing position on high_position mode
      double highpos_adjusted_decrease_place_ =
          param_set_.decrease_place_ -
          (_super_highpos_fraction_ *
           param_set_
               .highpos_thresh_decrease_);  // decreasing place-threshold of reducing position on high_position mode
      double highpos_adjusted_decrease_keep_ =
          param_set_.decrease_keep_ -
          (_super_highpos_fraction_ *
           param_set_
               .highpos_thresh_decrease_);  // decreasing keep-threshold of reducing position on high_position mode

      position_tradevarset_map_[i].Assign(
          highpos_adjusted_increase_place_ + very_high_barrier_, highpos_adjusted_increase_keep_ + very_high_barrier_,
          highpos_adjusted_decrease_place_, highpos_adjusted_decrease_keep_,
          highpos_adjusted_increase_place_ + param_set_.improve_ + very_high_barrier_,
          highpos_adjusted_increase_keep_ + param_set_.improve_ + very_high_barrier_,
          highpos_adjusted_increase_place_ + param_set_.aggressive_ + very_high_barrier_,
          highpos_adjusted_decrease_place_ + param_set_.improve_, highpos_adjusted_decrease_keep_ + param_set_.improve_,
          highpos_adjusted_decrease_place_ + param_set_.aggressive_, 0,
          HFSAT::MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param_set_.max_position_ + for_position_), highpos_adjusted_decrease_trade_size_)),
              min_order_size_));

    } else {  // ( maxpos, inf )
      double _super_highpos_fraction_ = ((double)for_position_ / (double)highpos_limit_position_);

      // high_position
      double highpos_adjusted_increase_place_ =
          param_set_.increase_place_ +
          (_super_highpos_fraction_ *
           param_set_
               .highpos_thresh_factor_);  // increasing place-threshold of increasing position on high_position mode
      double highpos_adjusted_increase_keep_ =
          param_set_.increase_keep_ +
          (_super_highpos_fraction_ *
           param_set_
               .highpos_thresh_factor_);  // increasing keep-threshold of increasing position on high_position mode
      double highpos_adjusted_decrease_place_ =
          param_set_.decrease_place_ -
          (_super_highpos_fraction_ *
           param_set_
               .highpos_thresh_decrease_);  // decreasing place-threshold of reducing position on high_position mode
      double highpos_adjusted_decrease_keep_ =
          param_set_.decrease_keep_ -
          (_super_highpos_fraction_ *
           param_set_
               .highpos_thresh_decrease_);  // decreasing keep-threshold of reducing position on high_position mode

      position_tradevarset_map_[i].Assign(
          highpos_adjusted_increase_place_ + very_high_barrier_, highpos_adjusted_increase_keep_ + very_high_barrier_,
          highpos_adjusted_decrease_place_ - override_signal_, highpos_adjusted_decrease_keep_ - override_signal_,
          highpos_adjusted_increase_place_ + param_set_.improve_ + very_high_barrier_,
          highpos_adjusted_increase_keep_ + param_set_.improve_ + very_high_barrier_,
          highpos_adjusted_increase_place_ + param_set_.aggressive_ + very_high_barrier_,
          highpos_adjusted_decrease_place_ + param_set_.improve_ +
              very_high_barrier_,  // disable agg decrease at these position levels since decrease place could be very
                                   // low, we are clearly here by accident
          highpos_adjusted_decrease_keep_ + param_set_.improve_ + very_high_barrier_,
          highpos_adjusted_decrease_place_ + param_set_.aggressive_ + very_high_barrier_, 0,
          HFSAT::MathUtils::GetFlooredMultipleOf(
              std::max(0, std::min((param_set_.max_position_ + for_position_), highpos_adjusted_decrease_trade_size_)),
              min_order_size_));
    }

    if (original_max_position_ != param_set_.max_position_) {  // True only for fractional MUR < 1.0
      if (for_position_ >=
          original_max_position_) {  // For long position beyond the original MUR * UTS , set bid-place and keep thresh.
        position_tradevarset_map_[i].MultiplyBidsBy(100.0);
      }
    }

    if (for_position_ <= param_set_.max_position_) {
      position_tradevarset_map_[i].MultiplyBy(min_price_increment_);
      std::cout << "Position " << for_position_ << " mapidx " << i << ' '
                << HFSAT::ToString(position_tradevarset_map_[i]).c_str() << std::endl;
    }
  }

  // for negative position values
  for (int i = (P2TV_zero_idx_ - 1); i >= 0; i--) {
    int for_position_ = (i - P2TV_zero_idx_) * map_pos_increment_;

    bool set_explicitly_ = false;
    if (param_set_.read_explicit_max_short_position_) {
      if (for_position_ == -param_set_.explicit_max_short_position_) {
        // == -explicit_max_short_position_ then no sell
        position_tradevarset_map_[i].Assign(
            param_set_.decrease_place_, param_set_.decrease_keep_, param_set_.increase_place_ + very_high_barrier_,
            param_set_.increase_keep_ + very_high_barrier_, param_set_.decrease_place_ + param_set_.improve_,
            param_set_.decrease_keep_ + param_set_.improve_, param_set_.decrease_place_ + param_set_.aggressive_,
            param_set_.increase_place_ + param_set_.improve_ + very_high_barrier_,
            param_set_.increase_keep_ + param_set_.improve_ + very_high_barrier_,
            param_set_.increase_place_ + param_set_.aggressive_ + very_high_barrier_,
            HFSAT::MathUtils::GetFlooredMultipleOf(
                std::max(0, std::min((param_set_.max_position_ - for_position_), param_set_.unit_trade_size_)),
                min_order_size_),
            0);

        set_explicitly_ = true;
      } else if (for_position_ < -param_set_.explicit_max_short_position_ + param_set_.unit_trade_size_) {
        // <= -explicit_max_short_position_ + unit_trade_size_ ... then sell only at most what is allowed

        position_tradevarset_map_[i].Assign(
            param_set_.decrease_place_, param_set_.decrease_keep_, param_set_.increase_place_,
            param_set_.increase_keep_, param_set_.decrease_place_ + param_set_.improve_,
            param_set_.decrease_keep_ + param_set_.improve_, param_set_.decrease_place_ + param_set_.aggressive_,
            param_set_.increase_place_ + param_set_.improve_ + very_high_barrier_,
            param_set_.increase_keep_ + param_set_.improve_ + very_high_barrier_,
            param_set_.increase_place_ + param_set_.aggressive_ + very_high_barrier_,
            HFSAT::MathUtils::GetFlooredMultipleOf(
                std::max(0, std::min((param_set_.max_position_ - for_position_), param_set_.unit_trade_size_)),
                min_order_size_),
            HFSAT::MathUtils::GetFlooredMultipleOf(
                std::max(0, std::min((param_set_.max_position_ + for_position_), param_set_.unit_trade_size_)),
                min_order_size_));

        set_explicitly_ = true;
      } else if (for_position_ <
                 -param_set_.explicit_max_short_position_) {  // ( -inf, - param_set_.explicit_max_short_position_ )

        position_tradevarset_map_[i].Assign(
            param_set_.decrease_place_, param_set_.decrease_keep_, param_set_.increase_place_ + very_high_barrier_,
            param_set_.increase_keep_ + very_high_barrier_,
            param_set_.decrease_place_ + param_set_.improve_ + very_high_barrier_,
            param_set_.decrease_keep_ + param_set_.improve_ + very_high_barrier_,
            param_set_.decrease_place_ + param_set_.aggressive_ + very_high_barrier_,
            param_set_.increase_place_ + param_set_.improve_ + very_high_barrier_,
            param_set_.increase_keep_ + param_set_.improve_ + very_high_barrier_,
            param_set_.increase_place_ + param_set_.aggressive_ + very_high_barrier_,
            HFSAT::MathUtils::GetFlooredMultipleOf(
                std::max(0, std::min((param_set_.max_position_ - for_position_), param_set_.unit_trade_size_)),
                min_order_size_),
            0);
        set_explicitly_ = true;
      }
    }

    if (!set_explicitly_) {
      if (for_position_ >= -zeropos_limit_position_) {  // [ -zeropos, 0 ]

        double _zeropos_fraction_ = ((double)-for_position_ / (double)zeropos_limit_position_);
        position_tradevarset_map_[i].Assign(
            (param_set_.zeropos_place_ +
             ((param_set_.decrease_place_ - param_set_.zeropos_place_) * _zeropos_fraction_)),
            (param_set_.zeropos_keep_ + ((param_set_.decrease_keep_ - param_set_.zeropos_keep_) * _zeropos_fraction_)),
            (param_set_.zeropos_place_ +
             ((param_set_.increase_place_ - param_set_.zeropos_place_) * _zeropos_fraction_)),
            (param_set_.zeropos_keep_ + ((param_set_.increase_keep_ - param_set_.zeropos_keep_) * _zeropos_fraction_)),
            (param_set_.zeropos_place_ +
             ((param_set_.decrease_place_ - param_set_.zeropos_place_) * _zeropos_fraction_)) +
                param_set_.improve_,
            (param_set_.zeropos_keep_ + ((param_set_.decrease_keep_ - param_set_.zeropos_keep_) * _zeropos_fraction_)) +
                param_set_.improve_,
            (param_set_.zeropos_place_ +
             ((param_set_.decrease_place_ - param_set_.zeropos_place_) * _zeropos_fraction_)) +
                param_set_.aggressive_,
            (param_set_.zeropos_place_ +
             ((param_set_.increase_place_ - param_set_.zeropos_place_) * _zeropos_fraction_)) +
                param_set_.improve_,
            (param_set_.zeropos_keep_ + ((param_set_.increase_keep_ - param_set_.zeropos_keep_) * _zeropos_fraction_)) +
                param_set_.improve_,
            (param_set_.zeropos_place_ +
             ((param_set_.increase_place_ - param_set_.zeropos_place_) * _zeropos_fraction_)) +
                param_set_.aggressive_,
            HFSAT::MathUtils::GetFlooredMultipleOf(
                std::max(0, std::min((param_set_.max_position_ - for_position_), param_set_.unit_trade_size_)),
                min_order_size_),
            HFSAT::MathUtils::GetFlooredMultipleOf(
                std::max(0, std::min((param_set_.max_position_ + for_position_), param_set_.unit_trade_size_)),
                min_order_size_));

      } else if (for_position_ >= -highpos_limit_position_) {  // [ -highpos, -zeropos )

        position_tradevarset_map_[i].Assign(
            param_set_.decrease_place_, param_set_.decrease_keep_, param_set_.increase_place_,
            param_set_.increase_keep_, param_set_.decrease_place_ + param_set_.improve_,
            param_set_.decrease_keep_ + param_set_.improve_, param_set_.decrease_place_ + param_set_.aggressive_,
            param_set_.increase_place_ + param_set_.improve_, param_set_.increase_keep_ + param_set_.improve_,
            param_set_.increase_place_ + param_set_.aggressive_,
            HFSAT::MathUtils::GetFlooredMultipleOf(
                std::max(0, std::min((param_set_.max_position_ - for_position_), param_set_.unit_trade_size_)),
                min_order_size_),
            HFSAT::MathUtils::GetFlooredMultipleOf(
                std::max(0, std::min((param_set_.max_position_ + for_position_), param_set_.unit_trade_size_)),
                min_order_size_));

      } else if (for_position_ > -param_set_.max_position_) {  // ( -maxpos, -highpos )

        double _super_highpos_fraction_ = ((double)-for_position_ / (double)highpos_limit_position_);

        // high_position
        double highpos_adjusted_increase_place_ =
            param_set_.increase_place_ +
            (_super_highpos_fraction_ *
             param_set_
                 .highpos_thresh_factor_);  // increasing place-threshold of increasing position on high_position mode
        double highpos_adjusted_increase_keep_ =
            param_set_.increase_keep_ +
            (_super_highpos_fraction_ *
             param_set_
                 .highpos_thresh_factor_);  // increasing keep-threshold of increasing position on high_position mode
        double highpos_adjusted_decrease_place_ =
            param_set_.decrease_place_ -
            (_super_highpos_fraction_ *
             param_set_
                 .highpos_thresh_decrease_);  // decreasing place-threshold of reducing position on high_position mode
        double highpos_adjusted_decrease_keep_ =
            param_set_.decrease_keep_ -
            (_super_highpos_fraction_ *
             param_set_
                 .highpos_thresh_decrease_);  // decreasing keep-threshold of reducing position on high_position mode

        position_tradevarset_map_[i].Assign(
            highpos_adjusted_decrease_place_, highpos_adjusted_decrease_keep_, highpos_adjusted_increase_place_,
            highpos_adjusted_increase_keep_, highpos_adjusted_decrease_place_ + param_set_.improve_,
            highpos_adjusted_decrease_keep_ + param_set_.improve_,
            highpos_adjusted_decrease_place_ + param_set_.aggressive_,
            highpos_adjusted_increase_place_ + param_set_.improve_ + very_high_barrier_,
            highpos_adjusted_increase_keep_ + param_set_.improve_ + very_high_barrier_,
            highpos_adjusted_increase_place_ + param_set_.aggressive_ + very_high_barrier_,
            HFSAT::MathUtils::GetFlooredMultipleOf(std::max(0, std::min((param_set_.max_position_ - for_position_),
                                                                        highpos_adjusted_decrease_trade_size_)),
                                                   min_order_size_),
            HFSAT::MathUtils::GetFlooredMultipleOf(std::max(0, std::min((param_set_.max_position_ + for_position_),
                                                                        highpos_adjusted_increase_trade_size_)),
                                                   min_order_size_));

      } else if (for_position_ == -param_set_.max_position_) {  // [ -maxpos, -maxpos ]

        double _super_highpos_fraction_ = ((double)-for_position_ / (double)highpos_limit_position_);

        // high_position
        double highpos_adjusted_increase_place_ =
            param_set_.increase_place_ +
            (_super_highpos_fraction_ *
             param_set_
                 .highpos_thresh_factor_);  // increasing place-threshold of increasing position on high_position mode
        double highpos_adjusted_increase_keep_ =
            param_set_.increase_keep_ +
            (_super_highpos_fraction_ *
             param_set_
                 .highpos_thresh_factor_);  // increasing keep-threshold of increasing position on high_position mode
        double highpos_adjusted_decrease_place_ =
            param_set_.decrease_place_ -
            (_super_highpos_fraction_ *
             param_set_
                 .highpos_thresh_decrease_);  // decreasing place-threshold of reducing position on high_position mode
        double highpos_adjusted_decrease_keep_ =
            param_set_.decrease_keep_ -
            (_super_highpos_fraction_ *
             param_set_
                 .highpos_thresh_decrease_);  // decreasing keep-threshold of reducing position on high_position mode

        position_tradevarset_map_[i].Assign(
            highpos_adjusted_decrease_place_, highpos_adjusted_decrease_keep_,
            highpos_adjusted_increase_place_ + very_high_barrier_, highpos_adjusted_increase_keep_ + very_high_barrier_,
            highpos_adjusted_decrease_place_ + param_set_.improve_,
            highpos_adjusted_decrease_keep_ + param_set_.improve_,
            highpos_adjusted_decrease_place_ + param_set_.aggressive_,
            highpos_adjusted_increase_place_ + param_set_.improve_ + very_high_barrier_,
            highpos_adjusted_increase_keep_ + param_set_.improve_ + very_high_barrier_,
            highpos_adjusted_increase_place_ + param_set_.aggressive_ + very_high_barrier_,
            HFSAT::MathUtils::GetFlooredMultipleOf(std::max(0, std::min((param_set_.max_position_ - for_position_),
                                                                        highpos_adjusted_decrease_trade_size_)),
                                                   min_order_size_),
            0);

      } else {  // ( -inf, -maxpos )
        double _super_highpos_fraction_ = ((double)-for_position_ / (double)highpos_limit_position_);

        // high_position
        double highpos_adjusted_increase_place_ =
            param_set_.increase_place_ +
            (_super_highpos_fraction_ *
             param_set_
                 .highpos_thresh_factor_);  // increasing place-threshold of increasing position on high_position mode
        double highpos_adjusted_increase_keep_ =
            param_set_.increase_keep_ +
            (_super_highpos_fraction_ *
             param_set_
                 .highpos_thresh_factor_);  // increasing keep-threshold of increasing position on high_position mode
        double highpos_adjusted_decrease_place_ =
            param_set_.decrease_place_ -
            (_super_highpos_fraction_ *
             param_set_
                 .highpos_thresh_decrease_);  // decreasing place-threshold of reducing position on high_position mode
        double highpos_adjusted_decrease_keep_ =
            param_set_.decrease_keep_ -
            (_super_highpos_fraction_ *
             param_set_
                 .highpos_thresh_decrease_);  // decreasing keep-threshold of reducing position on high_position mode

        position_tradevarset_map_[i].Assign(
            highpos_adjusted_decrease_place_ - override_signal_, highpos_adjusted_decrease_keep_ - override_signal_,
            highpos_adjusted_increase_place_ + very_high_barrier_, highpos_adjusted_increase_keep_ + very_high_barrier_,
            highpos_adjusted_decrease_place_ + param_set_.improve_ + very_high_barrier_,
            highpos_adjusted_decrease_keep_ + param_set_.improve_ + very_high_barrier_,
            highpos_adjusted_decrease_place_ + param_set_.aggressive_ + very_high_barrier_,
            highpos_adjusted_increase_place_ + param_set_.improve_ + very_high_barrier_,
            highpos_adjusted_increase_keep_ + param_set_.improve_ + very_high_barrier_,
            highpos_adjusted_increase_place_ + param_set_.aggressive_ + very_high_barrier_,
            HFSAT::MathUtils::GetFlooredMultipleOf(std::max(0, std::min((param_set_.max_position_ - for_position_),
                                                                        highpos_adjusted_decrease_trade_size_)),
                                                   min_order_size_),
            0);
      }
    }  // end !set_explicitly_

    if (original_max_position_ != param_set_.max_position_) {  // True only for fractional MUR < 1.0.
      if (for_position_ <= -original_max_position_) {          // For short position beyond the original MUR * UTS , set
                                                               // ask-place and keep thresh.
        position_tradevarset_map_[i].MultiplyAsksBy(100.0);
      }
    }

    if (for_position_ >= -param_set_.max_position_) {
      position_tradevarset_map_[i].MultiplyBy(min_price_increment_);
      std::cout << "Position " << for_position_ << " mapidx " << i << ' '
                << HFSAT::ToString(position_tradevarset_map_[i]).c_str() << std::endl;
    }
  }

  // Just in case the max-pos was changed to deal with fractional MURs
  param_set_.max_position_ = original_max_position_;
}
