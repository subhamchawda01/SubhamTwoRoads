#include "tradeengine/TheoCalc/MACDTheoCalculator.hpp"
#include "tradeengine/Utils/Parser.hpp"
/*! \brief This class inherited from MidTermTheo and details the MACD Theo/alpha
 * After loading key values from historic lookback, it reacts on every bar update to take a position or squareoff
 */

MACDTheoCalculator::MACDTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                                       HFSAT::DebugLogger& _dbglogger_, int _trading_start_utc_mfm_,
                                       int _trading_end_utc_mfm_, int _aggressive_get_flat_mfm_,
                                       int eff_squareoff_start_utc_mfm_ = 0, double bid_multiplier_ = 1,
                                       double ask_multiplier_ = 1)
    : MidTermTheoCalculator(key_val_map, _watch_, _dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_,
                            _aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_),
      sema_param_(0),
      lema_param_(0),
      signal_param_(0),
      bollinger_period_(0),
      num_bars_(0),
      day_bollinger_(0),
      sema_(0),
      lema_(0),
      macd_(0),
      signal_(0),
      prev_sema_(0),
      prev_lema_(0),
      prev_macd_(0),
      prev_signal_(0),
      last_crossover_(0),
      macd_crossover_lkbk_(0),
      current_directional_cascade_(0) {
  // InitializeDataSubscriptions();
  dbglogger_ << watch_.tv() << " Creating MACD THEO CALCULATOR secId " << secondary_id_ << DBGLOG_ENDL_FLUSH;
  LoadParams();
  InitializeDataSubscriptions();
}
/** Loads basic parameters and bollinger, historic volatility, volume and signal values
*/
void MACDTheoCalculator::LoadParams() {
  MidTermTheoCalculator::LoadParams();
  sema_param_ = Parser::GetInt(key_val_map_, "SEMA", 24);
  lema_param_ = Parser::GetInt(key_val_map_, "LEMA", 52);
  signal_param_ = Parser::GetInt(key_val_map_, "SIGNAL", 27);
  macd_crossover_lkbk_ = Parser::GetInt(key_val_map_, "CROSSOVER_HIST", 10);
  // bollinger_period_ = Parser::GetInt(key_val_map_,"BOLLINGER_PERIOD",20);
  double temp_;
  num_bars_ = Parser::GetInt(key_val_map_, "NUM_BARS", 300);
  if (watch_.msecs_from_midnight() < trading_start_utc_mfm_) {
    subclass_ = 2;
    BarGenerator bg_;
    bg_.getKeyFilters(dbglogger_, watch_, secondary_smv_->shortcode(), ticker_name_, watch_.YYYYMMDD(), open_vol_days_,
                      open_vol_ptile_, granularity_, cc_lkbk_, lt_days_, moment_cc_wt_mean_, moment_cc_wt_std_,
                      opening_volume_avg_, long_term_vol_std_, long_term_vol_mean_, prev_day_close_, adj_ratio_, temp_);
    bg_.getMACDParams(day_bollinger_, num_bars_, sema_param_, lema_param_, signal_param_, sema_, lema_, macd_, signal_,
                      last_crossover_);
    last_crossover_ = 100 + macd_crossover_lkbk_;
    prev_sema_ = sema_;
    prev_lema_ = lema_;
    prev_macd_ = macd_;
    prev_signal_ = signal_;
    if (prev_day_close_ == 0) {
      TurnOffTheo(CHILD_STATUS_UNSET);
    }
    moment_cc_wt_mean_ *= cc_beta_;
    moment_cc_wt_std_ *= cc_alpha_;
    long_term_vol_std_ /= prev_day_close_;
    last_aggregation_time_ = trading_start_utc_mfm_ / 1000 + watch_.last_midnight_sec();
    dbglogger_ << ticker_name_ << " std: " << moment_cc_wt_std_ << " day_bollinger: " << day_bollinger_
               << " Long_term_vol: " << long_term_vol_std_ << " " << long_term_vol_mean_
               << " last_crossover: " << last_crossover_ << " sema: " << sema_ << " lema: " << lema_
               << " macd: " << macd_ << " signal: " << signal_ << " prev_close: " << prev_day_close_
               << DBGLOG_ENDL_FLUSH;
  }
}

