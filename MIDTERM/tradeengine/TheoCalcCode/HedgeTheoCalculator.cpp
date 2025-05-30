#include "tradeengine/TheoCalc/HedgeTheoCalculator.hpp"
#include "tradeengine/Utils/Parser.hpp"

HedgeTheoCalculator::HedgeTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                                         HFSAT::DebugLogger& _dbglogger_, int _trading_start_utc_mfm_,
                                         int _trading_end_utc_mfm_, int _aggressive_get_flat_mfm_,
                                         int eff_squareoff_start_utc_mfm_ = 0, double bid_multiplier_ = 1,
                                         double ask_multiplier_ = 1)
    : BaseTheoCalculator(key_val_map, _watch_, _dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_,
                         _aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_),
      alpha_(1),
      primary0_price_type_(pORDER_BOOK_BEST_PRICE),
      primary0_vwap_levels_(1),
      min_primary_spread_(0),
      max_primary_spread_(0),
      min_secondary_spread_(0),
      max_secondary_spread_(0),
      passive_reduce_position_(false),
      aggressive_reduce_position_(false),
      min_ratio_(0.999),
      max_ratio_(1.001),
      estimated_bid_price_(0),
      estimated_ask_price_(0),
      ratio_(0),
      disable_parent_on_pos_limit_(false),
      use_stop_gain_loss_(0),
      use_drawdown_(0),
      target_position_to_reach_(0),
      last_position_exec_price_marked_(0),
      avg_price_on_parent_exec_(0),
      max_price_after_parent_open_exec_(MIN_PRICE),
      min_price_after_parent_open_exec_(MAX_PRICE),
      max_price_to_place_(MAX_PRICE),
      min_price_to_place_(MIN_PRICE),
      drawdown_max_price_(MAX_PRICE),
      drawdown_min_price_(MIN_PRICE),
      profit_per_trade_to_capture_(0),
      stop_loss_per_trade_allowed_(0),
      drawdown_per_trade_allowed_(0),
      tentative_hit_stop_loss_(false),
      msecs_last_tentative_stop_loss_(0),
      use_pos_cutoff_partial_hedge_(false),
      pos_cutoff_partial_hedge_frac_(0.5),
      pos_cutoff_partial_hedge_(0.5 * secondary_smv_->min_order_size_),
      min_order_size_(secondary_smv_->min_order_size_),
      mult_base_pnl_(NULL) {
  mult_base_pnl_ = new HFSAT::MultBasePNL(_dbglogger_, _watch_);
  InitializeDataSubscriptions();
  dbglogger_ << watch_.tv() << " Creating RATIO THEO CALCULATOR secId " << secondary_id_ << " primId " << primary0_id_
             << DBGLOG_ENDL_FLUSH;
  LoadParams();
}

