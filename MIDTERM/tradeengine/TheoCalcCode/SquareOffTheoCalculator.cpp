#include "tradeengine/TheoCalc/SquareOffTheoCalculator.hpp"
#include "tradeengine/Executioner/Dimer.hpp"
#include "tradeengine/Executioner/ElectronicEye.hpp"
#include "tradeengine/Executioner/Quoter.hpp"
#include "tradeengine/Executioner/RushQuoter.hpp"
#include "tradeengine/Utils/Parser.hpp"

SquareOffTheoCalculator::SquareOffTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                                                 HFSAT::DebugLogger& _dbglogger_, int _trading_start_utc_mfm_,
                                                 int _trading_end_utc_mfm_, int _aggressive_get_flat_mfm_)
    : BaseTheoCalculator(key_val_map, _watch_, _dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_,
                         _aggressive_get_flat_mfm_, 0, 1, 1),
      agg_sqoff_(false),
      is_activated_(false) {
  LoadParams();
}

void SquareOffTheoCalculator::LoadParams() {
  BaseTheoCalculator::LoadParams();
  bool status_ = Parser::GetBool(key_val_map_, "STATUS", false);
  status_mask_ = (status_ == true ? (status_mask_ | CONFIG_STATUS_SET) : (status_mask_ & CONFIG_STATUS_UNSET));
  bid_offset_ = Parser::GetDouble(key_val_map_, "BID_OFFSET", 1);
  ask_offset_ = Parser::GetDouble(key_val_map_, "ASK_OFFSET", 1);
  stop_loss_ = Parser::GetDouble(key_val_map_, "STOP_LOSS", 0);
  num_trades_limit_ = Parser::GetDouble(key_val_map_, "NUM_TRADES_LIMIT", 10);
  stop_loss_ = stop_loss_ * -1;
  // This is always greater than normal stop loss
  hard_stop_loss_ = Parser::GetDouble(key_val_map_, "HARD_STOP_LOSS", 50000);
  hard_stop_loss_ = hard_stop_loss_ * -1;
  hard_stop_loss_ = std::min(hard_stop_loss_, stop_loss_);
}

void SquareOffTheoCalculator::UpdateTheoPrices(const unsigned int _security_id_,
                                               const HFSAT::MarketUpdateInfo& _market_update_info_) {
  // DBGLOG_TIME_CLASS_FUNC << " Got Update " << _security_id_ << " secondary id " << secondary_id_ << " primary id " <<
  // primary0_id_ << DBGLOG_ENDL_FLUSH;

  int prev_theo_bid_price_ = theo_values_.theo_bid_price_;
  int prev_theo_ask_price_ = theo_values_.theo_ask_price_;

  if (num_trades_ > num_trades_limit_) {
    if (status_mask_ & SHOOT_STATUS_SET) {
      dbglogger_ << watch_.tv() << " ERROR SQUAREOFF TRADES LIMIT HIT "
                 << sec_name_indexer_.GetShortcodeFromId(primary0_id_) << " TOTAL_TRADES: " << num_trades_
                 << " TRADES_LIMIT: " << num_trades_limit_ << DBGLOG_ENDL_FLUSH;
    }
    TurnOffTheo(SHOOT_STATUS_UNSET);
  } else {
    if (is_ready_ && ((status_mask_ & SHOOT_STATUS_SET) == 0)) {
      TurnOnTheo(SHOOT_STATUS_SET);
    }
  }

  if (watch_.msecs_from_midnight() > trading_end_utc_mfm_ + 600000) {
    agg_sqoff_ = true;
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
      if ((!is_ready_ || config_reloaded_) && watch_.msecs_from_midnight() > trading_start_utc_mfm_) {
        OnReady(_market_update_info_);
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
      }
      if (agg_sqoff_) {
        if (theo_values_.position_to_offset_ > 0 &&
            secondary_smv_->market_update_info_.bestbid_price_ != kInvalidPrice) {
          if ((!is_ready_ || config_reloaded_) && watch_.msecs_from_midnight() > trading_start_utc_mfm_) {
            OnReady(_market_update_info_);
            is_ready_ = true;
          }
          dbglogger_ << watch_.tv() << " FAVOURABLE INVALID BOOK IN AGGRESSIVE FLAT "
                     << sec_name_indexer_.GetShortcodeFromId(secondary_id_) << " bid "
                     << secondary_smv_->market_update_info_.bestbid_price_ << " ask "
                     << secondary_smv_->market_update_info_.bestask_price_ << " position to close "
                     << theo_values_.position_to_offset_ << DBGLOG_ENDL_FLUSH;
          theo_values_.theo_bid_price_ = secondary_smv_->market_update_info().bestbid_price_ - bid_offset_;
          theo_values_.theo_ask_price_ = secondary_smv_->market_update_info().bestbid_price_ + bid_offset_;
          for (auto base_exec : base_exec_vec_) {
            base_exec->SetMaxOffset(-bid_offset_, -bid_offset_, true);
          }
          TurnOnTheo(INVALIDBOOK_STATUS_SET);
          UpdateTheoListeners();
        } else if (theo_values_.position_to_offset_ < 0 &&
                   secondary_smv_->market_update_info_.bestask_price_ != kInvalidPrice) {
          if ((!is_ready_ || config_reloaded_) && watch_.msecs_from_midnight() > trading_start_utc_mfm_) {
            OnReady(_market_update_info_);
            is_ready_ = true;
          }
          dbglogger_ << watch_.tv() << " FAVOURABLE INVALID BOOK IN AGGRESSIVE FLAT "
                     << sec_name_indexer_.GetShortcodeFromId(secondary_id_) << " bid "
                     << secondary_smv_->market_update_info_.bestbid_price_ << " ask "
                     << secondary_smv_->market_update_info_.bestask_price_ << " position to close "
                     << theo_values_.position_to_offset_ << DBGLOG_ENDL_FLUSH;
          theo_values_.theo_bid_price_ = secondary_smv_->market_update_info().bestask_price_ - ask_offset_;
          theo_values_.theo_ask_price_ = secondary_smv_->market_update_info().bestask_price_ + ask_offset_;
          for (auto base_exec : base_exec_vec_) {
            base_exec->SetMaxOffset(-ask_offset_, -ask_offset_, true);
          }
          TurnOnTheo(INVALIDBOOK_STATUS_SET);
          UpdateTheoListeners();
        }
      } else {
        TurnOffTheo(INVALIDBOOK_STATUS_UNSET);
      }
      secondary_book_valid_ = false;
    } else {
      secondary_book_valid_ = true;
      theo_values_.theo_bid_price_ = secondary_smv_->market_update_info().bestbid_price_ - bid_offset_;
      theo_values_.theo_ask_price_ = secondary_smv_->market_update_info().bestask_price_ + ask_offset_;
      if (agg_sqoff_) {
        for (auto base_exec : base_exec_vec_) {
          double spread =
              secondary_smv_->market_update_info().bestask_price_ - secondary_smv_->market_update_info().bestbid_price_;
          base_exec->SetMaxOffset(-bid_offset_ - spread, -ask_offset_ - spread);
        }
      }
      if ((prev_theo_bid_price_ + prev_theo_ask_price_) <
          (theo_values_.theo_bid_price_ + theo_values_.theo_ask_price_)) {
        theo_values_.movement_indicator_ = pUPWARD;
      } else {
        theo_values_.movement_indicator_ = pDOWNWARD;
      }
    }
  }

  if ((secondary_book_valid_) && (primary_book_valid_)) {
    if (is_ready_ && ((status_mask_ & INVALIDBOOK_STATUS_SET) == 0)) {
      TurnOnTheo(INVALIDBOOK_STATUS_SET);
    }

    UpdateTheoListeners();
  }

  /*DBGLOG_TIME_CLASS_FUNC << secondary_smv_->shortcode() << " SecID: " <<  _security_id_
          << " PPx " << primary0_smv_->market_update_info().bestbid_price_
          << " X " << primary0_smv_->market_update_info().bestask_price_
          << "::::SPx " << secondary_smv_->market_update_info().bestbid_price_
          << " X " << secondary_smv_->market_update_info().bestask_price_
          << "::theopx "<< theo_values_.theo_bid_price_ << " X " << theo_values_.theo_ask_price_ << DBGLOG_ENDL_FLUSH;*/
}