/** Main Code logic present here, called after the bar is complete, size of the bar is given in config.
 * Bar short term ema and long term ema are computed here, along with macd and signal.
 * If at the end of bar, closing price crosses support px, resistance px (whichever is suitable), squareoff is called.
 *
 */
void MACDTheoCalculator::onBarUpdate(const unsigned int _security_id_,
                                     const HFSAT::MarketUpdateInfo& _market_update_info_) {
  MidTermTheoCalculator::onBarUpdate(_security_id_, _market_update_info_);
  if (status_mask_ != BIT_SET_ALL) {
    return;
  }
  if (bar_close_price_ != 0) {
    sema_ = 2.0 * bar_close_price_ / (sema_param_ + 1) + prev_sema_ * (1.0 - (2.0 / (sema_param_ + 1)));
    lema_ = 2 * bar_close_price_ / (lema_param_ + 1) + prev_lema_ * (1.0 - (2.0 / (lema_param_ + 1)));
    macd_ = sema_ - lema_;
    signal_ = 2.0 * macd_ / (signal_param_ + 1) + prev_signal_ * (1.0 - (2.0 / (signal_param_ + 1)));
    dbglogger_ << watch_.tv() << " MACD " << sec_name_indexer_.GetShortcodeFromId(secondary_id_) << " " << sema_ << " "
               << lema_ << " " << macd_ << " " << signal_ << DBGLOG_ENDL_FLUSH;
  }
  if ((macd_ - signal_) * (prev_macd_ - prev_signal_) < 0) {
    last_crossover_ = 0;
  } else {
    last_crossover_++;
  }

  if (bar_close_price_ == 0) {
    bar_close_price_ = prev_bar_close_;
    bar_low_price_ = prev_bar_low_;
    bar_high_price_ = prev_bar_high_;
  }
  if (day_high_ == 0) {
    day_high_ = bar_high_price_;
  } else {
    if (day_high_ < bar_high_price_) {
      day_high_ = bar_high_price_;
    }
  }

  if (day_low_ == 0) {
    day_low_ = bar_low_price_;
  } else {
    if (day_low_ > bar_low_price_) {
      day_low_ = bar_low_price_;
    }
  }

  updateTrailingSL(bar_close_price_);
  if (midterm_pnl_ < midterm_stop_loss_ || (current_directional_cascade_ > 0 && support_price_ >= bar_close_price_) ||
      (current_directional_cascade_ < 0 && resist_price_ <= bar_close_price_)) {
    // If loss check is set, unset it
    if (status_mask_ & LOSS_STATUS_SET) {
      dbglogger_ << watch_.tv() << " Hitting STOP LOSS on BarUpdate " << secondary_smv_->shortcode()
                 << " PNL: " << midterm_pnl_ << " pos: " << -1*midterm_pos_offset_ << " MACD SL: " << midterm_stop_loss_
                 << " SL: " << stop_loss_ << " HardSL: " << hard_stop_loss_ << " Support px: " << support_price_
                 << " Resist px: " << resist_price_ << " secmkt[" << _market_update_info_.bestbid_price_ << " x "
                 << _market_update_info_.bestask_price_ << "] Stop loss decay "
                 << std::exp(decay_factor_ * (current_cascade_ - 1)) << DBGLOG_ENDL_FLUSH;
      dbglogger_ << secondary_smv_->shortcode() << " SQUAREOFF " << bar_close_price_ << " " << adj_ratio_
                 << DBGLOG_ENDL_FLUSH;
      SquareOff();
    }
    TurnOffTheo(LOSS_STATUS_UNSET);
  } else {
    if (current_cascade_ != 0 && ((current_directional_cascade_ > 0 && prev_bar_close_ < bar_close_price_ &&
                                   cascading_price_ <= bar_close_price_) ||
                                  (current_directional_cascade_ < 0 && prev_bar_close_ > bar_close_price_ &&
                                   cascading_price_ >= bar_close_price_))) {
      cascade_on_ = true;
    } else {
      cascade_on_ = false;
    }
    if (bar_open_price_ != 0 &&
        ((current_directional_cascade_ == 0 && last_crossover_ < macd_crossover_lkbk_) || cascade_on_ == true) &&
        opening_volume_avg_ < day_open_volume_) {
      passive_reduce_position_ = true;

      if (current_cascade_ < cascade_num_) {
        if (bar_close_price_ > day_bollinger_ && prev_day_close_ < day_bollinger_ && macd_ - signal_ > 0) {
          if (current_cascade_ != 0) {
            midterm_pos_offset_ += -1 * lot_size_;
            cascading_price_ = ((current_cascade_ - 1) * cascading_price_ + bar_close_price_) / (current_cascade_);
          } else {
            cascading_price_ = bar_close_price_;
          }
          if (current_cascade_ != 0 || (day_high_ == bar_high_price_ and bar_close_price_ > bar_open_price_)) {
            current_cascade_++;
            current_directional_cascade_++;
            updateTrailingSL(bar_close_price_);
            dbglogger_ << "MACD " << secondary_smv_->shortcode() << " CASCADE BUY "
                       << " " << bar_close_price_ << " " << prev_day_close_ << " " << day_open_price_ << " "
                       << moment_cc_wt_std_ + moment_cc_wt_mean_ << " " << adj_ratio_ << DBGLOG_ENDL_FLUSH;
            dbglogger_ << secondary_smv_->shortcode() << " " << current_cascade_ << " " << cascading_price_ << " "
                       << prev_bar_close_ << DBGLOG_ENDL_FLUSH;
            if (status_mask_ == BIT_SET_ALL) {
              ComputeAndUpdateTheoListeners();
            }
          }
        } else if (bar_close_price_ < day_bollinger_ && prev_day_close_ > day_bollinger_ && macd_ - signal_ < 0) {
          if (current_cascade_ != 0) {
            midterm_pos_offset_ += lot_size_;
            cascading_price_ = ((current_cascade_ - 1) * cascading_price_ + bar_close_price_) / (current_cascade_);
          } else {
            cascading_price_ = bar_close_price_;
          }
          if (current_cascade_ != 0 || (day_low_ == bar_low_price_ and bar_close_price_ < bar_open_price_)) {
            current_cascade_++;
            current_directional_cascade_--;
            updateTrailingSL(bar_close_price_);
            dbglogger_ << "MACD " << secondary_smv_->shortcode() << " CASCADE SELL " << bar_close_price_ << " " << prev_day_close_
                       << " " << day_open_price_ << " " << moment_cc_wt_std_ + moment_cc_wt_mean_ << " " << adj_ratio_
                       << DBGLOG_ENDL_FLUSH;
            dbglogger_ << secondary_smv_->shortcode() << " " << current_cascade_ << " " << cascading_price_ << " "
                       << prev_bar_close_ << DBGLOG_ENDL_FLUSH;
            if (status_mask_ == BIT_SET_ALL) {
              ComputeAndUpdateTheoListeners();
            }
          }
        }
      }
    }
  }
  prev_bar_pnl_ = midterm_pnl_;
  prev_bar_close_ = bar_close_price_;
  prev_bar_low_ = bar_low_price_;
  prev_bar_high_ = bar_high_price_;
  last_aggregation_time_ += granularity_;
  bar_open_price_ = 0;
  bar_low_price_ = 10000000;
  bar_high_price_ = 0;
  bar_close_price_ = 0;
  bar_volume_ = 0;
  prev_sema_ = sema_;
  prev_lema_ = lema_;
  prev_macd_ = macd_;
  prev_signal_ = signal_;
}

/** Update trailing stop loss with the closing price reached. Dependent on position and price and function of stdev of
 * close-prev close.
*/

void MACDTheoCalculator::updateTrailingSL(double _last_price_) {
  double desired_px_ = 0;
  if (current_directional_cascade_ > 0) {
    desired_px_ = _last_price_ - trailing_pt_ * moment_cc_wt_std_ * std::exp(decay_factor_ * (current_cascade_ - 1));
    if (support_price_ < desired_px_) {
      support_price_ = desired_px_;
    }
  } else if (current_directional_cascade_ < 0) {
    desired_px_ = _last_price_ + trailing_pt_ * moment_cc_wt_std_ * std::exp(decay_factor_ * (current_cascade_ - 1));
    if (resist_price_ > desired_px_) {
      resist_price_ = desired_px_;
    }
  }
}

void MACDTheoCalculator::ReloadConfig()  {
  MidTermTheoCalculator::ReloadConfig();
}