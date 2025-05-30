#ifndef _EXECUTIONER_ELECTRONICEYE_HPP
#define _EXECUTIONER_ELECTRONICEYE_HPP

#include "tradeengine/Executioner/BaseExecutioner.hpp"

class ElectronicEye : public BaseExecutioner {
 private:
  std::string identifier_;
  double bid_percentage_offset_;
  double ask_percentage_offset_;
  double bid_offset_;
  double ask_offset_;
  int eye_shoot_bid_size_;
  int eye_shoot_ask_size_;

  bool bid_overruled_;
  bool ask_overruled_;
  double overruled_leaway_percent_;
  double overruled_leaway_offset_;

  int delay_between_orders_;

  double eye_bid_price_;
  int eye_bid_int_price_;
  double eye_ask_price_;
  int eye_ask_int_price_;

  HFSAT::BaseOrder* bid_order_;
  HFSAT::BaseOrder* ask_order_;

  int target_position_;
  int mirror_factor_;

  int last_new_bid_order_time_;
  int last_new_ask_order_time_;

  bool shoot_only_on_sweep_;

  int max_bid_orders_to_shoot_;
  int max_ask_orders_to_shoot_;
  double min_price_move_percent_;
  double min_price_move_;
  bool shoot_only_on_primary_move_;
  int num_outstanding_bid_orders_;
  int num_outstanding_ask_orders_;

  double prev_market_bid_price_;
  double prev_market_ask_price_;

  void LoadParams();
  inline void UpdateOrders();
  inline void UpdateBidOrders();
  inline void UpdateAskOrders();

 public:
  ElectronicEye(std::string _exec_param_file, HFSAT::Watch& _watch_, HFSAT::DebugLogger& dbglogger_,
                HFSAT::SecurityMarketView* _secondary_smv, HFSAT::BasicOrderManager* _basic_om, bool _livetrading_,
                bool _is_modify_before_confirmation_, bool _is_cancellable_before_confirmation_,
                TheoValues& theo_values);
  ~ElectronicEye() {}

  void OnReady(const HFSAT::MarketUpdateInfo& _market_update_info_);
  void OnTheoUpdate();
  void OnOrderChange(HFSAT::BaseOrder* _order_);
  void OnOrderReject(HFSAT::BaseOrder* _order_);
  void CheckOrderStatus(HFSAT::BaseOrder* _order_);
  void DisableBid();
  void DisableAsk();
  std::string GetOrderCountString(){
      return identifier_ + " " + std::to_string(order_count_) + " ";
  }

  // Since Eye only contains IOC orders, no need to cancel.
  void CancelBid() {}
  void CancelAsk() {}
};

#endif  // _EXECUTIONER_ELECTRONICEYE_HPP
