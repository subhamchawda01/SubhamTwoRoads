#include "tradeengine/TheoCalc/MidTermTheoCalculator.hpp"
#include "tradeengine/Utils/Parser.hpp"
/*! \brief This class basic structure and variables for a MidTerm strategy.
 * It also handles the creation of a bar from market updates.
 */
MidTermTheoCalculator::MidTermTheoCalculator(std::map<std::string, std::string> *key_val_map, HFSAT::Watch &_watch_,
                                             HFSAT::DebugLogger &_dbglogger_, int _trading_start_utc_mfm_,
                                             int _trading_end_utc_mfm_, int _aggressive_get_flat_mfm_,
                                             int eff_squareoff_start_utc_mfm_ = 0, double bid_multiplier_ = 1,
                                             double ask_multiplier_ = 1)
    : RatioTheoCalculator(key_val_map, _watch_, _dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_,
                          _aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_),
      cc_lkbk_(0),
      trailing_pt_(0.0),
      max_exposure_(0),
      cc_alpha_(0),
      cc_beta_(0),
      time_wt_(0),
      pos_wt_(0),
      cascade_num_(0),
      cascade_size_(NULL),
      current_cascade_(0),
      cascade_on_(false),
      passive_reduce_position_(false),
      aggressive_reduce_position_(false),
      decay_factor_(0),
      lt_days_(0),
      obv_lt_days_(0),
      long_term_vol_std_(0),
      long_term_vol_mean_(0),
      long_term_obv_std_(0),
      prev_day_close_(0),
      open_vol_ptile_(0),
      open_vol_days_(0),
      granularity_(0),
      last_aggregation_time_(0),
      bar_open_price_(0),
      bar_close_price_(0),
      bar_low_price_(0),
      bar_high_price_(10000000),
      bar_volume_(0),
      day_open_volume_(0),
      day_open_vol_(0),
      day_open_price_(0),
      support_price_(0),
      resist_price_(10000000),
      moment_cc_wt_mean_(0),
      moment_cc_wt_std_(0),
      opening_volume_avg_(0),
      adj_ratio_(0),
      prev_bar_pnl_(0),
      lot_size_(0),
      agg_trading_(false),
      uncascade_start_utc_mfm_(0),
      midterm_stop_loss_(0),
      cascading_price_(0),
      prev_bar_close_(0),
      prev_bar_high_(0),
      prev_bar_low_(0),
      day_low_(0),
      day_high_(0),
      subclass_(0),
      max_pos_limit_(-1),
      midterm_pnl_(0),
      strat_param_file_("") {
  // InitializeDataSubscriptions();
  dbglogger_ << watch_.tv() << " Creating MIDTERM THEO CALCULATOR secId " << secondary_id_ << DBGLOG_ENDL_FLUSH;
  LoadParams();
  InitializeDataSubscriptions();
}

/** Transfers control to onBarUpdate at bar expiry.
*/
void MidTermTheoCalculator::OnMarketUpdate(const unsigned int _security_id_,
                                           const HFSAT::MarketUpdateInfo &_market_update_info_) {
  RatioTheoCalculator::OnMarketUpdate(_security_id_, _market_update_info_);
  int current_time_ = watch_.tv().tv_sec;
  int msecs_from_midnight = watch_.msecs_from_midnight();
  if(subclass_ == 0 && (uncascade_start_utc_mfm_ == 0 || msecs_from_midnight <= uncascade_start_utc_mfm_)){
    midterm_pos_offset_ = 0;
    for(unsigned int i =0;i <comp_theo_vec_.size();i++){
      // if(comp_theo_vec_[i]->GetStatus() == BIT_SET_ALL){
      if((comp_theo_vec_[i]->GetStatus() & NOSQUAREOFF_BITS_SET) == NOSQUAREOFF_BITS_SET && (comp_theo_vec_[i]->GetStatus() & CONFIG_STATUS_SET) == CONFIG_STATUS_SET){
        midterm_pos_offset_ += comp_theo_vec_[i]->GetMidTermPosition();
      }
    }
    if(max_pos_limit_ != -1 && (midterm_pos_offset_ < 0 && midterm_pos_offset_ < -1*max_pos_limit_)){
      midterm_pos_offset_ = -1*max_pos_limit_;
    }
    else if (max_pos_limit_ != -1 && (midterm_pos_offset_ > 0 && midterm_pos_offset_ > max_pos_limit_)){
      midterm_pos_offset_ = max_pos_limit_;
    }
    if (status_mask_ == BIT_SET_ALL) {
      ComputeAndUpdateTheoListeners();
    }
  }
  if (_market_update_info_.shortcode_ == primary0_smv_->shortcode()) {
    //int current_time_ = watch_.tv().tv_sec;
    //int msecs_from_midnight = watch_.msecs_from_midnight();
    if (uncascade_start_utc_mfm_ != 0 && msecs_from_midnight > uncascade_start_utc_mfm_) {
      if (midterm_pos_offset_ != 0 && status_mask_ == BIT_SET_ALL) {
        dbglogger_ << secondary_smv_->shortcode() << " UNCASCADE " << prev_bar_close_ << " " << adj_ratio_
                   << DBGLOG_ENDL_FLUSH;
      }
      midterm_pos_offset_ = 0;
      if (status_mask_ == BIT_SET_ALL) {
        ComputeAndUpdateTheoListeners();
      }
    } else if (current_time_ >= last_aggregation_time_ + granularity_) {
      onBarUpdate(_security_id_, _market_update_info_);
    }
  }


}

