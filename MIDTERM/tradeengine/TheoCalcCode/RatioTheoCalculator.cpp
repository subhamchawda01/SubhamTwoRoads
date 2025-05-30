#include "tradeengine/TheoCalc/RatioTheoCalculator.hpp"
#include "tradeengine/Utils/Parser.hpp"

RatioTheoCalculator::RatioTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                                         HFSAT::DebugLogger& _dbglogger_, int _trading_start_utc_mfm_,
                                         int _trading_end_utc_mfm_, int _aggressive_get_flat_mfm_,
                                         int eff_squareoff_start_utc_mfm_ = 0, double bid_multiplier_ = 1,
                                         double ask_multiplier_ = 1)
    : BaseTheoCalculator(key_val_map, _watch_, _dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_,
                         _aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_),
      alpha_(1),
      ratio_update_time_(0),
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
      use_constant_ratio_(false),
      constant_ratio_(0),
      use_start_ratio_(false),
      start_ratio_(0),
      ratio_diff_offset_(0),
      start_ratio_end_mfm_(trading_start_utc_mfm_),
      estimated_bid_price_(0),
      estimated_ask_price_(0),
      ratio_(0),
      last_ratio_update_time_(0),
      allow_sweep_mode_(false),
      use_sweep_price_(true),
      min_lots_traded_for_sweep_mode_(100),
      min_price_move_for_sweep_mode_(100),
      min_level_cleared_for_sweep_mode_(100),
      buy_sweep_bid_offset_(0),
      buy_sweep_ask_offset_(0),
      sell_sweep_bid_offset_(0),
      sell_sweep_ask_offset_(0),
      max_bid_offset_sweep_(0),
      max_ask_offset_sweep_(0),
      sweep_side_(HFSAT::TradeType_t::kTradeTypeNoInfo),
      lots_traded_in_current_sweep_(0),
      price_move_in_current_sweep_(0),
      first_sweep_traded_price_(0),
      last_traded_primary_usecs_(0),
      last_traded_primary_int_price_(0),
      last_primary_bid_int_price_(0),
      last_primary_ask_int_price_(0),
      midterm_pos_offset_(0),
      sleep_after_ready_(false),
      sleep_called_(false),
      training_msecs_after_start_ratio_(0),
      prev_day_close_(0),
      gen_price_returns_(false),
      train_sample_window_size_msecs_(300000),
      msecs_for_next_train_sample_start_(_trading_start_utc_mfm_ + 300000),
      mean_k_(0),
      count_k_(0),
      theo_k_(0),
      max_px_percent_threshold_(100),
      mean_volume_(0),
      volume_filter_(0),
      theo_decay_factor_(0),
      hist_mean_(0),
      hist_std_(0),
      hist_factor_(0),
      day_open_px_(0) {
  dbglogger_ << watch_.tv() << " Creating RATIO THEO CALCULATOR secId " << secondary_id_ << " primId " << primary0_id_
             << DBGLOG_ENDL_FLUSH;
  LoadParams();
  InitializeDataSubscriptions();
  strcpy(exec_log_buffer_.buffer_data_.query_exec_.sec_shortcode, secondary_smv_->shortcode().c_str());
}