void HedgeTheoCalculator::LoadParams() {
  BaseTheoCalculator::LoadParams();
  bool status_ = Parser::GetBool(key_val_map_, "STATUS", false);
  status_mask_ = (status_ == true ? (status_mask_ | CONFIG_STATUS_SET) : (status_mask_ & CONFIG_STATUS_UNSET));
  alpha_ = Parser::GetDouble(key_val_map_, "ALPHA", 1);
  primary0_price_type_str_ = Parser::GetString(key_val_map_, "PRIMARY0_PRICE_TYPE", "ORDER_BOOK_BEST_PRICE");
  primary0_price_type_ = GetPriceTypeFromStr(primary0_price_type_str_);
  primary0_vwap_levels_ = Parser::GetInt(key_val_map_, "PRIMARY0_VWAP_LEVELS", 1);
  primary0_size_filter_ = Parser::GetInt(key_val_map_, "PRIMARY0_SIZE_FILTER", 0);
  primary0_bid_size_filter_ = primary0_size_filter_;
  primary0_ask_size_filter_ = primary0_size_filter_;
  primary0_size_max_depth_ = Parser::GetInt(key_val_map_, "PRIMARY0_SIZE_MAX_DEPTH", 0);

  min_primary_spread_ = Parser::GetDouble(key_val_map_, "MIN_PRIMARY_SPREAD", 9999);
  max_primary_spread_ = Parser::GetDouble(key_val_map_, "MAX_PRIMARY_SPREAD", 0);

  min_secondary_spread_ = Parser::GetDouble(key_val_map_, "MIN_SECONDARY_SPREAD", 9999);
  max_secondary_spread_ = Parser::GetDouble(key_val_map_, "MAX_SECONDARY_SPREAD", 0);

  stop_loss_ = Parser::GetDouble(key_val_map_, "STOP_LOSS", 0);
  stop_loss_ = stop_loss_ * -1;

  // This is always greater than normal stop loss
  hard_stop_loss_ = Parser::GetDouble(key_val_map_, "HARD_STOP_LOSS", 50000);
  hard_stop_loss_ = hard_stop_loss_ * -1;
  hard_stop_loss_ = std::min(hard_stop_loss_, stop_loss_);

  passive_reduce_position_ = Parser::GetBool(key_val_map_, "PASSIVE_REDUCE_POSITION", false);
  aggressive_reduce_position_ = Parser::GetBool(key_val_map_, "AGGRESSIVE_REDUCE_POSITION", false);
  min_ratio_ = Parser::GetDouble(key_val_map_, "MIN_RATIO", 0.999);
  max_ratio_ = Parser::GetDouble(key_val_map_, "MAX_RATIO", 1.001);
  delta_pos_limit_ = Parser::GetInt(key_val_map_, "DELTA_POS_LIMIT", 0, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);

//  DBGLOG_CLASS_FUNC_LINE_INFO << "DELTA_POS_LIMIT :" << delta_pos_limit_ << DBGLOG_ENDL_FLUSH;

  disable_parent_on_pos_limit_ = Parser::GetBool(key_val_map_, "DISABLE_PARENT_ON_POS_LIMIT", false);

  use_stop_gain_loss_ = Parser::GetBool(key_val_map_, "USE_STOP_GAIN_LOSS", false);
  use_drawdown_ = Parser::GetBool(key_val_map_, "USE_DRAW_DOWN", false);

  percent_profit_per_trade_to_capture_ = Parser::GetDouble(key_val_map_, "STOP_GAIN_PER_TRADE", 0) / 100;
  percent_stop_loss_per_trade_allowed_ = Parser::GetDouble(key_val_map_, "STOP_LOSS_PER_TRADE", 0) / 100;
  percent_drawdown_per_trade_allowed_ = Parser::GetDouble(key_val_map_, "DRAW_DOWN_PER_TRADE", 0) / 100;

  use_pos_cutoff_partial_hedge_ = Parser::GetBool(key_val_map_, "USE_POS_CUTOFF_PARTIAL_HEDGE", false);
  pos_cutoff_partial_hedge_frac_ =
      std::max(0.5, Parser::GetDouble(key_val_map_, "POS_CUTOFF_PARTIAL_HEDGE_FRAC",
                                      0.5));  // Maximum is taken so it does not oscillate while closing hedge positions
  pos_cutoff_partial_hedge_ = std::min(pos_cutoff_partial_hedge_frac_ * min_order_size_, (double)min_order_size_);

  if (!position_shift_amount_) {
    dbglogger_ << watch_.tv() << " POSITION SHIFT MGR SET FALSE SINCE POSITION SHIFT AMOUNT IS "
               << position_shift_amount_ << " in " << theo_identifier_ << DBGLOG_ENDL_FLUSH;
    use_position_shift_manager_ = false;
  }
}

