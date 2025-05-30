#include "tradeengine/TheoCalc/VWAPTheoCalculator.hpp"
#include "tradeengine/Utils/Parser.hpp"

VWAPTheoCalculator::VWAPTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                                       HFSAT::DebugLogger& _dbglogger_, int _trading_start_utc_mfm_,
                                       int _trading_end_utc_mfm_, int _aggressive_get_flat_mfm_,
                                       int eff_squareoff_start_utc_mfm_, double bid_multiplier_, double ask_multiplier_)
    : BaseTheoCalculator(key_val_map, _watch_, _dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_,
                         _aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_),
      max_bid_percentage_offset_(0),
      max_ask_percentage_offset_(0),
      dynamic_bid_offset_(0),
      dynamic_ask_offset_(0),
      bid_increase_shift_percent_of_offset_(0),
      bid_decrease_shift_percent_of_offset_(0),
      ask_increase_shift_percent_of_offset_(0),
      ask_decrease_shift_percent_of_offset_(0),
      offset_decay_start_time_secs_(0),
      bid_offset_slope_(0),
      ask_offset_slope_(0),
      passive_reduce_position_(false),
      aggressive_reduce_position_(false),
      estimated_vwap_price_(0),
      estimated_bid_price_(0),
      estimated_ask_price_(0),
      last_traded_primary_usecs_(0),
      last_traded_primary_int_price_(0),
      primary_traded_value_(0),
      primary_traded_volume_(0),
      estimated_volume_(0),
      estimated_traded_value_(0),
      total_time_period_usecs_(0),
      data_listen_start_time_(0),
      data_listen_end_time_secs_(0),
      time_elapsed_usecs_(0),
      time_remaining_usecs_(0) {
  dbglogger_ << watch_.tv() << " Creating RATIO THEO CALCULATOR secId " << secondary_id_ << " primId " << primary0_id_
             << DBGLOG_ENDL_FLUSH;
  LoadParams();
  InitializeDataSubscriptions();
  total_time_period_usecs_ = 30 * 60 * 1000 * 1000;  // 30min * 60sec * 1000msec * 1000usec
  time_remaining_usecs_ = total_time_period_usecs_;
  int64_t data_listen_start_time_msecs =
      HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(watch_.YYYYMMDD(), 930, "UTC_");
  data_listen_start_time_ = data_listen_start_time_msecs * 1000;
  int64_t data_listen_end_time_msecs =
      HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(watch_.YYYYMMDD(), 1000, "UTC_");
  data_listen_end_time_secs_ = data_listen_end_time_msecs / 1000;
  dbglogger_ << watch_.tv() << " data_listen_start_time_ " << data_listen_start_time_ << " data_listen_end_time_secs_ "
             << data_listen_end_time_secs_ << DBGLOG_ENDL_FLUSH;
}

