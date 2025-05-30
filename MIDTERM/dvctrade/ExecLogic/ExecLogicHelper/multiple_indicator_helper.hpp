// =====================================================================================
//
//       Filename:  mulitple_indicator_helper.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  12/21/2015 04:34:43 PM
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

#include "dvctrade/InitCommon/options_paramset.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvctrade/Indicators/indicator_list.hpp"
#include "dvctrade/ExecLogic/exec_logic_utils.hpp"
#include "dvctrade/ExecLogic/base_trading_defines.hpp"
#include "dvctrade/ExecLogic/instrument_info.hpp"
#include "dvccode/CommonTradeUtils/sample_data_util.hpp"

namespace HFSAT {
// Using this kind of structure would add one more level of call back in base trading
// Though since this is required in very a few cases and many of them are not being used, we should be fine.
// Not sure if there's better way to it

// TODO make unique instance
class MultipleIndicatorHelper : public IndicatorListener {
 public:
  MultipleIndicatorHelper(DebugLogger& t_dbglogger, const Watch& r_watch, int _trading_start_mfm_,
                          int _trading_end_mfm_);
  ~MultipleIndicatorHelper();

  inline CommonIndicator* GetIndicatorFromTokens(DebugLogger& dbglogger, const Watch& watch,
                                                 const std::vector<const char*>& tokens,
                                                 PriceType_t dep_base_pricetype) {
    return (GetUniqueInstanceFunc(tokens[2]))(dbglogger, watch, tokens, dep_base_pricetype);
  }

  void AddProduct(SecurityMarketView* _dep_market_view_);

  void OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value);
  void OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value_decrease,
                         const double& new_value_nochange, const double& new_value_increase);

  void ComputeMovingBidAskSpread(double t_window_, const SecurityMarketView* p_smv_);
  void ComputeSlowStdev(double t_window_, const SecurityMarketView* p_smv_);

  inline double moving_bidask_spread(int t_security_id_) { return moving_bidask_spread_value_map_[t_security_id_]; }
  inline double slow_stdev(int t_security_id_) { return slow_stdev_value_map_[t_security_id_]; }

 private:
  DebugLogger& dbglogger_;
  const Watch& watch_;

  int trading_start_mfm_;
  int trading_end_mfm_;

  //  MultipleIndicatorHelperListener* listener_; listener only makes sense incase of regime shifts
  // else ondemand request works better ( slow calculators )

  // number of smvs are known and  so index can be feature_id_ ( starts with 0 ) * max_security_id_ + security_id_
  // map is made up of security_id, corresponding_value_

  int indices_used_so_far_;  // just to generate sequence numbers for feature_id_
  int total_products_;       // max_security_id_

  std::map<unsigned int, bool> computing_moving_bidask_spread_map_;
  std::map<unsigned int, bool> computing_slow_stdev_map_;

  int moving_bidask_spread_index_;
  int slow_stdev_index_;

  std::map<unsigned int, MovingAvgBidAskSpread*> moving_bidask_spread_indicator_map_;
  std::map<unsigned int, SlowStdevCalculator*> slow_stdev_calculator_indicator_map_;

  std::map<unsigned int, double> moving_bidask_spread_value_map_;
  std::map<unsigned int, double> slow_stdev_value_map_;
};
}