void HedgeTheoCalculator::UpdateRiskCheckVariables() {
  if ((!use_stop_gain_loss_) && (!use_drawdown_)) {
    target_position_to_reach_ = target_position_;
    theo_values_.position_to_offset_ = current_position_ + target_position_to_reach_;
  } else {
    if (position_ < 0) {
      if (use_stop_gain_loss_) {
        avg_price_on_parent_exec_ =
            (avg_price_on_parent_exec_ * last_position_exec_price_marked_ +
             secondary_smv_->market_update_info().bestbid_price_ * (position_ - last_position_exec_price_marked_)) /
            position_;
        min_price_to_place_ = avg_price_on_parent_exec_ - profit_per_trade_to_capture_;
        max_price_to_place_ = avg_price_on_parent_exec_ + stop_loss_per_trade_allowed_;
      }

      if (use_drawdown_) {
        min_price_after_parent_open_exec_ =
            (min_price_after_parent_open_exec_ * last_position_exec_price_marked_ +
             secondary_smv_->market_update_info().bestbid_price_ * (position_ - last_position_exec_price_marked_)) /
            position_;
        max_price_after_parent_open_exec_ = secondary_smv_->market_update_info().bestbid_price_;

        drawdown_min_price_ = MIN_PRICE;
        drawdown_max_price_ = min_price_after_parent_open_exec_ + drawdown_per_trade_allowed_;
      }

    } else if (position_ > 0) {
      if (use_stop_gain_loss_) {
        avg_price_on_parent_exec_ =
            (avg_price_on_parent_exec_ * last_position_exec_price_marked_ +
             secondary_smv_->market_update_info().bestask_price_ * (position_ - last_position_exec_price_marked_)) /
            position_;
        min_price_to_place_ = avg_price_on_parent_exec_ - stop_loss_per_trade_allowed_;
        max_price_to_place_ = avg_price_on_parent_exec_ + profit_per_trade_to_capture_;
      }

      if (use_drawdown_) {
        max_price_after_parent_open_exec_ =
            (max_price_after_parent_open_exec_ * last_position_exec_price_marked_ +
             secondary_smv_->market_update_info().bestask_price_ * (position_ - last_position_exec_price_marked_)) /
            position_;
        min_price_after_parent_open_exec_ = secondary_smv_->market_update_info().bestask_price_;

        drawdown_min_price_ = max_price_after_parent_open_exec_ - drawdown_per_trade_allowed_;
        drawdown_max_price_ = MAX_PRICE;
      }
    } else {
      avg_price_on_parent_exec_ = 0;
      max_price_to_place_ = MAX_PRICE;
      min_price_to_place_ = MIN_PRICE;

      min_price_after_parent_open_exec_ = secondary_smv_->market_update_info().bestbid_price_;
      max_price_after_parent_open_exec_ = secondary_smv_->market_update_info().bestask_price_;

      drawdown_min_price_ = MIN_PRICE;
      drawdown_max_price_ = MAX_PRICE;

      target_position_to_reach_ = target_position_;
      theo_values_.position_to_offset_ = current_position_ + target_position_to_reach_;
    }
  }
}

void HedgeTheoCalculator::SetTargetPosition(int target_position) {
#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
  dbglogger_ << watch_.tv() << " SET TARGET POSITION " << target_position << " CURRENT POSITION " << current_position_
             << " primarymkt[" << theo_values_.primary_best_bid_price_ << " x " << theo_values_.primary_best_ask_price_
             << "]" << DBGLOG_ENDL_FLUSH;
#endif

  target_position_ = target_position;
  position_ = current_position_ + target_position_;

  UpdateRiskCheckVariables();
  last_position_exec_price_marked_ = position_;
  CheckDeltaPosThrehold();
  SetPositionOffsets(position_);

  CheckDeltaPosLimits();
  if (sqoff_theo_) {
    sqoff_theo_->ChangePosition(position_);
  }
}

void HedgeTheoCalculator::PNLStats(HFSAT::BulkFileWriter* trades_writer_, bool dump_to_cout) {
  BaseTheoCalculator::PNLStats(trades_writer_, dump_to_cout);
  // POsrtfolio exposure based on secondary smv prices
  int delta_exposure_ =
      position_ *
      (secondary_smv_->market_update_info().bestbid_price_ + secondary_smv_->market_update_info().bestask_price_) * 0.5;
  int volume_ = 0;
  double ttv_ = 0;
  double mkt_percent_ = 0;
  if (parent_mm_theo_) {
    volume_ = basic_om_->trade_volume() + parent_mm_theo_->GetVolume();
    ttv_ = GetTTV() + parent_mm_theo_->GetTTV();
    mkt_percent_ = (parent_mm_theo_->GetTotalTradedTTV() != 0)
                       ? (parent_mm_theo_->GetTTV() / parent_mm_theo_->GetTotalTradedTTV()) * 100
                       : 0;
  }

  dbglogger_ << watch_.tv() << " PORTRESULT " << ticker_name_ << " PNL: " << total_pnl_ << " POS: " << position_
             << " VOLUME: " << volume_ << " EXP: " << delta_exposure_ << " MINPNL: " << mult_base_pnl_->min_pnl()
             << " MKTPERCENT: " << mkt_percent_ << DBGLOG_ENDL_FLUSH;

  if (dump_to_cout) {
    std::cout << "PORTRESULT " << ticker_name_ << "." << runtime_id_ << " " << total_pnl_ << " " << volume_ << " "
              << " 0 0 " << mkt_percent_ << " 0 " << delta_exposure_ << " " << ttv_ << " " << mult_base_pnl_->min_pnl()
              << DBGLOG_ENDL_FLUSH;
  }

  if (trades_writer_ != nullptr) {
    *trades_writer_ << "PORTRESULT " << ticker_name_ << "." << runtime_id_ << " " << total_pnl_ << " " << volume_ << " "
                    << " 0 0 " << mkt_percent_ << " 0 " << delta_exposure_ << " " << ttv_ << " "
                    << mult_base_pnl_->min_pnl() << "\n";
  }
}

