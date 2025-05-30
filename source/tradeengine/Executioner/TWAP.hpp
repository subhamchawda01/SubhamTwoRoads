#ifndef _EXECUTIONER_TWAP_HPP
#define _EXECUTIONER_TWAP_HPP

#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "baseinfra/OrderRouting/basic_order_manager.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "tradeengine/CommonInitializer/defines.hpp"

class TWAP : public HFSAT::ExecutionListener,
             public HFSAT::TimePeriodListener,
             public HFSAT::OrderChangeListener,
             public HFSAT::OrderRejectListener {
 private:
  std::string shc_;
  bool is_ready_;
  HFSAT::Watch& watch_;
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::SecurityMarketView* secondary_smv_;
  HFSAT::BasicOrderManager* basic_om_;
  HFSAT::BasePNL* base_pnl_;
  int time_to_exec_in_secs_;
  int start_time_mfm_;
  int window_width_;
  int abs_pos_to_exec_;
  HFSAT::TradeType_t buysell_;
  int notional_to_place_;
  int end_time_mfm_;
  int order_size_;
  int position_;
  // Variables to maintain
  std::vector<int> pos_vec_;
  std::vector<int> start_time_mfm_vec_;
  std::vector<int> end_time_mfm_vec_;

  TWAP* hedge_pos_exec_;
  double hedge_multiplier_;

  int num_buckets_;
  HFSAT::BaseOrder* order_;
  int current_index_;
  double total_traded_value_;
  int unique_exec_id_;

  inline void UpdateOrders();
  void InitializeVariables();

 public:
  TWAP(HFSAT::Watch& _watch_, HFSAT::DebugLogger& dbglogger_, HFSAT::SecurityMarketView* _secondary_smv,
       HFSAT::BasicOrderManager* _basic_om_, int _time_to_exec_in_secs_, int _start_time_mfm_, int _size_to_exec_,
       HFSAT::TradeType_t _buysell_, int _notional_to_place_);
  ~TWAP() {}

  void OnExec(const int _new_position_, const int _exec_quantity_, const HFSAT::TradeType_t _buysell_,
              const double _price_, const int r_int_price_, const int _security_id_, const int _caos_);
  void OnTimePeriodUpdate(const int num_pages_to_add_);
  void AddBucketToExecute(int _pos_to_exec_, int _bucket_time_width_msecs_);

  void OnOrderChange(HFSAT::BaseOrder* _order_);
  void OnOrderReject(HFSAT::BaseOrder* _order_);
  void OnOrderChange(){};
  void OnOrderReject(){};
  void CheckOrderStatus(HFSAT::BaseOrder* _order_);

  void PlaceAggressiveOrder();
  void PlacePassiveOrder();

  void SetHedgePosExec(TWAP* _hedge_pos_exec_, double _hedge_multiplier_) {
    hedge_pos_exec_ = _hedge_pos_exec_;
    hedge_multiplier_ = _hedge_multiplier_;
  }

  int GetPosition() { return position_; }
  int GetPosToExec() {
    return ((buysell_ == HFSAT::TradeType_t::kTradeTypeSell) ? -1 * abs_pos_to_exec_ : abs_pos_to_exec_);
  }
  std::string GetSecondaryShc() { return shc_; }
  void PrintStatus() {
    dbglogger_ << " SHC: " << shc_ << " POS: " << position_ << " POS REMAINING: " << (GetPosToExec() - position_)
               << " TTV: " << total_traded_value_ << " PNL: " << base_pnl_->total_pnl() << DBGLOG_ENDL_FLUSH;
  }

  void SetBasePNL(HFSAT::BasePNL* base_pnl) { base_pnl_ = base_pnl; }
  void SubscribeOM() {
    unique_exec_id_ = basic_om_->AddOrderChangeListener(this);
    basic_om_->AddOrderRejectListener(this, unique_exec_id_);
  }
};

#endif  // _EXECUTIONER_TWAP_HPP
