#ifndef _EXECUTIONER_ModifyEye_HPP
#define _EXECUTIONER_ModifyEye_HPP
//#define _DBGLOGGER_TRADE_ENGINE_INFO_

#include "tradeengine/Executioner/BaseExecutioner.hpp"

class ModifyEye : public BaseExecutioner {
 private:
  std::string identifier_;
  double bid_percentage_offset_;
  double ask_percentage_offset_;
  double bid_offset_;
  double ask_offset_;
  int quoter_bid_size_;
  int quoter_ask_size_;

  bool bid_overruled_;
  bool ask_overruled_;
  double overruled_leaway_percent_;
  double overruled_leaway_offset_;

  int delay_between_orders_;

  double eye_bid_price_;
  int eye_bid_int_price_;
  double eye_ask_price_;
  int eye_ask_int_price_;

  double quoter_bid_price_;
  int quoter_bid_int_price_;
  double quoter_ask_price_;
  int quoter_ask_int_price_;

  HFSAT::BaseOrder* bid_order_;
  HFSAT::BaseOrder* ask_order_;

  double bid_tighten_update_percent_;
  double bid_widen_update_percent_;
  double ask_tighten_update_percent_;
  double ask_widen_update_percent_;
  double quoter_bid_order_percent_;
  double quoter_ask_order_precent_;

  double bid_tighten_update_int_offset_;
  double bid_widen_update_int_offset_;
  double ask_tighten_update_int_offset_;
  double ask_widen_update_int_offset_;
  double quoter_bid_order_offset_;
  double quoter_ask_order_offset_;

  int last_new_bid_order_time_;
  int last_new_ask_order_time_;
  int last_modify_bid_order_time_;
  int last_modify_ask_order_time_;

  bool shoot_only_on_sweep_;

  void LoadParams();
  inline void UpdateOrders();
  inline void UpdateBidOrders();
  inline void UpdateAskOrders();

 public:
  ModifyEye(std::string _exec_param_file, HFSAT::Watch& _watch_, HFSAT::DebugLogger& dbglogger_,
            HFSAT::SecurityMarketView* _secondary_smv, HFSAT::BasicOrderManager* _basic_om, bool _livetrading_,
            bool _is_modify_before_confirmation_, bool _is_cancellable_before_confirmation_, TheoValues& theo_values);
  ~ModifyEye() {}

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

  void CancelBid();
  void CancelAsk();
};

#endif  // _EXECUTIONER_ModifyEye_HPP