void HedgeTheoCalculator::UpdateTheoPrices(const unsigned int _security_id_,
                                           const HFSAT::MarketUpdateInfo& _market_update_info_) {
  int msecs_from_midnight = watch_.msecs_from_midnight();

  if (msecs_from_midnight > trading_end_utc_mfm_) {
    TurnOffTheo(SQUAREOFF_STATUS_UNSET);
    eff_squareoff_on_ = false;
    if (parent_mm_theo_) {
      parent_mm_theo_->TurnOffTheo(SQUAREOFF_STATUS_UNSET);
    }
    if (start_trading_) {
      if ((status_mask_ & SQUAREOFF_BITS_SET) != SQUAREOFF_BITS_SET) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR : Not squaring off at EOD since status mask: " << status_mask_
                               << "has other square off related bits unset" << DBGLOG_ENDL_FLUSH;
        dbglogger_.DumpCurrentBuffer();
      } else {
        SquareOff();
      }
      dbglogger_ << watch_.tv() << " Getting PassiveFlat " << sec_name_indexer_.GetShortcodeFromId(_security_id_)
                 << " EndTimeMFM: " << trading_end_utc_mfm_ << DBGLOG_ENDL_FLUSH;
      start_trading_ = false;
    }
  }

  if (msecs_from_midnight > aggressive_get_flat_mfm_) {
    TurnOffTheo(SQUAREOFF_STATUS_UNSET);
    eff_squareoff_on_ = false;
    if (parent_mm_theo_) {
      parent_mm_theo_->TurnOffTheo(SQUAREOFF_STATUS_UNSET);
    }
    // In case it gets triggered before normal getflat (ideally shouldn't happen)
    if ((start_trading_) || (!is_agressive_getflat_)) {
      if ((!need_to_hedge_) && ((status_mask_ & SQUAREOFF_BITS_SET) != SQUAREOFF_BITS_SET)) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR : Not squaring off at EOD since status mask: " << status_mask_
                               << "has other square off related bits unset" << DBGLOG_ENDL_FLUSH;
        dbglogger_.DumpCurrentBuffer();
      } else {
        SquareOff(true);
      }
      dbglogger_ << watch_.tv() << " Getting AgressivelyFlat " << sec_name_indexer_.GetShortcodeFromId(_security_id_)
                 << " EndTimeMFM: " << aggressive_get_flat_mfm_ << DBGLOG_ENDL_FLUSH;
      start_trading_ = false;
      is_agressive_getflat_ = true;
    }
  }

  if ((eff_squareoff_start_utc_mfm_ != 0) && (msecs_from_midnight > eff_squareoff_start_utc_mfm_)) {
    if (start_trading_) {
      if (!eff_squareoff_on_) {
        dbglogger_ << watch_.tv() << " Getting Efficient SquareOff "
                   << sec_name_indexer_.GetShortcodeFromId(_security_id_)
                   << " EndTimeMFM: " << eff_squareoff_start_utc_mfm_ << DBGLOG_ENDL_FLUSH;
      }
      SetEfficientSquareOff(true);
    }
  }

  if (_security_id_ == primary0_id_) {
    theo_values_.is_primary_update_ = true;
    if (primary0_smv_->market_update_info_.bestbid_price_ == kInvalidPrice ||
        primary0_smv_->market_update_info_.bestask_price_ == kInvalidPrice ||
        primary0_smv_->market_update_info_.bestbid_price_ >= primary0_smv_->market_update_info_.bestask_price_) {
      if (is_ready_) {
        /*if (status_mask_ & INVALIDBOOK_STATUS_SET) {
          dbglogger_ << watch_.tv() << " ERROR INVALID BOOK " << sec_name_indexer_.GetShortcodeFromId(primary0_id_)
                     << " bid " << primary0_smv_->market_update_info_.bestbid_price_ << " ask "
                     << primary0_smv_->market_update_info_.bestask_price_ << " theoIsReady " << is_ready_
                     << DBGLOG_ENDL_FLUSH;
        }*/
        TurnOffTheo(INVALIDBOOK_STATUS_UNSET);
      }
      primary_book_valid_ = false;
    } else {
      primary_book_valid_ = true;
      if ((!is_ready_ || config_reloaded_) && msecs_from_midnight > trading_start_utc_mfm_ && start_trading_) {
        OnReady(_market_update_info_);
        is_ready_ = true;
      }

      if ((ratio_ != 0) && (position_ != 0 || theo_values_.position_to_offset_ != 0)) {
        theo_values_.reference_primary_bid_ = 0;
        theo_values_.reference_primary_ask_ = 0;
        CalculatePrices(theo_values_.reference_primary_bid_, theo_values_.reference_primary_ask_, primary0_smv_,
                        primary0_vwap_levels_, primary0_price_type_, primary0_bid_size_filter_,
                        primary0_ask_size_filter_, primary0_size_max_depth_);
        // DBGLOG_TIME_CLASS_FUNC << " PRIMARY BID " << primary_bid << " ASK " << primary_ask << DBGLOG_ENDL_FLUSH;
        estimated_bid_price_ = (theo_values_.reference_primary_bid_ * ratio_) -
                               bid_offset_ * (eff_squareoff_on_ ? (bid_multiplier_ * bid_factor_) : 1);
        estimated_ask_price_ = (theo_values_.reference_primary_ask_ * ratio_) +
                               ask_offset_ * (eff_squareoff_on_ ? (ask_multiplier_ * ask_factor_) : 1);
      }
    }
  }

  if (_security_id_ == secondary_id_) {
    theo_values_.is_primary_update_ = false;
    if (secondary_smv_->market_update_info_.bestbid_price_ == kInvalidPrice ||
        secondary_smv_->market_update_info_.bestask_price_ == kInvalidPrice ||
        secondary_smv_->market_update_info_.bestbid_price_ >= secondary_smv_->market_update_info_.bestask_price_) {
      if (is_ready_) {
        /*if (status_mask_ & INVALIDBOOK_STATUS_SET) {
          dbglogger_ << watch_.tv() << " ERROR INVALID BOOK " << sec_name_indexer_.GetShortcodeFromId(secondary_id_)
                     << " bid " << secondary_smv_->market_update_info_.bestbid_price_ << " ask "
                     << secondary_smv_->market_update_info_.bestask_price_ << " theoIsReady " << is_ready_
                     << DBGLOG_ENDL_FLUSH;
        }*/
        TurnOffTheo(INVALIDBOOK_STATUS_UNSET);
      }
      secondary_book_valid_ = false;
    } else {
      secondary_book_valid_ = true;
      if ((position_ != 0) && (use_drawdown_)) {
        if (secondary_smv_->market_update_info().bestask_price_ - DOUBLE_PRECISION >
            max_price_after_parent_open_exec_) {
          // dbglogger_ << watch_.tv() << " Prev max price " << max_price_after_parent_open_exec_
          //	<< " Cur max price " << secondary_smv_->market_update_info().bestask_price_ << DBGLOG_ENDL_FLUSH;
          max_price_after_parent_open_exec_ = secondary_smv_->market_update_info().bestask_price_;
          drawdown_min_price_ = max_price_after_parent_open_exec_ - drawdown_per_trade_allowed_;
        }

        if (secondary_smv_->market_update_info().bestbid_price_ + DOUBLE_PRECISION <
            min_price_after_parent_open_exec_) {
          // dbglogger_ << watch_.tv() << " Prev min price " << min_price_after_parent_open_exec_
          //	<< " Cur min price " << secondary_smv_->market_update_info().bestbid_price_ << DBGLOG_ENDL_FLUSH;
          min_price_after_parent_open_exec_ = secondary_smv_->market_update_info().bestbid_price_;
          drawdown_max_price_ = min_price_after_parent_open_exec_ + drawdown_per_trade_allowed_;
        }
      }
    }
  }

  if ((secondary_book_valid_) && (primary_book_valid_)) {
    if (is_ready_ && ((status_mask_ & INVALIDBOOK_STATUS_SET) == 0)) {
      TurnOnTheo(INVALIDBOOK_STATUS_SET);
    }

    if (ratio_ != 0) {
      ComputeAndUpdateTheoListeners();
    }

    /*dbglogger_ << watch_.tv() << " " << theo_identifier_ << " SecID: " <<  _security_id_
      << " Sec: " << sec_name_indexer_.GetShortcodeFromId(_security_id_)
      << " Pos: " << position_
      << " PPx " << theo_values_.primary_best_bid_price_
      << " X " << theo_values_.primary_best_ask_price_
      << "::::SPx " << secondary_smv_->market_update_info().bestbid_price_
      << " X " << secondary_smv_->market_update_info().bestask_price_
      << "::ratio "<< ratio_
      << "::theopx "<< theo_values_.theo_bid_price_ << " X " << theo_values_.theo_ask_price_ << DBGLOG_ENDL_FLUSH;*/

    if (_security_id_ == secondary_id_) {
      // double primary_bestbid_px = theo_values_.primary_best_bid_price_;
      // double primary_bestask_px = theo_values_.primary_best_ask_price_;
      double primary_bestbid_px = primary0_smv_->market_update_info().bestbid_price_;
      double primary_bestask_px = primary0_smv_->market_update_info().bestask_price_;
      double secondary_bestbid_px = _market_update_info_.bestbid_price_;
      double secondary_bestask_px = _market_update_info_.bestask_price_;
      double primary_spread = primary_bestask_px - primary_bestbid_px;
      double secondary_spread = secondary_bestask_px - secondary_bestbid_px;
      if (primary_spread <= max_primary_spread_ && primary_spread >= min_primary_spread_ &&
          secondary_spread <= max_secondary_spread_ && secondary_spread >= min_secondary_spread_) {
        double current_ratio =
            (secondary_bestbid_px + secondary_bestask_px) / (primary_bestbid_px + primary_bestask_px);
        if (current_ratio > min_ratio_ && current_ratio < max_ratio_) {
          if (ratio_ != 0) {
            ratio_ = ratio_ * (1 - alpha_) + alpha_ * current_ratio;
          } else {
            ratio_ = current_ratio;
          }
        }
        // dbglogger_ << watch_.tv() << " " << "CurrentRatio:" << current_ratio << " ratio "
        //	<< ratio_ << DBGLOG_ENDL_FLUSH;
      }
    }
  }
}

