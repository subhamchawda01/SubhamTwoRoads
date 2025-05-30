#pragma once

#include "tradeengine/Executioner/BaseExecutioner.hpp"
#include "tradeengine/TheoCalc/BaseTheoCalculator.hpp"
#include "tradeengine/TheoCalc/SquareOffTheoCalculator.hpp"
#include "tradeengine/Utils/HelperFunctions.hpp"
#include "tradeengine/Utils/BarGenerator.hpp"

class RatioTheoCalculator : public BaseTheoCalculator, public HFSAT::BasePNLListener {
 protected:
  double alpha_;
  int ratio_update_time_;
  std::string primary0_price_type_str_;
  SecPriceType_t primary0_price_type_;
  int primary0_vwap_levels_;

  double min_primary_spread_;
  double max_primary_spread_;
  double min_secondary_spread_;
  double max_secondary_spread_;

  bool passive_reduce_position_;
  bool aggressive_reduce_position_;

  double min_ratio_;
  double max_ratio_;

  bool use_constant_ratio_;
  double constant_ratio_;

  bool use_start_ratio_;
  double start_ratio_;
  double ratio_diff_offset_;
  int start_ratio_end_mfm_;

  double estimated_bid_price_;
  double estimated_ask_price_;

  double ratio_;
  int last_ratio_update_time_;

  // Params related to sweep in market
  bool allow_sweep_mode_;
  bool use_sweep_price_;
  double min_lots_traded_for_sweep_mode_;
  double min_percent_price_move_for_sweep_mode_;
  double min_price_move_for_sweep_mode_;
  double min_level_cleared_for_sweep_mode_;
  int max_usecs_allowed_between_sweep_trades_;
  int max_msecs_between_trades_sweep_mode_on;
  double buy_sweep_bid_offset_percent_;
  double buy_sweep_ask_offset_percent_;
  double sell_sweep_bid_offset_percent_;
  double sell_sweep_ask_offset_percent_;
  double buy_sweep_bid_offset_;
  double buy_sweep_ask_offset_;
  double sell_sweep_bid_offset_;
  double sell_sweep_ask_offset_;

  // To ensure cases such as very wide sweeps don't
  // negatively affect the prices
  double max_bid_offset_sweep_percent_;
  double max_ask_offset_sweep_percent_;
  double max_bid_offset_sweep_;
  double max_ask_offset_sweep_;

  // The fraction of ltp price above max bid/ask sweep offset
  // to be considered to determine fair price
  double fraction_above_max_sweep_offset_;

  // Variables related to sweep in market
  HFSAT::TradeType_t sweep_side_;
  double lots_traded_in_current_sweep_;
  double price_move_in_current_sweep_;
  double first_sweep_traded_price_;
  int64_t last_traded_primary_usecs_;
  int last_traded_primary_int_price_;
  int last_primary_bid_int_price_;
  int last_primary_ask_int_price_;
  double midterm_pos_offset_;
  bool sleep_after_ready_;
  bool sleep_called_;
  int training_msecs_after_start_ratio_;
  double prev_day_close_;
  bool gen_price_returns_;
  int train_sample_window_size_msecs_;
  int msecs_for_next_train_sample_start_;
  double mean_k_;
  int count_k_;
  double theo_k_;
  double max_px_percent_threshold_;
  double mean_volume_;
  double volume_filter_;
  double theo_decay_factor_;
  double hist_mean_;
  double hist_std_;
  double hist_factor_;
  bool after_first_trade_;
  bool spread_check_;
  double day_open_px_;
  std::vector<double> return_array_;
  std::vector<double> primary_smv_wt_vec_;
  std::vector<double> last_px_vec_;
  std::vector<double> primary_smv_volume_vec_;
  std::vector<double> primary_smv_curr_volume_vec_;
  std::vector<std::string> future_smv_vec_;

