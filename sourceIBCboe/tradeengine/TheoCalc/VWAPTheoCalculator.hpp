#ifndef _VWAP_THEO_CALCULATOR_H
#define _VWAP_THEO_CALCULATOR_H

#include "tradeengine/Executioner/BaseExecutioner.hpp"
#include "tradeengine/TheoCalc/BaseTheoCalculator.hpp"
#include "tradeengine/TheoCalc/SquareOffTheoCalculator.hpp"
#include "tradeengine/Utils/HelperFunctions.hpp"

class VWAPTheoCalculator : public BaseTheoCalculator, public HFSAT::BasePNLListener {
 protected:
  double max_bid_percentage_offset_;
  double max_ask_percentage_offset_;
  double max_bid_offset_;
  double max_ask_offset_;
  double dynamic_bid_offset_;
  double dynamic_ask_offset_;
  double bid_increase_shift_percent_of_offset_;
  double bid_decrease_shift_percent_of_offset_;
  double ask_increase_shift_percent_of_offset_;
  double ask_decrease_shift_percent_of_offset_;
  int64_t offset_decay_start_time_secs_;
  double bid_offset_slope_;
  double ask_offset_slope_;
  bool passive_reduce_position_;
  bool aggressive_reduce_position_;

  double estimated_vwap_price_;
  double estimated_bid_price_;
  double estimated_ask_price_;

  int64_t last_traded_primary_usecs_;
  int last_traded_primary_int_price_;

  double primary_traded_value_;
  double primary_traded_volume_;
  double estimated_volume_;
  double estimated_traded_value_;
  int64_t total_time_period_usecs_;
  int64_t data_listen_start_time_;
  int64_t data_listen_end_time_secs_;
  int64_t time_elapsed_usecs_;
  int64_t time_remaining_usecs_;

  void LoadParams();
  virtual void UpdateTheoPrices(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);
  void ComputeAndUpdateTheoListeners();
  void UpdateTheoListeners();
  void OnReady(const HFSAT::MarketUpdateInfo& _market_update_info_);

