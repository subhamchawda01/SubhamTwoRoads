// =====================================================================================
//
//       Filename:  multiple_indicator_helper.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  12/21/2015 04:34:27 PM
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

#include "dvctrade/ExecLogic/ExecLogicHelper/multiple_indicator_helper.hpp"

#define MAX_PRODUCT_COUNT 50

// we are interested in listening certain features from below fixed set for one or more products
// we can also read from offline files, the same values to make the comparision study online
// current we have following,
// smv_ pointer here will have the products which are listening to these features
// there is addition vector for each feature indicating whether particular product identifies the same index is
// listening to corresponding feature
// this class doesnt forward the values, instead interested candidates are suppose to read using functions

namespace HFSAT {
MultipleIndicatorHelper::MultipleIndicatorHelper(DebugLogger& t_dbglogger, const Watch& r_watch,
                                                 int _trading_start_mfm_, int _trading_end_mfm_)
    : dbglogger_(t_dbglogger),
      watch_(r_watch),
      trading_start_mfm_(_trading_start_mfm_),
      trading_end_mfm_(_trading_end_mfm_),
      total_products_(HFSAT::SecurityNameIndexer::GetNumSecurityId()),
      computing_moving_bidask_spread_map_(),
      computing_slow_stdev_map_(),
      moving_bidask_spread_indicator_map_(),
      slow_stdev_calculator_indicator_map_(),
      moving_bidask_spread_value_map_(),
      slow_stdev_value_map_() {
  ///

  moving_bidask_spread_index_ = -1;
  slow_stdev_index_ = -1;
  indices_used_so_far_ = 0;
}

MultipleIndicatorHelper::~MultipleIndicatorHelper() {}

void MultipleIndicatorHelper::OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value) {
  int indicator_type_ = indicator_index / total_products_;
  int product_index_ = indicator_index % total_products_;

  if (indicator_type_ == moving_bidask_spread_index_) {
    moving_bidask_spread_value_map_[product_index_] = new_value;
  }
  if (indicator_type_ == slow_stdev_index_) {
    slow_stdev_value_map_[product_index_] = new_value;
  }
}

void MultipleIndicatorHelper::OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value_decrease,
                                                const double& new_value_nochange, const double& new_value_increase) {}

void MultipleIndicatorHelper::ComputeMovingBidAskSpread(double t_window_, const SecurityMarketView* p_smv_) {
  unsigned int t_security_id_ = p_smv_->security_id();
  if (computing_moving_bidask_spread_map_[t_security_id_]) {
    ExitVerbose(kExitErrorCodeGeneral, "calling twice to create bid_ask_spread in MultipleIndicatorHelper\n");
  } else {
    moving_bidask_spread_indicator_map_[t_security_id_] =
        MovingAvgBidAskSpread::GetUniqueInstance(dbglogger_, watch_, p_smv_, t_window_);
    if (moving_bidask_spread_indicator_map_[t_security_id_] == nullptr) {
      ExitVerbose(kExitErrorCodeGeneral, "Could not create bidask_spread indicator in MultipleIndicatorHelper\n");
    }
    if (moving_bidask_spread_index_ < 0) {
      moving_bidask_spread_index_ = indices_used_so_far_;
      indices_used_so_far_++;
    }
    moving_bidask_spread_indicator_map_[t_security_id_]->add_unweighted_indicator_listener(
        moving_bidask_spread_index_ * total_products_ + t_security_id_, this);
    computing_moving_bidask_spread_map_[t_security_id_] = true;
    moving_bidask_spread_value_map_[t_security_id_] = 1;
  }
}

void MultipleIndicatorHelper::ComputeSlowStdev(double t_window_, const SecurityMarketView* p_smv_) {
  unsigned int t_security_id_ = p_smv_->security_id();
  if (computing_slow_stdev_map_[t_security_id_]) {
    ExitVerbose(kExitErrorCodeGeneral, "calling twice to create slow_stdev in MultipleIndicatorHelper\n");
  } else {
    slow_stdev_calculator_indicator_map_[t_security_id_] =
        SlowStdevCalculator::GetUniqueInstance(dbglogger_, watch_, p_smv_->shortcode(), t_window_ * 1000u, 0.001);

    if (slow_stdev_calculator_indicator_map_[t_security_id_] == nullptr) {
      ExitVerbose(kExitErrorCodeGeneral, "Could not create slow stdev calculator in MultipleIndicatorHelper\n");
    }

    if (slow_stdev_index_ < 0) {
      slow_stdev_index_ = indices_used_so_far_;
      indices_used_so_far_++;
    }
    slow_stdev_calculator_indicator_map_[t_security_id_]->add_unweighted_indicator_listener(
        slow_stdev_index_ * total_products_ + t_security_id_, this);
    computing_slow_stdev_map_[t_security_id_] = true;
    slow_stdev_value_map_[t_security_id_] = SampleDataUtil::GetAvgForPeriod(
        p_smv_->shortcode(), watch_.YYYYMMDD(), 20, trading_start_mfm_, trading_end_mfm_, "STDEV", false);
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << "STDEV INTIALIZED: " << p_smv_->shortcode()
                                  << slow_stdev_value_map_[t_security_id_] << DBGLOG_ENDL_FLUSH;
    }
  }
}
}
