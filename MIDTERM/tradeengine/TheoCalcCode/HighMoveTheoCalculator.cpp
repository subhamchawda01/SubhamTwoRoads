#include "tradeengine/TheoCalc/HighMoveTheoCalculator.hpp"
#include "tradeengine/Utils/Parser.hpp"
/*! \brief This class inherited from MidTermTheo and details the high move Theo/alpha
 * After loading key values from historic lookback, it reacts on every bar update to take a position or squareoff
 */

HighMoveTheoCalculator::HighMoveTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                                               HFSAT::DebugLogger& _dbglogger_, int _trading_start_utc_mfm_,
                                               int _trading_end_utc_mfm_, int _aggressive_get_flat_mfm_,
                                               int eff_squareoff_start_utc_mfm_ = 0, double bid_multiplier_ = 1,
                                               double ask_multiplier_ = 1)
    : MidTermTheoCalculator(key_val_map, _watch_, _dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_,
                            _aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_),
      current_directional_cascade_(0),
      threshold_(0),
      current_obv_(0),
      max_obv_(0),
      prev_bar_open_(0),
      prev_bar_vol_(0),
      prev_prev_bar_close_(0),
      neg_pnl_count_(0) {
  // InitializeDataSubscriptions();
  dbglogger_ << watch_.tv() << " Creating High Move THEO CALCULATOR secId " << secondary_id_ << DBGLOG_ENDL_FLUSH;
  LoadParams();
  InitializeDataSubscriptions();
}
/** Loads basic parameters and historic volatility, volume and signal values
*/
void HighMoveTheoCalculator::LoadParams() {
  MidTermTheoCalculator::LoadParams();

  if (watch_.msecs_from_midnight() < trading_start_utc_mfm_) {
    subclass_ = 7;
    threshold_ = Parser::GetInt(key_val_map_, "THRESHOLD", 0.0015);
    last_aggregation_time_ = trading_start_utc_mfm_ / 1000 + watch_.last_midnight_sec();
    BarGenerator bg_;
    bg_.getKeyFilters(dbglogger_, watch_, secondary_smv_->shortcode(), ticker_name_, watch_.YYYYMMDD(), open_vol_days_,
                      open_vol_ptile_, granularity_, cc_lkbk_, lt_days_, moment_cc_wt_mean_, moment_cc_wt_std_,
                      opening_volume_avg_, long_term_vol_std_, long_term_vol_mean_, prev_day_close_, adj_ratio_,
                      long_term_obv_std_, obv_lt_days_);
    // if (prev_day_close_ == 0) {
    //   TurnOffTheo(CHILD_STATUS_UNSET);
    // }
  //   moment_cc_wt_mean_ *= cc_beta_;
  //   moment_cc_wt_std_ *= cc_alpha_;
  //   long_term_vol_std_ /= prev_day_close_;
  //   long_term_vol_mean_ /= prev_day_close_;
  //   last_aggregation_time_ = trading_start_utc_mfm_ / 1000 + watch_.last_midnight_sec();
  //   // dbglogger_ << "SL: " <<stop_loss_ << " HARD SL: " << hard_stop_loss_ <<DBGLOG_ENDL_FLUSH;
  //   dbglogger_ << ticker_name_ << " std: " << moment_cc_wt_std_ << " mean: " << moment_cc_wt_mean_
  //              << " long_term_vol: " << long_term_vol_std_ << " " << long_term_vol_mean_ << " " << long_term_obv_std_
  //              << " prev_close: " << prev_day_close_ << DBGLOG_ENDL_FLUSH;
  }
}

/** Main Code logic present here, called after the bar is complete, size of the bar is given in config.
 * If at the end of bar, closing price crosses support px, resistance px (whichever is suitable), squareoff is called.
 *
 */

