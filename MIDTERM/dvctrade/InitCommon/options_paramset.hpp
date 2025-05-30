/**
 \file InitCommon/options_paramset.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 353, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551

 */
#ifndef BASE_INITCOMMON_OPTIONS_PARAMSET_H
#define BASE_INITCOMMON_OPTIONS_PARAMSET_H

#include "dvccode/CDef/control_messages.hpp"
#include "dvccode/CommonTradeUtils/economic_events_manager.hpp"
#include "dvccode/CommonTradeUtils/sample_data_util.hpp"
#include "dvctrade/Indicators/positioning.hpp"
#include "dvctrade/Indicators/recent_simple_volume_measure.hpp"

#define EZONE_MAXLEN 5

/* Current Available Required(?) Default

PARAMVALUE WORST_CASE_UNIT_RATIO 1
PARAMVALUE MAX_UNIT_RATIO 1
PARAMVALUE UNIT_TRADE_SIZE 1

PARAMVALUE HIGHPOS_LIMITS_UNIT_RATIO 0 MAX_UNIT_RATIO
PARAMVALUE HIGHPOS_THRESH_FACTOR 0 0
PARAMVALUE HIGHPOS_THRESH_DECREASE 0 0
PARAMVALUE HIGHPOS_SIZE_FACTOR 0 0
PARAMVALUE ZEROPOS_LIMITS_UNIT_RATIO 0 MAX_UNIT_RATIO

PARAMVALUE ZEROPOS_KEEP 1
PARAMVALUE PLACE_KEEP_DIFF/ZEROPOS_PLACE 1
PARAMVALUE INCREASE_ZEROPOS_DIFF/INCREASE_PLACE/INCREASE_KEEP 1
PARAMVALUE ZEROPOS_DECREASE_DIFF/DECREASE_PLACE/DECREASE_KEEP 1

PARAMVALUE NUM_NON_BEST_LEVELS_MONITORED 0 0

PARAMVALUE COOLOFF_INTERVAL 0 100
PARAMVALUE AGG_COOLOFF_INTERVAL 0 100

PARAMVALUE GLOBAL_MAX_LOSS 1
PARAMVALUE MAX_LOSS 1
PARAMVALUE MAX_OPENTRADE_LOSS 0 MAX_LOSS
PARAMVALUE BREAK_MSECS_ON_OPENTRADE_LOSS 0 10*60*1000

PARAMVALUE DELTA_HEDGE_UPPER_THRESHOLD 1
PARAMVALUE FRACTIONAL_SECOND_IMPLIED_VOL 1
PARAMVALUE DELTA_HEDGE_LOWER_THRESHOLD 1

*/

namespace HFSAT {

class OptionsParamSet {
 public:
  int tradingdate_;

  // premium vars
  int worst_case_unit_ratio_;  // this is in units
  int max_unit_ratio_;         // this in uts units
  int unit_trade_size_;        // this is in lots
  int highpos_limits_unit_ratio_;
  double highpos_thresh_factor_;
  double highpos_thresh_decrease_;
  double highpos_size_factor_;
  int zeropos_limits_unit_ratio_;
  // one way of specifying thresholds/premiums
  double increase_place_;
  double increase_keep_;
  double zeropos_place_;
  double zeropos_keep_;
  double decrease_place_;
  double decrease_keep_;

  // another way
  double place_keep_diff_;
  bool read_place_keep_diff_;
  double increase_zeropos_diff_;
  bool read_increase_zeropos_diff_;
  double zeropos_decrease_diff_;
  bool read_zeropos_decrease_diff_;
  double improve_;
  double aggressive_;
  int fut_stdev_duration_;
  bool scale_by_fut_stdev_;
  int opt_stdev_duration_;
  bool scale_by_opt_stdev_;
  int opt_bidaskspread_duration_;
  bool scale_by_opt_bidaskspread_;

  // Fut model params
  double fut_place_;
  double fut_keep_;
  double fut_place_keep_diff_;
  bool read_fut_place_keep_diff_;

  // risk vars
  int max_loss_;
  int global_max_loss_;
  int max_opentrade_loss_;
  int break_msecs_on_max_opentrade_loss_;  ///< only if overriding default value
  int max_drawdown_;
  double max_global_delta_;
  double max_global_gamma_;
  double max_global_vega_;
  double max_global_theta_;
  double delta_hedge_lower_threshold_;
  double delta_hedge_upper_threshold_;
  double fractional_second_implied_vol_;
  int distribute_risk_stats_;
  int delta_hedge_logic_;

  // exec vars
  int cooloff_interval_;
  int agg_cooloff_interval_;
  int aggflat_cooloff_interval_;
  int improve_cooloff_interval_;
  bool allowed_to_improve_;
  bool allowed_to_aggress_;
  // settings for aggressive and improve sanity checks
  int max_position_to_aggress_unit_ratio_;
  int max_position_to_aggress_;
  int max_position_to_improve_unit_ratio_;
  int max_position_to_improve_;
  int max_position_to_cancel_unit_ratio_;
  int max_position_to_cancel_;
  int max_int_spread_to_place_;
  int max_int_spread_to_cross_;
  int min_int_spread_to_improve_;
  int num_non_best_levels_monitored_;
  int min_size_to_join_;
  int min_int_price_to_place_;  // minimum int price to place order
  bool use_throttle_manager_;
  int throttle_message_limit_;

  // derived variables
  int max_position_;
  int worst_case_position_;

  std::string paramfilename_;
  std::vector<int> staggered_getflat_msecs_vec_;  // We reduce max pos by half everytime we encounter this
  int aggressive_getflat_msecs_;

  OptionsParamSet(const std::string& _paramfilename_, const int r_tradingdate_, std::string dep_short_code_);
  void LoadParamSet(std::string dep_short_code);
  void LoadParamSet();
  void WriteSendStruct(ParamSetSendStruct& retval) const;

  // there are two ways of specifying params ... building the other one
  bool VerifySanctity();
  void ReconcileParams(std::string dep_short_code);
};
}
#endif  // BASE_INITCOMMON_OPTIONS_PARAMSET_H
