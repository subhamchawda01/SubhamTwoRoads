#pragma once

#include <string>
#include <fstream>
#include <strings.h>
#include <string.h>
#include <sstream>
#include "dvccode/CDef/defines.hpp"

namespace NSE_SIMPLEEXEC {
typedef enum { kNoop = 0, kPassive, kAggressive } ProgramState_t;
typedef enum { kAggOnly = 0, kPassAndAgg, kPassOnly } AlgoType_t;
typedef enum { kDecideOrdersFromParamConstraints = 0, kGetOrdersFromStrategy } ExecLogicRunType_t;

struct ParamSet {
 public:
  double avg_sprd_;         // historical average bidask spread
  double agg_sprd_factor_;  // we aggress if spread <= avg_sprd_*agg_sprd_factor_

  std::string instrument_;  // product to buy/sell

  int trade_cooloff_interval_;          // time interval between successive trades in msecs
  double market_participation_factor_;  // target factor for market participation
  double market_participation_factor_tolerance_;

  AlgoType_t exec_algo_;                    // execution algo to be used
  ExecLogicRunType_t exec_logic_run_type_;  // if exec needs to be run between T1-T2 or all day long

  double max_notional_;         // maximum amount that is bought/sold
  int max_lots_per_trade_;      // max size per trade
  int max_lots_;                // overrides the max_notional_ check(if provided)
  HFSAT::TradeType_t buysell_;  // whether trade is buy/sell

  int start_time_msecs_;  // msecs from midnight in GMT at start
  int end_time_msecs_;    // msecs from midnight in GMT at end
  int yyyymmdd_;          // date of the trade

  ParamSet(const std::string& filename);
  void ParseParamFile(const std::string& filename);
  std::string ToString();
};
}
