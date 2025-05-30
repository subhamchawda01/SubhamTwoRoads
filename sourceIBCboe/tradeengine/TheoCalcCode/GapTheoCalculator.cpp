#include "tradeengine/TheoCalc/GapTheoCalculator.hpp"
#include "tradeengine/Utils/Parser.hpp"
/*! \brief This class inherited from MidTermTheo and details the Gap Theo/alpha
 * After loading key values from historic lookback, it reacts on every bar update to take a position or squareoff
 */

GapTheoCalculator::GapTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                                               HFSAT::DebugLogger& _dbglogger_, int _trading_start_utc_mfm_,
                                               int _trading_end_utc_mfm_, int _aggressive_get_flat_mfm_,
                                               int eff_squareoff_start_utc_mfm_ = 0, double bid_multiplier_ = 1,
                                               double ask_multiplier_ = 1)
    : MidTermTheoCalculator(key_val_map, _watch_, _dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_,
                            _aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_),
      current_directional_cascade_(0),
      open_bar_close_px_(0),
      bar_count_(0),
      open_move_max_(0),
      num_bars_(0),
      trade_bar_(0) {
  // InitializeDataSubscriptions();
  dbglogger_ << watch_.tv() << " Creating GAP THEO CALCULATOR secId " << secondary_id_ << DBGLOG_ENDL_FLUSH;
  LoadParams();
  InitializeDataSubscriptions();
}
/** Loads basic parameters and historic volatility, volume and signal values
*/
void GapTheoCalculator::LoadParams() {
  MidTermTheoCalculator::LoadParams();
  num_bars_ = Parser::GetInt(key_val_map_, "NUM_BARS", 30);
  trade_bar_ = Parser::GetInt(key_val_map_, "TRADE_BAR", 4);
  if (watch_.msecs_from_midnight() < trading_start_utc_mfm_) {
    subclass_ = 4;
    BarGenerator bg_;
    bg_.getGapValues(dbglogger_, watch_, secondary_smv_->shortcode(), ticker_name_, watch_.YYYYMMDD(), num_bars_,
                      granularity_,adj_ratio_, prev_day_close_, open_move_max_);
    // dbglogger_ << ticker_name_ << " open_move_max: " << open_move_max_ << " prev_close: " << prev_day_close_ << DBGLOG_ENDL_FLUSH;
    if (prev_day_close_ == 0) {
      TurnOffTheo(CHILD_STATUS_UNSET);
    }

    last_aggregation_time_ = trading_start_utc_mfm_ / 1000 + watch_.last_midnight_sec();
    // dbglogger_ << "SL: " <<stop_loss_ << " HARD SL: " << hard_stop_loss_ <<DBGLOG_ENDL_FLUSH;
    dbglogger_ << ticker_name_ << " open_move_max: " << open_move_max_ << " prev_close: " << prev_day_close_ << DBGLOG_ENDL_FLUSH;
  }
}

/** Main Code logic present here, called after the bar is complete, size of the bar is given in config.
 * If at the end of bar, closing price crosses support px, resistance px (whichever is suitable), squareoff is called.
 *
 */

