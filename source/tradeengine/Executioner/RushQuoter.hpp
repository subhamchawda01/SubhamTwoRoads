#ifndef _EXECUTIONER_RUSHQUOTER_HPP
#define _EXECUTIONER_RUSHQUOTER_HPP

#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "dvccode/CDef/math_utils.hpp"
#include "tradeengine/Executioner/BaseExecutioner.hpp"

class RushQuoter : public BaseExecutioner, public HFSAT::ThrottleNumberListener {
 private:
  std::string identifier_;
  int max_depth_;

  double bid_percentage_offset_;
  double ask_percentage_offset_;
  double bid_offset_;
  double ask_offset_;
  int rushquoter_bid_size_;
  int rushquoter_ask_size_;

  int delay_between_orders_;
  int delay_between_aggress_;

  int drag_tighten_delay_;
  int config_drag_tighten_delay_;
  int min_drag_tighten_delay_;
  int max_drag_tighten_delay_;
  int drag_widen_delay_;

  double rushquoter_bid_price_;
  int rushquoter_bid_int_price_;
  double rushquoter_ask_price_;
  int rushquoter_ask_int_price_;

  double previous_bid_price_;
  int previous_bid_int_price_;
  double previous_ask_price_;
  int previous_ask_int_price_;

  int last_new_bid_order_time_;
  int last_modify_bid_order_time_;
  int last_aggress_bid_order_time_;
  int last_new_ask_order_time_;
  int last_modify_ask_order_time_;
  int last_aggress_ask_order_time_;

  int price_upper_limit_;
  int price_lower_limit_;

  HFSAT::BaseOrder* bid_order_;
  HFSAT::BaseOrder* ask_order_;

  bool disable_one_percent_order_limit_;

  bool allow_to_shoot_;
  bool allow_to_improve_;
  int side_to_shoot_; // 0 for both side ; 1(or +ve) for buy side only ; -1(or -ve) for sell side only
  int side_to_improve_; // 0 for both side ; 1(or +ve) for buy side only ; -1(or -ve) for sell side only

  int max_int_spread_to_cross_;
  int min_int_spread_to_improve_;

  void LoadParams();
  void UpdateOrders();
  void UpdateBidOrders();
  void UpdateAskOrders();
  void SendBidOrder(int current_order_bid_int_price_);
  void SendAskOrder(int current_order_bid_int_price_);

 public:
  RushQuoter(std::string _exec_param_file, HFSAT::Watch& _watch_, HFSAT::DebugLogger& dbglogger_,
           HFSAT::SecurityMarketView* _secondary_smv, HFSAT::BasicOrderManager* _base_om, bool _livetrading_,
           bool _is_modify_before_confirmation_, bool _is_cancellable_before_confirmation_, TheoValues& theovalues);
  ~RushQuoter() {}

  void OnReady(const HFSAT::MarketUpdateInfo& _market_update_info_);
  void OnTheoUpdate();
  void OnOrderChange(HFSAT::BaseOrder* _order_);
  void OnOrderReject(HFSAT::BaseOrder* _order_);
  void CheckOrderStatus(HFSAT::BaseOrder* _order_);
  void DisableBid();
  void CancelBid();
  void CancelAsk();
  void DisableAsk();
  std::string GetOrderCountString(){
      return identifier_ + " " + std::to_string(order_count_) + " ";
  }

  void OnThrottleChange(int _num_throttle_, const unsigned int security_id_) {
    if (min_throttles_per_min_ > _num_throttle_) {
      drag_tighten_delay_ = std::max((int)(drag_tighten_delay_ * 0.5), min_drag_tighten_delay_);
      //			DBGLOG_TIME_CLASS_FUNC << identifier_ << " MIN THROTTLE HIT NUM_THROTTLE: " <<
      //_num_throttle_
      //					<< " NEW TIGHTEN DELAY: " <<  drag_tighten_delay_ << DBGLOG_ENDL_FLUSH;
    } else if (max_throttles_per_min_ < _num_throttle_) {
      drag_tighten_delay_ = std::min(drag_tighten_delay_ * 2 + 1, max_drag_tighten_delay_);
      DBGLOG_TIME_CLASS_FUNC << identifier_ << " MAX THROTTLE HIT NUM_THROTTLE: " << _num_throttle_
                             << " NEW TIGHTEN DELAY: " << drag_tighten_delay_ << DBGLOG_ENDL_FLUSH;
    }
  }
};

#endif  // _EXECUTIONER_IMPROVER_HPP