/** Aggregate trades for bar details (OCLHV).
 * Transfers control to onBarUpdate at bar expiry.
*/
void MidTermTheoCalculator::OnTradePrint(const unsigned int _security_id_,
                                         const HFSAT::TradePrintInfo &_trade_print_info_,
                                         const HFSAT::MarketUpdateInfo &_market_update_info_) {
  RatioTheoCalculator::OnTradePrint(_security_id_, _trade_print_info_, _market_update_info_);
  if (_market_update_info_.shortcode_ == primary0_smv_->shortcode()) {
    int current_time_ = watch_.tv().tv_sec;
    int msecs_from_midnight = watch_.msecs_from_midnight();
    if (uncascade_start_utc_mfm_ != 0 && msecs_from_midnight > uncascade_start_utc_mfm_) {
      if (midterm_pos_offset_ != 0 && status_mask_ == BIT_SET_ALL) {
        dbglogger_ << secondary_smv_->shortcode() << " UNCASCADE " << prev_bar_close_ << " " << adj_ratio_
                   << DBGLOG_ENDL_FLUSH;
      }
      midterm_pos_offset_ = 0;
      if (status_mask_ == BIT_SET_ALL) {
        ComputeAndUpdateTheoListeners();
      }
    } else if (current_time_ >= last_aggregation_time_ + granularity_) {
      onBarUpdate(_security_id_, _market_update_info_);
    }
    BarAggregator(_trade_print_info_);
  }
}

/** Load basic midterm configs
*/

