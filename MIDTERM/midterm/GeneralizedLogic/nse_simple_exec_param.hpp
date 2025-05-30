#pragma once

#include <string>
#include <fstream>
#include <strings.h>
#include <string.h>
#include <sstream>
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CDef/defines.hpp"
#include "nse_risk_manager.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#define WORST_CASE_MAX_ALLOWED_ORDERS 250

namespace NSE_SIMPLEEXEC {
typedef enum { kNoop = 0, kPassive, kAggressive } ProgramState_t;
typedef enum { kAggOnly = 0, kPassAndAgg, kPassOnly } AlgoType_t;
typedef enum {
  kDecideOrdersFromParamConstraints = 0,
  kGetOrdersFromStrategy
} ExecLogicRunType_t;
typedef enum {
  kEquals = 0,
  kGreater,
  kLesser,
  kGreaterEquals,
  kLesserEquals,
  kAbsGreater,
  kAbsLesser,
  kAbsGreaterEquals,
  kAbsLesserEquals,
  kError
} Comparator_t;
typedef enum { kSim = 0, kLive } Mode_t;
typedef enum { kEntry = 0, kExit, kModify, kCancel, kRollover, kForceCancel } OrderType_t;

struct ParamSet {
public:
  double avg_sprd_;        // historical average bidask spread
  double agg_sprd_factor_; // we aggress if spread <= avg_sprd_*agg_sprd_factor_

  std::string instrument_; // product to buy/sell

  int trade_cooloff_interval_; // time interval between successive trades in
                               // msecs
  double market_participation_factor_; // target factor for market participation
  double market_participation_factor_tolerance_;

  AlgoType_t exec_algo_;                   // execution algo to be used
  ExecLogicRunType_t exec_logic_run_type_; // if exec needs to be run between
                                           // T1-T2 or all day long
  double moneyness_; // ATM +- moneyness_*ATM_STRIKE will be subscribed
  double aggress_threshold_;

  double max_notional_;        // maximum amount that is bought/sold
  int max_lots_per_trade_;     // max size per trade
  int max_lots_;               // overrides the max_notional_ check(if provided)
  HFSAT::TradeType_t buysell_; // whether trade is buy/sell

  int start_time_msecs_; // msecs from midnight in GMT at start
  int end_time_msecs_;   // msecs from midnight in GMT at end
  int yyyymmdd_;         // date of the trade
  int expiry_to_look_till;

  ParamSet(const std::string &filename, std::string &shortcode);
  void ParseParamFile(const std::string &filename);
  std::string ToString();
};
}
