#ifndef _HEDGE_THEO_CALCULATOR_H
#define _HEDGE_THEO_CALCULATOR_H

#define STOP_LOSS_DEFER_TIME_MSECS 60000

#include "baseinfra/SmartOrderRouting/mult_base_pnl.hpp"
#include "tradeengine/Executioner/BaseExecutioner.hpp"
#include "tradeengine/TheoCalc/BaseTheoCalculator.hpp"
#include "tradeengine/TheoCalc/SquareOffTheoCalculator.hpp"

class HedgeTheoCalculator : public BaseTheoCalculator, public HFSAT::MultBasePNLListener {
  double alpha_;
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

  double estimated_bid_price_;
  double estimated_ask_price_;

  double ratio_;
  bool disable_parent_on_pos_limit_;

  bool use_stop_gain_loss_;
  bool use_drawdown_;

  int target_position_to_reach_;
  int last_position_exec_price_marked_;

  double avg_price_on_parent_exec_;
  double max_price_after_parent_open_exec_;
  double min_price_after_parent_open_exec_;

  double max_price_to_place_;
  double min_price_to_place_;

  double drawdown_max_price_;
  double drawdown_min_price_;

  double percent_profit_per_trade_to_capture_;
  double percent_stop_loss_per_trade_allowed_;
  double percent_drawdown_per_trade_allowed_;

  double profit_per_trade_to_capture_;
  double stop_loss_per_trade_allowed_;
  double drawdown_per_trade_allowed_;

  // To avoid false cases of hitting stop in which
  // there is huge movement in primary which is yet
  // to be reflected in secondary
  bool tentative_hit_stop_loss_;
  int msecs_last_tentative_stop_loss_;

  // This is used to determine when to hedge when only
  // partial hedge can happen (right now we round of
  // target position to min order size)
  bool use_pos_cutoff_partial_hedge_;
  double pos_cutoff_partial_hedge_frac_;
  double pos_cutoff_partial_hedge_;
  int min_order_size_;

  HFSAT::MultBasePNL *mult_base_pnl_;

  void LoadParams();
  void UpdateRiskCheckVariables();
  void UpdateTheoPrices(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo &_market_update_info_);
  void ComputeAndUpdateTheoListeners();
  void UpdateTheoListeners();
  void OnReady(const HFSAT::MarketUpdateInfo &_market_update_info_);
  void CheckDeltaPosThrehold();

 public:
  HedgeTheoCalculator(std::map<std::string, std::string> *key_val_map, HFSAT::Watch &_watch_,
                      HFSAT::DebugLogger &dbglogger_, int _trading_start_utc_mfm_, int _trading_end_utc_mfm_,
                      int _aggressive_get_flat_mfm_, int eff_squareoff_start_utc_mfm_, double bid_multiplier_,
                      double ask_multiplier_);

  virtual ~HedgeTheoCalculator() {}

  void SetupPNLHooks();
  void SubscribeBasePNL(HFSAT::BasePNL *);
  void PNLStats(HFSAT::BulkFileWriter *trades_writer_ = nullptr, bool dump_to_cout = true);