void VWAPTheoCalculator::LoadParams() {
  BaseTheoCalculator::LoadParams();
  bool status_ = Parser::GetBool(key_val_map_, "STATUS", false);
  status_mask_ = (status_ == true ? (status_mask_ | CONFIG_STATUS_SET) : (status_mask_ & CONFIG_STATUS_UNSET));
  max_bid_percentage_offset_ = Parser::GetDouble(key_val_map_, "MAX_BID_PERCENTAGE_OFFSET", 0) / 100;
  max_ask_percentage_offset_ = Parser::GetDouble(key_val_map_, "MAX_ASK_PERCENTAGE_OFFSET", 0) / 100;
  if (max_bid_percentage_offset_ == 0 || max_ask_percentage_offset_ == 0 ||
      max_bid_percentage_offset_ < bid_percentage_offset_ || max_ask_percentage_offset_ < ask_percentage_offset_) {
    max_bid_percentage_offset_ = bid_percentage_offset_;
    max_ask_percentage_offset_ = ask_percentage_offset_;
  }
  int offset_decay_start_time = Parser::GetInt(key_val_map_, "OFFSET_DECAY_START_TIME", 930);
  int64_t offset_decay_start_time_msecs =
      HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(watch_.YYYYMMDD(), offset_decay_start_time, "UTC_");
  offset_decay_start_time_secs_ = offset_decay_start_time_msecs / 1000;

  dbglogger_ << watch_.tv() << " LOADPARAM MAX " << max_bid_percentage_offset_ << " " << max_ask_percentage_offset_
             << " MIN " << bid_percentage_offset_ << " " << ask_percentage_offset_ << " offsetstattime "
             << offset_decay_start_time_secs_ << " int " << offset_decay_start_time << DBGLOG_ENDL_FLUSH;

  use_position_shift_manager_ = Parser::GetBool(key_val_map_, "USE_POSITION_SHIFT_MANAGER", true);
  position_shift_amount_ = Parser::GetInt(key_val_map_, "POSITION_SHIFT_AMOUNT", 0);

  // Since we use dynamic offsets, shift offsets are given as a percentage of the current offset being used.
  // so BID_INCREASE_SHIFT_PERCENT_OF_OFFSET = 50 would mean 50% of current dynamic offset.
  bid_increase_shift_percent_of_offset_ =
      Parser::GetDouble(key_val_map_, "BID_INCREASE_SHIFT_PERCENT_OF_OFFSET", 0) / 100;
  bid_decrease_shift_percent_of_offset_ =
      Parser::GetDouble(key_val_map_, "BID_DECREASE_SHIFT_PERCENT_OF_OFFSET", 0) / 100;
  ask_increase_shift_percent_of_offset_ =
      Parser::GetDouble(key_val_map_, "ASK_INCREASE_SHIFT_PERCENT_OF_OFFSET", 0) / 100;
  ask_decrease_shift_percent_of_offset_ =
      Parser::GetDouble(key_val_map_, "ASK_DECREASE_SHIFT_PERCENT_OF_OFFSET", 0) / 100;
  stop_loss_ = Parser::GetDouble(key_val_map_, "STOP_LOSS", 0);
  stop_loss_ = stop_loss_ * -1;

  // This is always greater than normal stop loss
  hard_stop_loss_ = Parser::GetDouble(key_val_map_, "HARD_STOP_LOSS", 50000);
  hard_stop_loss_ = hard_stop_loss_ * -1;
  hard_stop_loss_ = std::min(hard_stop_loss_, stop_loss_);

  passive_reduce_position_ = Parser::GetBool(key_val_map_, "PASSIVE_REDUCE_POSITION", false);
  aggressive_reduce_position_ = Parser::GetBool(key_val_map_, "AGGRESSIVE_REDUCE_POSITION", false);

  if (!position_shift_amount_) {
    dbglogger_ << watch_.tv() << " POSITION SHIFT MGR SET FALSE SINCE POSITION SHIFT AMOUNT IS "
               << position_shift_amount_ << " in " << theo_identifier_ << DBGLOG_ENDL_FLUSH;
    use_position_shift_manager_ = false;
  }

  for (auto base_exec : base_exec_vec_) {
    base_exec->SetPassiveReduce(passive_reduce_position_);
    base_exec->SetAggressiveReduce(aggressive_reduce_position_);
  }
}

void VWAPTheoCalculator::UpdateTheoPrices(const unsigned int _security_id_,
                                          const HFSAT::MarketUpdateInfo& _market_update_info_) {
  int msecs_from_midnight = watch_.msecs_from_midnight();

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
      if ((!is_ready_ || config_reloaded_) && (msecs_from_midnight > trading_start_utc_mfm_) && (start_trading_)) {
        OnReady(primary0_smv_->market_update_info());
        is_ready_ = true;
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
    }
  }

  if ((secondary_book_valid_) && (primary_book_valid_)) {
    if (is_ready_ && ((status_mask_ & INVALIDBOOK_STATUS_SET) == 0)) {
      TurnOnTheo(INVALIDBOOK_STATUS_SET);
    }

    ComputeAndUpdateTheoListeners();
    /*dbglogger_ << watch_.tv() << " " << theo_identifier_ << " SecID: " <<  _security_id_
      << " Sec: " << sec_name_indexer_.GetShortcodeFromId(_security_id_)
      << " PPx " << theo_values_.primary_best_bid_price_
      << " X " << theo_values_.primary_best_ask_price_
      << "::::SPx " << secondary_smv_->market_update_info().bestbid_price_
      << " X " << secondary_smv_->market_update_info().bestask_price_
      << "::ratio "<< ratio_
      << "::theopx "<< theo_values_.theo_bid_price_ << " X " << theo_values_.theo_ask_price_ << DBGLOG_ENDL_FLUSH;*/
  }
}

