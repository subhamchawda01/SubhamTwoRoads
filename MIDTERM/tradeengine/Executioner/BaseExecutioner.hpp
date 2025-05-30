#pragma once
//#define _DBGLOGGER_TRADE_ENGINE_INFO_

#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "baseinfra/OrderRouting/basic_order_manager.hpp"
#include "tradeengine/CommonInitializer/defines.hpp"
#include "dvccode/CDef/shared_logging_defines.hpp"

struct TheoValues {
  bool is_valid_;
  bool is_primary_update_;
  int is_big_trade_;
  double theo_bid_price_;
  double theo_ask_price_;
  int position_to_offset_;
  MovementIndicator_t movement_indicator_;
  int last_traded_int_price_;
  int sweep_mode_active_;
  double primary_best_bid_price_;
  double primary_best_ask_price_;
  double reference_primary_bid_;
  double reference_primary_ask_;
};

class BaseExecutioner : public HFSAT::OrderChangeListener, HFSAT::OrderRejectListener {
 protected:
  TheoValues& theo_values_;
  std::string exec_param_file_;
  HFSAT::Watch& watch_;
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::SecurityMarketView* secondary_smv_;
  std::map<std::string, std::string>* exec_key_val_map_;
  HFSAT::BasicOrderManager* basic_om_;
  uint16_t status_mask_;
  bool bid_status_;
  bool ask_status_;
  int order_limit_;
  int order_count_;
  double inv_trade_factor_;
  int exec_ratio_;
  int bad_ratio_limit_;
  int min_throttles_per_min_;
  int max_throttles_per_min_;
  double tick_size_;
  double inverse_tick_size_;
  int bid_size_;
  int ask_size_;
  double size_multiplier_;
  bool eff_squareoff_;
  bool passive_reduce_;
  bool aggressive_reduce_;
  bool enable_bid_on_reload_;
  bool enable_ask_on_reload_;
  bool use_reserve_msg_;
  std::string sec_shortcode_;
  int unique_exec_id_;
  bool livetrading_;
  bool is_modify_before_confirmation_;
  bool is_cancellable_before_confirmation_;
  uint32_t DEP_ONE_LOT_SIZE_;
  uint32_t INDEP_ONE_LOT_SIZE_;
  HFSAT::CDef::LogBuffer order_log_buffer_;
  virtual void LoadParams() {}

 public:
  BaseExecutioner(std::string _exec_param_file, HFSAT::Watch& _watch_, HFSAT::DebugLogger& dbglogger_,
                  HFSAT::SecurityMarketView* _secondary_smv, HFSAT::BasicOrderManager* _basic_om, bool _livetrading_,
                  bool _is_modify_before_confirmation_, bool _is_cancellable_before_confirmation_,
                  TheoValues& theo_values);
  ~BaseExecutioner() {}

  void ReloadConfig();

  virtual void OnTheoUpdate() = 0;
  virtual void OnReady(const HFSAT::MarketUpdateInfo& _market_update_info_){};
  virtual void OnOrderChange(HFSAT::BaseOrder* _order_){};
  virtual void OnOrderReject(HFSAT::BaseOrder* _order_) = 0;
  void OnOrderChange(){};
  void OnOrderReject(){};
  virtual void CheckOrderStatus(HFSAT::BaseOrder* _order_){};

  void SetEfficientSquareOff(bool eff_squareoff) {
    eff_squareoff_ = eff_squareoff;
    if (!eff_squareoff_) {
      if (enable_ask_on_reload_) {
        EnableAsk();
        enable_ask_on_reload_ = false;
      }
      if (enable_bid_on_reload_) {
        EnableBid();
        enable_bid_on_reload_ = false;
      }
    }
  }

  void SetPassiveReduce(bool passive_reduce) { passive_reduce_ = passive_reduce; }
  void SetAggressiveReduce(bool agg_reduce) { aggressive_reduce_ = agg_reduce; }
  void SubscribeOM() {
    unique_exec_id_ = basic_om_->AddOrderChangeListener(this);
    basic_om_->AddOrderRejectListener(this, unique_exec_id_);
  }
  void SetInverseTradeFactor(double _inv_trade_factor_) { inv_trade_factor_ = _inv_trade_factor_; }
  virtual void TurnOff(uint16_t mask_to_unset_);
  virtual void TurnOn(uint16_t mask_to_set_);
  virtual void EnableBid();
  virtual void EnableAsk();
  virtual void DisableBid();
  virtual void DisableAsk();
  virtual void CancelBid();
  virtual void CancelAsk();

  virtual void SetSizeMultiplier(double _size_multiplier_) {
    bid_size_ =
        std::max(HFSAT::MathUtils::GetFlooredMultipleOf(bid_size_ * (_size_multiplier_ / size_multiplier_) + 0.001,
                                                        secondary_smv_->min_order_size_),
                 secondary_smv_->min_order_size_);
    ask_size_ =
        std::max(HFSAT::MathUtils::GetFlooredMultipleOf(ask_size_ * (_size_multiplier_ / size_multiplier_) + 0.001,
                                                        secondary_smv_->min_order_size_),
                 secondary_smv_->min_order_size_);
    size_multiplier_ = _size_multiplier_;
  }

  virtual void SetMaxOffset(double, double, bool a = false) {}
  virtual int GetOrderCount() { return order_count_; }
  virtual std::string GetOrderCountString() = 0;

  virtual void OnLargeDirectionalTrades(const HFSAT::TradeType_t t_trade_type_, const int t_min_price_levels_cleared_,
                                        const bool is_valid_book_end_) {}
};