void MidTermTheoCalculator::LoadParams() {
  RatioTheoCalculator::LoadParams();
  bool status_ = Parser::GetBool(key_val_map_, "STATUS", false);
  status_mask_ = (status_ == true ? (status_mask_ | CONFIG_STATUS_SET) : (status_mask_ & CONFIG_STATUS_UNSET));
  lot_size_ = Parser::GetInt(key_val_map_, "UNIT_SIZE", 0, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);
  cc_lkbk_ = Parser::GetInt(key_val_map_, "NUM_DAYS", 5);
  granularity_ = Parser::GetInt(key_val_map_, "GRANULARITY", 900);
  trailing_pt_ = Parser::GetDouble(key_val_map_, "TRAILING_STOP_LOSS_PERCENT", 0.75);
  cc_alpha_ = Parser::GetDouble(key_val_map_, "CC_ALPHA", 1.8);
  cc_beta_ = Parser::GetDouble(key_val_map_, "CC_BETA", 0.4);
  time_wt_ = Parser::GetDouble(key_val_map_, "TIME_WT", 0.5);
  pos_wt_ = Parser::GetDouble(key_val_map_, "POS_WT", 0.5);
  cascade_num_ = Parser::GetDouble(key_val_map_, "CASCADE_NUM", 10);
  cascade_size_ = new int[cascade_num_];
  cascade_size_[0] = Parser::GetInt(key_val_map_, "FIRST_CASCADE", 1);
  decay_factor_ = Parser::GetDouble(key_val_map_, "DECAY_FACTOR", -0.0231);
  lt_days_ = Parser::GetInt(key_val_map_, "VOLATILITY_LOOKBACK", 30);
  obv_lt_days_ = Parser::GetInt(key_val_map_, "MOM_VOLATILITY_LOOKBACK", 30);
  open_vol_ptile_ = Parser::GetDouble(key_val_map_, "OPEN_VOL_PTILE", 50);
  open_vol_days_ = Parser::GetInt(key_val_map_, "OPEN_VOL_LOOKBK", 180);
  status_mask_ = (status_ == true ? (status_mask_ | CONFIG_STATUS_SET) : (status_mask_ & CONFIG_STATUS_UNSET));
  stop_loss_ = Parser::GetDouble(key_val_map_, "STOP_LOSS", 50000);
  stop_loss_ = stop_loss_ * -1;
  midterm_stop_loss_ = Parser::GetDouble(key_val_map_, "MIDTERM_STOP_LOSS", 50000);
  midterm_stop_loss_ = midterm_stop_loss_ * -1;
  passive_reduce_position_ = Parser::GetBool(key_val_map_, "PASSIVE_REDUCE_POSITION", false);
  aggressive_reduce_position_ = Parser::GetBool(key_val_map_, "AGGRESSIVE_REDUCE_POSITION", false);
  agg_trading_ = Parser::GetBool(key_val_map_, "AGGRESSIVE_TRADING_ON", false);
  max_pos_limit_ = Parser::GetInt(key_val_map_, "MAX_POS_LIMIT", -1, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);
  // This is always greater than normal stop loss
  hard_stop_loss_ = Parser::GetDouble(key_val_map_, "HARD_STOP_LOSS", 100000);
  hard_stop_loss_ = hard_stop_loss_ * -1;
  hard_stop_loss_ = std::min(hard_stop_loss_, stop_loss_);
  uncascade_time_ = Parser::GetString(key_val_map_, "UNCASCADE_START_TIME", "");
  if (uncascade_time_ == "") {
    uncascade_start_utc_mfm_ = 0;
  } else {
    int uncascade_start_utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(
        watch_.YYYYMMDD(), atoi(uncascade_time_.c_str() + 4), uncascade_time_.c_str());
    uncascade_start_utc_mfm_ =
        HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(watch_.YYYYMMDD(), uncascade_start_utc_hhmm_, "UTC_");
  }

  dbglogger_ << watch_.tv() << " Uncascading time " << watch_.YYYYMMDD() << " " << uncascade_time_ << " "
             << uncascade_start_utc_mfm_ << DBGLOG_ENDL_FLUSH;

  DBGLOG_CLASS_FUNC_LINE_INFO << "MIDTERMTHEO :" << secondary_smv_->shortcode() << " UNIT_SIZE " << lot_size_ << " MAX_POS " << max_pos_limit_ << DBGLOG_ENDL_FLUSH;

}
/** Generic function to be overloaded by specific theos
*/
void MidTermTheoCalculator::onBarUpdate(const unsigned int _security_id_,
                                        const HFSAT::MarketUpdateInfo &_market_update_info_) {
  if (day_open_vol_ > 0) {
    day_open_volume_ = day_open_vol_;
  }
  day_open_vol_ = -1;
  if (status_mask_ != BIT_SET_ALL) {
    CancelBid();
    CancelAsk();
    return;
  }
  if(subclass_ == 1){
    if(bar_close_price_ != 0){
      midterm_pnl_ = -1*(bar_close_price_ - cascading_price_)*midterm_pos_offset_;
    }
    dbglogger_ << watch_.tv() << " MOMENTUM " << sec_name_indexer_.GetShortcodeFromId(secondary_id_) << " " << bar_open_price_
             << " " << bar_close_price_ << " " << bar_low_price_ << " " << bar_high_price_ << " " << bar_volume_ << " "
             << support_price_ << " " << resist_price_ << " " << prev_bar_pnl_ << " " << midterm_pnl_ << DBGLOG_ENDL_FLUSH;
  }
  else if(subclass_ == 2){
    if(bar_close_price_ != 0){
      midterm_pnl_ = -1*(bar_close_price_ - cascading_price_)*midterm_pos_offset_;
    }
    dbglogger_ << watch_.tv() << " MACD " << sec_name_indexer_.GetShortcodeFromId(secondary_id_) << " " << bar_open_price_
             << " " << bar_close_price_ << " " << bar_low_price_ << " " << bar_high_price_ << " " << bar_volume_ << " "
             << support_price_ << " " << resist_price_ << " " << prev_bar_pnl_ << " " << midterm_pnl_ << DBGLOG_ENDL_FLUSH;
  }
  else if(subclass_ == 3){
    if(bar_close_price_ != 0){
      midterm_pnl_ = -1*(bar_close_price_ - cascading_price_)*midterm_pos_offset_;
    }
    dbglogger_ << watch_.tv() << " MR " << sec_name_indexer_.GetShortcodeFromId(secondary_id_) << " " << bar_open_price_
             << " " << bar_close_price_ << " " << bar_low_price_ << " " << bar_high_price_ << " " << bar_volume_ << " "
             << support_price_ << " " << resist_price_ << " " << prev_bar_pnl_ << " " << midterm_pnl_ << DBGLOG_ENDL_FLUSH;
  }
  else if(subclass_ == 4){
    if(bar_close_price_ != 0){
      midterm_pnl_ = -1*(bar_close_price_ - cascading_price_)*midterm_pos_offset_;
    }
    dbglogger_ << watch_.tv() << " GAP " << sec_name_indexer_.GetShortcodeFromId(secondary_id_) << " " << bar_open_price_
               << " " << bar_close_price_ << " " << bar_low_price_ << " " << bar_high_price_ << " " << bar_volume_ << " "
             << support_price_ << " " << resist_price_ << " " << prev_bar_pnl_ << " " << midterm_pnl_ << DBGLOG_ENDL_FLUSH;
  }
  //MA logging inside childtheo
  // else if(subclass_ == 5){
  //   if(bar_close_price_ != 0){
  //     midterm_pnl_ = -1*(bar_close_price_ - cascading_price_)*midterm_pos_offset_;
  //   }
  //   dbglogger_ << watch_.tv() << " MA " << sec_name_indexer_.GetShortcodeFromId(secondary_id_) << " " << bar_open_price_
  //              << " " << bar_close_price_ << " " << bar_low_price_ << " " << bar_high_price_ << " " << bar_volume_ << " "
  //            << support_price_ << " " << resist_price_ << " " << prev_bar_pnl_ << " " << midterm_pnl_ << DBGLOG_ENDL_FLUSH;
  // }
  else if(subclass_ == 6){
    if(bar_close_price_ != 0){
      midterm_pnl_ = -1*(bar_close_price_ - cascading_price_)*midterm_pos_offset_;
    }
    dbglogger_ << watch_.tv() << " GAPREVERT " << sec_name_indexer_.GetShortcodeFromId(secondary_id_) << " " << bar_open_price_
               << " " << bar_close_price_ << " " << bar_low_price_ << " " << bar_high_price_ << " " << bar_volume_ << " "
             << support_price_ << " " << resist_price_ << " " << prev_bar_pnl_ << " " << midterm_pnl_ << " " << last_aggregation_time_ << DBGLOG_ENDL_FLUSH;
  }

  else if(subclass_ == 7){
    if(bar_close_price_ != 0){
      midterm_pnl_ = -1*(bar_close_price_ - cascading_price_)*midterm_pos_offset_;
    }
    dbglogger_ << watch_.tv() << " HMT " << sec_name_indexer_.GetShortcodeFromId(secondary_id_) << " " << bar_open_price_
               << " " << bar_close_price_ << " " << bar_low_price_ << " " << bar_high_price_ << " " << bar_volume_ 
               << " " << prev_bar_pnl_ << " " << midterm_pnl_ << DBGLOG_ENDL_FLUSH;
  }
}

