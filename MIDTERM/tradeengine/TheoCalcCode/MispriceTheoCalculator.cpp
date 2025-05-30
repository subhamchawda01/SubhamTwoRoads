#include "tradeengine/TheoCalc/MispriceTheoCalculator.hpp"
#include "tradeengine/Utils/Parser.hpp"

MispriceTheoCalculator::MispriceTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                                               HFSAT::DebugLogger& _dbglogger_, int _trading_start_utc_mfm_,
                                               int _trading_end_utc_mfm_, int _aggressive_get_flat_mfm_,
                                               int _eff_squareoff_start_utc_mfm_ = 0, double _bid_multiplier_ = 1,
                                               double _ask_multiplier_ = 1)
    : RatioTheoCalculator(key_val_map, _watch_, _dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_,
                          _aggressive_get_flat_mfm_, _eff_squareoff_start_utc_mfm_, _bid_multiplier_, _ask_multiplier_),
      ratio_threshold_(0),
      bid_start_ratio_(0),
      ask_start_ratio_(0),
      current_bid_ratio_(0),
      current_ask_ratio_(0),
      current_mid_ratio_(0),
      ready_to_trade_(false) {
  dbglogger_ << watch_.tv() << " Creating MISPRICE THEO CALCULATOR secId " << secondary_id_ << " primId "
             << primary0_id_ << DBGLOG_ENDL_FLUSH;
  LoadInitialParams();
}

void MispriceTheoCalculator::LoadParams() {
  RatioTheoCalculator::LoadParams();
  LoadInitialParams();
}

void MispriceTheoCalculator::LoadInitialParams() {
  ratio_threshold_ = Parser::GetDouble(key_val_map_, "RATIO_THRESHOLD", 0) / 100;
  bid_start_ratio_ = ratio_ - ratio_diff_offset_;
  ask_start_ratio_ = ratio_ + ratio_diff_offset_;
  DBGLOG_TIME_CLASS_FUNC << "Bid Ratio: " << bid_start_ratio_ << " Ask Ratio: " << ask_start_ratio_
                         << DBGLOG_ENDL_FLUSH;
}