void SquareOffTheoCalculator::OnReady(const HFSAT::MarketUpdateInfo& _market_update_info_) {
  dbglogger_ << watch_.tv() << "SQUAREOFF THEO Offsets:" << bid_offset_ << "|" << ask_offset_ << "\n";
  avg_int_spread_ = std::max(int(avg_spread_percent_ * _market_update_info_.bestbid_int_price_), 1);
  long_ema_int_spread_ = avg_int_spread_;
  short_ema_int_spread_ = avg_int_spread_;
  inv_avg_int_spread_ = 1 / avg_int_spread_;
  total_offset_in_avg_spread_ = (bid_percentage_offset_ + ask_percentage_offset_) / avg_spread_percent_;
  slope_obb_offset_mult_ = 1 / (total_offset_in_avg_spread_);
  intercept_obb_offset_mult_ = (total_offset_in_avg_spread_ + 1) / (total_offset_in_avg_spread_);

  for (auto base_exec : base_exec_vec_) {
    base_exec->OnReady(_market_update_info_);
  }
  config_reloaded_ = false;
}

void SquareOffTheoCalculator::OnExec(const int _new_position_, const int _exec_quantity_,
                                     const HFSAT::TradeType_t _buysell_, const double _price_, const int r_int_price_,
                                     const int _security_id_, const int _caos_) {
  if (!is_activated_) return;
  num_trades_++;
  int side = (_buysell_ == HFSAT::kTradeTypeBuy) ? 1 : -1;
  theo_values_.position_to_offset_ = theo_values_.position_to_offset_ + (side * _exec_quantity_);
  strat_ltp_ = _price_;
  dbglogger_ << watch_.tv() << secondary_smv_->shortcode()
             << " SquareOffTheo OnExec Position: " << theo_values_.position_to_offset_ << " volume "
             << basic_om_->trade_volume() << DBGLOG_ENDL_FLUSH;
  if (status_mask_ == BIT_SET_ALL) {
    UpdateTheoListeners();
  }
}

void SquareOffTheoCalculator::UpdateTheoListeners() {
  if ((!is_activated_) || (status_mask_ != BIT_SET_ALL)) {
    theo_values_.is_valid_ = false;
    CancelBid();
    CancelAsk();
    return;
  }
  if (is_activated_ && is_ready_) {
    if (theo_values_.theo_ask_price_ - theo_values_.theo_bid_price_ <
        spread_check_percent_ * theo_values_.theo_bid_price_) {
      /*DBGLOG_TIME_CLASS_FUNC << "Error! theo spread is violated "
              << DBGLOG_ENDL_FLUSH;*/
    }

    theo_values_.reference_primary_bid_ = theo_values_.primary_best_bid_price_;
    theo_values_.reference_primary_ask_ = theo_values_.primary_best_ask_price_;

    theo_values_.is_valid_ = true;
    for (auto base_exec : base_exec_vec_) {
      base_exec->OnTheoUpdate();
    }
  }
}
