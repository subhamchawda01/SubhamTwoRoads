#pragma once

#include "tradeengine/Executioner/BaseExecutioner.hpp"
#include "tradeengine/TheoCalc/BaseTheoCalculator.hpp"

class SquareOffTheoCalculator : public BaseTheoCalculator {
  bool agg_sqoff_;
  bool is_activated_;
  int num_trades_limit_;

  void LoadParams();
  void UpdateTheoPrices(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo &_market_update_info_);
  void ComputeAndUpdateTheoListeners();
  void UpdateTheoListeners();
  void OnReady(const HFSAT::MarketUpdateInfo &_market_update_info_);

 public:
  SquareOffTheoCalculator(std::map<std::string, std::string> *key_val_map, HFSAT::Watch &_watch_,
                          HFSAT::DebugLogger &_dbglogger_, int _trading_start_utc_mfm_, int _trading_end_utc_mfm_,
                          int _aggressive_get_flat_mfm_);

  virtual ~SquareOffTheoCalculator() {}

  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo &_market_update_info_) {
    theo_values_.primary_best_bid_price_ = primary0_smv_->market_update_info().bestbid_price_;
    theo_values_.primary_best_ask_price_ = primary0_smv_->market_update_info().bestask_price_;
    if (!is_activated_) {
      CancelBid();
      CancelAsk();
      return;
    }

    UpdateTheoPrices(_security_id_, _market_update_info_);
  }

  inline void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo &_trade_print_info_,
                           const HFSAT::MarketUpdateInfo &_market_update_info_) {
    theo_values_.primary_best_bid_price_ = primary0_smv_->market_update_info().bestbid_price_;
    theo_values_.primary_best_ask_price_ = primary0_smv_->market_update_info().bestask_price_;
    if (!is_activated_) {
      CancelBid();
      CancelAsk();
      return;
    }
    total_traded_qty_ += _trade_print_info_.size_traded_;
    theo_values_.last_traded_int_price_ = _trade_print_info_.int_trade_price_;
    UpdateTheoPrices(_security_id_, _market_update_info_);
  }

  void OnExec(const int _new_position_, const int _exec_quantity_, const HFSAT::TradeType_t _buysell_,
              const double _price_, const int r_int_price_, const int _security_id_, const int _caos_);

  void Activate(int target_position) {
    if (!is_activated_) {
      InitializeDataSubscriptions();
      dbglogger_ << watch_.tv() << " Activating SQUAREOFF THEO CALCULATOR secId " << secondary_id_ << " primId "
                 << primary0_id_ << " tpos " << target_position << " cpos " << current_position_ << DBGLOG_ENDL_FLUSH;
      theo_values_.position_to_offset_ = target_position;
      // target_position_ = target_position;
      // current_position_ = current_position;
      // theo_values_.position_to_offset_ = current_position_ + target_position_;
      for (auto base_exec : base_exec_vec_) {
        base_exec->SetPassiveReduce(true);
        base_exec->SetAggressiveReduce(true);
        base_exec->TurnOn(SQUAREOFF_STATUS_SET);
      }
      SetOMSubscriptions();
      is_activated_ = true;
    }
    StartTrading();
  }

  void DeActivate() {
    if (is_activated_) {
      dbglogger_ << watch_.tv() << " Deactivating SQUAREOFF THEO CALCULATOR secId " << secondary_id_ << " primId "
                 << primary0_id_ << DBGLOG_ENDL_FLUSH;
      for (auto base_exec : base_exec_vec_) {
        base_exec->TurnOff(SQUAREOFF_STATUS_UNSET);
      }
      is_activated_ = false;
    }
  }

  // Only applicable when square off already started
  void ChangePosition(int new_position_) {
    if (is_activated_) {
      DBGLOG_TIME_CLASS_FUNC << "Changing positions SQUAREOFF THEO CALCULATOR secId " << secondary_id_ << " primId "
                             << primary0_id_ << " newpos " << new_position_ << " cpos " << current_position_
                             << DBGLOG_ENDL_FLUSH;
      theo_values_.position_to_offset_ = new_position_;
    }
  }

  void SetAggSqoff(bool _agg_sqoff_) {
    agg_sqoff_ = _agg_sqoff_;
    if (_agg_sqoff_ == true) {
      UpdateTheoPrices(GetSecondaryID(), secondary_smv_->market_update_info());
    }
  }

  void SquareOff(bool set_aggressive_ = false) {}
  void NoSquareOff() {}
};