 public:
  VWAPTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                     HFSAT::DebugLogger& _dbglogger_, int _trading_start_utc_mfm_, int _trading_end_utc_mfm_,
                     int _aggressive_get_flat_mfm_, int eff_squareoff_start_utc_mfm_ = 0, double bid_multiplier_ = 1,
                     double ask_multiplier_ = 1);

  virtual ~VWAPTheoCalculator() {}

  void SetupPNLHooks();
  virtual int total_pnl() { return total_pnl_; }

  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) {
    if ((hit_stoploss_) && (!hit_hard_stoploss_)) {
      // If loss check is set, unset it
      if (status_mask_ & LOSS_STATUS_SET) {
        dbglogger_ << watch_.tv() << " HITTING STOP LOSS " << secondary_smv_->shortcode() << " PNL: " << total_pnl_
                   << " SL: " << stop_loss_ << " secmkt[" << secondary_smv_->market_update_info().bestbid_price_
                   << " x " << secondary_smv_->market_update_info().bestask_price_ << "] primkt["
                   << theo_values_.primary_best_bid_price_ << " x " << theo_values_.primary_best_ask_price_ << "] "
                   << DBGLOG_ENDL_FLUSH;
        SquareOff();
      }
      TurnOffTheo(LOSS_STATUS_UNSET);
    }

    UpdateTheoPrices(_security_id_, _market_update_info_);
  }

  inline void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                           const HFSAT::MarketUpdateInfo& _market_update_info_) {
    if (watch_.usecs_from_midnight() > data_listen_start_time_) {
      if (_security_id_ == secondary_id_) {
        total_traded_qty_ += _trade_print_info_.size_traded_;
        theo_values_.last_traded_int_price_ = _trade_print_info_.int_trade_price_;
      } else {
        // dbglogger_ << watch_.tv() << " OnTrade secid " << _security_id_ << " " << _trade_print_info_.trade_price_ <<
        // " " <<  _trade_print_info_.size_traded_ << " side " << _trade_print_info_.buysell_ << DBGLOG_ENDL_FLUSH;

        // Offsets are linearly reduced with time after offset_decay_start_time
        // starting from max_bid_percentage_offset to bid_percentage_offset
        if (watch_.msecs_from_midnight() / 1000 > offset_decay_start_time_secs_) {
          int64_t time_elapsed_secs = (watch_.msecs_from_midnight() / 1000) - offset_decay_start_time_secs_;
          dynamic_bid_offset_ = max_bid_offset_ - bid_offset_slope_ * time_elapsed_secs;
          dynamic_ask_offset_ = max_ask_offset_ - ask_offset_slope_ * time_elapsed_secs;
          // dbglogger_ << watch_.tv() << " offset bid:" <<  dynamic_bid_offset_ << " ask:" << dynamic_ask_offset_ << "
          // timelapsed:" << time_elapsed_secs <<  " bidslope:" << bid_offset_slope_ << " askslope:" <<
          // ask_offset_slope_ << DBGLOG_ENDL_FLUSH;
        } else {
          dynamic_bid_offset_ = max_bid_offset_;
          dynamic_ask_offset_ = max_ask_offset_;
        }
        primary_traded_value_ += (_trade_print_info_.size_traded_ * _trade_print_info_.trade_price_);
        primary_traded_volume_ += _trade_print_info_.size_traded_;
        time_elapsed_usecs_ = watch_.usecs_from_midnight() - data_listen_start_time_;  // 1500 time
        time_remaining_usecs_ = total_time_period_usecs_ - time_elapsed_usecs_;

        estimated_volume_ = primary_traded_volume_ * (time_remaining_usecs_ / time_elapsed_usecs_);
        // TODO Try using mid price here instead of LTP
        estimated_traded_value_ = _trade_print_info_.trade_price_ * estimated_volume_;
        estimated_vwap_price_ =
            (estimated_traded_value_ + primary_traded_value_) / (estimated_volume_ + primary_traded_volume_);
        /*dbglogger_ << watch_.tv() << " Estimated vwap price "
                << " Sec: " << sec_name_indexer_.GetShortcodeFromId(_security_id_)
                << " " << estimated_vwap_price_ << DBGLOG_ENDL_FLUSH;*/
      }
    }
    UpdateTheoPrices(_security_id_, _market_update_info_);
  }

  void OnExec(const int _new_position_, const int _exec_quantity_, const HFSAT::TradeType_t _buysell_,
              const double _price_, const int r_int_price_, const int _security_id_, const int _caos_);

  void SetPositionOffsets(int _position_) {
    int bid_int_shifts = 0;
    int ask_int_shifts = 0;
    if (use_position_shift_manager_ && _position_ != 0) {
      if (_position_ < 0) {
        bid_int_shifts = (int)(-1 * (double)_position_ / (double)position_shift_amount_);
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
        bid_theo_shifts_ = bid_int_shifts * bid_increase_shift_percent_of_offset_;
        ask_theo_shifts_ = ask_int_shifts * ask_increase_shift_percent_of_offset_;
      } else {
        bid_int_shifts = (int)((double)_position_ / (double)position_shift_amount_);
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
        bid_theo_shifts_ = -1 * bid_int_shifts * bid_decrease_shift_percent_of_offset_;
        ask_theo_shifts_ = -1 * ask_int_shifts * ask_decrease_shift_percent_of_offset_;
      }
    } else {
      bid_theo_shifts_ = 0;
      ask_theo_shifts_ = 0;
      primary0_bid_size_filter_ = primary0_size_filter_;
      primary0_ask_size_filter_ = primary0_size_filter_;
    }
    theo_values_.position_to_offset_ = _position_;
    // dbglogger_ << watch_.tv() << " Theo ID: " << theo_identifier_ << " Bid Shift: " << bid_theo_shifts_ << " Ask
    // Shift: "  << ask_theo_shifts_ << " Position: " << _position_ << DBGLOG_ENDL_FLUSH;
  }

  virtual void OnPNLUpdate(int index_, int t_pnl_, int& t_mult_pnl_, double& t_mult_risk_, int& t_port_pnl_,
                           double& t_port_risk_) {
    if (t_pnl_ < hard_stop_loss_) {
      hit_hard_stoploss_ = true;
      if (status_mask_ & HARDLOSS_STATUS_SET) {
        dbglogger_ << watch_.tv() << " HITTING HARD STOP LOSS " << secondary_smv_->shortcode() << " PNL: " << t_pnl_
                   << " SL: " << stop_loss_ << " secmkt[" << secondary_smv_->market_update_info().bestbid_price_
                   << " x " << secondary_smv_->market_update_info().bestask_price_ << "] primkt["
                   << theo_values_.primary_best_bid_price_ << " x " << theo_values_.primary_best_ask_price_ << "] "
                   << DBGLOG_ENDL_FLUSH;
      }
      TurnOffTheo(HARDLOSS_STATUS_UNSET);
      if (sqoff_theo_) {
        sqoff_theo_->TurnOffTheo(HARDLOSS_STATUS_UNSET);
      }
    } else if (!hit_hard_stoploss_) {
      if (t_pnl_ < stop_loss_) {
        hit_stoploss_ = true;
      }
    }
    total_pnl_ = t_pnl_;
  }

  void SquareOff(bool set_aggressive_ = false) {
    // Ratio Theo will not square off in case of hedge.
    // Hedge Theo will square off the delta position
    if (need_to_hedge_ && !is_secondary_sqoff_needed_) return;
    if (!sqoff_theo_) {
      DBGLOG_TIME_CLASS_FUNC << "NO SQUARE OFF THEO PRESENT for secID " << secondary_id_ << DBGLOG_ENDL_FLUSH;
      return;
    }
    dbglogger_ << watch_.tv() << " SQUARING OFF SECONDARY " << sec_name_indexer_.GetShortcodeFromId(secondary_id_)
               << DBGLOG_ENDL_FLUSH;
    basic_om_->AddExecutionListener((BaseTheoCalculator*)sqoff_theo_);
    if (need_to_hedge_) {
      sqoff_theo_->Activate(remaining_pos_to_close_);
    } else {
      sqoff_theo_->Activate(current_position_);
    }
    sqoff_theo_->SetAggSqoff(set_aggressive_);
  }

  void NoSquareOff() {
    // Ratio Theo will not square off in case of hedge.
    // Hedge Theo will square off the delta position
    if (need_to_hedge_ && !is_secondary_sqoff_needed_) return;
    if ((status_mask_ & NOSQUAREOFF_BITS_SET) != NOSQUAREOFF_BITS_SET) {
      DBGLOG_TIME_CLASS_FUNC << "Not deactivatigng square off for " << secondary_id_ << " since status mask "
                             << status_mask_ << "has other nosquare off related bits unset" << DBGLOG_ENDL_FLUSH;
      return;
    }
    if (!sqoff_theo_) {
      DBGLOG_TIME_CLASS_FUNC << "NO SQUARE OFF THEO PRESENT for secID " << secondary_id_ << DBGLOG_ENDL_FLUSH;
      return;
    }
    DBGLOG_TIME_CLASS_FUNC << "STOPPING SQUARING OFF SECONDARY " << sec_name_indexer_.GetShortcodeFromId(secondary_id_)
                           << " STATUS MASK: " << status_mask_ << DBGLOG_ENDL_FLUSH;
    basic_om_->RemoveExecutionLister((BaseTheoCalculator*)sqoff_theo_);
    sqoff_theo_->DeActivate();
  }
};

#endif  // _VWAP_THEO_CALCULATOR_H