  void SetRuntimeID(int _runtime_id_) {
    if (parent_mm_theo_) {
      runtime_id_ = parent_mm_theo_->GetRuntimeID();
    }
  }

  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo &_market_update_info_) {
    theo_values_.primary_best_bid_price_ = primary0_smv_->market_update_info().bestbid_price_;
    theo_values_.primary_best_ask_price_ = primary0_smv_->market_update_info().bestask_price_;
    if ((hit_stoploss_) && (!hit_hard_stoploss_)) {
      // If loss check is set, unset it
      if (status_mask_ & LOSS_STATUS_SET) {
        DBGLOG_TIME_CLASS_FUNC << "HITTING STOP LOSS for secID " << secondary_smv_->shortcode()
                               << " PNL: " << total_pnl_ << " pos: " << position_ << " SL: " << stop_loss_
                               << " HardSL: " << hard_stop_loss_ << " secmkt["
                               << secondary_smv_->market_update_info().bestbid_price_ << " x "
                               << secondary_smv_->market_update_info().bestask_price_ << "] " << DBGLOG_ENDL_FLUSH;
        SquareOff();
        if (parent_mm_theo_) {
          parent_mm_theo_->SquareOff();
          parent_mm_theo_->PrintBook();
        }
      }
      if (parent_mm_theo_) {
        parent_mm_theo_->TurnOffTheo(LOSS_STATUS_UNSET);
      }
      TurnOffTheo(LOSS_STATUS_UNSET);
    }

    UpdateTheoPrices(_security_id_, _market_update_info_);
  }

  inline void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo &_trade_print_info_,
                           const HFSAT::MarketUpdateInfo &_market_update_info_) {
    theo_values_.primary_best_bid_price_ = primary0_smv_->market_update_info().bestbid_price_;
    theo_values_.primary_best_ask_price_ = primary0_smv_->market_update_info().bestask_price_;
    if (_security_id_ == secondary_id_) {
      total_traded_qty_ += _trade_print_info_.size_traded_;
      theo_values_.last_traded_int_price_ = _trade_print_info_.int_trade_price_;
    }

    UpdateTheoPrices(_security_id_, _market_update_info_);
  }

  void OnExec(const int _new_position_, const int _exec_quantity_, const HFSAT::TradeType_t _buysell_,
              const double _price_, const int r_int_price_, const int _security_id_, const int _caos_);

  void SetTargetPosition(int target_position);

  void CheckDeltaPosLimits() {
    if (!parent_mm_theo_) return;
    if (position_ >= delta_pos_limit_) {
      // Turn off theo side appropriately
      parent_mm_theo_->DisableBid();
      if (!disable_parent_on_pos_limit_) {
        parent_mm_theo_->EnableAsk();
      } else {
        parent_mm_theo_->DisableAsk();
      }
    } else if (position_ <= -1 * delta_pos_limit_) {
      if (!disable_parent_on_pos_limit_) {
        parent_mm_theo_->EnableBid();
      } else {
        parent_mm_theo_->DisableBid();
      }
      parent_mm_theo_->DisableAsk();
    } else {
      // Ensure both theo sides trading
      parent_mm_theo_->EnableBid();
      parent_mm_theo_->EnableAsk();
    }
  }

  virtual void UpdateTotalRisk() { mult_base_pnl_->UpdateTotalRisk(position_); }

  virtual void UpdatePNL(int t_pnl_) {
    if (t_pnl_ < hard_stop_loss_) {
      hit_hard_stoploss_ = true;
      // If hard loss check is set, unset it
      if (status_mask_ & HARDLOSS_STATUS_SET) {
        DBGLOG_TIME_CLASS_FUNC << "HITTING HARD STOP LOSS for secID " << secondary_smv_->shortcode()
                               << " PNL: " << t_pnl_ << " pos: " << position_ << " SL: " << stop_loss_
                               << " HardSL: " << hard_stop_loss_ << " secmkt["
                               << secondary_smv_->market_update_info().bestbid_price_ << " x "
                               << secondary_smv_->market_update_info().bestask_price_ << "] " << DBGLOG_ENDL_FLUSH;
        if (parent_mm_theo_) {
          parent_mm_theo_->PrintBook();
        }
      }
      if (parent_mm_theo_) {
        parent_mm_theo_->TurnOffTheo(HARDLOSS_STATUS_UNSET);
      }
      TurnOffTheo(HARDLOSS_STATUS_UNSET);
      if (sqoff_theo_) {
        sqoff_theo_->TurnOffTheo(HARDLOSS_STATUS_UNSET);
      }
    } else if (!hit_hard_stoploss_) {
      if ((t_pnl_ < stop_loss_) && (!hit_stoploss_)) {
        if ((position_ != 0) || (t_pnl_ < hard_stop_loss_)) {
          hit_stoploss_ = true;
          tentative_hit_stop_loss_ = false;

        } else {
          if (!tentative_hit_stop_loss_) {
            tentative_hit_stop_loss_ = true;
            msecs_last_tentative_stop_loss_ = watch_.msecs_from_midnight();
            DBGLOG_TIME_CLASS_FUNC << "TENTATIVE STOP LOSS for secID " << secondary_smv_->shortcode()
                                   << " PNL: " << t_pnl_ << " SL: " << stop_loss_ << " secmkt["
                                   << secondary_smv_->market_update_info().bestbid_price_ << " x "
                                   << secondary_smv_->market_update_info().bestask_price_ << "] " << DBGLOG_ENDL_FLUSH;
          }
        }
      }

      if ((tentative_hit_stop_loss_) &&
          ((watch_.msecs_from_midnight() - msecs_last_tentative_stop_loss_) > STOP_LOSS_DEFER_TIME_MSECS)) {
        if (t_pnl_ < stop_loss_) {
          hit_stoploss_ = true;
        }
        tentative_hit_stop_loss_ = false;
      }
    }
    total_pnl_ = t_pnl_;
  }

  void SquareOff(bool set_aggressive_ = false) {
    if (!sqoff_theo_) {
      DBGLOG_TIME_CLASS_FUNC << "NO SQUARE OFF THEO PRESENT for secID " << secondary_id_ << DBGLOG_ENDL_FLUSH;
      return;
    }
    DBGLOG_TIME_CLASS_FUNC << "SQUARING OFF SECONDARY " << sec_name_indexer_.GetShortcodeFromId(secondary_id_)
                           << DBGLOG_ENDL_FLUSH;
    basic_om_->AddExecutionListener((BaseTheoCalculator *)sqoff_theo_);
    if (parent_mm_theo_) {
      parent_mm_theo_->TurnOffTheo(SQUAREOFF_STATUS_UNSET);
    }
    sqoff_theo_->Activate(position_);
    sqoff_theo_->SetAggSqoff(set_aggressive_);
  }

  void NoSquareOff() {
    if ((status_mask_ & NOSQUAREOFF_BITS_SET) != NOSQUAREOFF_BITS_SET) {
      DBGLOG_TIME_CLASS_FUNC << "Not deactivatigng square off for " << secondary_id_ << " since status mask "
                             << status_mask_ << "has other nosquare off related bits unset" << DBGLOG_ENDL_FLUSH;
      return;
    }
    if (!sqoff_theo_) {
      DBGLOG_TIME_CLASS_FUNC << "NO SQUARE OFF THEO PRESENT for secID " << secondary_id_ << DBGLOG_ENDL_FLUSH;
      return;
    }
    if (parent_mm_theo_) {
      parent_mm_theo_->TurnOnTheo(SQUAREOFF_STATUS_SET);
      DBGLOG_TIME_CLASS_FUNC << "STOPPING SQUARING OFF SECONDARY "
                             << sec_name_indexer_.GetShortcodeFromId(secondary_id_) << " STATUS MASK: " << status_mask_
                             << " PARENT STATUS MASK: " << parent_mm_theo_->GetStatus() << DBGLOG_ENDL_FLUSH;
    } else {
      DBGLOG_TIME_CLASS_FUNC << "STOPPING SQUARING OFF SECONDARY "
                             << sec_name_indexer_.GetShortcodeFromId(secondary_id_) << " STATUS MASK: " << status_mask_
                             << DBGLOG_ENDL_FLUSH;
    }

    basic_om_->RemoveExecutionLister((BaseTheoCalculator *)sqoff_theo_);
    sqoff_theo_->DeActivate();
  }
};

#endif  // _HEDGE_THEO_CALCULATOR_H
