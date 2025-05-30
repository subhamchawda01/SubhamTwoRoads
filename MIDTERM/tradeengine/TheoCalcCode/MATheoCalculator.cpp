#include "tradeengine/TheoCalc/MATheoCalculator.hpp"
#include "tradeengine/Utils/Parser.hpp"
/*! \brief This class inherited from MidTermTheo and details the MA Theo/alpha
 * After loading key values from historic lookback, it reacts on every bar update to take a position or squareoff
 */

MATheoCalculator::MATheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                                               HFSAT::DebugLogger& _dbglogger_, int _trading_start_utc_mfm_,
                                               int _trading_end_utc_mfm_, int _aggressive_get_flat_mfm_,
                                               int eff_squareoff_start_utc_mfm_ = 0, double bid_multiplier_ = 1,
                                               double ask_multiplier_ = 1)
    : MidTermTheoCalculator(key_val_map, _watch_, _dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_,
                            _aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_),
      crossover_threshold_(0),
      ma_entry_thesh_(0){
  // InitializeDataSubscriptions();
  dbglogger_ << watch_.tv() << " Creating MA THEO CALCULATOR secId " << secondary_id_ << DBGLOG_ENDL_FLUSH;
  LoadParams();
  InitializeDataSubscriptions();
}
/** Loads basic parameters and historic volatility, volume and signal values
*/
void MATheoCalculator::LoadParams() {
  MidTermTheoCalculator::LoadParams();
  crossover_threshold_ = Parser::GetInt(key_val_map_, "CROSSOVER_THRESH", 3);
  ma_entry_thesh_ = Parser::GetDouble(key_val_map_, "MA_THRESH", 1.015);
  if (watch_.msecs_from_midnight() < trading_start_utc_mfm_) {
    subclass_ = 5;
    BarGenerator bg_;
    bg_.getMAValues(dbglogger_, watch_, secondary_smv_->shortcode(), ticker_name_, watch_.YYYYMMDD(),day_ma_lkbk_,
                  day_moving_avg_,granularity_,lt_days_,long_term_vol_std_,prev_day_close_,adj_ratio_);
    // bg_.getGapValues(dbglogger_, watch_, secondary_smv_->shortcode(), ticker_name_, watch_.YYYYMMDD(), num_bars_,
    //                   granularity_,adj_ratio_, prev_day_close_, open_move_max_);
    // dbglogger_ << ticker_name_ << " open_move_max: " << open_move_max_ << " prev_close: " << prev_day_close_ << DBGLOG_ENDL_FLUSH;
    if (prev_day_close_ == 0) {
      TurnOffTheo(CHILD_STATUS_UNSET);
    }

    last_aggregation_time_ = trading_start_utc_mfm_ / 1000 + watch_.last_midnight_sec();
    // dbglogger_ << "SL: " <<stop_loss_ << " HARD SL: " << hard_stop_loss_ <<DBGLOG_ENDL_FLUSH;
    dbglogger_ << ticker_name_ << " moving_avg: " ;
    for(unsigned int i = 0;i<day_moving_avg_.size();i++){
      dbglogger_ << day_moving_avg_[i] << " ";
    }
    dbglogger_ << "prev_close: " << prev_day_close_ << DBGLOG_ENDL_FLUSH;

  }
}

/** Main Code logic present here, called after the bar is complete, size of the bar is given in config.
 * If at the end of bar, closing price crosses support px, resistance px (whichever is suitable), squareoff is called.
 *
 */