void HighMoveTheoCalculator::onBarUpdate(const unsigned int _security_id_,
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
  // dbglogger_ << watch_.tv() << " HMT " << secondary_smv_->shortcode() << " " << day_high_ << " " << day_low_ << " " << day_open_price_*threshold_
  //           << " " << midterm_pos_offset_ << " " << std::abs(bar_close_price_ - prev_bar_close_ )*bar_volume_ << " "
  //           << std::abs(prev_bar_close_ - prev_prev_bar_close_)*prev_bar_vol_  << " " << current_obv_ << " " << max_obv_ << DBGLOG_ENDL_FLUSH;
  // // if (-1*midterm_pos_offset_ > 0) {
  //   updateTrailingSL(day_high_);
  // } else if (-1*midterm_pos_offset_ < 0) {
  //   updateTrailingSL(day_low_);
  // }
  // dbglogger_ << watch_.tv() << " MOMENTUM " << midterm_pnl_ << " " << midterm_stop_loss_ << " " << current_directional_cascade_ << " " <<support_price_ <<" " << bar_low_price_ << DBGLOG_ENDL_FLUSH; 
  if(midterm_pnl_ > prev_bar_pnl_ && midterm_pos_offset_ !=0){
    neg_pnl_count_ = 0;
  }
  else if(midterm_pnl_ < prev_bar_pnl_ && midterm_pos_offset_ != 0){
    neg_pnl_count_++;
  }
  if (midterm_pnl_ < midterm_stop_loss_ ) {
    // If loss check is set, unset it
    if (status_mask_ & LOSS_STATUS_SET) {
      dbglogger_ << watch_.tv() << " Hitting STOP LOSS on BarUpdate " << secondary_smv_->shortcode()
                 << " PNL: " << midterm_pnl_ << " pos: " << -1*midterm_pos_offset_ << " MOMENTUM SL: " << midterm_stop_loss_
                 << " SL: " << stop_loss_ << " HardSL: " << hard_stop_loss_ << " secmkt[" << _market_update_info_.bestbid_price_ << " x "
                 << _market_update_info_.bestask_price_ << DBGLOG_ENDL_FLUSH;

      dbglogger_ << secondary_smv_->shortcode() << " SQUAREOFF " << bar_close_price_ << " " << adj_ratio_
                 << DBGLOG_ENDL_FLUSH;
      SquareOff();
    }
    TurnOffTheo(LOSS_STATUS_UNSET);
  } else if (day_high_ - day_low_ > day_open_price_*threshold_ && last_aggregation_time_>= trading_start_utc_mfm_ / 1000 + watch_.last_midnight_sec() + 2*granularity_
          && bar_close_price_ && prev_bar_close_ && prev_prev_bar_close_){
    if (midterm_pos_offset_ == 0){
      if (std::abs(bar_close_price_ - prev_bar_close_ )*bar_volume_ > std::abs(prev_bar_close_ - prev_prev_bar_close_)*prev_bar_vol_ ){
        if (bar_close_price_ > prev_bar_close_ && bar_close_price_  > prev_prev_bar_close_ && bar_close_price_ > bar_open_price_){
          midterm_pos_offset_ = -1*lot_size_;
          current_obv_ = (bar_close_price_ - prev_bar_close_ )*bar_volume_;
          max_obv_ = current_obv_;
          cascading_price_ = bar_close_price_;
          dbglogger_ << watch_.tv() << " HMT " << secondary_smv_->shortcode() << " NEW BUY "  << DBGLOG_ENDL_FLUSH;
        }
        else if(bar_close_price_ < prev_bar_close_ && bar_close_price_  < prev_prev_bar_close_ && bar_close_price_ < bar_open_price_) {
          midterm_pos_offset_ = lot_size_;
          current_obv_ = (bar_close_price_ - prev_bar_close_ )*bar_volume_;
          max_obv_ = -1*current_obv_;
          cascading_price_ = bar_close_price_;
          dbglogger_ << watch_.tv() << " HMT " << secondary_smv_->shortcode() << " NEW SELL "  << DBGLOG_ENDL_FLUSH;
        }
      }
    }
    else if(midterm_pos_offset_ > 0){
      current_obv_ += (bar_close_price_ - prev_bar_close_ )*bar_volume_;
      if (max_obv_ < -1*current_obv_){
        max_obv_  = -1*current_obv_;
      }
      if (current_obv_ > -0.75*max_obv_ || neg_pnl_count_ > 1){// or self.neg_casc_[_counter] > 1:
        midterm_pos_offset_ = 0;
        current_obv_ = 0;
        max_obv_ = 0;
        neg_pnl_count_ = 0;
        dbglogger_ << watch_.tv() << " HMT " << secondary_smv_->shortcode() << " CLOSE SELL "  << DBGLOG_ENDL_FLUSH;
      }
    }
    else{
      current_obv_ += (bar_close_price_ - prev_bar_close_ )*bar_volume_;
      if (max_obv_ < current_obv_){
        max_obv_  = current_obv_;
      }
      if (current_obv_ < 0.75*max_obv_ || neg_pnl_count_ > 1){// or self.neg_casc_[_counter] > 1:
        midterm_pos_offset_ = 0;
        current_obv_ = 0;
        max_obv_ = 0;
        neg_pnl_count_ = 0;
        dbglogger_ << watch_.tv() << " HMT " << secondary_smv_->shortcode() << " CLOSE BUY "  << DBGLOG_ENDL_FLUSH;
      }
    }
  }
  if (status_mask_ == BIT_SET_ALL) {
    ComputeAndUpdateTheoListeners();
  }
  prev_bar_pnl_ = midterm_pnl_;
  prev_prev_bar_close_ = prev_bar_close_;
  prev_bar_close_ = bar_close_price_;
  prev_bar_low_ = bar_low_price_;
  prev_bar_high_ = bar_high_price_;
  prev_bar_open_ = bar_open_price_;
  prev_bar_vol_ = bar_volume_;
  last_aggregation_time_ += granularity_;
  bar_open_price_ = 0;
  bar_low_price_ = 10000000;
  bar_high_price_ = 0;
  bar_close_price_ = 0;
  bar_volume_ = 0;
}
/** Update trailing stop loss with the maximum favourable directional price reached. Dependent on position and price and
 * function of stdev of close-prev close.
*/




void HighMoveTheoCalculator::ReloadConfig()  {
  MidTermTheoCalculator::ReloadConfig();
}
