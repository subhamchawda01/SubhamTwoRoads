// =====================================================================================
//
//       Filename:  tradevarset_helper.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  12/23/2015 05:32:38 PM
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

#include "dvctrade/ExecLogic/base_trading_defines.hpp"
#include "dvctrade/ExecLogic/trade_vars.hpp"

#include "dvctrade/InitCommon/paramset.hpp"

namespace HFSAT {

class TradeVarSetBuilder {
 public:
  TradeVarSetBuilder(DebugLogger& dbglogger, const Watch& watch, bool livetrading);
  ~TradeVarSetBuilder();

  void BuildConstantTradeVarSets(
      ParamSet* paramset, const SecurityMarketView* smv_, TradeVars_t& closeout_zeropos_tradevarset_,
      TradeVars_t& closeout_long_tradevarset_,
      TradeVars_t closeout_short_tradevarset_);  ///< building tradevar sets for closeout modes
  ///< Initial setup of the maps so that wuerying for TradeVars_t given position becomes fast
  void BuildPositionTradeVarSetMap(ParamSet* param, const SecurityMarketView* smv,
                                   PositionTradeVarSetMap& position_tradevarset_map, int& map_pos_increment,
                                   const int& P2TV_zero_idx, bool livetrading);

 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  bool livetrading_;
};
}