/** Update trailing stop loss with the closing price reached. Dependent on position and price and function of stdev of
 * close-prev close.
*/
void MidTermTheoCalculator::updateTrailingSL(double _last_price_) {
  double desired_px_ = 0;
  if (midterm_pos_offset_ < 0) {
    desired_px_ = _last_price_ - trailing_pt_ * moment_cc_wt_std_ * std::exp(decay_factor_ * (current_cascade_ - 1));
    if (support_price_ < desired_px_) {
      support_price_ = desired_px_;
    }
  } else if (midterm_pos_offset_ > 0) {
    desired_px_ = _last_price_ + trailing_pt_ * moment_cc_wt_std_ * std::exp(decay_factor_ * (current_cascade_ - 1));
    if (resist_price_ > desired_px_) {
      resist_price_ = desired_px_;
    }
  }
}


void MidTermTheoCalculator::ConfigureComponentDetails(MidTermTheoCalculator* comp_theo_) {
  comp_theo_vec_.push_back(comp_theo_);
}


void MidTermTheoCalculator::ReloadConfig() {
  if(subclass_ == 0){
    RatioTheoCalculator::ReloadConfig();
  }
  else{

    // std::string theo_cal = (*key_val_map_)["THEO_FOLDER"] + std::string("MainConfig.cfg");
    Parser::ParseConfig(strat_param_file_, *key_val_map_);
    // dbglogger_ << GetSecondaryShc() << " " << (*key_val_map_)["THEO_IDENTIFIER"] << "\n";
    double prev_stop_loss_ = stop_loss_;
    double prev_hard_stop_loss_ = hard_stop_loss_;
    bid_factor_ = 1;
    ask_factor_ = 1;
    SetEfficientSquareOff(false);
    LoadParams();
    if ((hit_hard_stoploss_) && ((hard_stop_loss_ != prev_hard_stop_loss_) && (total_pnl_ > hard_stop_loss_))) {
      // If loss check is set, but stop loss is now not reached
      if ((status_mask_ & HARDLOSS_STATUS_SET) == 0) {
        dbglogger_ << watch_.tv() << " RESETTING HARD STOP LOSS STATUS "
                   << sec_name_indexer_.GetShortcodeFromId(secondary_id_) << " PNL: " << total_pnl_
                   << " Hard SL: " << hard_stop_loss_ << " secmkt[" << secondary_smv_->market_update_info().bestbid_price_
                   << " x " << secondary_smv_->market_update_info().bestask_price_ << "] primkt["
                   << primary0_smv_->market_update_info().bestbid_price_ << " x "
                   << primary0_smv_->market_update_info().bestask_price_ << "] " << DBGLOG_ENDL_FLUSH;
        hit_hard_stoploss_ = false;
        TurnOnTheo(HARDLOSS_STATUS_SET);
        if (sqoff_theo_) {
          sqoff_theo_->TurnOnTheo(HARDLOSS_STATUS_SET);
        }
        if (parent_mm_theo_) {
          parent_mm_theo_->TurnOnTheo(HARDLOSS_STATUS_SET);
        }
      }
    }

    if ((hit_stoploss_) && ((stop_loss_ != prev_stop_loss_) && (total_pnl_ > stop_loss_))) {
      // If loss check is set, but stop loss is now not reached
      if ((status_mask_ & LOSS_STATUS_SET) == 0) {
        dbglogger_ << watch_.tv() << " RESETTING STOP LOSS STATUS " << sec_name_indexer_.GetShortcodeFromId(secondary_id_)
                   << " PNL: " << total_pnl_ << " SL: " << stop_loss_ << " secmkt["
                   << secondary_smv_->market_update_info().bestbid_price_ << " x "
                   << secondary_smv_->market_update_info().bestask_price_ << "] primkt["
                   << primary0_smv_->market_update_info().bestbid_price_ << " x "
                   << primary0_smv_->market_update_info().bestask_price_ << "] " << DBGLOG_ENDL_FLUSH;
        hit_stoploss_ = false;
        TurnOnTheo(LOSS_STATUS_SET);
        NoSquareOff();
        if (parent_mm_theo_) {
          parent_mm_theo_->TurnOnTheo(LOSS_STATUS_SET);
        }
      }
    }
    for (auto base_exec : base_exec_vec_) {
      base_exec->ReloadConfig();
    }

    if ((status_mask_ & CONFIG_STATUS_SET) == 0) {
      TurnOffTheo(CTRLMSG_STATUS_UNSET);
      if (need_to_hedge_) {
        for (uint8_t num = 0; num < hedge_vector_.size(); num++) {
          if (hedge_vector_[num]->hedge_status_) {
            hedge_vector_[num]->hedge_theo_->SquareOff();
          }
        }
        if (is_secondary_sqoff_needed_) {
          SquareOff();
        }
      } else {
        SquareOff();
      }
    } else {
      TurnOnTheo(CTRLMSG_STATUS_SET);
      if (need_to_hedge_) {
        for (uint8_t num = 0; num < hedge_vector_.size(); num++) {
          if (hedge_vector_[num]->hedge_status_) {
            hedge_vector_[num]->hedge_theo_->NoSquareOff();
          }
        }
      } else {
        NoSquareOff();
      }
    }

    config_reloaded_ = true;  
  }
  
}