void HedgeTheoCalculator::ComputeAndUpdateTheoListeners() {
  int prev_theo_bid_price_ = theo_values_.theo_bid_price_;
  int prev_theo_ask_price_ = theo_values_.theo_ask_price_;

  theo_values_.theo_bid_price_ =
      estimated_bid_price_ + bid_theo_shifts_ * (eff_squareoff_on_ ? (bid_multiplier_ * bid_factor_) : 1);
  theo_values_.theo_ask_price_ =
      estimated_ask_price_ + ask_theo_shifts_ * (eff_squareoff_on_ ? (ask_multiplier_ * ask_factor_) : 1);

  if ((prev_theo_bid_price_ + prev_theo_ask_price_) < (theo_values_.theo_bid_price_ + theo_values_.theo_ask_price_)) {
    theo_values_.movement_indicator_ = pUPWARD;
  } else {
    theo_values_.movement_indicator_ = pDOWNWARD;
  }

  if ((use_stop_gain_loss_) || (use_drawdown_)) {
    if (theo_values_.position_to_offset_ != position_) {
      if (secondary_smv_->market_update_info_.bestbid_price_ == kInvalidPrice ||
          secondary_smv_->market_update_info_.bestask_price_ == kInvalidPrice ||
          secondary_smv_->market_update_info_.bestbid_price_ >= secondary_smv_->market_update_info_.bestask_price_) {
        target_position_to_reach_ = target_position_;
        theo_values_.position_to_offset_ = current_position_ + target_position_to_reach_;
      } else if (position_ < 0) {
        if (((use_stop_gain_loss_) &&
             ((secondary_smv_->market_update_info().bestbid_price_ + DOUBLE_PRECISION >= max_price_to_place_) ||
              (secondary_smv_->market_update_info().bestbid_price_ - DOUBLE_PRECISION <= min_price_to_place_))) ||
            ((use_drawdown_) &&
             (secondary_smv_->market_update_info().bestbid_price_ + DOUBLE_PRECISION >= drawdown_max_price_))) {
          target_position_to_reach_ = target_position_;
          theo_values_.position_to_offset_ = current_position_ + target_position_to_reach_;
          /*} else {
            dbglogger_ << watch_.tv() << " Not placing new buy order Max Price: "
            << max_price_to_place_  << " Min Price: " << min_price_to_place_
            << " Drawdown price: " << drawdown_max_price_
            << " Bid Price " <<  secondary_smv_->market_update_info().bestbid_price_ << DBGLOG_ENDL_FLUSH;*/
        }
      } else if (position_ > 0) {
        if (((use_stop_gain_loss_) &&
             ((secondary_smv_->market_update_info().bestask_price_ + DOUBLE_PRECISION >= max_price_to_place_) ||
              (secondary_smv_->market_update_info().bestask_price_ - DOUBLE_PRECISION <= min_price_to_place_))) ||
            ((use_drawdown_) &&
             (secondary_smv_->market_update_info().bestask_price_ - DOUBLE_PRECISION <= drawdown_min_price_))) {
          target_position_to_reach_ = target_position_;
          theo_values_.position_to_offset_ = current_position_ + target_position_to_reach_;
          /*} else {
            dbglogger_ << watch_.tv() << " Not placing new ask order Max Price: "
            << max_price_to_place_  << " Min Price: " << min_price_to_place_
            << " Drawdown price: " << drawdown_min_price_
            << " Ask Price " <<  secondary_smv_->market_update_info().bestask_price_ << DBGLOG_ENDL_FLUSH;*/
        }
      } else {
        target_position_to_reach_ = target_position_;
        theo_values_.position_to_offset_ = current_position_ + target_position_to_reach_;
      }
    }
    CheckDeltaPosThrehold();
  }

  UpdateTheoListeners();
}