void VWAPTheoCalculator::ComputeAndUpdateTheoListeners() {
  if (!need_to_hedge_) {
    SetPositionOffsets(position_);
  }

  double prev_theo_bid_price_ = theo_values_.theo_bid_price_;
  double prev_theo_ask_price_ = theo_values_.theo_ask_price_;

  theo_values_.theo_bid_price_ = estimated_vwap_price_ - dynamic_bid_offset_ * (1 - bid_theo_shifts_);
  theo_values_.theo_ask_price_ = estimated_vwap_price_ + dynamic_ask_offset_ * (1 + ask_theo_shifts_);

  if ((prev_theo_bid_price_ + prev_theo_ask_price_) < (theo_values_.theo_bid_price_ + theo_values_.theo_ask_price_)) {
    theo_values_.movement_indicator_ = pUPWARD;
  } else {
    theo_values_.movement_indicator_ = pDOWNWARD;
  }

  UpdateTheoListeners();
}

void VWAPTheoCalculator::OnReady(const HFSAT::MarketUpdateInfo& _market_update_info_) {
  bid_offset_ = bid_percentage_offset_ * _market_update_info_.bestbid_price_;
  ask_offset_ = ask_percentage_offset_ * _market_update_info_.bestbid_price_;

  max_bid_offset_ = max_bid_percentage_offset_ * _market_update_info_.bestbid_price_;
  max_ask_offset_ = max_ask_percentage_offset_ * _market_update_info_.bestbid_price_;

  dynamic_bid_offset_ = max_bid_offset_;
  dynamic_ask_offset_ = max_ask_offset_;

  bid_offset_slope_ = (max_bid_offset_ - bid_offset_) / (data_listen_end_time_secs_ - offset_decay_start_time_secs_);
  ask_offset_slope_ = (max_ask_offset_ - ask_offset_) / (data_listen_end_time_secs_ - offset_decay_start_time_secs_);

  bid_increase_shift_ = bid_increase_shift_percent_ * _market_update_info_.bestbid_price_;
  bid_decrease_shift_ = bid_decrease_shift_percent_ * _market_update_info_.bestbid_price_;
  ask_increase_shift_ = ask_increase_shift_percent_ * _market_update_info_.bestbid_price_;
  ask_decrease_shift_ = ask_decrease_shift_percent_ * _market_update_info_.bestbid_price_;

  avg_int_spread_ = std::max(int(avg_spread_percent_ * _market_update_info_.bestbid_int_price_), 1);
  long_ema_int_spread_ = avg_int_spread_;
  short_ema_int_spread_ = avg_int_spread_;
  inv_avg_int_spread_ = 1 / avg_int_spread_;
  total_offset_in_avg_spread_ = (bid_percentage_offset_ + ask_percentage_offset_) / avg_spread_percent_;
  slope_obb_offset_mult_ = 1 / (total_offset_in_avg_spread_);
  intercept_obb_offset_mult_ = (total_offset_in_avg_spread_ + 1) / (total_offset_in_avg_spread_);

  dbglogger_ << watch_.tv() << " " << theo_identifier_ << " Offsets:" << bid_offset_ << "|" << ask_offset_ << " max "
             << max_bid_offset_ << "|" << max_ask_offset_ << " slope " << bid_offset_slope_ << "|" << ask_offset_slope_
             << "|" << bid_increase_shift_ << "|" << bid_decrease_shift_ << "|" << ask_increase_shift_ << "|"
             << ask_decrease_shift_ << "\n";
  for (auto base_exec : base_exec_vec_) {
    base_exec->SetPassiveReduce(passive_reduce_position_);
    base_exec->SetAggressiveReduce(aggressive_reduce_position_);
    base_exec->OnReady(_market_update_info_);
  }
  config_reloaded_ = false;
}

