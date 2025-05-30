#ifndef _EXECUTIONER_MULTIQUOTER_HPP
#define _EXECUTIONER_MULTIQUOTER_HPP

#include "tradeengine/Executioner/BaseExecutioner.hpp"

class MultiQuoter : public BaseExecutioner {
 private:
  std::string identifier_;
  double bid_percentage_offset_;
  double ask_percentage_offset_;
  double bid_offset_;
  double ask_offset_;
  int step_offset_;
  int num_levels_to_place_;
  int quoter_bid_size_;
  int quoter_ask_size_;

  double bid_tighten_update_percent_;
  double bid_widen_update_percent_;
  double ask_tighten_update_percent_;
  double ask_widen_update_percent_;

  double bid_tighten_update_int_offset_;
  double bid_widen_update_int_offset_;
  double ask_tighten_update_int_offset_;
  double ask_widen_update_int_offset_;

  bool bid_overruled_;
  bool ask_overruled_;
  double overruled_leaway_percent_;
  double overruled_leaway_offset_;

  int delay_between_orders_;
  int delay_between_modify_usecs_;

  double quoter_bid_price_;
  int quoter_bid_int_price_;
  double quoter_ask_price_;
  int quoter_ask_int_price_;

  double previous_bid_price_;
  int previous_bid_int_price_;
  double previous_ask_price_;
  int previous_ask_int_price_;

  std::deque<std::pair<HFSAT::BaseOrder*, int> > bid_order_price_pair_vec_;
  std::deque<std::pair<HFSAT::BaseOrder*, int> > ask_order_price_pair_vec_;

  int cancel_on_sweep_;
  bool use_cancel_on_widen_;
  int last_new_bid_order_time_;
  int64_t last_modify_bid_order_time_;
  int last_new_ask_order_time_;
  int64_t last_modify_ask_order_time_;

  void LoadParams();
  inline void UpdateOrders();
  inline void UpdateBidOrders();
  inline void UpdateAskOrders();

 public:
  MultiQuoter(std::string _exec_param_file, HFSAT::Watch& _watch_, HFSAT::DebugLogger& dbglogger_,
              HFSAT::SecurityMarketView* _secondary_smv, HFSAT::BasicOrderManager* _basic_om, bool _livetrading_,
              bool _is_modify_before_confirmation_, bool _is_cancellable_before_confirmation_, TheoValues& theo_values);
  ~MultiQuoter() {}

  void OnReady(const HFSAT::MarketUpdateInfo& _market_update_info_);
  void OnTheoUpdate();
  void OnOrderChange(HFSAT::BaseOrder* _order_);
  void OnOrderReject(HFSAT::BaseOrder* _order_);
  void CheckOrderStatus(HFSAT::BaseOrder* _order_);
  std::string GetOrderCountString(){
      return identifier_ + " " + std::to_string(order_count_) + " ";
  }

  void DisableBid();
  void DisableAsk();
  void CancelBid();
  void CancelAsk();
};

#endif  // _EXECUTIONER_MULTIQUOTER_HPP