void HedgeTheoCalculator::OnReady(const HFSAT::MarketUpdateInfo& _market_update_info_) {
  bid_offset_ = bid_percentage_offset_ * _market_update_info_.bestbid_price_;
  ask_offset_ = ask_percentage_offset_ * _market_update_info_.bestbid_price_;

  bid_increase_shift_ = bid_increase_shift_percent_ * _market_update_info_.bestbid_price_;
  bid_decrease_shift_ = bid_decrease_shift_percent_ * _market_update_info_.bestbid_price_;
  ask_increase_shift_ = ask_increase_shift_percent_ * _market_update_info_.bestbid_price_;
  ask_decrease_shift_ = ask_decrease_shift_percent_ * _market_update_info_.bestbid_price_;
  profit_per_trade_to_capture_ = percent_profit_per_trade_to_capture_ * _market_update_info_.bestbid_price_;
  stop_loss_per_trade_allowed_ = percent_stop_loss_per_trade_allowed_ * _market_update_info_.bestbid_price_;
  drawdown_per_trade_allowed_ = percent_drawdown_per_trade_allowed_ * _market_update_info_.bestbid_price_;

  avg_int_spread_ = std::max(int(avg_spread_percent_ * _market_update_info_.bestbid_int_price_), 1);
  long_ema_int_spread_ = avg_int_spread_;
  short_ema_int_spread_ = avg_int_spread_;
  inv_avg_int_spread_ = 1 / avg_int_spread_;
  total_offset_in_avg_spread_ = (bid_percentage_offset_ + ask_percentage_offset_) / avg_spread_percent_;
  slope_obb_offset_mult_ = 1 / (total_offset_in_avg_spread_);
  intercept_obb_offset_mult_ = (total_offset_in_avg_spread_ + 1) / (total_offset_in_avg_spread_);

  dbglogger_ << watch_.tv() << " " << theo_identifier_ << " Offsets:" << bid_offset_ << "|" << ask_offset_ << "|"
             << bid_increase_shift_ << "|" << bid_decrease_shift_ << "|" << ask_increase_shift_ << "|"
             << ask_decrease_shift_ << "|" << profit_per_trade_to_capture_ << "|" << stop_loss_per_trade_allowed_ << "|"
             << drawdown_per_trade_allowed_ << "\n";
  for (auto base_exec : base_exec_vec_) {
    base_exec->SetPassiveReduce(passive_reduce_position_);
    base_exec->SetAggressiveReduce(aggressive_reduce_position_);
    base_exec->SetEfficientSquareOff(eff_squareoff_on_);
    base_exec->OnReady(_market_update_info_);
  }
  config_reloaded_ = false;
}

