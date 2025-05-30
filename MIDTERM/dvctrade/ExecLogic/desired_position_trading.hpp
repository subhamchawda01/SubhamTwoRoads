/**
   \file ExecLogic/desired_position_trading.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#pragma once

#include "dvctrade/ExecLogic/base_trading.hpp"

namespace HFSAT {

/// @brief Class to trade with a target price that is not dependant on top level sizes
///
/// We calculate the desired position as ( sumvars / model_stdev_ ) * ( max_position_ / dpt_range_=2.0 )
/// and place orders as long as it does not take us too far away from the desired_position_
/// Buy UTS if current_position + UTS <= Desired_Position_ + desired_position_leeway_
class DesiredPositionTrading : public virtual BaseTrading {
 protected:
 public:
  DesiredPositionTrading(DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
                         SmartOrderManager& _order_manager_, const std::string& _paramfilename_,
                         const bool _livetrading_, MulticastSenderSocket* _p_strategy_param_sender_socket_,
                         EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                         const int t_trading_end_utc_mfm_, const int _runtime_id_,
                         const std::vector<std::string> _this_model_source_shortcode_vec_);

  /// destructor not made virtual ... please do so when making child classes
  ~DesiredPositionTrading(){};

  static std::string StrategyName() {
    return "DesiredPositionTrading";
  }  ///< used in trade_init to check which strategy is to be initialized

  void SetModelMathComponent(BaseModelMath* t_base_model_math_) {
    p_base_model_math_ = t_base_model_math_;
    targetbias_to_postion_ =
        double(param_set_.max_position_ / param_set_.dpt_range_) / (p_base_model_math_->model_stdev());
    DBGLOG_TIME_CLASS_FUNC << " tgtbias_top_position: " << targetbias_to_postion_
                           << " dpt_range_: " << param_set_.dpt_range_
                           << " model stdev: " << p_base_model_math_->model_stdev() << DBGLOG_ENDL_FLUSH;
  }

 protected:
  double targetbias_to_postion_;

  // queue positions of the orders in the best level only, should give fairly good estimation of
  // qA qB of all orders
  int qA_best_bid_;
  int qB_best_bid_;
  int qA_best_ask_;
  int qB_best_ask_;
  int last_updated_bid_int_price_;
  int last_updated_ask_int_price_;
  void UpdateBidQueueSizesBestLevel(bool first_time_);
  void UpdateAskQueueSizesBestLevel(bool first_time_);
  bool CheckToCancelBestBid();
  bool CheckToCancelBestAsk();
  void TradingLogic();  ///< All the strategy based trade execution is written here

  void PrintFullStatus();
};
}