void MispriceTheoCalculator::UpdateTheoPrices(const unsigned int _security_id_,
                                              const HFSAT::MarketUpdateInfo& _market_update_info_) {
  int msecs_from_midnight = watch_.msecs_from_midnight();
  // double present_update_ratio_ = 0;


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
          DBGLOG_TIME_CLASS_FUNC << "AUTO SQUAREOFF CALLED : " << secondary_smv_->shortcode() << " ON PRIMARY : " << primary0_smv_->shortcode() << " NEARING A CIRCUIT LEVEL : " << primary0_smv_->market_update_info_.bestbid_int_price_ << " X " << primary0_smv_->market_update_info_.bestask_int_price_ << " THRESHOLD : " << lower_int_price_threshold << " X "  << upper_int_price_threshold << DBGLOG_ENDL_FLUSH ;
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
          is_squaredoff_due_to_autogetflat_near_primary_circuit_ = false ;
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
          DBGLOG_TIME_CLASS_FUNC << "AUTO SQUAREOFF CALLED : " << secondary_smv_->shortcode() << " ON SELF NEARING A CIRCUIT LEVEL : " << secondary_smv_->market_update_info_.bestbid_int_price_ << " X " << secondary_smv_->market_update_info_.bestask_int_price_ << " THRESHOLD : " << lower_int_price_threshold << " X "  << upper_int_price_threshold << DBGLOG_ENDL_FLUSH ;
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
      dbglogger_ << watch_.tv() << " RATIO THEO " << sec_name_indexer_.GetShortcodeFromId(_security_id_) << " ratio_ "
                 << ratio_ << DBGLOG_ENDL_FLUSH;
      start_trading_ = false;
    }
  }

  if (msecs_from_midnight > aggressive_get_flat_mfm_) {
    eff_squareoff_on_ = false;
    TurnOffTheo(SQUAREOFF_STATUS_UNSET);
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
      dbglogger_ << watch_.tv() << " RATIO THEO " << sec_name_indexer_.GetShortcodeFromId(_security_id_) << " ratio_ "
                 << ratio_ << DBGLOG_ENDL_FLUSH;
      dbglogger_ << watch_.tv() << " Getting AgressivelyFlat " << sec_name_indexer_.GetShortcodeFromId(_security_id_)
                 << " EndTimeMFM: " << aggressive_get_flat_mfm_ << DBGLOG_ENDL_FLUSH;
      start_trading_ = false;
      is_agressive_getflat_ = true;
    }
  }

  if (_security_id_ == secondary_id_) {
    theo_values_.is_primary_update_ = false;
    if (false == is_secondary_book_valid){
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
    if (false == is_primary_book_valid){
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
    }
  }

  if ((secondary_book_valid_) && (primary_book_valid_)) {
    if (is_ready_ && ((status_mask_ & INVALIDBOOK_STATUS_SET) == 0)) {
      TurnOnTheo(INVALIDBOOK_STATUS_SET);
    }

    // Compute current bid ratio and current ask ratio
    if (_security_id_ == secondary_id_) {
      double primary_bestbid_px = primary0_smv_->market_update_info().bestbid_price_;
      double primary_bestask_px = primary0_smv_->market_update_info().bestask_price_;
      double secondary_bestbid_px = _market_update_info_.bestbid_price_;
      double secondary_bestask_px = _market_update_info_.bestask_price_;
      double primary_spread = primary_bestask_px - primary_bestbid_px;
      double secondary_spread = secondary_bestask_px - secondary_bestbid_px;
      if (msecs_from_midnight - last_ratio_update_time_ >= ratio_update_time_ &&
          primary_spread <= max_primary_spread_ && primary_spread >= min_primary_spread_ &&
          secondary_spread <= max_secondary_spread_ && secondary_spread >= min_secondary_spread_) {
        double bid_ratio = (secondary_bestbid_px / ((primary_bestbid_px + primary_bestask_px) * 0.5));
        double ask_ratio = (secondary_bestask_px / ((primary_bestbid_px + primary_bestask_px) * 0.5));
        // present_update_ratio_ =  (bid_ratio + ask_ratio)*0.5;
        if (!is_ready_) {
          current_bid_ratio_ = bid_ratio;
          current_ask_ratio_ = ask_ratio;
          if (use_start_ratio_) {
            bid_start_ratio_ = bid_start_ratio_ * (1 - alpha_) + alpha_ * bid_ratio;
            ask_start_ratio_ = ask_start_ratio_ * (1 - alpha_) + alpha_ * ask_ratio;
            start_ratio_ = (bid_start_ratio_ + ask_start_ratio_) * 0.5;
          }
        } else {
          if (bid_ratio > min_ratio_ && bid_ratio < max_ratio_) {
            current_bid_ratio_ = current_bid_ratio_ * (1 - alpha_) + alpha_ * bid_ratio;
            last_ratio_update_time_ = msecs_from_midnight;
          }
          if (ask_ratio > min_ratio_ && ask_ratio < max_ratio_) {
            current_ask_ratio_ = current_ask_ratio_ * (1 - alpha_) + alpha_ * ask_ratio;
            last_ratio_update_time_ = msecs_from_midnight;
          }
        }
        current_mid_ratio_ = (current_bid_ratio_ + current_ask_ratio_) * 0.5;
      }
    }

    if ((!is_ready_) && (use_start_ratio_) &&
        (((bid_start_ratio_ - ratio_threshold_ <= current_bid_ratio_) && (current_bid_ratio_ <= start_ratio_)) &&
         ((start_ratio_ <= current_ask_ratio_) && (current_ask_ratio_ <= ask_start_ratio_ + ratio_threshold_)))) {
      // Ratios are in line now, can start trading
      if (!ready_to_trade_) {
        DBGLOG_TIME_CLASS_FUNC << " " << secondary_smv_->shortcode() << ": Ready to trade now" << DBGLOG_ENDL_FLUSH;
      }
      ready_to_trade_ = true;
    }

    if ((!is_ready_ || config_reloaded_) && (ready_to_trade_) && (msecs_from_midnight > trading_start_utc_mfm_) &&
        (start_trading_) && (use_start_ratio_) && (_security_id_ == primary0_id_)) {
      OnReady(primary0_smv_->market_update_info());
      if (!is_ready_) {
        // if(bid_start_ratio_ - ratio_threshold_ > current_bid_ratio_ || current_bid_ratio_ > start_ratio_) {
        //	current_bid_ratio_ = bid_start_ratio_;
        //}

        // if(ask_start_ratio_ + ratio_threshold_ < current_ask_ratio_ || current_ask_ratio_ < start_ratio_) {
        //        current_ask_ratio_ = ask_start_ratio_;
        //}

        current_bid_ratio_ = bid_start_ratio_;
        current_ask_ratio_ = ask_start_ratio_;
        current_mid_ratio_ = (current_bid_ratio_ + current_ask_ratio_) * 0.5;
      }
      is_ready_ = true;
    }
  }

  if ((current_mid_ratio_ != 0) && (primary_book_valid_)) {
    // TODO NOTE the behaviour here.
    // The estimated price only updates on primary update even though the ratio
    // could have been updating because of multiple secondary updates
    // Might want to update at every update instead of just primary update
    if (_security_id_ == primary0_id_) {
      double primary_bid = 0;
      double primary_ask = 0;
      if (theo_values_.sweep_mode_active_ != 0) {
        double ltp_ = primary0_smv_->trade_print_info().trade_price_;
        if (sweep_side_ == HFSAT::TradeType_t::kTradeTypeBuy) {
          double max_bid_threshold_ = primary0_smv_->market_update_info().bestbid_price_ + max_bid_offset_sweep_;
          // Large Sweep move taking bid price very far from top
          if (max_bid_threshold_ < ltp_) {
            ltp_ = max_bid_threshold_ + (ltp_ - max_bid_threshold_) * fraction_above_max_sweep_offset_;
          }

          primary_bid = std::max(ltp_ - buy_sweep_bid_offset_, primary0_smv_->market_update_info().bestbid_price_);
          primary_ask = primary0_smv_->market_update_info().bestask_price_ + buy_sweep_ask_offset_;
        } else {
          double min_ask_threshold_ = primary0_smv_->market_update_info().bestask_price_ - max_ask_offset_sweep_;
          // Large Sweep move taking bid price very far from top
          if (min_ask_threshold_ > ltp_) {
            ltp_ = min_ask_threshold_ - (min_ask_threshold_ - ltp_) * fraction_above_max_sweep_offset_;
          }

          primary_bid = primary0_smv_->market_update_info().bestbid_price_ - sell_sweep_bid_offset_;
          primary_ask = std::min(ltp_ + sell_sweep_ask_offset_, primary0_smv_->market_update_info().bestask_price_);
        }
        if (primary_bid >= primary_ask) {
          DBGLOG_TIME_CLASS_FUNC << " ERROR: CROSS SWEEP SPREAD BID: " << primary_bid << " ASK: " << primary_ask
                                 << " ltp " << ltp_ << DBGLOG_ENDL_FLUSH;
        }
      } else {
        CalculatePrices(primary_bid, primary_ask, primary0_smv_, primary0_vwap_levels_, primary0_price_type_,
                        primary0_bid_size_filter_, primary0_ask_size_filter_, primary0_size_max_depth_);
      }
      estimated_bid_price_ =
          (primary_bid * current_mid_ratio_) - bid_offset_ * (eff_squareoff_on_ ? (bid_multiplier_ * bid_factor_) : 1);
      estimated_ask_price_ =
          (primary_ask * current_mid_ratio_) + ask_offset_ * (eff_squareoff_on_ ? (ask_multiplier_ * ask_factor_) : 1);
      // DBGLOG_TIME_CLASS_FUNC << " ESTIMATED BID " << estimated_bid_price_ << " ASK " << estimated_ask_price_ << "
      // RATIO " << ratio_ << DBGLOG_ENDL_FLUSH;
    }
    if (secondary_book_valid_) {
      ComputeAndUpdateTheoListeners();
    }
  }

  /*	dbglogger_ << watch_.tv() << " " << theo_identifier_ << " SecID: " <<  _security_id_
            << " Sec: " << sec_name_indexer_.GetShortcodeFromId(_security_id_)
            << " PPx " << primary0_smv_->market_update_info().bestbid_price_
            << " X " << primary0_smv_->market_update_info().bestask_price_
            << "::::SPx " << secondary_smv_->market_update_info().bestbid_price_
            << " X " << secondary_smv_->market_update_info().bestask_price_
            << "::BidRatio "<< current_bid_ratio_
            << "::AskRatio "<< current_ask_ratio_
            << "::BidStartRatio "<< bid_start_ratio_
            << "::AskStartRatio "<< ask_start_ratio_
            << "::theopx "<< estimated_bid_price_ << " X " << estimated_ask_price_ << DBGLOG_ENDL_FLUSH;*/
}