void VWAPTheoCalculator::OnExec(const int _new_position_, const int _exec_quantity_, const HFSAT::TradeType_t _buysell_,
                                const double _price_, const int r_int_price_, const int _security_id_,
                                const int _caos_) {
  num_trades_++;
  current_position_ = _new_position_;
  position_ = _new_position_ /*+ target_position_*/;
  // volume_ += _exec_quantity_;
  total_traded_value_ += _exec_quantity_ * _price_;
  strat_ltp_ = _price_;
  bool fill_type = false;
  if ((_buysell_ == HFSAT::kTradeTypeBuy && _price_ < theo_values_.theo_bid_price_) ||
      (_buysell_ == HFSAT::kTradeTypeSell && _price_ > theo_values_.theo_ask_price_)) {
    fill_type = true;
  }

  if (need_to_hedge_) {
    NotifyHedgeTheoListeners();
  } else {
    SetPositionOffsets(position_);
  }

  if (status_mask_ == BIT_SET_ALL) {
    ComputeAndUpdateTheoListeners();
  }
  dbglogger_ << watch_.tv() << " " << secondary_smv_->shortcode() << " TheoOnExec pos: " << position_
             << (_buysell_ == HFSAT::kTradeTypeBuy ? " BUY" : " SELL") << " px: " << _price_ << " caos: " << _caos_
             << " theo[" << theo_values_.theo_bid_price_ << " X " << theo_values_.theo_ask_price_ << "]"
             << " mkt[" << secondary_smv_->market_update_info().bestbid_price_ << " X "
             << secondary_smv_->market_update_info().bestask_price_ << "]"
             << " primkt[" << theo_values_.primary_best_bid_price_ << " X " << theo_values_.primary_best_ask_price_
             << "]"
             << " refprim[" << theo_values_.reference_primary_bid_ << " X " << theo_values_.reference_primary_ask_
             << "]"
             << " vol: " << basic_om_->trade_volume() << " tpos: " << target_position_
             << " ttv: " << total_traded_value_ << " evwap: " << estimated_vwap_price_ << (fill_type ? " GOOD" : " BAD")
             << "\n";
}

void VWAPTheoCalculator::UpdateTheoListeners() {
  if (status_mask_ != BIT_SET_ALL) {
    theo_values_.is_valid_ = false;
    CancelBid();
    CancelAsk();
    return;
  }
  if (start_trading_ && is_ready_) {
    if (!theo_values_.theo_bid_price_ || !theo_values_.theo_ask_price_ ||
        theo_values_.theo_ask_price_ - theo_values_.theo_bid_price_ <
            spread_check_percent_ * theo_values_.theo_bid_price_) {
      dbglogger_ << watch_.tv() << " ERROR theo spread is violated " << secondary_smv_->shortcode() << " theo bid "
                 << theo_values_.theo_bid_price_ << " ask " << theo_values_.theo_ask_price_ << " spread "
                 << theo_values_.theo_ask_price_ - theo_values_.theo_bid_price_ << " spread limit "
                 << spread_check_percent_ * theo_values_.theo_bid_price_ << DBGLOG_ENDL_FLUSH;
      TurnOffTheo(SPREADLIMIT_STATUS_UNSET);
      return;
    }

    theo_values_.is_valid_ = true;
    for (auto base_exec : base_exec_vec_) {
      base_exec->OnTheoUpdate();
    }
  }
}

void VWAPTheoCalculator::SetupPNLHooks() {
  if (!sim_base_pnl_) {
    std::cerr << "ERROR SIM BASE PNL NOT SUBSCRIBED (exiting)" << std::endl;
    exit(0);
  }

  if (!need_to_hedge_) {
    sim_base_pnl_->AddListener(0, this);
  } else {
    for (auto hdg_elem : hedge_vector_) {
      hdg_elem->hedge_theo_->SubscribeBasePNL(sim_base_pnl_);
    }
  }
}
