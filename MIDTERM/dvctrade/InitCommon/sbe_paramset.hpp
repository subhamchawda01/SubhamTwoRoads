/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
*/

#pragma once

#include <string>
#include <fstream>
#include <strings.h>
#include <string.h>
#include <sstream>
#include "dvccode/CDef/defines.hpp"

namespace HFSAT {

typedef enum { kNoop = 0, kPassive, kAggressive } ProgramState_t;
typedef enum { kAggOnly = 0, kPassAndAgg, kPassOnly } AlgoType_t;

struct SBEParamSet {
 public:
  double avg_sprd_;         // historical average bidask spread
  double agg_sprd_factor_;  // we aggress if spread <= avg_sprd_*agg_sprd_factor_

  std::string instrument_;  // product to buy/sell

  int trade_cooloff_interval_;          // time interval between successive trades in msecs
  double market_participation_factor_;  // target factor for market participation
  double market_participation_factor_tolerance_;

  AlgoType_t exec_algo_;                    // execution algo to be used

  double l1size_limit_per_trade_;      // max size per trade
  int max_lots_;                // overrides the max_notional_ check(if provided)
  int max_position_;

  bool use_nonbest_support_;
  int start_time_msecs_;  // msecs from midnight in GMT at start
  int end_time_msecs_;    // msecs from midnight in GMT at end
  int yyyymmdd_;          // date of the trade

  SBEParamSet(const std::string& filename);
  void ParseParamFile(const std::string& filename);
  void SetSizeFactors(const int _min_order_size_);
  std::string ToString();
};
}