void GapTheoCalculator::onBarUpdate(const unsigned int _security_id_,
                                         const HFSAT::MarketUpdateInfo& _market_update_info_) {
  MidTermTheoCalculator::onBarUpdate(_security_id_, _market_update_info_);
  if (status_mask_ != BIT_SET_ALL) {
    return;
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
  if(bar_count_ == 0){
    open_bar_close_px_ = bar_close_price_;
  }  
  updateTrailingSL(bar_close_price_);
  // if (-1*midterm_pos_offset_ > 0) {
  //   updateTrailingSL(day_high_);
  // } else if (-1*midterm_pos_offset_ < 0) {
  //   updateTrailingSL(day_low_);
  // }
  // dbglogger_ << watch_.tv() << " GAP " << midterm_pnl_ << " " << midterm_stop_loss_ << " " << current_directional_cascade_ << " " <<support_price_ <<" " << bar_low_price_ << DBGLOG_ENDL_FLUSH; 
  if (current_cascade_ != 0 && (midterm_pnl_ < midterm_stop_loss_ || bar_volume_ > day_open_volume_|| (current_directional_cascade_ > 0 && (support_price_ >= bar_close_price_ || (bar_count_ >=trade_bar_ && bar_close_price_ < open_bar_close_px_))) ||
      (current_directional_cascade_ < 0 && (resist_price_ <= bar_close_price_ || (bar_count_ >=trade_bar_ && bar_close_price_ > open_bar_close_px_))))) {
    // If loss check is set, unset it
    if (status_mask_ & LOSS_STATUS_SET) {
      dbglogger_ << watch_.tv() << " Hitting STOP LOSS on BarUpdate " << secondary_smv_->shortcode()
                 << " PNL: " << midterm_pnl_ << " pos: " << -1*midterm_pos_offset_ << " GAP SL: " << midterm_stop_loss_
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
    // dbglogger_ << cascading_price_ << " " << current_directional_cascade_ << " " << prev_bar_close_ << " " << bar_close_price_ << " " << bar_count_ << DBGLOG_ENDL_FLUSH;
    if (bar_open_price_ != 0) {
      passive_reduce_position_ = true;
      //930 action
      if (bar_count_ == 0){
        if(open_bar_close_px_ > bar_open_price_ && bar_low_price_ >= prev_day_close_ &&
          (open_bar_close_px_ - bar_open_price_)/bar_open_price_ >= open_move_max_){

          cascading_price_ = bar_close_price_;
          current_cascade_++;
          current_directional_cascade_++;
          dbglogger_ << "GAP " << secondary_smv_->shortcode() << " CASCADE BUY "
                     << " " << bar_close_price_ << " " << prev_day_close_ << " " << day_open_price_ << DBGLOG_ENDL_FLUSH;
          if (status_mask_ == BIT_SET_ALL) {
            ComputeAndUpdateTheoListeners();
          }
        }
        else if(open_bar_close_px_ < bar_open_price_ && bar_high_price_ <= prev_day_close_ &&
          -1*(open_bar_close_px_ - bar_open_price_)/bar_open_price_ >= open_move_max_){

          cascading_price_ = bar_close_price_;
          current_cascade_++;
          current_directional_cascade_--;
          dbglogger_ << "GAP " << secondary_smv_->shortcode() << " CASCADE SELL "
                     << " " << bar_close_price_ << " " << prev_day_close_ << " " << day_open_price_ << DBGLOG_ENDL_FLUSH;
          if (status_mask_ == BIT_SET_ALL) {
            ComputeAndUpdateTheoListeners();
          }

        }
      }
      //1030 action
      else if(bar_count_ == trade_bar_ && current_cascade_ > 0 && current_cascade_ < cascade_num_ && cascade_on_ == true){
        if(current_directional_cascade_ > 0 && bar_close_price_ > open_bar_close_px_){
          midterm_pos_offset_ += -1*lot_size_;
          cascading_price_ = ((current_cascade_ - 1) * cascading_price_ + bar_close_price_) / (current_cascade_);
          current_cascade_++;
          current_directional_cascade_++;
          dbglogger_ << "GAP " << secondary_smv_->shortcode() << " CASCADE BUY "
                     << " " << bar_close_price_ << " " << prev_day_close_ << " " << day_open_price_ << DBGLOG_ENDL_FLUSH;
          if (status_mask_ == BIT_SET_ALL) {
            ComputeAndUpdateTheoListeners();
          }

        }
        else if(current_directional_cascade_ < 0 && bar_close_price_ < open_bar_close_px_){
          midterm_pos_offset_ += lot_size_;
          cascading_price_ = ((current_cascade_ - 1) * cascading_price_ + bar_close_price_) / (current_cascade_);
          current_cascade_++;
          current_directional_cascade_--;
          dbglogger_ << "GAP " << secondary_smv_->shortcode() << " CASCADE SELL "
                     << " " << bar_close_price_ << " " << prev_day_close_ << " " << day_open_price_ << DBGLOG_ENDL_FLUSH;
          if (status_mask_ == BIT_SET_ALL) {
            ComputeAndUpdateTheoListeners();
          }
        }
      }
      //beyond 1030
      else if (bar_count_ > trade_bar_ && current_cascade_ > 1 && cascade_on_ == true && current_cascade_ < cascade_num_){
        if(current_directional_cascade_ > 0){
          midterm_pos_offset_ += -1*lot_size_;
          cascading_price_ = ((current_cascade_ - 1) * cascading_price_ + bar_close_price_) / (current_cascade_);
          current_cascade_++;
          current_directional_cascade_++;
          dbglogger_ << "GAP " << secondary_smv_->shortcode() << " CASCADE BUY "
                     << " " << bar_close_price_ << " " << prev_day_close_ << " " << day_open_price_ << DBGLOG_ENDL_FLUSH;
          if (status_mask_ == BIT_SET_ALL) {
            ComputeAndUpdateTheoListeners();
          }
        }

        else if(current_directional_cascade_ < 0){
          midterm_pos_offset_ += lot_size_;
          cascading_price_ = ((current_cascade_ - 1) * cascading_price_ + bar_close_price_) / (current_cascade_);
          current_cascade_++;
          current_directional_cascade_--;
          dbglogger_ << "GAP " << secondary_smv_->shortcode() << " CASCADE SELL "
                     << " " << bar_close_price_ << " " << prev_day_close_ << " " << day_open_price_ << DBGLOG_ENDL_FLUSH;
          if (status_mask_ == BIT_SET_ALL) {
            ComputeAndUpdateTheoListeners();
          }
        }

      }
      // dbglogger_ << current_diff_price_ <<" "<< bar_close_price_ - day_open_price_ << " "<< DBGLOG_ENDL_FLUSH;
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
  bar_count_++;
}
/** Update trailing stop loss with the maximum favourable directional price reached. Dependent on position and price and
 * function of stdev of close-prev close.
*/

void GapTheoCalculator::updateTrailingSL(double _last_price_) {
  double desired_px_ = 0;
  if (current_directional_cascade_ > 0) {
    desired_px_ = _last_price_ - trailing_pt_ * (open_bar_close_px_ - day_open_price_) * std::exp(decay_factor_ * (current_cascade_ - 1));
    // dbglogger_ << _last_price_ << " " <<desired_px_ << " " << open_bar_close_px_ - day_open_price_ << " " <<std::exp(decay_factor_ * (current_cascade_ - 1)) << DBGLOG_ENDL_FLUSH;
    if (support_price_ < desired_px_) {
      support_price_ = desired_px_;
    }
  } else if (current_directional_cascade_ < 0) {
    desired_px_ = _last_price_ + trailing_pt_ * (day_open_price_ - open_bar_close_px_) * std::exp(decay_factor_ * (current_cascade_ - 1));
    if (resist_price_ > desired_px_) {
      resist_price_ = desired_px_;
    }
  }
}


void GapTheoCalculator::ReloadConfig()  {
  MidTermTheoCalculator::ReloadConfig();
}