void RatioTheoCalculator::LoadParams() {
  BaseTheoCalculator::LoadParams();
  bool status_ = Parser::GetBool(key_val_map_, "STATUS", false);
  status_mask_ = (status_ == true ? (status_mask_ | CONFIG_STATUS_SET) : (status_mask_ & CONFIG_STATUS_UNSET));
  alpha_ = Parser::GetDouble(key_val_map_, "ALPHA", 1);
  ratio_update_time_ = Parser::GetInt(key_val_map_, "RATIO_UPDATE_TIME", 0);
  primary0_price_type_str_ = Parser::GetString(key_val_map_, "PRIMARY0_PRICE_TYPE", "ORDER_BOOK_BEST_PRICE");
  primary0_price_type_ = GetPriceTypeFromStr(primary0_price_type_str_);
  if (primary0_price_type_ == pCUSTOM_PRICE_TYPE && !is_ready_) {
    custom_price_aggregator_ = new CustomPriceAggregator(primary0_smv_, watch_, dbglogger_, key_val_map_);
  }
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
  use_start_ratio_ = Parser::GetBool(key_val_map_, "USE_START_RATIO", false);
  use_constant_ratio_ = Parser::GetBool(key_val_map_, "USE_CONSTANT_RATIO", false);
  constant_ratio_ = Parser::GetDouble(key_val_map_, "CONSTANT_RATIO", 0);
  sleep_after_ready_ = Parser::GetBool(key_val_map_, "SLEEP_AFTER_READY", false);
  training_msecs_after_start_ratio_ = Parser::GetInt(key_val_map_, "TRAINING_SECS_AFTER_START_RATIO", 0) * 1000;
  if ((use_constant_ratio_) && (constant_ratio_ > min_ratio_) && (constant_ratio_ < max_ratio_)) {
    ratio_ = constant_ratio_;
    use_start_ratio_ = false;
  } else {
    use_constant_ratio_ = false;
  }

  if ((use_start_ratio_) && (!is_ready_)) {
    int num_days_ = Parser::GetInt(key_val_map_, "NUM_DAYS_LOOKBACK_START_RATIO", 0);
    double std_multiplier_ = Parser::GetDouble(key_val_map_, "SD_MULTIPLIER_START_RATIO", 0);
    bool is_end_ratio_available_ = HelperFunctions::GetStartRatio(
        dbglogger_, watch_, secondary_smv_->shortcode(), primary0_smv_->shortcode(), watch_.YYYYMMDD(), num_days_,
        min_ratio_, max_ratio_, std_multiplier_, start_ratio_, ratio_diff_offset_);
    start_ratio_end_mfm_ = trading_start_utc_mfm_ + training_msecs_after_start_ratio_;
    if (!is_end_ratio_available_) {
      use_start_ratio_ = false;
      trading_start_utc_mfm_ = trading_start_utc_mfm_ + training_msecs_after_start_ratio_;
    } else {
      double avg_spread_multiplier_ = Parser::GetDouble(key_val_map_, "AVG_SPREAD_MULTIPLIER", 0);
      ratio_diff_offset_ = std::max(ratio_diff_offset_, avg_spread_percent_ * avg_spread_multiplier_);
    }
  }
  gen_price_returns_ = Parser::GetBool(key_val_map_, "GEN_PRICE_RETURNS", false);
  train_sample_window_size_msecs_ = Parser::GetInt(key_val_map_, "TRAIN_SAMPLE_WINDOW", 300000);
  if(gen_price_returns_ && !is_ready_){
    int num_days_for_return_ = Parser::GetInt(key_val_map_, "NUM_DAYS_LOOKBACK_RETURNS", 1);
    int return_window_ = Parser::GetInt(key_val_map_, "RETURN_WINDOW", 60);
    max_px_percent_threshold_ = Parser::GetDouble(key_val_map_, "MAX_PX_PERCENT_THRESHOLD", 1);
    volume_filter_ = Parser::GetDouble(key_val_map_, "VOLUME_FILTER", 0);
    theo_decay_factor_ = Parser::GetDouble(key_val_map_, "THEO_DECAY_FACTOR", 0);
    hist_factor_ = Parser::GetDouble(key_val_map_, "HIST_FACTOR", 1);
    BarGenerator bg1_,bg2_;
    double adj_ratio_;
    double mean_,std_,tmp1_,tmp2_,tmp3_,tmp4_;
    bg1_.getKeyFilters(dbglogger_, watch_, secondary_smv_->shortcode(), ticker_name_, watch_.YYYYMMDD(), 180,
                      50, 900, 5, 30, mean_, std_,
                      tmp1_, tmp2_, tmp3_, prev_day_close_, adj_ratio_,
                      tmp4_, 30);
    bg2_.getPriceReturns(dbglogger_, watch_, secondary_smv_->shortcode(), ticker_name_, watch_.YYYYMMDD(), num_days_for_return_,
                       return_window_, adj_ratio_, return_array_, prev_day_close_,mean_volume_);
    hist_mean_ = 0.4*mean_;
    hist_std_ = 1.8*std_;
    // bg_.getKeyFilters(dbglogger_, watch_, secondary_smv_->shortcode(), ticker_name_, watch_.YYYYMMDD(), open_vol_days_,
    //                   open_vol_ptile_, granularity_, cc_lkbk_, lt_days_, moment_cc_wt_mean_, moment_cc_wt_std_,
    //                   opening_volume_avg_, long_term_vol_std_, long_term_vol_mean_, prev_day_close_, adj_ratio_,
    //                   long_term_obv_std_, obv_lt_days_);
    dbglogger_ << watch_.tv() << " " << secondary_smv_->shortcode() << " mean std " << mean_ << " " << std_ << DBGLOG_ENDL_FLUSH;
    prev_day_close_ /= adj_ratio_;
  }
  if (use_start_ratio_ && !is_ready_) {
    ratio_ = start_ratio_;
  }

  allow_sweep_mode_ = Parser::GetBool(key_val_map_, "ALLOW_SWEEP_MODE", false);
  use_sweep_price_ = Parser::GetBool(key_val_map_, "USE_SWEEP_PRICE", true);
  min_lots_traded_for_sweep_mode_ = Parser::GetInt(key_val_map_, "MIN_LOTS_TRADED_SWEEP", 100);
  min_percent_price_move_for_sweep_mode_ = Parser::GetDouble(key_val_map_, "MIN_PERCENT_PRICE_SWEEP", 100) / 100;
  min_level_cleared_for_sweep_mode_ = Parser::GetInt(key_val_map_, "MIN_LEVEL_CLEARED_SWEEP", 100);
  max_usecs_allowed_between_sweep_trades_ = Parser::GetDouble(key_val_map_, "MAX_USECS_BETWEEN_SWEEP_TRADES", 1);
  max_msecs_between_trades_sweep_mode_on = Parser::GetDouble(key_val_map_, "MAX_MSECS_BETWEEN_TRADES_SWEEP_MODE", 1);
  buy_sweep_bid_offset_percent_ = Parser::GetDouble(key_val_map_, "BUY_SWEEP_BID_OFFSET_PERCENT", 0) / 100;
  buy_sweep_ask_offset_percent_ = Parser::GetDouble(key_val_map_, "BUY_SWEEP_ASK_OFFSET_PERCENT", 0) / 100;
  sell_sweep_bid_offset_percent_ = Parser::GetDouble(key_val_map_, "SELL_SWEEP_BID_OFFSET_PERCENT", 0) / 100;
  sell_sweep_ask_offset_percent_ = Parser::GetDouble(key_val_map_, "SELL_SWEEP_ASK_OFFSET_PERCENT", 0) / 100;

  max_bid_offset_sweep_percent_ = Parser::GetDouble(key_val_map_, "MAX_BID_OFFSET_SWEEP_PERCENT", 0) / 100;
  max_ask_offset_sweep_percent_ = Parser::GetDouble(key_val_map_, "MAX_ASK_OFFSET_SWEEP_PERCENT", 0) / 100;

  fraction_above_max_sweep_offset_ = Parser::GetDouble(key_val_map_, "FRACTION_ABOVE_MAX_SWEEP_OFFSET", 0);
  fraction_above_max_sweep_offset_ = std::min(fraction_above_max_sweep_offset_, 1.0);

  if (!position_shift_amount_) {
    dbglogger_ << watch_.tv() << " POSITION SHIFT MGR SET FALSE SINCE POSITION SHIFT AMOUNT IS "
               << position_shift_amount_ << " in " << theo_identifier_ << DBGLOG_ENDL_FLUSH;
    use_position_shift_manager_ = false;
  }
  for (auto base_exec : base_exec_vec_) {
    base_exec->SetPassiveReduce(passive_reduce_position_);
    base_exec->SetAggressiveReduce(aggressive_reduce_position_);
    base_exec->SetEfficientSquareOff(eff_squareoff_on_);
  }
}