void MATheoCalculator::onBarUpdate(const unsigned int _security_id_,
                                         const HFSAT::MarketUpdateInfo& _market_update_info_) {
  MidTermTheoCalculator::onBarUpdate(_security_id_, _market_update_info_);
  if (status_mask_ != BIT_SET_ALL) {
    return;
  }
  midterm_pnl_ = 0;
   dbglogger_ << watch_.tv() << " MA " << sec_name_indexer_.GetShortcodeFromId(secondary_id_) << " " << bar_open_price_
               << " " << bar_close_price_ << " " << bar_low_price_ << " " << bar_high_price_ << " " << bar_volume_ << " ";
  for(unsigned int i = 0;i<day_moving_avg_.size();i++){
    if(bar_close_price_ != 0){
      midterm_pnl_vec_[i] = -1*(bar_close_price_ - cascade_px_vec_[i])*ma_pos_offset_[i];
      midterm_pnl_ += -1*(bar_close_price_ - cascade_px_vec_[i])*ma_pos_offset_[i];
    }
    dbglogger_ << midterm_pnl_vec_[i] << " ";
    // dbglogger_ << support_vec_[i] << " " << resist_vec_[i] << " " << prev_pnl_vec_[i] << " " << midterm_pnl_vec_[i] << " ";
    prev_pnl_vec_[i] = midterm_pnl_vec_[i];
  }
  dbglogger_ << DBGLOG_ENDL_FLUSH;
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

  for(unsigned int i = 0;i<day_moving_avg_.size();i++){

    if(bar_close_price_ > day_moving_avg_[i]  && ((prev_bar_close_ !=0 && prev_bar_close_ < day_moving_avg_[i]) || 
      (prev_bar_close_ == 0 and prev_day_close_ < day_moving_avg_[i]))){
      crossover_lkbk_[i] = 0;
    }
    else if(bar_close_price_ < day_moving_avg_[i]  && ((prev_bar_close_ !=0 && prev_bar_close_ > day_moving_avg_[i]) || 
      (prev_bar_close_ == 0 and prev_day_close_ > day_moving_avg_[i]))){
      crossover_lkbk_[i] = 0;
    }
    else{
      crossover_lkbk_[i]++;
    }
  }
  updateTrailingSL(bar_close_price_);
  // if (-1*midterm_pos_offset_ > 0) {
  //   updateTrailingSL(day_high_);
  // } else if (-1*midterm_pos_offset_ < 0) {
  //   updateTrailingSL(day_low_);
  // }
  // dbglogger_ << watch_.tv() << " MA " << midterm_pnl_ << " " << midterm_stop_loss_ << " " << current_directional_cascade_ << " " <<support_price_ <<" " << bar_low_price_ << DBGLOG_ENDL_FLUSH; 
  int min_ma_=-1,max_ma_=-1;
  for(unsigned int i =0;i<day_moving_avg_.size();i++){
    if(day_moving_avg_[i] !=0){
      if (min_ma_ == -1 && day_moving_avg_[i]  < bar_close_price_){
          min_ma_ = i;        
      }
      else if(min_ma_ !=-1 && day_moving_avg_[i]  < bar_close_price_ &&  day_moving_avg_[i] > day_moving_avg_[min_ma_]){
          min_ma_ = i;
      }

      if (max_ma_ == -1 && day_moving_avg_[i]  > bar_close_price_){
          max_ma_ = i;        
      }
      else if(max_ma_ !=-1 && day_moving_avg_[i]  > bar_close_price_ &&  day_moving_avg_[i] < day_moving_avg_[max_ma_]){
          max_ma_ = i;
      }

    }
  }
  for(unsigned int i =0;i<day_moving_avg_.size();i++){
    if ((midterm_pnl_vec_[i] < midterm_stop_loss_ || (current_directional_cascade_[i] > 0 && support_vec_[i]/ma_entry_thesh_ > bar_close_price_ ) ||
        (current_directional_cascade_[i] < 0 && resist_vec_[i]*ma_entry_thesh_ < bar_close_price_ ))) {
      // If loss check is set, unset it
      if (status_mask_ & LOSS_STATUS_SET) {
        dbglogger_ << watch_.tv() << " Hitting STOP LOSS on BarUpdate for " << i << " " << secondary_smv_->shortcode()
                   << " PNL: " << midterm_pnl_vec_[i] << " pos: " << -1*ma_pos_offset_[i] << " MA SL: " << midterm_stop_loss_
                   << " SL: " << stop_loss_ << " HardSL: " << hard_stop_loss_ << " Support px: " << support_vec_[i]
                   << " Resist px: " << resist_vec_[i] << " secmkt[" << _market_update_info_.bestbid_price_ << " x "
                   << _market_update_info_.bestask_price_ << "] Stop loss decay "
                   << std::exp(decay_factor_ * (std::abs(current_directional_cascade_[i]) - 1)) << DBGLOG_ENDL_FLUSH;
        dbglogger_ << secondary_smv_->shortcode() << " SQUAREOFF " << bar_close_price_ << " " << adj_ratio_
                   << DBGLOG_ENDL_FLUSH;
        // SquareOff();
        midterm_pos_offset_ -= ma_pos_offset_[i];
        ma_pos_offset_[i] =0;
        current_directional_cascade_[i] = 0;
        crossover_lkbk_[i] = 1000000;
        midterm_pnl_vec_[i] = 0;
        sqoff_flag_[i] = true;
      }
      // TurnOffTheo(LOSS_STATUS_UNSET);
    } else {
      if ((current_directional_cascade_[i] > 0 && prev_bar_close_ < bar_close_price_ &&
                                     cascade_px_vec_[i] <= bar_close_price_) ||
                                    (current_directional_cascade_[i] < 0 && prev_bar_close_ > bar_close_price_ &&
                                     cascade_px_vec_[i] >= bar_close_price_)) {
        cascade_condn_[i] = true;
      } else {
        cascade_condn_[i] = false;
      }
      // dbglogger_ << cascade_px_vec_[i] << " " << current_directional_cascade_ << " " << prev_bar_close_ << " " << bar_close_price_ << " " << bar_count_ << DBGLOG_ENDL_FLUSH;
      if (bar_open_price_ != 0 && sqoff_flag_[i] == false) {

        passive_reduce_position_ = true;
        if(crossover_lkbk_[i] < crossover_threshold_ && current_directional_cascade_[i] == 0 && day_moving_avg_[i] !=0){
          if(bar_close_price_ > day_moving_avg_[i]*ma_entry_thesh_ && (int)i == min_ma_){

            cascade_px_vec_[i] = bar_close_price_;
            current_directional_cascade_[i]++;
            updateTrailingSL(bar_close_price_);
            dbglogger_ << "MA " << secondary_smv_->shortcode() << " CASCADE BUY " << bar_close_price_ << " " << prev_day_close_
                       << " " << day_open_price_ << " " << adj_ratio_ << DBGLOG_ENDL_FLUSH;
            dbglogger_ << secondary_smv_->shortcode() << " " << current_directional_cascade_[i] << " " << cascade_px_vec_[i] << " "
                       << day_moving_avg_[i] << " " << i << DBGLOG_ENDL_FLUSH;
            if (status_mask_ == BIT_SET_ALL) {
              ComputeAndUpdateTheoListeners();
            }
          }
          else if(bar_close_price_ < day_moving_avg_[i]/ma_entry_thesh_ && (int)i == max_ma_){

            cascade_px_vec_[i] = bar_close_price_;
            current_directional_cascade_[i]--;
            updateTrailingSL(bar_close_price_);
            dbglogger_ << "MA " << secondary_smv_->shortcode() << " CASCADE SELL " << bar_close_price_ << " " << prev_day_close_
                       << " " << day_open_price_ << " " << adj_ratio_ << DBGLOG_ENDL_FLUSH;
            dbglogger_ << secondary_smv_->shortcode() << " " << -1*current_directional_cascade_[i] << " " << cascade_px_vec_[i] << " "
                       << day_moving_avg_[i] << " " << i << DBGLOG_ENDL_FLUSH;
            if (status_mask_ == BIT_SET_ALL) {
              ComputeAndUpdateTheoListeners();
            }
          }

        }
        else if(cascade_condn_[i] == true){
          if(current_directional_cascade_[i] >0 && current_directional_cascade_[i] < cascade_num_){
            midterm_pos_offset_ -= lot_size_;
            ma_pos_offset_[i]-= lot_size_;
            cascade_px_vec_[i] = ((current_directional_cascade_[i] - 1) * cascade_px_vec_[i] + bar_close_price_) / (current_directional_cascade_[i]);
            current_directional_cascade_[i]++;
            updateTrailingSL(bar_close_price_);
            dbglogger_ << "MA " << secondary_smv_->shortcode() << " CASCADE BUY " << bar_close_price_ << " " << prev_day_close_
                       << " " << day_open_price_ << " " << adj_ratio_ << DBGLOG_ENDL_FLUSH;
            dbglogger_ << secondary_smv_->shortcode() << " " << current_directional_cascade_[i] << " " << cascade_px_vec_[i] << " "
                       << day_moving_avg_[i] << " " << i  << DBGLOG_ENDL_FLUSH;
            if (status_mask_ == BIT_SET_ALL) {
              ComputeAndUpdateTheoListeners();
            }
          }
          else if(current_directional_cascade_[i] < 0 && current_directional_cascade_[i] > -1*cascade_num_){
            midterm_pos_offset_ += lot_size_;
            ma_pos_offset_[i] += lot_size_;
            cascade_px_vec_[i] = ((-1*current_directional_cascade_[i] - 1) * cascade_px_vec_[i] + bar_close_price_) / (-1*current_directional_cascade_[i]);
            current_directional_cascade_[i]--;
            updateTrailingSL(bar_close_price_);
            dbglogger_ << "MA " << secondary_smv_->shortcode() << " CASCADE SELL " << bar_close_price_ << " " << prev_day_close_
                       << " " << day_open_price_ << " " << adj_ratio_ << DBGLOG_ENDL_FLUSH;
            dbglogger_ << secondary_smv_->shortcode() << " " << -1*current_directional_cascade_[i] << " " << cascade_px_vec_[i] << " "
                       << day_moving_avg_[i] << " " << i << DBGLOG_ENDL_FLUSH;
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
}
/** Update trailing stop loss with the maximum favourable directional price reached. Dependent on position and price and
 * function of stdev of close-prev close.
*/

void MATheoCalculator::updateTrailingSL(double _last_price_) {
  for(unsigned int i =0;i<day_moving_avg_.size();i++){
    if (current_directional_cascade_[i] > 0) {
      if (support_vec_[i] < day_high_) {
        support_vec_[i] = day_high_;
      }
    } else if (current_directional_cascade_[i] < 0) {
      if (resist_vec_[i] > day_low_) {
        resist_vec_[i] = day_low_;
      }
    }
  }
}


void MATheoCalculator::ReloadConfig()  {
  MidTermTheoCalculator::ReloadConfig();
}