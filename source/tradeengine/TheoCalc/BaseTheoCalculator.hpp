#pragma once

#include <inttypes.h>
#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "baseinfra/SmartOrderRouting/base_pnl.hpp"
#include "baseinfra/TradeUtils/big_trades_listener.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "tradeengine/CommonInitializer/defines.hpp"
#include "tradeengine/Executioner/BaseExecutioner.hpp"
#include "tradeengine/Indicator/CustomPriceAggregator.hpp"

class SquareOffTheoCalculator;
class HedgeDetails;

class BaseTheoCalculator : public HFSAT::SecurityMarketViewChangeListener,
                           public HFSAT::ExecutionListener,
                           public HFSAT::BigTradesListener,
                           public HFSAT::ExchangeRejectsListener {
 private:
  void CreateExecutioner(std::string exec_name, bool _livetrading_);

 protected:
  TheoValues theo_values_;
  std::map<std::string, std::string>* key_val_map_;
  HFSAT::Watch& watch_;
  HFSAT::DebugLogger& dbglogger_;
  bool config_reloaded_;
  bool size_offset_reloaded_;
  unsigned int secondary_id_;
  unsigned int primary0_id_;
  std::vector<int> primary_id_;
  std::string theo_identifier_;
  std::vector<double> avg_vol_vec_;

  HFSAT::BasicOrderManager* basic_om_;

  CustomPriceAggregator* custom_price_aggregator_;
  int trading_start_utc_mfm_;
  int trading_end_utc_mfm_;
  int aggressive_get_flat_mfm_;
  int eff_squareoff_start_utc_mfm_;
  double bid_multiplier_;
  double ask_multiplier_;
  double bid_factor_;
  double ask_factor_;
  int runtime_id_;
  bool is_ready_;
  int num_trades_;
  int position_;
  int current_position_;
  int target_position_;
  double exposure_;
  int total_pnl_;
  int total_traded_qty_;
  double total_traded_value_;
  double strat_ltp_;
  double reference_primary_bid_;
  double reference_primary_ask_;
  bool use_position_shift_manager_;
  int position_shift_amount_;
  double bid_increase_shift_percent_;
  double bid_decrease_shift_percent_;
  double ask_increase_shift_percent_;
  double ask_decrease_shift_percent_;
  double bid_increase_shift_;
  double bid_decrease_shift_;
  double ask_increase_shift_;
  double ask_decrease_shift_;
  int bid_increase_max_shift_;
  int bid_decrease_max_shift_;
  int ask_increase_max_shift_;
  int ask_decrease_max_shift_;
  double bid_theo_shifts_;
  double ask_theo_shifts_;
  int primary0_size_filter_;
  int primary0_bid_size_filter_;
  int primary0_ask_size_filter_;
  int primary0_size_max_depth_;
  double initial_bid_increase_shift_;
  double initial_bid_decrease_shift_;
  double initial_ask_increase_shift_;
  double initial_ask_decrease_shift_;
  double initial_bid_offset_;
  double initial_ask_offset_;
  double initial_px_;
  double tick_size_;

  int min_level_clear_for_big_trade_;
  int big_trade_int_ltp_;

  bool use_delta_pos_to_shift_;

  std::vector<HedgeDetails*> hedge_vector_;
  // This is basically the indication of whether this theo
  // has a hedger ot not.
  bool need_to_hedge_;
  bool eff_squareoff_on_;

  // Banned multiplier is only for banned products and normal multiplier is for PNL/Vol scaling
  bool is_banned_prod_;
  bool use_banned_scaling_;
  bool trade_banned_prod_;
  double banned_prod_offset_multiplier_;
  double banned_prod_size_multiplier_;
  double pnl_offset_multiplier_;
  double pnl_size_multiplier_;
  double vol_offset_multiplier_;
  HFSAT::SecurityMarketView* secondary_smv_;
  HFSAT::SecurityMarketView* primary0_smv_;
  std::vector<HFSAT::SecurityMarketView*> primary_smv_vec_;

  SquareOffTheoCalculator* sqoff_theo_;
  std::vector<BaseExecutioner*> base_exec_vec_;

  virtual void LoadParams();

  // keeping track of all the possible turnoff theo scenarios
  // 1st bit : ConfigStatus
  // 2nd bit : Control Message Start/Stop
  // 3rd bit : StopLoss Check
  // 4th bit : Freeze Check
  // 5th bit : Shoot limit Check
  // 6th bit : SquareOff Check
  uint16_t status_mask_;

  double bid_percentage_offset_;
  double ask_percentage_offset_;
  double initial_bid_percentage_offset_;
  double initial_ask_percentage_offset_;
  double bid_offset_;
  double ask_offset_;

  bool start_trading_;
  bool is_agressive_getflat_;
  bool hit_stoploss_;
  bool hit_hard_stoploss_;

  bool primary_book_valid_;
  bool secondary_book_valid_;
  double spread_check_percent_;

  double inverse_tick_size_primary_;
  double avg_spread_percent_;
  int avg_int_spread_;
  double inv_avg_int_spread_;
  double long_ema_int_spread_;
  double short_ema_int_spread_;
  double long_ema_factor_spread_;
  double short_ema_factor_spread_;
  double total_offset_in_avg_spread_;
  double slope_obb_offset_mult_;
  double intercept_obb_offset_mult_;
  double obb_offset_mult_;
  bool use_spread_facor_in_obb_;

  double stop_loss_;
  double hard_stop_loss_;  // We freeze everything if this is hit
  int delta_pos_limit_;

  bool is_modify_before_confirmation_;
  bool is_cancellable_before_confirmation_;

  bool is_secondary_sqoff_needed_;  // In case hedge is already there but we also need a squareoff of primary in case of
                                    // remaining positions

  BaseTheoCalculator* parent_mm_theo_;
  HFSAT::BasePNL* sim_base_pnl_;
  int position_to_offset_;
  int remaining_pos_to_close_;  // Positions which cannot be closed by hedge
  bool close_all_positions_;  // Whether we need to close all positions in Ratio as well as Hedge Theo (Must be used in
                              // conjuction with is_secondary_sqoff_needed_)

  double mkt_percent_limit_;

  // pnl scaling vars
  bool use_pnl_scaling_;
  int pnl_threshold_to_scale_up_;
  int pnl_threshold_to_scale_down_;
  double pnl_offset_scaling_step_factor_;
  double pnl_size_scaling_step_factor_;
  double pnl_offset_multiplier_limit_;
  double pnl_inv_offset_multiplier_limit_;
  double pnl_size_multiplier_limit_;
  double pnl_inv_size_multiplier_limit_;
  int pnl_sample_window_size_msecs_;
  int msecs_for_next_pnl_sample_start_;
  double pnl_in_current_sample_;
  double pnl_last_sample_end_;
  double max_pnl_from_last_down_;

  // vol scaling vars
  bool use_vol_scaling_;
  bool use_historic_vol_;
  double vol_threshold_to_scale_up_;
  double vol_threshold_to_scale_down_;
  double vol_offset_scaling_step_factor_;
  double vol_offset_multiplier_limit_;
  double vol_inv_offset_multiplier_limit_;
  bool outside_position_shift_amount_;  // check if current position is more than position shift amount
  int vol_sample_window_size_msecs_;
  int msecs_for_next_vol_sample_start_;
  int mean_vol_in_current_sample_;
  int std_vol_in_current_sample_;
  int std_vol_in_previous_sample_;
  int vol_int_first_trade_;
  int num_vol_entries_;
  int num_long_term_vol_entries_;
  double vol_ema_;
  int vol_ema_up_threshold_;
  // int vol_ema_down_threshold_;
  double vol_ema_factor_;
  int msecs_for_vol_start_;
  bool is_historic_vol_available_;
  // int volume_in_current_sample_;
  // int volume_in_open_sample_;

  bool listen_big_trades_;
  int max_int_spread_for_mid_price_;

  bool is_primary_last_close_valid_;
  double primary_last_close_int_price_;

  bool is_secondary_last_close_valid_;
  double secondary_last_close_int_price_;

  bool are_we_using_auto_getflat_near_circuit_;
  bool are_we_using_agg_auto_getflat_near_circuit_;
  double auto_getflat_near_circuit_factor_;
  bool is_squaredoff_due_to_autogetflat_near_primary_circuit_;
  bool is_squaredoff_due_to_autogetflat_near_secondary_circuit_;
  bool is_theoturnoff_due_to_autogetflat_near_primary_circuit_;
  bool is_theoturnoff_due_to_autogetflat_near_secondary_circuit_;
  double ipo_settlement_price_;
  double ipo_circuit_;

  double primary_int_px_value_for_autogetflat_;
  double secondary_int_px_value_for_autogetflat_;

  double primary_int_px_value_for_autoresume_;
  double secondary_int_px_value_for_autoresume_;

  uint32_t DEP_ONE_LOT_SIZE_;
  uint32_t INDEP_ONE_LOT_SIZE_;

  bool using_grt_control_file_;
  std::string grt_control_filename_;
  std::map<std::string, std::string>* grt_control_key_map_;

  HFSAT::SecurityNameIndexer& sec_name_indexer_;

  HFSAT::CDef::LogBuffer exec_log_buffer_;

  SecPriceType_t GetPriceTypeFromStr(std::string pstr);
  bool CalculatePrices(double& bid_price_, double& ask_price_, HFSAT::SecurityMarketView* smv_, int vwap_levels,
                       SecPriceType_t price_type, int bid_size_filter, int ask_size_filter, int max_depth);

  void InitializeSecurityID();
  void InitializeDataSubscriptions();

 public:
  std::string ticker_name_;
  BaseTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                     HFSAT::DebugLogger& dbglogger_, int _trading_start_utc_mfm_, int _trading_end_utc_mfm_,
                     int _aggressive_get_flat_mfm_);
  BaseTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                     HFSAT::DebugLogger& dbglogger_, int _trading_start_utc_mfm_, int _trading_end_utc_mfm_,
                     int _aggressive_get_flat_mfm_, int _eff_squareoff_start_utc_mfm_, double _bid_multiplier_,
                     double _ask_multiplier_);
  virtual ~BaseTheoCalculator();

  virtual void ReloadConfig();
  virtual void printStats();

  virtual void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) {}

  virtual void OnCircuitUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) {}

  virtual void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                            const HFSAT::MarketUpdateInfo& _market_update_info_) {}

  virtual void UpdateTheoPrices(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) {
  }

  virtual void OnExec(const int _new_position_, const int _exec_quantity_, const HFSAT::TradeType_t _buysell_,
                      const double _price_, const int r_int_price_, const int _security_id_, const int _caos_) = 0;

  virtual void OnPNLUpdate(int index_, int t_pnl_, int& t_mult_pnl_, double& t_mult_risk_, int& t_port_pnl_,
                           double& t_port_risk_){};

  virtual int GetPNL() { return total_pnl_; }

  virtual double GetExposure() { return (current_position_ * strat_ltp_); }

  inline double GetTTV() { return total_traded_value_; }

  inline double GetTotalTradedTTV() { return total_traded_qty_ * strat_ltp_; }

  inline int GetVolume() {
    if (basic_om_)
      return basic_om_->trade_volume();
    else
      return 0;
  }

  inline bool IsNeedToHedge() { return need_to_hedge_; }

  inline bool IsEfficientSqaureoff() { return eff_squareoff_on_; }

  inline bool IsParentTheoPresent() { return (parent_mm_theo_ != nullptr); }
  virtual int GetPrimaryID() { return primary0_id_; }
  virtual int GetSecondaryID() { return secondary_id_; }
  virtual std::string GetSecondaryShc() { return secondary_smv_->shortcode(); }
  virtual std::string GetSecondarySecname() { return secondary_smv_->secname(); }
  virtual int GetPosition() { return current_position_; }
  virtual int GetTotalPosition() { return position_; }

  virtual void CreateAllExecutioners(HFSAT::BasicOrderManager* _basic_om, bool _livetrading_);

  void SetSquareOffTheo(SquareOffTheoCalculator* sqoff_theo) { sqoff_theo_ = sqoff_theo; }

  virtual void PNLStats(HFSAT::BulkFileWriter* trades_writer_ = nullptr, bool dump_to_cout = true);
  virtual void SetBasePNL(HFSAT::BasePNL* sim_base_pnl) { sim_base_pnl_ = sim_base_pnl; }

  void SetParentTheo(BaseTheoCalculator* parent_theo) { parent_mm_theo_ = parent_theo; }

  virtual void SetupPNLHooks() {}
  virtual void SubscribeBasePNL(HFSAT::BasePNL*) {}

  virtual void SetTargetPosition(int target_position) {}

  void SetOMSubscriptions() {
    for (auto base_exec : base_exec_vec_) {
      base_exec->SubscribeOM();
    }
  }

  virtual void SetPositionOffsets(int _position_) {
    double pos_to_shift = _position_;
    if (!use_delta_pos_to_shift_) {
      pos_to_shift = position_;
    }
    double bid_int_shifts = 0;
    double ask_int_shifts = 0;
    if (use_position_shift_manager_ && pos_to_shift != 0) {
      if (pos_to_shift < 0) {
        bid_int_shifts = (-1 * (double)pos_to_shift / (double)position_shift_amount_);
        ask_int_shifts = bid_int_shifts;
        if (bid_int_shifts > bid_increase_max_shift_) {
          bid_int_shifts = bid_increase_max_shift_;
        }
        if (ask_int_shifts > ask_increase_max_shift_) {
          ask_int_shifts = ask_increase_max_shift_;
        }
        if (bid_int_shifts > 0) {
          primary0_bid_size_filter_ = 0;
          primary0_ask_size_filter_ = primary0_size_filter_;
        } else {
          primary0_bid_size_filter_ = primary0_size_filter_;
          primary0_ask_size_filter_ = primary0_size_filter_;
        }
        bid_theo_shifts_ = bid_int_shifts * bid_increase_shift_;
        ask_theo_shifts_ = ask_int_shifts * ask_increase_shift_;
      } else {
        bid_int_shifts = ((double)pos_to_shift / (double)position_shift_amount_);
        ask_int_shifts = bid_int_shifts;
        if (bid_int_shifts > bid_decrease_max_shift_) {
          bid_int_shifts = bid_decrease_max_shift_;
        }
        if (ask_int_shifts > ask_decrease_max_shift_) {
          ask_int_shifts = ask_decrease_max_shift_;
        }
        if (bid_int_shifts > 0) {
          primary0_bid_size_filter_ = primary0_size_filter_;
          primary0_ask_size_filter_ = 0;
        } else {
          primary0_bid_size_filter_ = primary0_size_filter_;
          primary0_ask_size_filter_ = primary0_size_filter_;
        }
        bid_theo_shifts_ = -1 * bid_int_shifts * bid_decrease_shift_;
        ask_theo_shifts_ = -1 * ask_int_shifts * ask_decrease_shift_;
      }
    } else {
      bid_theo_shifts_ = 0;
      ask_theo_shifts_ = 0;
      primary0_bid_size_filter_ = primary0_size_filter_;
      primary0_ask_size_filter_ = primary0_size_filter_;
    }
    position_to_offset_ = _position_;
    // dbglogger_ << watch_.tv() << " Theo ID: " << theo_identifier_ << " Bid Shift: " << bid_theo_shifts_ << " Ask
    // "Shift: "  << ask_theo_shifts_ << " Position: " << _position_ << DBGLOG_ENDL_FLUSH;
  }

  void ConfigureHedgeDetails(std::map<std::string, BaseTheoCalculator*>& theo_map_);
  void NotifyHedgeTheoListeners();

  void TurnOffTheo(uint16_t mask_to_unset_) {
    theo_values_.is_valid_ = false;
    status_mask_ = status_mask_ & mask_to_unset_;
    for (auto base_exec : base_exec_vec_) {
      base_exec->TurnOff(mask_to_unset_);
      // base_exec->DisableAsk();
      // base_exec->DisableBid();
    }
  }

  void TurnOnTheo(uint16_t mask_to_set_) {
    status_mask_ = status_mask_ | mask_to_set_;
    for (auto base_exec : base_exec_vec_) {
      base_exec->TurnOn(mask_to_set_);
    }
  }

  void DisableBid() {
    for (auto base_exec : base_exec_vec_) {
      base_exec->DisableBid();
    }
  }

  void CancelBid() {
    for (auto base_exec : base_exec_vec_) {
      base_exec->CancelBid();
    }
  }

  void CancelAsk() {
    for (auto base_exec : base_exec_vec_) {
      base_exec->CancelAsk();
    }
  }

  void DisableAsk() {
    for (auto base_exec : base_exec_vec_) {
      base_exec->DisableAsk();
    }
  }

  void EnableBid() {
    for (auto base_exec : base_exec_vec_) {
      base_exec->EnableBid();
    }
  }

  void EnableAsk() {
    for (auto base_exec : base_exec_vec_) {
      base_exec->EnableAsk();
    }
  }

  int GetTotalOrderCount() {
    int _order_count_ = 0;
    for (auto base_exec : base_exec_vec_) {
      _order_count_ += base_exec->GetOrderCount();
    }
    return _order_count_;
    // return (basic_om_->ModifyOrderCount() + basic_om_->SendOrderCount() + basic_om_->CxlOrderCount());
  }
  
  std::string GetTotalOrderCountString(){
    std::string _order_string = "";
    for (auto base_exec : base_exec_vec_) {
      _order_string += base_exec->GetOrderCountString();
    }
    return _order_string;
  }

  virtual void SetRuntimeID(int _runtime_id_) { runtime_id_ = _runtime_id_; }

  virtual int GetRuntimeID() { return runtime_id_; }

  bool IsOnFlyModifyAllowed() { return is_modify_before_confirmation_; }
  bool IsOnFlyCancelAllowed() { return is_cancellable_before_confirmation_; }
  bool IsSecondarySqOffNeeded() { return is_secondary_sqoff_needed_; }
  bool IsBigTradesListener() { return listen_big_trades_; }

  virtual void OnLargeDirectionalTrades(const unsigned int t_security_id_, const HFSAT::TradeType_t t_trade_type_,
                                        const int t_min_price_levels_cleared_, const int last_traded_price_,
                                        const int t_cum_traded_size_, const bool is_valid_book_end_) {
    if (t_min_price_levels_cleared_ >= min_level_clear_for_big_trade_) {
      // dbglogger_ << watch_.tv() << " LargeTrade: of " <<
      // HFSAT::SecurityNameIndexer::GetUniqueInstance().GetShortcodeFromId(t_security_id_) << " BS: " <<
      // ((t_trade_type_
      // == HFSAT::TradeType_t::kTradeTypeSell) ? "SELL":"BUY")  << " MinLvlCleared: " << t_min_price_levels_cleared_ <<
      // " Sz: " << t_cum_traded_size_ << " LTP: " << last_traded_price_ << " " << last_traded_price_*0.05 << " BookEnd:
      // " << is_valid_book_end_ << "\n";
      theo_values_.is_big_trade_ = (t_trade_type_ == HFSAT::TradeType_t::kTradeTypeSell) ? -1 : 1;
      big_trade_int_ltp_ = last_traded_price_;
      UpdateTheoPrices(t_security_id_, primary0_smv_->market_update_info());
    }

    /*for (auto base_exec : base_exec_vec_) {
            base_exec->OnLargeDirectionalTrades(t_trade_type_,t_min_price_levels_cleared_,is_valid_book_end_);
    }*/
  }

  virtual void SquareOff(bool set_aggressive_ = false) = 0;
  virtual void NoSquareOff() = 0;

  virtual void UpdateTotalRisk() {}
  virtual void ChangePosition(int new_position_) {}
  void UpdateSizesAndOffsets();

  virtual void PrintBook() {
    dbglogger_ << watch_.tv() << " Printing Book " << secondary_smv_->shortcode() << " secmkt["
               << secondary_smv_->market_update_info().bestbid_price_ << " x "
               << secondary_smv_->market_update_info().bestask_price_ << "]" << DBGLOG_ENDL_FLUSH;
  }

  virtual void OnGetFreezeDueToExchangeRejects(HFSAT::BaseUtils::FreezeEnforcedReason const& freeze_reason) {
    DBGLOG_TIME_CLASS_FUNC << "Turning off theo " << theo_identifier_ << " due to "
                           << HFSAT::BaseUtils::FreezeEnforcedReasonString(freeze_reason) << DBGLOG_ENDL_FLUSH;
    if (freeze_reason == HFSAT::BaseUtils::FreezeEnforcedReason::kFreezeOnRiskCheckHit) {
      TurnOffTheo(ORSRISK_STATUS_UNSET);  // This does not get set by OnResetByManualInterventionOverRejects (have to be
                                          // done by control manager)
    } else {
      TurnOffTheo(FREEZE_STATUS_UNSET);
    }
  }

  virtual void OnResetByManualInterventionOverRejects(HFSAT::BaseUtils::FreezeEnforcedReason const& freeze_reason) {
    DBGLOG_TIME_CLASS_FUNC << "Turning on theo " << theo_identifier_ << DBGLOG_ENDL_FLUSH;
    TurnOnTheo(FREEZE_STATUS_SET);
  }

  virtual void UpdateAutoFreezeSystem(bool const& should_enforce_autofreeze) {
    if (basic_om_) basic_om_->UpdateAutoFreezeSystem(should_enforce_autofreeze);
  }
  virtual void OrderDataEntryFreezeDisable(){
    basic_om_->OrderDataEntryFreezeDisable();
  }

  virtual void ResetFreezeCounters() {
    if (basic_om_) basic_om_->ResetRejectsBasedFreeze();
  }

  virtual void ResetOMRiskCounters() {
    if (basic_om_) basic_om_->ResetRiskBasedFreeze();
    TurnOnTheo(ORSRISK_STATUS_SET);
  }

  virtual void LoadPositionCheck(std::map<std::string, std::string>& pos_key_val_map_) {
    if (basic_om_) basic_om_->LoadPositionCheck(pos_key_val_map_);
  }

  virtual void LogFullOrderStatus() {
    if (basic_om_) basic_om_->LogFullStatus();
  }

  virtual uint16_t GetStatus() { return status_mask_; }

  void StartTrading() {
    start_trading_ = true;
    is_agressive_getflat_ = false;
    bid_factor_ = 1;
    ask_factor_ = 1;
    SetEfficientSquareOff(false);
    TurnOnTheo(CTRLMSG_STATUS_SET);
  }
  void EnableEfficientSquareoff(double _factor) {
    if (start_trading_) {
      bid_factor_ = _factor;
      ask_factor_ = _factor;
      SetEfficientSquareOff(true);
    }
  }

  void SetEfficientSquareOff(bool status) {
    eff_squareoff_on_ = status;
    for (auto base_exec : base_exec_vec_) {
      base_exec->SetEfficientSquareOff(status);
    }
  }

  void SetPassiveReduce(bool status) {
    eff_squareoff_on_ = false;
    for (auto base_exec : base_exec_vec_) {
      base_exec->SetPassiveReduce(status);
      base_exec->SetEfficientSquareOff(false);
    }
  }

  void SetAggressiveReduce(bool status) {
    eff_squareoff_on_ = false;
    for (auto base_exec : base_exec_vec_) {
      base_exec->SetAggressiveReduce(status);
      base_exec->SetEfficientSquareOff(false);
    }
  }
  std::string GetTheoType() { return (*key_val_map_)["THEO_TYPE"]; }
};

class HedgeDetails {
 public:
  HedgeDetails() : hedge_status_(false), hedge_factor_(1), hedge_theo_(NULL) {}
  bool hedge_status_;
  double hedge_factor_;
  BaseTheoCalculator* hedge_theo_;
};