  std::deque<double> training_ratio_values_;
  void LoadParams();
  virtual void UpdateTheoPrices(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);
  void ComputeAndUpdateTheoListeners();
  void UpdateTheoListeners();
  void OnReady(const HFSAT::MarketUpdateInfo& _market_update_info_);

 public:
  RatioTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                      HFSAT::DebugLogger& dbglogger_, int _trading_start_utc_mfm_, int _trading_end_utc_mfm_,
                      int _aggressive_get_flat_mfm_, int eff_squareoff_start_utc_mfm_, double bid_multiplier_,
                      double ask_multiplier_);

  virtual ~RatioTheoCalculator() {}

  void SetupPNLHooks();
  virtual int total_pnl() { return total_pnl_; }

  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) {
    theo_values_.is_big_trade_ = 0;
    // dbglogger_ << watch_.tv() << " OnMarketUpdate" << DBGLOG_ENDL_FLUSH;
    // dbglogger_ << watch_.tv() << primary0_smv_->ShowMarket() << DBGLOG_ENDL_FLUSH;
    // dbglogger_ << watch_.tv() << secondary_smv_->ShowMarket() << DBGLOG_ENDL_FLUSH;
    theo_values_.primary_best_bid_price_ = primary0_smv_->market_update_info().bestbid_price_;
    theo_values_.primary_best_ask_price_ = primary0_smv_->market_update_info().bestask_price_;
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

    if ((_security_id_ == primary0_id_) && (allow_sweep_mode_) && (theo_values_.sweep_mode_active_ != 0)) {
      if (((watch_.usecs_from_midnight() - last_traded_primary_usecs_) >
           max_msecs_between_trades_sweep_mode_on * 1000) ||
          ((sweep_side_ == HFSAT::TradeType_t::kTradeTypeBuy)
               ? (_market_update_info_.bestask_int_price_ < last_traded_primary_int_price_ ||
                  _market_update_info_.bestbid_int_price_ < last_primary_bid_int_price_)
               : (_market_update_info_.bestbid_int_price_ > last_traded_primary_int_price_ ||
                  _market_update_info_.bestask_int_price_ > last_primary_ask_int_price_))) {
#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
        if (theo_values_.sweep_mode_active_ != 0)
          DBGLOG_TIME_CLASS_FUNC << theo_identifier_ << " SWEEP MODE DEACTIVATED" << DBGLOG_ENDL_FLUSH;
#endif
        theo_values_.sweep_mode_active_ = 0;
        sweep_side_ = HFSAT::TradeType_t::kTradeTypeNoInfo;
      }
    }

    UpdateTheoPrices(_security_id_, _market_update_info_);
    last_primary_bid_int_price_ = primary0_smv_->market_update_info().bestbid_int_price_;
    last_primary_ask_int_price_ = primary0_smv_->market_update_info().bestask_int_price_;
    if(gen_price_returns_ && primary_smv_vec_.size() > 1){
      TrainWeights(_security_id_, _market_update_info_);
    }
  }

  inline void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                           const HFSAT::MarketUpdateInfo& _market_update_info_) {
    theo_values_.is_big_trade_ = 0;
    // dbglogger_ << watch_.tv() << " OnTradeUpdate" << DBGLOG_ENDL_FLUSH;
    // dbglogger_ << watch_.tv() << primary0_smv_->ShowMarket() << DBGLOG_ENDL_FLUSH;
    // dbglogger_ << watch_.tv() << secondary_smv_->ShowMarket() << DBGLOG_ENDL_FLUSH;
    // dbglogger_ << watch_.tv() << " OnTrade secid " << _security_id_ << " " << _trade_print_info_.trade_price_ << " "
    // <<  _trade_print_info_.size_traded_ << " side " << _trade_print_info_.buysell_ << DBGLOG_ENDL_FLUSH;
    theo_values_.primary_best_bid_price_ = primary0_smv_->market_update_info().bestbid_price_;
    theo_values_.primary_best_ask_price_ = primary0_smv_->market_update_info().bestask_price_;
    if (_security_id_ == secondary_id_) {
      total_traded_qty_ += _trade_print_info_.size_traded_;
      theo_values_.last_traded_int_price_ = _trade_print_info_.int_trade_price_;
    }

    if ((_security_id_ == primary0_id_) && allow_sweep_mode_ && is_ready_) {
      if (_trade_print_info_.num_levels_cleared_ >= min_level_cleared_for_sweep_mode_) {
        if ((theo_values_.sweep_mode_active_ == 0) || (sweep_side_ != _trade_print_info_.buysell_)) {
          sweep_side_ = _trade_print_info_.buysell_;
#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
          if (theo_values_.sweep_mode_active_ != 0) {
            DBGLOG_TIME_CLASS_FUNC << theo_identifier_ << " SWEEP MODE DEACTIVATED" << DBGLOG_ENDL_FLUSH;
          }
          DBGLOG_TIME_CLASS_FUNC << theo_identifier_ << " SWEEP MODE ACTIVATED " << HFSAT::GetTradeTypeChar(sweep_side_)
                                 << " NumLevelsCleared: " << _trade_print_info_.num_levels_cleared_
                                 << DBGLOG_ENDL_FLUSH;
#endif
          theo_values_.sweep_mode_active_ = (sweep_side_ == HFSAT::TradeType_t::kTradeTypeSell) ? -1 : 1;
        }
      } else {
        if ((sweep_side_ != _trade_print_info_.buysell_) ||
            ((theo_values_.sweep_mode_active_ == 0) ? ((watch_.usecs_from_midnight() - last_traded_primary_usecs_) >
                                                       max_usecs_allowed_between_sweep_trades_)
                                                    : ((watch_.usecs_from_midnight() - last_traded_primary_usecs_) >
                                                       max_msecs_between_trades_sweep_mode_on * 1000)) ||
            ((sweep_side_ == HFSAT::TradeType_t::kTradeTypeBuy)
                 ? (_market_update_info_.bestask_int_price_ < last_traded_primary_int_price_)
                 : (_market_update_info_.bestbid_int_price_ > last_traded_primary_int_price_))) {
#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
          if (theo_values_.sweep_mode_active_ != 0) {
            DBGLOG_TIME_CLASS_FUNC << theo_identifier_ << " SWEEP MODE DEACTIVATED" << DBGLOG_ENDL_FLUSH;
          }
#endif
          theo_values_.sweep_mode_active_ = 0;
          first_sweep_traded_price_ = _trade_print_info_.trade_price_;
          sweep_side_ = _trade_print_info_.buysell_;
          lots_traded_in_current_sweep_ = _trade_print_info_.size_traded_ / primary0_smv_->min_order_size();
          price_move_in_current_sweep_ = 0;
        } else if (theo_values_.sweep_mode_active_ == 0) {
          lots_traded_in_current_sweep_ += _trade_print_info_.size_traded_ / primary0_smv_->min_order_size();
          price_move_in_current_sweep_ = _trade_print_info_.trade_price_ - first_sweep_traded_price_;
          // Sweep side indicates aggressive trades' side
          if (sweep_side_ == HFSAT::TradeType_t::kTradeTypeSell) {
            price_move_in_current_sweep_ = -1 * price_move_in_current_sweep_;
          }
          if ((lots_traded_in_current_sweep_ >= min_lots_traded_for_sweep_mode_) &&
              (price_move_in_current_sweep_ >= min_price_move_for_sweep_mode_)) {
#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
            DBGLOG_TIME_CLASS_FUNC << theo_identifier_ << " SWEEP MODE ACTIVATED "
                                   << HFSAT::GetTradeTypeChar(sweep_side_)
                                   << " LTP: " << _trade_print_info_.trade_price_
                                   << " FP: " << first_sweep_traded_price_ << DBGLOG_ENDL_FLUSH;
#endif
            theo_values_.sweep_mode_active_ = (sweep_side_ == HFSAT::TradeType_t::kTradeTypeSell) ? -1 : 1;
          }
        }
        last_traded_primary_int_price_ = _trade_print_info_.int_trade_price_;
        last_traded_primary_usecs_ = watch_.usecs_from_midnight();
      }
    }
    UpdateTheoPrices(_security_id_, _market_update_info_);
    last_primary_bid_int_price_ = primary0_smv_->market_update_info().bestbid_int_price_;
    last_primary_ask_int_price_ = primary0_smv_->market_update_info().bestask_int_price_;
    // Switching vol scaling on midday is not possible
    // Remove use_vol_scaling condition here to reenable it
    if (use_vol_scaling_ && _security_id_ == primary0_id_) {
      updateVolOffset(_trade_print_info_.int_trade_price_, _trade_print_info_.size_traded_);
    }
    if(gen_price_returns_ && primary_smv_vec_.size() > 1){
      UpdateVolume(_security_id_, _trade_print_info_.size_traded_);
    }
  }

  inline void TrainWeights(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) {
    int msecs_from_midnight = watch_.msecs_from_midnight();
    if (msecs_from_midnight > msecs_for_next_train_sample_start_ ) {
      if(count_k_){
        mean_k_ /= count_k_;
      }
      dbglogger_ << watch_.tv() << " " << secondary_smv_->shortcode() << " " << mean_k_ << DBGLOG_ENDL_FLUSH;
      if(theo_k_ == 0){
        theo_k_ = mean_k_;
      }
      theo_k_ = theo_decay_factor_*theo_k_  + (1- theo_decay_factor_)*mean_k_;
      mean_k_ = 0;
      count_k_ = 0;
      msecs_for_next_train_sample_start_ = msecs_from_midnight + train_sample_window_size_msecs_;
      for (unsigned int i = 0;i< primary_smv_vec_.size() ;i++) {
        primary_smv_curr_volume_vec_[i] = 0;
      }
    }
    // if (msecs_from_midnight < msecs_for_next_train_sample_start_) {
    double current_k_ = 0;
    // return;
    for (unsigned int i = 1;i< primary_smv_vec_.size() ;i++) {
      if (primary_smv_vec_[i]->security_id() == _security_id_){
        last_px_vec_[i] = _market_update_info_.mid_price_;

      }
      if(last_px_vec_[i] < 0 ){
        // dbglogger_ << watch_.tv() << " " <<  secondary_smv_->shortcode() << " " << primary_smv_vec_[i]->shortcode() << " " << last_px_vec_[i] << DBGLOG_ENDL_FLUSH;
        current_k_ = 0;
        break;
      }
      current_k_ += last_px_vec_[i]*primary_smv_wt_vec_[i];
    }
    if(current_k_ && secondary_smv_->market_update_info().mid_price_ > 0){
      if(day_open_px_ == 0){
        day_open_px_ = secondary_smv_->market_update_info().mid_price_;
      }
      current_k_ = secondary_smv_->market_update_info().mid_price_ / current_k_;
      mean_k_ += current_k_;
      count_k_++;
    }
    // }
    // for (unsigned int i = 1;i< primary_smv_vec_.size() ;i++) {
    //   std::cout << " " << primary_smv_vec_[i]->shortcode() << " " << primary_smv_wt_vec_[i] << " ";
    // }
    // std::cout << std::endl;
  }

  inline void UpdateVolume(const unsigned int _security_id_, const unsigned int _size_traded_){
    for (unsigned int i = 0;i< primary_smv_vec_.size() ;i++) {
      if (primary_smv_vec_[i]->security_id() == _security_id_){
        primary_smv_curr_volume_vec_[i] += _size_traded_;

      }
    }
  }

  inline void updateVolOffset(int _trade_px_, int _size_traded_) {
    int msecs_from_midnight = watch_.msecs_from_midnight();
    if (msecs_from_midnight < msecs_for_vol_start_) {
      return;
    }

    // dbglogger_<< watch_.tv() << " BAR " << ticker_name_ << " " << _trade_px_ << DBGLOG_ENDL_FLUSH;
    if (msecs_from_midnight > msecs_for_next_vol_sample_start_ && num_vol_entries_) {
      int mean_ = mean_vol_in_current_sample_ / num_vol_entries_;
      int curr_std_ = (std_vol_in_current_sample_ - num_vol_entries_ * mean_ * mean_) / num_vol_entries_;
      curr_std_--;
      // if(volume_in_open_sample_ == 0){
      // 	volume_in_open_sample_ = volume_in_current_sample_;
      // }

      if (vol_ema_ == 0) {
        vol_ema_ = curr_std_;
        // if(is_historic_vol_available_ && avg_vol_vec_.size() !=0 ){
        // 	if(vol_ema_ > avg_vol_vec_[0]){
        // 		vol_ema_ = avg_vol_vec_[0];
        // 		// dbglogger_ << watch_.tv() << " init_val "<< ticker_name_ << " " << vol_ema_ <<
        // DBGLOG_ENDL_FLUSH;
        // 	}
        // }
      } else if (num_vol_entries_ > 5 &&
                 msecs_from_midnight < msecs_for_next_vol_sample_start_ + vol_sample_window_size_msecs_) {
        if (curr_std_ < vol_ema_ * vol_ema_up_threshold_ && curr_std_ > 0) {
          vol_ema_ = vol_ema_ * vol_ema_factor_ + (1 - vol_ema_factor_) * curr_std_;
        }
      }
      if (use_vol_scaling_) {
        // if(mean_long_term_vol_in_current_sample_ &&  )
        // vol_offset_multiplier_ *= vol_offset_scaling_step_factor_;
        if (dbglogger_.CheckLoggingLevel(VOL_INFO)) {
          dbglogger_ << "STDVOL " << ticker_name_ << " " << msecs_from_midnight / 60000 << " " << curr_std_
                     << DBGLOG_ENDL_FLUSH;
        }
        // dbglogger_<< watch_.tv() << "  "<< ticker_name_ << " Current Stdev: "  <<curr_std_ << " ema: " << vol_ema_ <<
        // " num_vol_entries_: " << num_vol_entries_ << DBGLOG_ENDL_FLUSH;

        if (curr_std_ > vol_threshold_to_scale_up_ * vol_ema_ && num_vol_entries_ > 5 &&
            curr_std_ > std_vol_in_previous_sample_ &&
            msecs_from_midnight < msecs_for_next_vol_sample_start_ + vol_sample_window_size_msecs_) {
          if (vol_offset_multiplier_ < vol_offset_multiplier_limit_) {
            vol_offset_multiplier_ *= vol_offset_scaling_step_factor_;
            if (vol_offset_multiplier_ > vol_offset_multiplier_limit_) {
              vol_offset_multiplier_ = vol_offset_multiplier_limit_;
            }
            dbglogger_ << watch_.tv() << "  " << ticker_name_ << " Offset increased to " << vol_offset_multiplier_
                       << DBGLOG_ENDL_FLUSH;
            UpdateSizesAndOffsets();
          }
        } else if ((curr_std_ < vol_threshold_to_scale_down_ * vol_ema_ && num_vol_entries_ > 5 &&
                    msecs_from_midnight < msecs_for_next_vol_sample_start_ + vol_sample_window_size_msecs_) ||
                   num_vol_entries_ <= 5 ||
                   msecs_from_midnight > msecs_for_next_vol_sample_start_ + vol_sample_window_size_msecs_) {
          if (vol_offset_multiplier_ > vol_inv_offset_multiplier_limit_) {
            vol_offset_multiplier_ /= vol_offset_scaling_step_factor_;
            if (vol_offset_multiplier_ < vol_inv_offset_multiplier_limit_) {
              vol_offset_multiplier_ = vol_inv_offset_multiplier_limit_;
            }
            dbglogger_ << watch_.tv() << "  " << ticker_name_ << " Offset decreased to " << vol_offset_multiplier_
                       << DBGLOG_ENDL_FLUSH;
            UpdateSizesAndOffsets();
          }
        }
        std_vol_in_previous_sample_ = curr_std_;
      }

      mean_vol_in_current_sample_ = 0;
      msecs_for_next_vol_sample_start_ = msecs_from_midnight + vol_sample_window_size_msecs_;
      vol_int_first_trade_ = _trade_px_;
      num_vol_entries_ = 1;
      // volume_in_current_sample_ = _size_traded_;
      std_vol_in_current_sample_ = 0;
    } else {
      if (!num_vol_entries_) {
        vol_int_first_trade_ = _trade_px_;
        num_vol_entries_ = 1;
        // volume_in_current_sample_ = _size_traded_;
      } else {
        int diff_trade_ = _trade_px_ - vol_int_first_trade_;
        mean_vol_in_current_sample_ += diff_trade_;
        std_vol_in_current_sample_ += diff_trade_ * diff_trade_;
        num_vol_entries_++;
        // volume_in_current_sample_ += _size_traded_;
      }
    }
  }

  void OnExec(const int _new_position_, const int _exec_quantity_, const HFSAT::TradeType_t _buysell_,
              const double _price_, const int r_int_price_, const int _security_id_, const int _caos_);

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

    if (use_pnl_scaling_) {
      pnl_in_current_sample_ = t_pnl_ - pnl_last_sample_end_;
      if (watch_.msecs_from_midnight() > msecs_for_next_pnl_sample_start_) {
        // dbglogger_ << watch_.tv() << " " << ticker_name_ << " pnl_cycle " << pnl_in_current_sample_ << " " <<
        // pnl_threshold_to_scale_up_*pnl_offset_multiplier_ << " " << pnl_last_sample_end_ <<DBGLOG_ENDL_FLUSH;
        msecs_for_next_pnl_sample_start_ = (watch_.msecs_from_midnight() + pnl_sample_window_size_msecs_);
        pnl_last_sample_end_ = t_pnl_;
        if (max_pnl_from_last_down_ < t_pnl_) {
          max_pnl_from_last_down_ = t_pnl_;
        }
      }
      int volume_ = basic_om_->trade_volume();

      if (pnl_in_current_sample_ > pnl_threshold_to_scale_up_ * pnl_size_multiplier_ &&
          (pnl_offset_multiplier_ > pnl_inv_offset_multiplier_limit_ ||
           pnl_size_multiplier_ < pnl_size_multiplier_limit_) &&
          (!total_traded_qty_ || (double)volume_ * 100 / (double)total_traded_qty_ < mkt_percent_limit_)) {
        // dbglogger_ << watch_.tv() << " " << ticker_name_ << "mktpt " << volume_ << " " << total_traded_qty_
        // <<DBGLOG_ENDL_FLUSH;
        dbglogger_ << watch_.tv() << " " << ticker_name_ << " pnl_up " << pnl_in_current_sample_ << " "
                   << pnl_threshold_to_scale_up_ * pnl_size_multiplier_ << " " << pnl_last_sample_end_
                   << DBGLOG_ENDL_FLUSH;
        pnl_offset_multiplier_ /= pnl_offset_scaling_step_factor_;
        pnl_size_multiplier_ *= pnl_size_scaling_step_factor_;
        pnl_offset_multiplier_ =
            std::max(std::min(pnl_offset_multiplier_, pnl_offset_multiplier_limit_), pnl_inv_offset_multiplier_limit_);
        msecs_for_next_pnl_sample_start_ = (watch_.msecs_from_midnight() + pnl_sample_window_size_msecs_);
        pnl_last_sample_end_ = t_pnl_;
        if (max_pnl_from_last_down_ < t_pnl_) {
          max_pnl_from_last_down_ = t_pnl_;
        }
        UpdateSizesAndOffsets();

      } else if (t_pnl_ - max_pnl_from_last_down_ < pnl_threshold_to_scale_down_ * pnl_size_multiplier_ &&
                 (!position_shift_amount_ ||
                  (position_ < position_shift_amount_ && position_ > -1 * position_shift_amount_)) &&
                 (pnl_offset_multiplier_ < pnl_offset_multiplier_limit_ ||
                  pnl_size_multiplier_ > pnl_inv_size_multiplier_limit_)) {
        dbglogger_ << watch_.tv() << " " << ticker_name_ << " pnl_down " << t_pnl_ - max_pnl_from_last_down_ << " "
                   << pnl_threshold_to_scale_down_ * pnl_size_multiplier_ << " " << max_pnl_from_last_down_
                   << DBGLOG_ENDL_FLUSH;
        // } else if(pnl_in_current_sample_ < pnl_threshold_to_scale_down_*pnl_size_multiplier_ &&
        // (pnl_offset_multiplier_ < pnl_offset_multiplier_limit_ || pnl_size_multiplier_ >
        // pnl_inv_size_multiplier_limit_)) { dbglogger_ << watch_.tv() << " " << ticker_name_ << " pnl_down " <<
        // pnl_in_current_sample_ << " " << pnl_threshold_to_scale_down_*pnl_size_multiplier_ << " " <<
        // pnl_last_sample_end_ <<DBGLOG_ENDL_FLUSH;
        pnl_offset_multiplier_ *= pnl_offset_scaling_step_factor_;
        pnl_size_multiplier_ /= pnl_size_scaling_step_factor_;
        pnl_offset_multiplier_ =
            std::max(std::min(pnl_offset_multiplier_, pnl_offset_multiplier_limit_), pnl_inv_offset_multiplier_limit_);
        msecs_for_next_pnl_sample_start_ = (watch_.msecs_from_midnight() + pnl_sample_window_size_msecs_);
        pnl_last_sample_end_ = t_pnl_;
        max_pnl_from_last_down_ = t_pnl_;

        UpdateSizesAndOffsets();
      }
    }
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
               << " " << current_position_ << DBGLOG_ENDL_FLUSH;
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
      DBGLOG_TIME_CLASS_FUNC << "Not deactivating square off for " << secondary_id_ << " since status mask "
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

  std::vector<double> GetReturnVec(){ return return_array_; }
  HFSAT::SecurityMarketView* GetSecondarySMV(){ return secondary_smv_; } 
  void AddPrimarySMV(HFSAT::SecurityMarketView* _smv){
    primary_smv_vec_.push_back(_smv);
  }
  void ReInitializeDataSubscriptions(){ 
    InitializeDataSubscriptions();
    dbglogger_ << "ReInitializeDataSubscriptions called " << secondary_smv_->shortcode() << DBGLOG_ENDL_FLUSH;
    dbglogger_ << secondary_smv_->shortcode() << " ";
    for (auto pr_smv : primary_smv_vec_) {
      dbglogger_ << pr_smv->shortcode() << " ";
    }
    dbglogger_ << DBGLOG_ENDL_FLUSH;
  }

  double GetPreviousDayPrice(){
    return prev_day_close_;
  }
  double GetAvgVolume(){
    return mean_volume_;
  }
  void AddPrimarySMVWeight(double weight_, double volume_){
    while(primary_smv_wt_vec_.size() < primary_smv_vec_.size() -1){
      primary_smv_wt_vec_.push_back(0);
      primary_smv_volume_vec_.push_back(0);
      last_px_vec_.push_back(0);
      primary_smv_curr_volume_vec_.push_back(0);
    }
    primary_smv_wt_vec_.push_back(weight_);
    primary_smv_volume_vec_.push_back(volume_);
    last_px_vec_.push_back(0);
    primary_smv_curr_volume_vec_.push_back(0);
  }
  std::vector<std::string> GetFutureVec(){ return future_smv_vec_; }
};