void HedgeTheoCalculator::OnExec(const int _new_position_, const int _exec_quantity_,
                                 const HFSAT::TradeType_t _buysell_, const double _price_, const int r_int_price_,
                                 const int _security_id_, const int _caos_) {
  num_trades_++;
  int prev_position_ = position_;
  current_position_ = _new_position_;
  position_ = _new_position_ + target_position_;
  total_traded_value_ += _exec_quantity_ * _price_;
  strat_ltp_ = _price_;
  // This case handles the situation when delta pos is opened by hedge order
  if ((prev_position_ == 0) || ((prev_position_ > 0) && (position_ < 0)) || ((prev_position_ < 0) && (position_ > 0))) {
    last_position_exec_price_marked_ = 0;
    UpdateRiskCheckVariables();
  }

  last_position_exec_price_marked_ = position_;
  theo_values_.position_to_offset_ = _new_position_ + target_position_to_reach_;
  CheckDeltaPosThrehold();
  SetPositionOffsets(position_);
  if (parent_mm_theo_) {
    parent_mm_theo_->SetPositionOffsets(position_);
  }

  CheckDeltaPosLimits();
  if (status_mask_ == BIT_SET_ALL) {
    // This means that position is opened by Hedge
    // (most probably order got executed before we are able to cancel)
    if (prev_position_ == 0) {
      UpdateTheoPrices(primary0_id_, primary0_smv_->market_update_info_);
    } else {
      ComputeAndUpdateTheoListeners();
    }
  }
  mult_base_pnl_->UpdateTotalRisk(position_);
  dbglogger_ << watch_.tv() << " " << secondary_smv_->shortcode() << " HedgeOnExec pos: " << position_ << " "
             << (_buysell_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL") << " price: " << _price_ << " CAOS: " << _caos_
             << " theo[" << theo_values_.theo_bid_price_ << " X " << theo_values_.theo_ask_price_ << "]"
             << " mkt[" << secondary_smv_->market_update_info().bestbid_price_ << " X "
             << secondary_smv_->market_update_info().bestask_price_ << "]"
             << " volume: " << basic_om_->trade_volume() << " tpos: " << target_position_
             << " cpos: " << current_position_ << " ttv: " << total_traded_value_ << "\n";

  // NotifyHedgeTheoListeners();
}

void HedgeTheoCalculator::UpdateTheoListeners() {
  if (status_mask_ != BIT_SET_ALL) {
    theo_values_.is_valid_ = false;
    CancelBid();
    CancelAsk();
    return;
  }
  if (start_trading_ && is_ready_) {
    if (theo_values_.theo_ask_price_ - theo_values_.theo_bid_price_ <
        spread_check_percent_ * theo_values_.theo_bid_price_) {
      dbglogger_ << watch_.tv() << " Error! theo spread is violated " << secondary_smv_->shortcode() << " theo bid "
                 << theo_values_.theo_bid_price_ << " ask " << theo_values_.theo_ask_price_ << " spread "
                 << theo_values_.theo_ask_price_ - theo_values_.theo_bid_price_ << " spread limit "
                 << spread_check_percent_ * theo_values_.theo_bid_price_ << DBGLOG_ENDL_FLUSH;
      TurnOffTheo(SPREADLIMIT_STATUS_UNSET);
    }

    theo_values_.is_valid_ = true;
    for (auto base_exec : base_exec_vec_) {
      base_exec->OnTheoUpdate();
    }
  }
}

void HedgeTheoCalculator::CheckDeltaPosThrehold() {
  if (use_pos_cutoff_partial_hedge_) {
    if (theo_values_.position_to_offset_ > pos_cutoff_partial_hedge_) {
      theo_values_.position_to_offset_ =
          min_order_size_ * (int)ceil((theo_values_.position_to_offset_ -
                                       pos_cutoff_partial_hedge_frac_ * min_order_size_ - DOUBLE_PRECISION) /
                                      min_order_size_);
    } else if (theo_values_.position_to_offset_ <= -1 * pos_cutoff_partial_hedge_) {
      theo_values_.position_to_offset_ =
          min_order_size_ * (int)floor((theo_values_.position_to_offset_ +
                                        pos_cutoff_partial_hedge_frac_ * min_order_size_ - DOUBLE_PRECISION) /
                                       min_order_size_);
    } else {
      theo_values_.position_to_offset_ = 0;
    }
  }
}

void HedgeTheoCalculator::SetupPNLHooks() {
  if (!sim_base_pnl_) {
    std::cerr << "ERROR SIM BASE PNL NOT SUBSCRIBED IN HDG (exiting) " << std::endl;
    exit(0);
  }
  int index = mult_base_pnl_->AddSecurity(sim_base_pnl_) - 1;
  sim_base_pnl_->AddListener(index, mult_base_pnl_);
  mult_base_pnl_->AddListener(this);
}

void HedgeTheoCalculator::SubscribeBasePNL(HFSAT::BasePNL* base_pnl) {
  int index = mult_base_pnl_->AddSecurity(base_pnl) - 1;
  base_pnl->AddListener(index, mult_base_pnl_);
}
