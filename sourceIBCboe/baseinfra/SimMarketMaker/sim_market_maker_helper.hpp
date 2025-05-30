// =====================================================================================
//
//       Filename:  sim_market_maker_helper.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  07/01/2015 05:28:18 PM
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
#include "baseinfra/SimMarketMaker/base_sim_market_maker.hpp"
#include "baseinfra/SimMarketMaker/sim_market_maker_list.hpp"
#include "baseinfra/LoggedSources/filesource_list.hpp"
#include "dvccode/ExternalData/historical_dispatcher.hpp"
#include "baseinfra/Tools/common_smv_source.hpp"

namespace HFSAT {

class SimMarketMakerHelper {
 public:
  static BaseSimMarketMaker* GetSimMarketMaker(DebugLogger& dbglogger_, HFSAT::Watch& watch_,
                                               HFSAT::SecurityMarketView* dep_smv, HFSAT::SecurityMarketView* sim_smv,
                                               int market_model_index, HFSAT::SimTimeSeriesInfo& sim_time_series_info,
                                               std::vector<HFSAT::MarketOrdersView*> sid_to_mov_ptr_map,
                                               HFSAT::HistoricalDispatcher& historical_dispatcher,
                                               CommonSMVSource* common_smv_source, bool uniqueinstance = true);

  static SMMType GetSMMType(const HFSAT::SecurityMarketView* _dep_smv_, int _YYYYMMDD_);
  // returns true if given shortcode corresponds to an HK equity
  static bool IsHKEquity(std::string _shortcode) { return _shortcode.substr(0, 3) == "HK_"; }
};
}