void RatioTheoCalculator::UpdateTheoPrices(const unsigned int _security_id_,
                                           const HFSAT::MarketUpdateInfo& _market_update_info_) {
  int msecs_from_midnight = watch_.msecs_from_midnight();

  bool is_primary_book_valid = true;
  bool is_secondary_book_valid = true;

  //Since this being done anyways, check at the top
  if (_security_id_ == primary0_id_){
    if (primary0_smv_->market_update_info_.bestbid_price_ == kInvalidPrice ||
        primary0_smv_->market_update_info_.bestask_price_ == kInvalidPrice ||
        primary0_smv_->market_update_info_.bestbid_price_ >= primary0_smv_->market_update_info_.bestask_price_) {
      is_primary_book_valid = false ;
    }
  }
  
  //ideally this should be under else if/else, even the code below this ignores this fact that 
  //at a time only 1 sec_id update can be there, will refactor both togather
  if(_security_id_ == secondary_id_){
    if (secondary_smv_->market_update_info_.bestbid_price_ == kInvalidPrice ||
        secondary_smv_->market_update_info_.bestask_price_ == kInvalidPrice ||
        secondary_smv_->market_update_info_.bestbid_price_ >= secondary_smv_->market_update_info_.bestask_price_) {
      is_secondary_book_valid = false ;
    }
  }


  //Applicable based on param
  if( true == are_we_using_auto_getflat_near_circuit_ ) {

    if((_security_id_ == primary0_id_) && (true == is_primary_book_valid)) {
      //get mid price
      double primary_mid_int_px = (primary0_smv_->market_update_info_.bestbid_int_price_ + primary0_smv_->market_update_info_.bestask_int_price_)*0.5;
      double upper_int_price_threshold = 0;
      double lower_int_price_threshold = 0;

      //auto getflat bits are not unset
      if(false == is_squaredoff_due_to_autogetflat_near_primary_circuit_){

        if( true == is_primary_last_close_valid_ ){
          upper_int_price_threshold = (primary0_smv_->upper_int_price_limit_ - primary_int_px_value_for_autogetflat_);
          lower_int_price_threshold = (primary0_smv_->lower_int_price_limit_ + primary_int_px_value_for_autogetflat_);
        }else{
          upper_int_price_threshold = 0.99*primary0_smv_->upper_int_price_limit_;
          lower_int_price_threshold = 1.01*primary0_smv_->lower_int_price_limit_;
        }

        //violation
        if(primary_mid_int_px >= upper_int_price_threshold || primary_mid_int_px <= lower_int_price_threshold){
          TurnOffTheo(NEAR_CIRCUIT_PRIMARY_STATUS_UNSET);
          SquareOff(are_we_using_agg_auto_getflat_near_circuit_);
          is_squaredoff_due_to_autogetflat_near_primary_circuit_ = true ;
          DBGLOG_TIME_CLASS_FUNC << "AUTO SQUAREOFF CALLED : " << secondary_smv_->shortcode() << " ON PRIMARY : " << primary0_smv_->shortcode() << " NEARING A CIRCUIT LEVEL : " << primary0_smv_->market_update_info_.bestbid_int_price_ << " X " << primary0_smv_->market_update_info_.bestask_int_price_ << " THRESHOLD : " << lower_int_price_threshold << " X "  << upper_int_price_threshold << " AGG ? " << are_we_using_agg_auto_getflat_near_circuit_ << DBGLOG_ENDL_FLUSH;
        }

      }else{ 

        if( true == is_primary_last_close_valid_ ){
          upper_int_price_threshold = (primary0_smv_->upper_int_price_limit_ - primary_int_px_value_for_autoresume_);
          lower_int_price_threshold = (primary0_smv_->lower_int_price_limit_ + secondary_int_px_value_for_autoresume_);
        }else{
          upper_int_price_threshold = 0.98*primary0_smv_->upper_int_price_limit_;
          lower_int_price_threshold = 1.02*primary0_smv_->lower_int_price_limit_;
        }

        //resume
        if(primary_mid_int_px < upper_int_price_threshold && primary_mid_int_px > lower_int_price_threshold){
          TurnOnTheo(NEAR_CIRCUIT_PRIMARY_STATUS_SET);
          NoSquareOff();
          is_squaredoff_due_to_autogetflat_near_primary_circuit_ = false;
          DBGLOG_TIME_CLASS_FUNC << "AUTO RESUME CALLED : " << secondary_smv_->shortcode() << " ON PRIMARY : " << primary0_smv_->shortcode() << " MOVING AWAY FROM CIRCUIT LEVEL : " << primary0_smv_->market_update_info_.bestbid_int_price_ << " X " << primary0_smv_->market_update_info_.bestask_int_price_ << " THRESHOLD : " << lower_int_price_threshold << " X "  << upper_int_price_threshold << DBGLOG_ENDL_FLUSH ;
        }

      }
    }

    if((_security_id_ == secondary_id_) && (true == is_secondary_book_valid)){
      double secondary_mix_int_px = (secondary_smv_->market_update_info_.bestbid_int_price_ + secondary_smv_->market_update_info_.bestask_int_price_)*0.5;
      double upper_int_price_threshold = 0;
      double lower_int_price_threshold = 0;

      //auto getflat bits are not unset
      if(false == is_squaredoff_due_to_autogetflat_near_secondary_circuit_){

        if( true == is_secondary_last_close_valid_ ){
          upper_int_price_threshold = (secondary_smv_->upper_int_price_limit_ - secondary_int_px_value_for_autogetflat_);
          lower_int_price_threshold = (secondary_smv_->lower_int_price_limit_ + secondary_int_px_value_for_autogetflat_);
        }else{
          upper_int_price_threshold = 0.99*secondary_smv_->upper_int_price_limit_;
          lower_int_price_threshold = 1.01*secondary_smv_->lower_int_price_limit_;
        }

        //violation
        if(secondary_mix_int_px >= upper_int_price_threshold || secondary_mix_int_px <= lower_int_price_threshold){
          TurnOffTheo(CTRLMSG_STATUS_UNSET);
          SquareOff(are_we_using_agg_auto_getflat_near_circuit_);
          is_squaredoff_due_to_autogetflat_near_secondary_circuit_ = true;
          DBGLOG_TIME_CLASS_FUNC << "AUTO SQUAREOFF CALLED : " << secondary_smv_->shortcode() << " ON SELF NEARING A CIRCUIT LEVEL : " << secondary_smv_->market_update_info_.bestbid_int_price_ << " X " << secondary_smv_->market_update_info_.bestask_int_price_ << " THRESHOLD : " << lower_int_price_threshold << " X "  << upper_int_price_threshold << " AGG ? " << are_we_using_agg_auto_getflat_near_circuit_ << DBGLOG_ENDL_FLUSH;
        }
      }else{

        if( true == is_secondary_last_close_valid_ ){
          upper_int_price_threshold = (secondary_smv_->upper_int_price_limit_ - secondary_int_px_value_for_autoresume_);
          lower_int_price_threshold = (secondary_smv_->lower_int_price_limit_ + secondary_int_px_value_for_autoresume_);
        }else{
          upper_int_price_threshold = 0.98*secondary_smv_->upper_int_price_limit_;
          lower_int_price_threshold = 1.02*secondary_smv_->lower_int_price_limit_;
        }

        //resume
        if(secondary_mix_int_px < upper_int_price_threshold && secondary_mix_int_px > lower_int_price_threshold){
          TurnOnTheo(CTRLMSG_STATUS_SET);
          NoSquareOff();
          is_squaredoff_due_to_autogetflat_near_secondary_circuit_ = false;
          DBGLOG_TIME_CLASS_FUNC << "AUTO RESUME CALLED : " << secondary_smv_->shortcode() << " ON SELF : " << secondary_smv_->shortcode() << " MOVING AWAY FROM CIRCUIT LEVEL : " << secondary_smv_->market_update_info_.bestbid_int_price_ << " X " << secondary_smv_->market_update_info_.bestask_int_price_ << " THRESHOLD : " << lower_int_price_threshold << " X "  << upper_int_price_threshold << DBGLOG_ENDL_FLUSH ;
        }

      }

    }//end of seconary id check

  }//flag check are_we_using_auto_getflat_near_circuit_

  if (msecs_from_midnight > trading_end_utc_mfm_) {
    eff_squareoff_on_ = false;
    TurnOffTheo(SQUAREOFF_STATUS_UNSET);
    if (start_trading_) {
      if ((!need_to_hedge_ || is_secondary_sqoff_needed_) &&
          ((status_mask_ & SQUAREOFF_BITS_SET) != SQUAREOFF_BITS_SET)) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR : Not squaring off at EOD since status mask: " << status_mask_
                               << "has other square off related bits unset" << DBGLOG_ENDL_FLUSH;
        dbglogger_.DumpCurrentBuffer();
      } else {
        SquareOff();
      }
      dbglogger_ << watch_.tv() << " RATIO THEO " << secondary_smv_->shortcode() << " ratio_ " << ratio_
                 << DBGLOG_ENDL_FLUSH;
      dbglogger_ << watch_.tv() << " Getting PassiveFlat " << secondary_smv_->shortcode()
                 << " EndTimeMFM: " << trading_end_utc_mfm_ << DBGLOG_ENDL_FLUSH;
      start_trading_ = false;
    }
  }

  if (msecs_from_midnight > aggressive_get_flat_mfm_) {
    TurnOffTheo(SQUAREOFF_STATUS_UNSET);
    eff_squareoff_on_ = false;
    // In case it gets triggered before normal getflat (ideally shouldn't happen)
    if ((start_trading_) || (!is_agressive_getflat_)) {
      if ((!need_to_hedge_ || is_secondary_sqoff_needed_) &&
          ((status_mask_ & SQUAREOFF_BITS_SET) != SQUAREOFF_BITS_SET)) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR : Not squaring off at EOD since status mask: " << status_mask_
                               << "has other square off related bits unset" << DBGLOG_ENDL_FLUSH;
        dbglogger_.DumpCurrentBuffer();
      } else {
        SquareOff(true);
      }
      dbglogger_ << watch_.tv() << " RATIO THEO " << secondary_smv_->shortcode() << " ratio_ " << ratio_
                 << DBGLOG_ENDL_FLUSH;
      dbglogger_ << watch_.tv() << " Getting AgressivelyFlat " << secondary_smv_->shortcode()
                 << " EndTimeMFM: " << aggressive_get_flat_mfm_ << DBGLOG_ENDL_FLUSH;
      start_trading_ = false;
      is_agressive_getflat_ = true;
    }
  }

  if ((eff_squareoff_start_utc_mfm_ != 0) && (msecs_from_midnight > eff_squareoff_start_utc_mfm_)) {
    if (start_trading_) {
      if (!eff_squareoff_on_) {
        dbglogger_ << watch_.tv() << " Getting Efficient SquareOff " << secondary_smv_->shortcode()
                   << " EndTimeMFM: " << eff_squareoff_start_utc_mfm_ << DBGLOG_ENDL_FLUSH;
      }
      SetEfficientSquareOff(true);
    }
  }

  if (_security_id_ == primary0_id_) {
    theo_values_.is_primary_update_ = true;
    if ( false == is_primary_book_valid ){
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
      if ((!is_ready_ || config_reloaded_) && (msecs_from_midnight > trading_start_utc_mfm_) && (start_trading_) &&
          (training_ratio_values_.size() > 10 || use_start_ratio_ || use_constant_ratio_)) {
        if (!sleep_after_ready_ || sleep_called_) {
          OnReady(primary0_smv_->market_update_info());
          is_ready_ = true;
        }
      }

      if (ratio_ != 0) {
        // TODO NOTE the behaviour here.
        // The estimated price only updates on primary update even though the ratio
        // could have been updating because of multiple secondary updates
        // Might want to update at every update instead of just primary update
        theo_values_.reference_primary_bid_ = 0;
        theo_values_.reference_primary_ask_ = 0;
        if (theo_values_.sweep_mode_active_ != 0 && use_sweep_price_) {
          double reference_trade_price_ = 0;
          if (sweep_side_ == HFSAT::TradeType_t::kTradeTypeBuy) {
            double max_bid_threshold_ = theo_values_.primary_best_bid_price_ + max_bid_offset_sweep_;
            double best_ask_price_ = theo_values_.primary_best_ask_price_;
            reference_trade_price_ = best_ask_price_;
            // Large Sweep move taking bid price very far from top
            if (max_bid_threshold_ < best_ask_price_) {
              reference_trade_price_ =
                  max_bid_threshold_ + (reference_trade_price_ - max_bid_threshold_) * fraction_above_max_sweep_offset_;
            }

            theo_values_.reference_primary_bid_ =
                std::max(reference_trade_price_ - buy_sweep_bid_offset_, theo_values_.primary_best_bid_price_);
            theo_values_.reference_primary_ask_ = best_ask_price_ + buy_sweep_ask_offset_;
          } else {
            double min_ask_threshold_ = theo_values_.primary_best_ask_price_ - max_ask_offset_sweep_;
            double best_bid_price_ = theo_values_.primary_best_bid_price_;
            reference_trade_price_ = best_bid_price_;
            // Large Sweep move taking bid price very far from top
            if (min_ask_threshold_ > best_bid_price_) {
              reference_trade_price_ =
                  min_ask_threshold_ - (min_ask_threshold_ - reference_trade_price_) * fraction_above_max_sweep_offset_;
            }

            theo_values_.reference_primary_bid_ = theo_values_.primary_best_bid_price_ - sell_sweep_bid_offset_;
            theo_values_.reference_primary_ask_ =
                std::min(reference_trade_price_ + sell_sweep_ask_offset_, theo_values_.primary_best_ask_price_);
          }
          if (theo_values_.reference_primary_bid_ >= theo_values_.reference_primary_ask_) {
            DBGLOG_TIME_CLASS_FUNC << " ERROR: CROSS SWEEP SPREAD BID: " << theo_values_.reference_primary_bid_
                                   << " ASK: " << theo_values_.reference_primary_ask_ << " ltp "
                                   << reference_trade_price_ << DBGLOG_ENDL_FLUSH;
          }
        } else if (listen_big_trades_ && theo_values_.is_big_trade_ != 0) {
          double big_trade_px_ = big_trade_int_ltp_ * tick_size_;
          if (theo_values_.is_big_trade_ == -1) {
            theo_values_.reference_primary_bid_ = big_trade_px_ - tick_size_;
            theo_values_.reference_primary_ask_ = big_trade_px_;
          } else {
            theo_values_.reference_primary_bid_ = big_trade_px_;
            theo_values_.reference_primary_ask_ = big_trade_px_ + tick_size_;
          }
        } else {
          CalculatePrices(theo_values_.reference_primary_bid_, theo_values_.reference_primary_ask_, primary0_smv_,
                          primary0_vwap_levels_, primary0_price_type_, primary0_bid_size_filter_,
                          primary0_ask_size_filter_, primary0_size_max_depth_);
          if(gen_price_returns_){

            double current_k_ = 0;
            for (unsigned int i = 1;i< primary_smv_vec_.size() ;i++) {
              current_k_ += last_px_vec_[i]*primary_smv_wt_vec_[i];
              if(last_px_vec_[i] < 0){
                current_k_ = 0;
                break;
              }
            }
            current_k_ *= theo_k_;
            if(theo_k_ > 0 && current_k_ && (day_open_px_ - secondary_smv_->market_update_info().mid_price_ < hist_factor_*(hist_mean_ + hist_std_))
              && (secondary_smv_->market_update_info().mid_price_ - day_open_px_ < hist_factor_*(hist_mean_ + hist_std_)) ){
              SetEfficientSquareOff(false);
              double percent_diff_ =  100*std::abs(secondary_smv_->market_update_info_.mid_price_ - current_k_)/current_k_;
              // dbglogger_ << watch_.tv() << " RELATIVE " << secondary_smv_->shortcode() << " " << theo_values_.reference_primary_bid_ << " " << current_k_ << 
                    // " " << secondary_smv_->market_update_info_.bestbid_price_ << " " << secondary_smv_->market_update_info_.bestask_price_ << DBGLOG_ENDL_FLUSH;
              // dbglogger_ << watch_.tv() << " VOLUME " << secondary_smv_->shortcode() << " " << mean_volume_ << " " <<primary_smv_volume_vec_[1]
                        // << " " << primary_smv_curr_volume_vec_[0] << " " << primary_smv_curr_volume_vec_[1] << DBGLOG_ENDL_FLUSH;
                         
              if(percent_diff_ < max_px_percent_threshold_){
                bool volume_flag_ = true;
                for (unsigned int i = 1;i< primary_smv_vec_.size() ;i++) {
                  if(primary_smv_curr_volume_vec_[i] && primary_smv_volume_vec_[i] && mean_volume_ && volume_filter_){
                    if(primary_smv_curr_volume_vec_[0]/primary_smv_curr_volume_vec_[i] > volume_filter_*mean_volume_/primary_smv_volume_vec_[i]){
                      volume_flag_ = false;
                      // dbglogger_ << watch_.tv() << " VOLUME " << secondary_smv_->shortcode() << " " << mean_volume_ << " " <<primary_smv_volume_vec_[1]
                      //   << " " << primary_smv_curr_volume_vec_[0] << " " << primary_smv_curr_volume_vec_[1] << DBGLOG_ENDL_FLUSH;
                      break;
                    }
                  }
                }
                if(volume_flag_){
                  theo_values_.reference_primary_bid_ = current_k_;
                  theo_values_.reference_primary_ask_ = current_k_;
                  // dbglogger_ << watch_.tv() << "PX Under limit " << secondary_smv_->shortcode() << DBGLOG_ENDL_FLUSH;
                }
              }
            }
            else{
              SetEfficientSquareOff(true);
            }
          }
        }

        if (msecs_from_midnight > start_ratio_end_mfm_) {
          estimated_bid_price_ =
              (theo_values_.reference_primary_bid_ * ratio_) -
              bid_offset_ * obb_offset_mult_ * (eff_squareoff_on_ ? (bid_multiplier_ * bid_factor_) : 1);
          estimated_ask_price_ =
              (theo_values_.reference_primary_ask_ * ratio_) +
              ask_offset_ * obb_offset_mult_ * (eff_squareoff_on_ ? (ask_multiplier_ * ask_factor_) : 1);
        } else {
          estimated_bid_price_ =
              (theo_values_.reference_primary_bid_ * (ratio_ - ratio_diff_offset_)) -
              bid_offset_ * obb_offset_mult_ * (eff_squareoff_on_ ? (bid_multiplier_ * bid_factor_) : 1);
          estimated_ask_price_ =
              (theo_values_.reference_primary_ask_ * (ratio_ + ratio_diff_offset_)) +
              ask_offset_ * obb_offset_mult_ * (eff_squareoff_on_ ? (ask_multiplier_ * ask_factor_) : 1);
        }

        // DBGLOG_TIME_CLASS_FUNC << " ESTIMATED BID " << estimated_bid_price_ << " ASK " << estimated_ask_price_ << "
        // RATIO " << ratio_ << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  if (_security_id_ == secondary_id_) {
    theo_values_.is_primary_update_ = false;
    if ( false == is_secondary_book_valid ){
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
    if (!is_ready_ and sleep_after_ready_ and !sleep_called_) {
      // dbglogger_ << watch_.tv() << " " << theo_identifier_ <<" SLEEP CALLED\n";
      sleep_called_ = true;
      if (trading_start_utc_mfm_ > msecs_from_midnight) {
        trading_start_utc_mfm_ = trading_start_utc_mfm_ + training_msecs_after_start_ratio_;
      } else {
        trading_start_utc_mfm_ = msecs_from_midnight + training_msecs_after_start_ratio_;
      }
    }
    if (is_ready_ && ((status_mask_ & INVALIDBOOK_STATUS_SET) == 0)) {
      TurnOnTheo(INVALIDBOOK_STATUS_SET);
    }

    if (ratio_ != 0) {
      ComputeAndUpdateTheoListeners();
    }

    /*dbglogger_ << watch_.tv() << " " << theo_identifier_ << " SecID: " <<  _security_id_
      << " Sec: " << sec_name_indexer_.GetShortcodeFromId(_security_id_)
      << " PPx " << theo_values_.primary_best_bid_price_
      << " X " << theo_values_.primary_best_ask_price_
      << "::::SPx " << secondary_smv_->market_update_info().bestbid_price_
      << " X " << secondary_smv_->market_update_info().bestask_price_
      << "::ratio "<< ratio_
      << "::theopx "<< theo_values_.theo_bid_price_ << " X " << theo_values_.theo_ask_price_ << DBGLOG_ENDL_FLUSH;*/
    if ((_security_id_ == secondary_id_) && (!use_constant_ratio_) && theo_values_.is_big_trade_ == 0) {
      double primary_bestbid_px = theo_values_.primary_best_bid_price_;
      double primary_bestask_px = theo_values_.primary_best_ask_price_;
      double secondary_bestbid_px = _market_update_info_.bestbid_price_;
      double secondary_bestask_px = _market_update_info_.bestask_price_;
      double primary_spread = primary_bestask_px - primary_bestbid_px;
      double secondary_spread = secondary_bestask_px - secondary_bestbid_px;
      if (msecs_from_midnight - last_ratio_update_time_ >= ratio_update_time_ &&
          primary_spread <= max_primary_spread_ && primary_spread >= min_primary_spread_ &&
          secondary_spread <= max_secondary_spread_ && secondary_spread >= min_secondary_spread_) {
        double current_ratio =
            (secondary_bestbid_px + secondary_bestask_px) / (primary_bestbid_px + primary_bestask_px);
        if (current_ratio > min_ratio_ && current_ratio < max_ratio_) {
          if (ratio_ != 0) {
            ratio_ = ratio_ * (1 - alpha_) + alpha_ * current_ratio;
          } else {
            if (!(is_ready_)) {
              if (training_ratio_values_.size() >= MAX_QUEUE_SIZE) {
                training_ratio_values_.pop_front();
              }
              training_ratio_values_.push_back(current_ratio);
            }
          }
          last_ratio_update_time_ = msecs_from_midnight;
        }
#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
        if (dbglogger_.CheckLoggingLevel(RETAIL_INFO) && (msecs_from_midnight <= trading_end_utc_mfm_) &&
            (msecs_from_midnight > trading_start_utc_mfm_)) {
          dbglogger_ << watch_.tv() << " " << secondary_smv_->shortcode() << "_" << primary0_smv_->shortcode()
                     << " CurrentRatio: " << current_ratio << " ratio " << ratio_ << DBGLOG_ENDL_FLUSH;
        }
#endif

// #ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
        // if (dbglogger_.CheckLoggingLevel(THEOK_INFO) && (msecs_from_midnight <= trading_end_utc_mfm_) &&
        //     (msecs_from_midnight > trading_start_utc_mfm_)) {
        //   dbglogger_ << watch_.tv() << " " << secondary_smv_->shortcode() << " CurrentK: " << theo_k_ << " ";
        //   for (unsigned int i = 1;i< primary_smv_vec_.size() ;i++) {
        //     dbglogger_ << primary_smv_vec_[i]->shortcode() << " ";
        //   }
        //   dbglogger_ << DBGLOG_ENDL_FLUSH;
        // }
// #endif
      }
    }
  }
}

void RatioTheoCalculator::ComputeAndUpdateTheoListeners() {
  double prev_theo_bid_price_ = theo_values_.theo_bid_price_;
  double prev_theo_ask_price_ = theo_values_.theo_ask_price_;

  theo_values_.theo_bid_price_ =
      estimated_bid_price_ + bid_theo_shifts_ * (eff_squareoff_on_ ? (bid_multiplier_ * bid_factor_) : 1);
  theo_values_.theo_ask_price_ =
      estimated_ask_price_ + ask_theo_shifts_ * (eff_squareoff_on_ ? (ask_multiplier_ * ask_factor_) : 1);

  if ((prev_theo_bid_price_ + prev_theo_ask_price_) < (theo_values_.theo_bid_price_ + theo_values_.theo_ask_price_)) {
    theo_values_.movement_indicator_ = pUPWARD;
  } else {
    theo_values_.movement_indicator_ = pDOWNWARD;
  }

  /*dbglogger_ << watch_.tv() << " primarymkt: " << theo_values_.primary_best_bid_price_
            << " X " << theo_values_.primary_best_ask_price_
            << " secmkt: " << secondary_smv_->market_update_info().bestbid_price_
            << " X " << secondary_smv_->market_update_info().bestask_price_
            << " currenttheo: " <<  theo_values_.theo_bid_price_ << " X " << theo_values_.theo_ask_price_
            << " prevtheo: " << prev_theo_bid_price_ << " X " << prev_theo_ask_price_
            << " movement " << ((theo_values_.movement_indicator_ == pUPWARD)?"1":"0")
            << DBGLOG_ENDL_FLUSH;*/

  UpdateTheoListeners();
}

void RatioTheoCalculator::OnReady(const HFSAT::MarketUpdateInfo& _market_update_info_) {
  bid_offset_ = bid_percentage_offset_ * _market_update_info_.bestbid_price_;
  ask_offset_ = ask_percentage_offset_ * _market_update_info_.bestbid_price_;

  bid_increase_shift_ = bid_increase_shift_percent_ * _market_update_info_.bestbid_price_;
  bid_decrease_shift_ = bid_decrease_shift_percent_ * _market_update_info_.bestbid_price_;
  ask_increase_shift_ = ask_increase_shift_percent_ * _market_update_info_.bestbid_price_;
  ask_decrease_shift_ = ask_decrease_shift_percent_ * _market_update_info_.bestbid_price_;

  buy_sweep_bid_offset_ = buy_sweep_bid_offset_percent_ * _market_update_info_.bestbid_price_;
  buy_sweep_ask_offset_ = buy_sweep_ask_offset_percent_ * _market_update_info_.bestbid_price_;
  sell_sweep_bid_offset_ = sell_sweep_bid_offset_percent_ * _market_update_info_.bestbid_price_;
  sell_sweep_ask_offset_ = sell_sweep_ask_offset_percent_ * _market_update_info_.bestbid_price_;

  max_bid_offset_sweep_ = max_bid_offset_sweep_percent_ * _market_update_info_.bestbid_price_;
  max_ask_offset_sweep_ = max_ask_offset_sweep_percent_ * _market_update_info_.bestbid_price_;

  min_price_move_for_sweep_mode_ = min_percent_price_move_for_sweep_mode_ * _market_update_info_.bestbid_price_;

  if (!is_ready_ && !use_start_ratio_ && !use_constant_ratio_) {
    std::sort(training_ratio_values_.begin(), training_ratio_values_.end());
    ratio_ = training_ratio_values_[training_ratio_values_.size() / 2];
  }
  if (!is_ready_) {
    int num_vol_days_ = Parser::GetInt(key_val_map_, "NUM_DAYS_LOOKBACK_VOL", 10);
    int num_val_ = Parser::GetInt(key_val_map_, "NUM_VAL_VOL", 3);
    if (use_historic_vol_) {
      is_historic_vol_available_ =
          HelperFunctions::GetAvgVol(dbglogger_, watch_, secondary_smv_->shortcode(), ticker_name_, watch_.YYYYMMDD(),
                                     num_vol_days_, num_val_, avg_vol_vec_);
      // dbglogger_ << watch_.tv() << " GetVol " << is_vol_available_  << " " << avg_vol_vec_.size() <<
      // DBGLOG_ENDL_FLUSH;
    }
  }
  if (!initial_px_) {
    initial_px_ = _market_update_info_.bestbid_price_;

    initial_bid_increase_shift_ = bid_increase_shift_percent_ * initial_px_;
    initial_bid_decrease_shift_ = bid_decrease_shift_percent_ * initial_px_;
    initial_ask_increase_shift_ = ask_increase_shift_percent_ * initial_px_;
    initial_ask_decrease_shift_ = ask_decrease_shift_percent_ * initial_px_;
    initial_bid_offset_ = bid_percentage_offset_ * initial_px_;
    initial_ask_offset_ = ask_percentage_offset_ * initial_px_;
  }

  avg_int_spread_ = std::max(int(avg_spread_percent_ * _market_update_info_.bestbid_int_price_), 1);
  long_ema_int_spread_ = avg_int_spread_;
  short_ema_int_spread_ = avg_int_spread_;
  inv_avg_int_spread_ = 1.0 / avg_int_spread_;
  total_offset_in_avg_spread_ = (bid_percentage_offset_ + ask_percentage_offset_) / avg_spread_percent_;
  slope_obb_offset_mult_ = 1 / (total_offset_in_avg_spread_);
  intercept_obb_offset_mult_ = (total_offset_in_avg_spread_ + 1) / (total_offset_in_avg_spread_);

  dbglogger_ << watch_.tv() << " " << theo_identifier_ << " Offsets:" << bid_offset_ << "|" << ask_offset_ << "|"
             << bid_increase_shift_ << "|" << bid_decrease_shift_ << "|" << ask_increase_shift_ << "|"
             << ask_decrease_shift_ << "|" << ratio_ << "|" << min_price_move_for_sweep_mode_ << "|"
             << buy_sweep_bid_offset_ << "|" << buy_sweep_ask_offset_ << "|" << sell_sweep_bid_offset_ << "|"
             << sell_sweep_ask_offset_ << "\n";
  for (auto base_exec : base_exec_vec_) {
    base_exec->SetPassiveReduce(passive_reduce_position_);
    base_exec->SetAggressiveReduce(aggressive_reduce_position_);
    base_exec->SetEfficientSquareOff(eff_squareoff_on_);
    base_exec->OnReady(_market_update_info_);
  }

  config_reloaded_ = false;
  // size_offset_reloaded_ = false;
}

void RatioTheoCalculator::OnExec(const int _new_position_, const int _exec_quantity_,
                                 const HFSAT::TradeType_t _buysell_, const double _price_, const int r_int_price_,
                                 const int _security_id_, const int _caos_) {
  num_trades_++;
  // undo the reversion done in updatesizesandoffsets
  if (outside_position_shift_amount_ && use_vol_scaling_) {
    if (position_ > position_shift_amount_ && _new_position_ < position_shift_amount_) {
      ask_offset_ *= vol_offset_multiplier_;
      ask_increase_shift_ *= vol_offset_multiplier_;
      ask_decrease_shift_ *= vol_offset_multiplier_;
      outside_position_shift_amount_ = false;
      //   	dbglogger_ << watch_.tv() << " " << secondary_smv_->shortcode() << " psa shift " <<
      //   position_shift_amount_ << " " << position_ << " " << _new_position_ << DBGLOG_ENDL_FLUSH; 	dbglogger_ <<
      //   watch_.tv() << " " << theo_identifier_
      // << " Offsets:" << bid_offset_ << "|" << ask_offset_
      // << "|" << bid_increase_shift_ << "|" << bid_decrease_shift_
      // << "|" << ask_increase_shift_ << "|" << ask_decrease_shift_
      // << "\n";
    } else if (position_ < -1 * position_shift_amount_ && _new_position_ > -1 * position_shift_amount_) {
      bid_offset_ *= vol_offset_multiplier_;
      bid_increase_shift_ *= vol_offset_multiplier_;
      bid_decrease_shift_ *= vol_offset_multiplier_;
      outside_position_shift_amount_ = false;
      //   	dbglogger_ << watch_.tv() << " " << secondary_smv_->shortcode() << " psa shift " <<
      //   position_shift_amount_ << " " << position_ << " " << _new_position_ << DBGLOG_ENDL_FLUSH; 	dbglogger_ <<
      //   watch_.tv() << " " << theo_identifier_
      // << " Offsets:" << bid_offset_ << "|" << ask_offset_
      // << "|" << bid_increase_shift_ << "|" << bid_decrease_shift_
      // << "|" << ask_increase_shift_ << "|" << ask_decrease_shift_
      // << "\n";
    }
  }

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
  // dbglogger_ << watch_.tv() << " " << secondary_smv_->shortcode()
  // 	<< " position_to_offset_: " << (need_to_hedge_?position_to_offset_:position_)
  // 	<< " need_to_hedge_: " << need_to_hedge_
  // 	<< DBGLOG_ENDL_FLUSH;
  if (status_mask_ == BIT_SET_ALL) {
    ComputeAndUpdateTheoListeners();
  }

  for (auto base_exec : base_exec_vec_) {
    base_exec->SetInverseTradeFactor(1.0/num_trades_); // num_trades_ can't be 0 since we increment the value on every OnExec call
  }

  //dbglogger_ << "----- RatioTheoCalculator::OnExec----\n";
  dbglogger_.LogQueryExec(exec_log_buffer_, watch_.tv(), _buysell_, position_, _caos_, _price_,
			theo_values_.theo_bid_price_, theo_values_.theo_ask_price_, secondary_smv_->market_update_info().bestbid_price_,
			secondary_smv_->market_update_info().bestask_price_, theo_values_.primary_best_bid_price_,
			theo_values_.primary_best_ask_price_, theo_values_.reference_primary_bid_, theo_values_.reference_primary_ask_,
			basic_om_->trade_volume(), target_position_, total_traded_value_, fill_type);

}

void RatioTheoCalculator::UpdateTheoListeners() {
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

    // std::cout << " update theo status " <<position_to_offset_ + midterm_pos_offset_ <<std::endl;
    theo_values_.is_valid_ = true;
    theo_values_.position_to_offset_ = position_to_offset_ + midterm_pos_offset_;
    for (auto base_exec : base_exec_vec_) {
      base_exec->OnTheoUpdate();
    }
  }
}

void RatioTheoCalculator::SetupPNLHooks() {
  if (!sim_base_pnl_) {
    std::cerr << "ERROR SIM BASE PNL NOT SUBSCRIBED (exiting)" << std::endl;
    exit(0);
  }

  if (!need_to_hedge_) {
    // In case we are not hedging SL checks are being done in RatioTheo itself
    sim_base_pnl_->AddListener(0, this);
  } else {
    // In case we are hedgin the SL checks will be done by Hedge Theo
    // mult base pnl of hdg will be listener of sim_base_pnl
    for (auto hdg_elem : hedge_vector_) {
      hdg_elem->hedge_theo_->SubscribeBasePNL(sim_base_pnl_);
    }
  }
}
