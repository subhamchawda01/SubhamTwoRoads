#include "tradeengine/Executioner/Dimer.hpp"
#include "tradeengine/Utils/Parser.hpp"

Dimer::Dimer(std::string _exec_param_file, HFSAT::Watch& _watch_, HFSAT::DebugLogger& dbglogger_,
             HFSAT::SecurityMarketView* _secondary_smv, HFSAT::BasicOrderManager* _base_om, bool _livetrading_,
             bool _is_modify_before_confirmation_, bool _is_cancellable_before_confirmation_, TheoValues& theo_values)
    : BaseExecutioner(_exec_param_file, _watch_, dbglogger_, _secondary_smv, _base_om, _livetrading_,
                      _is_modify_before_confirmation_, _is_cancellable_before_confirmation_, theo_values),
      max_depth_(1),
      bid_percentage_offset_(1),
      ask_percentage_offset_(1),
      bid_offset_(0),
      ask_offset_(0),
      dimer_bid_size_(0),
      dimer_ask_size_(0),
      max_bid_join_size_(0),
      max_ask_join_size_(0),
      min_bid_dime_size_(0),
      min_ask_dime_size_(0),
      max_bid_offset_(0),
      max_ask_offset_(0),
      max_bid_offset_update_(0),
      max_ask_offset_update_(0),
      /*max_bid_offset_nojoindime_(0),
      max_ask_offset_nojoindime_(0),
      max_bid_offset_nojoindime_update_(0),
      max_ask_offset_nojoindime_update_(0),*/
      bid_overruled_(false),
      ask_overruled_(false),
      overruled_leaway_percent_(0.1),
      overruled_leaway_offset_(0),
      delay_between_orders_(0),
      drag_tighten_delay_(0),
      min_drag_tighten_delay_(0),
      max_drag_tighten_delay_(0),
      drag_widen_delay_(0),
      order_timeout_(0),
      use_spread_threshold_(false),
      spread_threshold_(0),
      passive_only_(false),
      allow_orders_beyond_maxoffset_(true),
      dimer_bid_price_(-1),
      dimer_bid_int_price_(-1),
      dimer_ask_price_(-1),
      dimer_ask_int_price_(-1),
      previous_bid_price_(-1),
      previous_bid_int_price_(-1),
      previous_ask_price_(-1),
      previous_ask_int_price_(-1),
      bid_order_at_maxoffset_(false),
      ask_order_at_maxoffset_(false),
      last_new_bid_order_time_(0),
      last_modify_bid_order_time_(0),
      last_new_ask_order_time_(0),
      last_modify_ask_order_time_(0),
      price_upper_limit_(0),
      price_lower_limit_(0),
      cancel_on_maxoffset_(false),
      unselective_if_maxoffset_(true),
      cancel_on_sweep_(0),
      safe_order_offset_percent_(0),
      safe_order_offset_int_(0),
      dime_two_ticks_(false),
      bid_order_(NULL),
      ask_order_(NULL),
      disable_one_percent_order_limit_(false),
      order_to_trade_ratio_percent_(1 / 100),
      order_to_trade_ratio_percent_upper_multiplier_(0),
      order_to_trade_ratio_percent_lower_multiplier_(0),
      min_levels_to_clear_for_cancel_on_big_trade_(100),
      num_modify_(0),
      num_priority_modify_(0),
      priority_limit_start_mfm_(0),
      priority_relative_percent_lower_limit_(0),
      priority_relative_percent_upper_limit_(100),
      last_priority_check_time_(0),
      priority_check_time_cooloff_(60000),
      disallow_priority_modify_(false),
      bad_fill_check_multiplier_(1),
      num_modify_threshold_(500) {
  LoadParams();
  bid_status_ = true;
  ask_status_ = true;
  basic_om_->AddThrottleNumberListeners(this);
  strcpy(order_log_buffer_.buffer_data_.query_order_.identifier_, identifier_.c_str());
}

void Dimer::LoadParams() {
  identifier_ = Parser::GetString(exec_key_val_map_, "IDENTIFIER", "");
  bool status_ = Parser::GetBool(exec_key_val_map_, "STATUS", false);
  status_mask_ = (status_ == true ? (status_mask_ | CONFIG_STATUS_SET) : (status_mask_ & CONFIG_STATUS_UNSET));
  max_depth_ = Parser::GetInt(exec_key_val_map_, "MAX_DEPTH", 1);
  bid_percentage_offset_ = Parser::GetDouble(exec_key_val_map_, "BID_PERCENTAGE_OFFSET", 1) / 100;
  ask_percentage_offset_ = Parser::GetDouble(exec_key_val_map_, "ASK_PERCENTAGE_OFFSET", 1) / 100;
  bid_size_ = Parser::GetInt(exec_key_val_map_, "BID_SIZE", 0, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);
  ask_size_ = Parser::GetInt(exec_key_val_map_, "ASK_SIZE", 0, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);
  max_bid_join_size_ = Parser::GetInt(exec_key_val_map_, "MAX_BID_JOIN_SIZE", 0, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);
  max_ask_join_size_ = Parser::GetInt(exec_key_val_map_, "MAX_ASK_JOIN_SIZE", 0, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);
  min_bid_dime_size_ = Parser::GetInt(exec_key_val_map_, "MIN_BID_DIME_SIZE", 0, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);
  min_ask_dime_size_ = Parser::GetInt(exec_key_val_map_, "MIN_ASK_DIME_SIZE", 0, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);

//  DBGLOG_CLASS_FUNC_LINE_INFO << "DIMER :" << secondary_smv_->shortcode() << " " << bid_size_ << " " << ask_size_ << " " << max_bid_join_size_ << " " << max_ask_join_size_ << " " << min_bid_dime_size_ << " " << min_ask_dime_size_ << DBGLOG_ENDL_FLUSH;

  max_bid_offset_ = Parser::GetDouble(exec_key_val_map_, "MAX_BID_OFFSET", 0);
  max_ask_offset_ = Parser::GetDouble(exec_key_val_map_, "MAX_ASK_OFFSET", 0);
  max_bid_offset_update_ = Parser::GetDouble(exec_key_val_map_, "MAX_BID_OFFSET_UPDATE", 0);
  max_ask_offset_update_ = Parser::GetDouble(exec_key_val_map_, "MAX_ASK_OFFSET_UPDATE", 0);
  /*max_bid_offset_nojoindime_ = Parser::GetDouble(exec_key_val_map_, "MAX_BID_OFFSET_NOJOINDIME", 0);
  max_ask_offset_nojoindime_ = Parser::GetDouble(exec_key_val_map_, "MAX_ASK_OFFSET_NOJOINDIME", 0);
  max_bid_offset_nojoindime_update_ = Parser::GetDouble(exec_key_val_map_, "MAX_BID_OFFSET_NOJOINDIME_UPDATE", 0);
  max_ask_offset_nojoindime_update_ = Parser::GetDouble(exec_key_val_map_, "MAX_ASK_OFFSET_NOJOINDIME_UPDATE", 0);*/
  order_limit_ = Parser::GetInt(exec_key_val_map_, "ORDER_LIMIT", 0);
  bad_ratio_limit_ =  Parser::GetInt(exec_key_val_map_, "BAD_RATIO_LIMIT", 1000000);
  bid_overruled_ = Parser::GetBool(exec_key_val_map_, "BID_OVERRULED", false);
  ask_overruled_ = Parser::GetBool(exec_key_val_map_, "ASK_OVERRULED", false);
  overruled_leaway_percent_ = Parser::GetDouble(exec_key_val_map_, "OVERRULED_LEAWAY_PERCENT", 0.1) / 100;
  delay_between_orders_ = Parser::GetInt(exec_key_val_map_, "DELAY_BETWEEN_ORDERS", 0);
  config_drag_tighten_delay_ = Parser::GetInt(exec_key_val_map_, "DRAG_TIGHTEN_DELAY", 0);
  drag_tighten_delay_ = config_drag_tighten_delay_;
  min_drag_tighten_delay_ = Parser::GetInt(exec_key_val_map_, "MIN_DRAG_TIGHTEN_DELAY", 0);
  max_drag_tighten_delay_ = Parser::GetInt(exec_key_val_map_, "MAX_DRAG_TIGHTEN_DELAY", 200);
  min_throttles_per_min_ = Parser::GetInt(exec_key_val_map_, "MIN_THROTTLE_PER_MIN", 1);
  max_throttles_per_min_ = Parser::GetInt(exec_key_val_map_, "MAX_THROTTLE_PER_MIN", 100);
  drag_widen_delay_ = Parser::GetInt(exec_key_val_map_, "DRAG_WIDEN_DELAY", 0);
  order_timeout_ = Parser::GetInt(exec_key_val_map_, "ORDER_TIMEOUT", 0);
  use_spread_threshold_ = Parser::GetBool(exec_key_val_map_, "USE_SPREAD_THRESHOLD", false);
  spread_threshold_ = Parser::GetDouble(exec_key_val_map_, "SPREAD_THRESHOLD", 0);
  passive_only_ = Parser::GetBool(exec_key_val_map_, "PASSIVE_ONLY", false);
  allow_orders_beyond_maxoffset_ = Parser::GetBool(exec_key_val_map_, "ALLOW_BEYOND_MAXOFFSET", true);
  disable_one_percent_order_limit_ = Parser::GetBool(exec_key_val_map_, "DISABLE_ONE_PERCENT_ORDER_LIMIT", false);
  order_to_trade_ratio_percent_ = Parser::GetDouble(exec_key_val_map_, "ORDER_TO_TRADE_RATIO_PERCENT", 0.99) / 100;
  order_to_trade_ratio_percent_upper_multiplier_ = 1 + order_to_trade_ratio_percent_;
  order_to_trade_ratio_percent_lower_multiplier_ = 1 - order_to_trade_ratio_percent_;
  min_levels_to_clear_for_cancel_on_big_trade_ =
      Parser::GetInt(exec_key_val_map_, "MIN_LVL_CLEAR_CANCEL_BIG_TRADE", 100);
  use_reserve_msg_ = Parser::GetBool(exec_key_val_map_, "USE_RESERVE_MSG", false);
  cancel_on_maxoffset_ = Parser::GetBool(exec_key_val_map_, "CANCEL_ON_MAXOFFSET", false);
  unselective_if_maxoffset_ = Parser::GetBool(exec_key_val_map_, "UNSELECTIVE_IF_MAXOFFSET", true);
  cancel_on_sweep_ = Parser::GetInt(exec_key_val_map_, "CANCEL_ON_SWEEP", 0);
  safe_order_offset_percent_ = Parser::GetDouble(exec_key_val_map_, "SAFE_ORDER_OFFSET_PERCENT", 1) / 100;
  dime_two_ticks_ = Parser::GetBool(exec_key_val_map_, "DIME_TWO_TICKS", false);
  priority_limit_start_mfm_ = HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(watch_.YYYYMMDD(), Parser::GetInt(exec_key_val_map_, "PRIORITY_LIMIT_START_TIME", 400), "UTC_");
  priority_relative_percent_lower_limit_ = Parser::GetInt(exec_key_val_map_, "PRIORITY_RELATIVE_PERCENT_LOWER_LIMIT", 8);
  priority_relative_percent_upper_limit_ = Parser::GetInt(exec_key_val_map_, "PRIORITY_RELATIVE_PERCENT_UPPER_LIMIT", 10);
  priority_self_percent_lower_limit_ = Parser::GetInt(exec_key_val_map_, "PRIORITY_SELF_PERCENT_LOWER_LIMIT", 40);
  priority_self_percent_upper_limit_ = Parser::GetInt(exec_key_val_map_, "PRIORITY_SELF_PERCENT_UPPER_LIMIT", 45);
  bad_fill_check_multiplier_ = Parser::GetDouble(exec_key_val_map_, "BAD_FILL_CHECK_MULTIPLIER", 1);
  num_modify_threshold_ = Parser::GetInt(exec_key_val_map_, "NUM_MODIFY_THRESHOLD", 500);

  dimer_bid_size_ = bid_size_;
  dimer_ask_size_ = ask_size_;

  if ((bid_size_ % secondary_smv_->min_order_size_ != 0) || (ask_size_ % secondary_smv_->min_order_size_ != 0)) {
    dbglogger_ << watch_.tv() << " ERROR: " << secondary_smv_->shortcode() << " WRONG SIZES BS: " << bid_size_
               << " AS: " << ask_size_ << " LS: " << secondary_smv_->min_order_size_ << " UNSETTING CONFIG STATUS"
               << DBGLOG_ENDL_FLUSH;
    status_mask_ = status_mask_ & CONFIG_STATUS_UNSET;
  }

  bid_size_ = std::max(
      HFSAT::MathUtils::GetFlooredMultipleOf(bid_size_ * size_multiplier_ + 0.001, secondary_smv_->min_order_size_),
      secondary_smv_->min_order_size_);
  ask_size_ = std::max(
      HFSAT::MathUtils::GetFlooredMultipleOf(ask_size_ * size_multiplier_ + 0.001, secondary_smv_->min_order_size_),
      secondary_smv_->min_order_size_);
  dimer_bid_size_ = bid_size_;
  dimer_ask_size_ = ask_size_;

  if ((max_bid_offset_ < 0) || (max_ask_offset_ < 0)) {
    dbglogger_ << watch_.tv() << " ERROR: " << secondary_smv_->shortcode()
               << " NEGATIVE MAX OFFSETS Bid: " << max_bid_offset_ << " Ask: " << max_ask_offset_
               << " UNSETTING CONFIG STATUS" << DBGLOG_ENDL_FLUSH;
    status_mask_ = status_mask_ & CONFIG_STATUS_UNSET;
  }
}

void Dimer::OnReady(const HFSAT::MarketUpdateInfo& _market_update_info_) {
  if (_market_update_info_.bestbid_price_ != kInvalidPrice) {
    bid_offset_ = bid_percentage_offset_ * _market_update_info_.bestbid_price_;
    ask_offset_ = ask_percentage_offset_ * _market_update_info_.bestbid_price_;
  } else {
    bid_offset_ = bid_percentage_offset_ * _market_update_info_.bestask_price_;
    ask_offset_ = ask_percentage_offset_ * _market_update_info_.bestask_price_;
  }
  max_bid_offset_int_ = HFSAT::MathUtils::RoundOff(max_bid_offset_ * inverse_tick_size_);
  max_ask_offset_int_ = HFSAT::MathUtils::RoundOff(max_ask_offset_ * inverse_tick_size_);
  max_bid_offset_update_int_ = HFSAT::MathUtils::RoundOff(max_bid_offset_update_ * inverse_tick_size_);
  max_ask_offset_update_int_ = HFSAT::MathUtils::RoundOff(max_ask_offset_update_ * inverse_tick_size_);

  safe_order_offset_int_ = safe_order_offset_percent_ * _market_update_info_.bestbid_int_price_;
  overruled_leaway_offset_ = overruled_leaway_percent_ * _market_update_info_.bestbid_price_;
  dbglogger_ << watch_.tv() << "DIMER Offsets: " << bid_offset_ << "|" << ask_offset_ << "|" << max_bid_offset_int_
             << "|" << max_ask_offset_int_ << "|" << max_bid_offset_update_int_ << "|" << max_ask_offset_update_int_
             << "|" << safe_order_offset_int_ << "\n";
}

void Dimer::OnTheoUpdate() {
  dimer_bid_price_ = theo_values_.theo_bid_price_ - bid_offset_;
  dimer_bid_int_price_ = int(dimer_bid_price_ * inverse_tick_size_ + DOUBLE_PRECISION);
  dimer_bid_price_ = dimer_bid_int_price_ * tick_size_;

  dimer_ask_price_ = theo_values_.theo_ask_price_ + ask_offset_;
  dimer_ask_int_price_ = int(std::ceil(dimer_ask_price_ * inverse_tick_size_ - DOUBLE_PRECISION));
  dimer_ask_price_ = dimer_ask_int_price_ * tick_size_;

  CheckOrderStatus(bid_order_);
  CheckOrderStatus(ask_order_);
  UpdateOrders();
}

void Dimer::CheckOrderStatus(HFSAT::BaseOrder* _order_) {
  if (bid_order_ && _order_ &&
      bid_order_->client_assigned_order_sequence_ == _order_->client_assigned_order_sequence_) {
    if ((_order_->order_status_ == HFSAT::kORRType_Exec || _order_->order_status_ == HFSAT::kORRType_Rejc ||
         _order_->order_status_ == HFSAT::kORRType_Cxld)) {
      bid_order_ = NULL;
      previous_bid_price_ = -1;
      previous_bid_int_price_ = -1;
    }
  } else if (ask_order_ && _order_ &&
             ask_order_->client_assigned_order_sequence_ == _order_->client_assigned_order_sequence_) {
    if ((_order_->order_status_ == HFSAT::kORRType_Exec || _order_->order_status_ == HFSAT::kORRType_Rejc ||
         _order_->order_status_ == HFSAT::kORRType_Cxld)) {
      // dbglogger_ << watch_.tv() << "Order status " << _order_->order_status_ << DBGLOG_ENDL_FLUSH;
      ask_order_ = NULL;
      previous_ask_price_ = -1;
      previous_ask_int_price_ = -1;
    }
  }
  //   else{
  //   	dbglogger_ << watch_.tv() << " "
  // << sec_shortcode_ << " " << identifier_
  // <<" Invalid order_update " << DBGLOG_ENDL_FLUSH;
  //   }
}

void Dimer::OnOrderReject(HFSAT::BaseOrder* _order_) {
  // DBGLOG_TIME_CLASS_FUNC << " DIMER: OrderReject callback for " << secondary_smv_->shortcode() << DBGLOG_ENDL_FLUSH;
  CheckOrderStatus(_order_);
  //	dbglogger_.DumpCurrentBuffer();
}

void Dimer::OnOrderChange(HFSAT::BaseOrder* _order_) {
  CheckOrderStatus(_order_);
  UpdateOrders();
  dbglogger_.DumpCurrentBuffer();
}

void Dimer::UpdateOrders() {
  if (passive_reduce_ || eff_squareoff_) {
    // To avoid cases like DRREDDY on 20171201 where we are constantly trying to close
    // positions aggressively and caught in a loop when position to close is exactly
    // half of min order size
    int target_pos_to_close_ =
        HFSAT::MathUtils::RoundOff((-1 * theo_values_.position_to_offset_) + 0.001, secondary_smv_->min_order_size_);
    if (target_pos_to_close_ > 0) {
      EnableBid();
      DisableAsk();
      dimer_bid_size_ = std::min(bid_size_, target_pos_to_close_);
      enable_ask_on_reload_ = true;
    } else if (target_pos_to_close_ < 0) {
      EnableAsk();
      DisableBid();
      dimer_ask_size_ = std::min(ask_size_, -1 * target_pos_to_close_);
      enable_bid_on_reload_ = true;
    } else {
      DisableBid();
      DisableAsk();
      enable_bid_on_reload_ = true;
      enable_ask_on_reload_ = true;
      return;
    }
  } else {
    dimer_bid_size_ = bid_size_;
    dimer_ask_size_ = ask_size_;
  }

  if (!theo_values_.is_valid_) {
    CancelBid();
    CancelAsk();
    return;
  } else if (agg_sqoff_fav_invalid_) {
    if ((theo_values_.position_to_offset_ < 0 && secondary_smv_->market_update_info().bestask_price_ < 0) ||
        (theo_values_.position_to_offset_ > 0 && secondary_smv_->market_update_info().bestbid_price_ < 0)) {
      CancelBid();
      CancelAsk();
      return;
    }
  } else if (secondary_smv_->market_update_info().bestbid_price_ < 0 ||
             secondary_smv_->market_update_info().bestask_price_ < 0) {
    CancelBid();
    CancelAsk();
    return;
  }

  price_upper_limit_ = order_to_trade_ratio_percent_upper_multiplier_ * theo_values_.last_traded_int_price_;
  price_lower_limit_ = order_to_trade_ratio_percent_lower_multiplier_ * theo_values_.last_traded_int_price_;
  if ((cancel_on_sweep_ != 0) && (theo_values_.sweep_mode_active_ != 0)) {
    if (cancel_on_sweep_ == 2) {
      CancelBid();
      CancelAsk();
      return;
    } else {
      if (theo_values_.sweep_mode_active_ > 0) {
        CancelAsk();
        UpdateBidOrders();
      } else {
        CancelBid();
        UpdateAskOrders();
      }
    }
  } else {
    if (theo_values_.movement_indicator_ == pUPWARD) {
      UpdateAskOrders();
      UpdateBidOrders();
    } else {
      UpdateBidOrders();
      UpdateAskOrders();
    }
  }

  if (order_count_ >= order_limit_) {
    if (status_mask_ & SHOOT_STATUS_SET) {
      DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " " << identifier_  << "ORDER COUNT HIT LIMIT: " << order_limit_ << " DISABLING DIMER " << DBGLOG_ENDL_FLUSH;
    }
    TurnOff(SHOOT_STATUS_UNSET);
  }

  int current_time_ = watch_.msecs_from_midnight();
  if((current_time_ > priority_limit_start_mfm_) && (secondary_smv_->num_events_modify_) && (num_modify_ > num_modify_threshold_)) {
    if(current_time_ - last_priority_check_time_ > priority_check_time_cooloff_) {
      last_priority_check_time_ = current_time_;
      int trade_volume_ = basic_om_->trade_volume();
      
      if((trade_volume_ < dimer_bid_size_*bad_fill_check_multiplier_) ||  (trade_volume_ < dimer_ask_size_*bad_fill_check_multiplier_)) { //Check for priority message limit only if trade volume is low
        double priority_relative_percent_ = num_priority_modify_*100.0/secondary_smv_->num_events_modify_;
        if(priority_relative_percent_ > priority_relative_percent_upper_limit_) {
          if(status_mask_ & BAD_FILL_RATIO_STATUS_SET) {
            DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " " << identifier_ << " BAD PRIORITY RELATIVE HIT LIMIT: " << priority_relative_percent_ << " MarketEvents: " << secondary_smv_->num_events_modify_ << " OurModify: " << num_priority_modify_ << DBGLOG_ENDL_FLUSH;
          }
          TurnOff(BAD_FILL_RATIO_STATUS_UNSET);
        } else if (priority_relative_percent_ < priority_relative_percent_lower_limit_) {
          if((status_mask_ & BAD_FILL_RATIO_STATUS_SET) == 0) {
            DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " " << identifier_ << " BAD PRIORITY RELATIVE RESET LIMIT: " << priority_relative_percent_ << " MarketEvents: " << secondary_smv_->num_events_modify_ << " OurModify: " << num_priority_modify_ << DBGLOG_ENDL_FLUSH;
          }
          TurnOn(BAD_FILL_RATIO_STATUS_SET);
        }

        double priority_self_percent_ = num_priority_modify_*100.0/num_modify_;
        if(priority_self_percent_ > priority_self_percent_upper_limit_) {
          if(!disallow_priority_modify_) {
            DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " " << identifier_ << " BAD PRIORITY SELF HIT LIMIT: " << priority_self_percent_ << " TotalModify: " << num_modify_ << " OurModify: " << num_priority_modify_ << DBGLOG_ENDL_FLUSH;
          }
          disallow_priority_modify_ = true;
        } else if (priority_self_percent_ < priority_self_percent_lower_limit_) {
          if(disallow_priority_modify_) {
            DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " " << identifier_ << " BAD PRIORITY SELF RESET LIMIT: " << priority_self_percent_ << " TotalModify: " << num_modify_ << " OurModify: " << num_priority_modify_ << DBGLOG_ENDL_FLUSH;
          }
          disallow_priority_modify_ = false;
        }
      }
    }
  }
}

void Dimer::DisableBid() {
  bid_status_ = false;
  enable_bid_on_reload_ = false;
  CancelBid();
}

void Dimer::DisableAsk() {
  ask_status_ = false;
  enable_ask_on_reload_ = false;
  CancelAsk();
}

void Dimer::CancelBid() {
  if (bid_order_ && bid_order_->order_status_ != HFSAT::kORRType_Cxld) basic_om_->Cancel(*bid_order_);
}

void Dimer::CancelAsk() {
  if (ask_order_ && ask_order_->order_status_ != HFSAT::kORRType_Cxld) basic_om_->Cancel(*ask_order_);
}

void Dimer::UpdateBidOrders() {
  if ((!bid_status_) || (status_mask_ != BIT_SET_ALL)) {
    CancelBid();
    return;
  }
  if (dimer_bid_price_ <= 0 || dimer_ask_price_ <= 0) return;

  if (bid_order_ && ((bid_order_->canceled_) || ((bid_order_->modified_) && (!is_modify_before_confirmation_)))) {
    // Cant do anything with the order till order modify sent
    return;
  }

  // DBGLOG_TIME_CLASS_FUNC << "UpdateBid bidpx" << dimer_bid_price_ << " askpx " << dimer_ask_price_ <<
  // DBGLOG_ENDL_FLUSH;

  bool sending_order_to_maxoffset_ = false;
  int current_order_bid_int_price_ = dimer_bid_int_price_;

  int level_count = 0;
  int level_dimer_price_ = 0;
  int price_int_diff_ = 0;
  bool already_at_good_price_ = false;
  bool end_of_book_reached_ = false;

  if (theo_values_.is_big_trade_ == -1) {
    if (bid_order_) {
      current_order_bid_int_price_ =
          (dimer_bid_int_price_ < bid_order_->int_price_) ? dimer_bid_int_price_ : bid_order_->int_price_;
      // dbglogger_ << watch_.tv() << " OnBigTrade Sending bid order to " << current_order_bid_int_price_ << " currpx: "
      // << bid_order_->int_price_ << DBGLOG_ENDL_FLUSH;
      SendBidOrder(current_order_bid_int_price_);
    }
    return;
  } else if (theo_values_.is_big_trade_ == 1) {
    return;
  }

  for (level_count = 0; level_count < max_depth_; level_count++) {
    HFSAT::MarketUpdateInfoLevelStruct* bid_info_ = secondary_smv_->bid_info(level_count);

    if (bid_info_ == NULL) {
      end_of_book_reached_ = true;
      break;
    }

    int t_bid_int_price_ = bid_info_->limit_int_price_;
    // Violating dimer price
    if (t_bid_int_price_ > dimer_bid_int_price_) {
      level_dimer_price_++;
      continue;
    }

    int t_size_at_level = bid_info_->limit_size_;

    if (bid_order_ && (bid_order_->int_price_ == t_bid_int_price_) &&
        (t_size_at_level - bid_order_->size_remaining_*livetrading_ >= min_bid_dime_size_)) {
      // do NOT modify this order; already at good price
      already_at_good_price_ = true;
      break;
    } else if (t_size_at_level >= min_bid_dime_size_ &&
               (!bid_order_ || (bid_order_ && bid_order_->int_price_ != t_bid_int_price_))) {
      price_int_diff_ = dimer_bid_int_price_ - t_bid_int_price_;
      break;
    }
  }

  // At this point invariants are:
  // i) price_int_diff : Difference between dimer price and order price
  // ii) level_count is number of valid levels seen till now
  if (already_at_good_price_) {
    current_order_bid_int_price_ = bid_order_->int_price_;
  } else if ((level_count >= max_depth_) || (end_of_book_reached_)) {
    // Hit MaxDepth, go to max offset, no place to dime
    current_order_bid_int_price_ = dimer_bid_int_price_ - max_bid_offset_int_;
    if(bid_order_ && (bid_order_->int_price_ <= current_order_bid_int_price_)) {
      current_order_bid_int_price_ = bid_order_->int_price_;
    }
    sending_order_to_maxoffset_ = true;
    // DBGLOG_TIME_CLASS_FUNC << " Hit MAXDEPTH " << DBGLOG_ENDL_FLUSH;
  } else {
    if (price_int_diff_ == 0) {
      // Dime Size present at dimer price itself.
      // So cant find a place to dime going to max offset
      current_order_bid_int_price_ = dimer_bid_int_price_ - max_bid_offset_int_;
      sending_order_to_maxoffset_ = true;
      // DBGLOG_TIME_CLASS_FUNC << "DIME SIZE AT TOP LEVEL" << DBGLOG_ENDL_FLUSH;
    } else {
      // Need to traverse sizes from current level -> top_level to find join size
      // In this case we need to look into empty level also
      int prev_level_int_price_ = secondary_smv_->bid_int_price(level_count);
      int t_level_ = 0;
      if (dime_two_ticks_ && bid_order_ && bid_order_->int_price_ <= dimer_bid_int_price_ &&
          bid_order_->int_price_ - prev_level_int_price_ <= 2) {
        current_order_bid_int_price_ = bid_order_->int_price_;
      } else {
        for (t_level_ = level_count - 1; t_level_ >= level_dimer_price_; t_level_--) {
          HFSAT::MarketUpdateInfoLevelStruct* bid_info_ = secondary_smv_->bid_info(t_level_);
          int cur_level_int_price_ = bid_info_->limit_int_price_;
          if ((cur_level_int_price_ - prev_level_int_price_) > 1) {
            current_order_bid_int_price_ = prev_level_int_price_ + 1;
            break;
          } else {
            if (bid_order_ && bid_order_->int_price_ == cur_level_int_price_) {
              current_order_bid_int_price_ = bid_order_->int_price_;
              break;
            } else {
              int size_at_level = bid_info_->limit_size_;
              if (size_at_level <= max_bid_join_size_) {
                current_order_bid_int_price_ = cur_level_int_price_;
                break;
              }
            }
          }
          prev_level_int_price_ = cur_level_int_price_;
        }
      }

      if (t_level_ < level_dimer_price_) {
        int t_price_int_diff_ = dimer_bid_int_price_ - prev_level_int_price_;
        if (dime_two_ticks_ && (!bid_order_ || bid_order_->int_price_ < prev_level_int_price_) &&
            prev_level_int_price_ + 2 <= dimer_bid_int_price_) {
          current_order_bid_int_price_ = prev_level_int_price_ + 2;
        } else if (dime_two_ticks_ && bid_order_ && bid_order_->int_price_ <= dimer_bid_int_price_ &&
                   bid_order_->int_price_ - prev_level_int_price_ <= 2) {
          current_order_bid_int_price_ = bid_order_->int_price_;
        } else if (t_price_int_diff_ > 0) {
          current_order_bid_int_price_ = prev_level_int_price_ + 1;
        } else {
          // Did not find a place to join
          // go to max offset
          current_order_bid_int_price_ = dimer_bid_int_price_ - max_bid_offset_int_;
          sending_order_to_maxoffset_ = true;
          // DBGLOG_TIME_CLASS_FUNC << "NO LEVEL FOUND TO JOIN" << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  if (!allow_orders_beyond_maxoffset_ && current_order_bid_int_price_ < dimer_bid_int_price_ - max_bid_offset_int_) {
    // DBGLOG_TIME_CLASS_FUNC << " In case when current order less than max offset" << DBGLOG_ENDL_FLUSH;
    current_order_bid_int_price_ = dimer_bid_int_price_ - max_bid_offset_int_;
    sending_order_to_maxoffset_ = true;
  }

  // Sanity checks once above logic is done
  // i) Do NOT dime your own order for now (till we are able to get size ahead)
  // ii) If current and previous order at max offset; dont modify until theo change > update%
  // iii) drag tighten/widen logic
  //

  // DBGLOG_TIME_CLASS_FUNC << " sending to maxoff " << sending_order_to_maxoffset_ << " already at max " <<
  // bid_order_at_maxoffset_ << DBGLOG_ENDL_FLUSH;
  // MaxOffset Update logic for ordering moderation
  if (sending_order_to_maxoffset_ && bid_order_at_maxoffset_ && bid_order_) {
    // previous order was also at max offset
    if (bid_order_->int_price_ > dimer_bid_int_price_ ||
        current_order_bid_int_price_ - bid_order_->int_price_ >= max_bid_offset_update_int_ ||
        bid_order_->int_price_ - current_order_bid_int_price_ >= max_bid_offset_update_int_) {
      // modify order to current_order_bid_int_price
    } else {
      // Do NOT modify order as price within update range
      // DBGLOG_TIME_CLASS_FUNC << "ORDER WITHIN UPDATE, NOT UPDATING ORDER " << DBGLOG_ENDL_FLUSH;
      current_order_bid_int_price_ = bid_order_->int_price_;
    }
  }

  // DBGLOG_TIME_CLASS_FUNC << "current final px " << current_order_bid_int_price_ << DBGLOG_ENDL_FLUSH;
  // SendOrder at current_order_bid_int_price_
  if (sending_order_to_maxoffset_) {
    sending_order_to_maxoffset_ = false;
    if((unselective_if_maxoffset_) || (!bid_order_) || (bid_order_->int_price_ > dimer_bid_int_price_)) {
      if(cancel_on_maxoffset_) {
        CancelBid();
      } else {
        SendBidOrder(current_order_bid_int_price_);
        sending_order_to_maxoffset_ = true;
      } 
    } else {
      current_order_bid_int_price_ = bid_order_->int_price_;
    }
  } else {
    SendBidOrder(current_order_bid_int_price_);
  }

  if (!sending_order_to_maxoffset_) {
    bid_order_at_maxoffset_ = false;
  } else {
    bid_order_at_maxoffset_ = true;
  }
}

void Dimer::SendBidOrder(int current_order_bid_int_price_) {
  if (current_order_bid_int_price_ <= 0) return;
  int current_time = watch_.msecs_from_midnight();
  if ((current_order_bid_int_price_ > dimer_bid_int_price_) && (max_bid_offset_ >= 0)) {
    dbglogger_ << watch_.tv() << " " << sec_shortcode_ << " " << identifier_
               << " ERROR SENDBIDORDER CURRENT ORDER PRICE SENT " << current_order_bid_int_price_
               << " GREATER THAN DIMER PRICE " << dimer_bid_int_price_ << DBGLOG_ENDL_FLUSH;
  }

  if ((current_order_bid_int_price_ > secondary_smv_->upper_int_price_limit_) ||
      (current_order_bid_int_price_ < secondary_smv_->lower_int_price_limit_)) {
    if (bid_order_) {
      CancelBid();
    }
    return;
  }

  if (bid_order_) {
    // either the order is partial exec; order still exists
    if (bid_order_->int_price_ <= dimer_bid_int_price_) {
      if ((bid_order_->int_price_ < current_order_bid_int_price_ &&
           current_time - last_modify_bid_order_time_ < drag_tighten_delay_) ||
          (bid_order_->int_price_ > current_order_bid_int_price_ &&
           current_time - last_modify_bid_order_time_ < drag_widen_delay_)) {
        // Order Tighten/Widen case
        return;
      }
    }
    if (theo_values_.last_traded_int_price_ != 0 && !passive_reduce_ && !eff_squareoff_ &&
        !disable_one_percent_order_limit_) {
      if (current_order_bid_int_price_ > price_upper_limit_ || current_order_bid_int_price_ < price_lower_limit_) {
        bid_order_at_maxoffset_ = false;
        if (dimer_bid_int_price_ < price_lower_limit_) {
          // Cancel bid order. will have to sit out
          /*dbglogger_ << watch_.tv() << " " << sec_shortcode_
                  << " cancelling bid " << dimer_bid_int_price_
                  << " ltp " << theo_values_.last_traded_int_price_ << DBGLOG_ENDL_FLUSH;*/
          CancelBid();
          return;
        } else if (current_order_bid_int_price_ > price_upper_limit_) {
          current_order_bid_int_price_ = price_upper_limit_;
        } else {
          CancelBid();
        }
      }
    }

    if (bid_order_->int_price_ != current_order_bid_int_price_ && bid_order_->canceled_ == false &&
        bid_order_->order_status_ == HFSAT::kORRType_Conf) {
      double current_order_bid_price_ = current_order_bid_int_price_ * tick_size_;
      bool is_priority_ = ((bid_order_->int_price_ - current_order_bid_int_price_) > 0);
      if(is_priority_ && disallow_priority_modify_) {
        if(bid_order_->int_price_ > dimer_bid_int_price_) {
          CancelBid();
        }
        return;
      }
      bool is_modified_ =
          basic_om_->Modify(bid_order_, current_order_bid_price_, current_order_bid_int_price_, dimer_bid_size_,
                            ((use_reserve_msg_) && (is_priority_)));
      if (is_modified_) {
        /*dbglogger_ << watch_.tv() << " "
               << sec_shortcode_ << " " << identifier_
               << "SEND MODIFYORDER BUY " << current_order_bid_price_
               << " size: " << dimer_bid_size_ << " intpx: "
               << current_order_bid_int_price_ << " theo["
               << theo_values_.theo_bid_price_ << " X " << theo_values_.theo_ask_price_ << "]"
               << " mkt[" << secondary_smv_->market_update_info().bestbid_price_
               << " X " << secondary_smv_->market_update_info().bestask_price_ << "]"
               << " dimerintpx " << dimer_bid_int_price_ << DBGLOG_ENDL_FLUSH;*/
        previous_bid_price_ = current_order_bid_price_;
        previous_bid_int_price_ = current_order_bid_int_price_;
        last_modify_bid_order_time_ = current_time;
        order_count_++;
        num_modify_++;
        if(is_priority_) num_priority_modify_++;
      }
    }
  } else {
    // First order of the day
    if (current_time - last_new_bid_order_time_ < delay_between_orders_) return;
    if (theo_values_.last_traded_int_price_ != 0 && !passive_reduce_ && !eff_squareoff_ &&
        !disable_one_percent_order_limit_ &&
        (current_order_bid_int_price_ > price_upper_limit_ || current_order_bid_int_price_ < price_lower_limit_))
      return;
    double current_order_bid_price_ = current_order_bid_int_price_ * tick_size_;
    bid_order_ = basic_om_->SendTrade(current_order_bid_price_, current_order_bid_int_price_, dimer_bid_size_,
                                      HFSAT::kTradeTypeBuy, 'B', HFSAT::kOrderDay, unique_exec_id_, 1, false);

    if (bid_order_) {
#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
     dbglogger_.LogQueryOrder(order_log_buffer_,sec_shortcode_.c_str(), identifier_.c_str(), watch_.tv(), HFSAT::kTradeTypeSell, false, false, false, false,
					current_order_bid_price_, dimer_bid_size_, theo_values_.theo_bid_price_, theo_values_.theo_ask_price_,
					secondary_smv_->market_update_info().bestbid_price_, secondary_smv_->market_update_info().bestask_price_, 
					-1, -1, reference_primary_bid_, reference_primary_ask_, secondary_smv_->market_update_info().bestask_size_,
					bid_order_->client_assigned_order_sequence_, unique_exec_id_, -1, current_order_bid_int_price_, dimer_bid_int_price_, true); 
#endif
      order_count_++;
      previous_bid_price_ = current_order_bid_price_;
      previous_bid_int_price_ = current_order_bid_int_price_;
      last_new_bid_order_time_ = current_time;
    }
  }
}

void Dimer::UpdateAskOrders() {
  if ((!ask_status_) || (status_mask_ != BIT_SET_ALL)) {
    CancelAsk();
    return;
  }
  if (dimer_bid_price_ <= 0 || dimer_ask_price_ <= 0) return;

  if (ask_order_ && ((ask_order_->canceled_) || ((ask_order_->modified_) && (!is_modify_before_confirmation_)))) {
    // Cant do anything with the order till order modify sent
    return;
  }

  // DBGLOG_TIME_CLASS_FUNC << "UpdateAsk bidpx" << dimer_bid_price_ << " askpx " << dimer_ask_price_ <<
  // DBGLOG_ENDL_FLUSH;

  bool sending_order_to_maxoffset_ = false;
  int current_order_ask_int_price_ = dimer_ask_int_price_;

  int level_count = 0;
  int level_dimer_price_ = 0;
  int price_int_diff_ = 0;
  bool already_at_good_price_ = false;
  bool end_of_book_reached_ = false;

  if (theo_values_.is_big_trade_ == 1) {
    if (ask_order_) {
      current_order_ask_int_price_ =
          (dimer_ask_int_price_ > ask_order_->int_price_) ? dimer_ask_int_price_ : ask_order_->int_price_;
      // dbglogger_ << watch_.tv() << " OnBigTrade Sending ask order to " << current_order_ask_int_price_ << " currpx: "
      // << ask_order_->int_price_ << DBGLOG_ENDL_FLUSH;
      SendAskOrder(current_order_ask_int_price_);
    }
    return;
  } else if (theo_values_.is_big_trade_ == -1) {
    return;
  }

  for (level_count = 0; level_count < max_depth_; level_count++) {
    HFSAT::MarketUpdateInfoLevelStruct* ask_info_ = secondary_smv_->ask_info(level_count);

    if (ask_info_ == NULL) {
      end_of_book_reached_ = true;
      break;
    }

    int t_ask_int_price_ = ask_info_->limit_int_price_;
    // Violating dimer price
    if (t_ask_int_price_ < dimer_ask_int_price_) {
      level_dimer_price_++;
      continue;
    }

    int t_size_at_level = ask_info_->limit_size_;

    if (ask_order_ && (ask_order_->int_price_ == t_ask_int_price_) &&
        (t_size_at_level - ask_order_->size_remaining_*livetrading_ >= min_ask_dime_size_)) {
      // do NOT modify this order; already at good price
      already_at_good_price_ = true;
      break;
    } else if (t_size_at_level >= min_ask_dime_size_ &&
               (!ask_order_ || (ask_order_ && ask_order_->int_price_ != t_ask_int_price_))) {
      price_int_diff_ = t_ask_int_price_ - dimer_ask_int_price_;
      break;
    }
  }

  // At this point invariants are:
  // i) price_int_diff : Difference between dimer price and order price
  // ii) level_count is number of valid levels seen till now
  if (already_at_good_price_) {
    current_order_ask_int_price_ = ask_order_->int_price_;
  } else if ((level_count >= max_depth_) || (end_of_book_reached_)) {
    // Hit MaxDepth, go to max offset, no place to dime
    current_order_ask_int_price_ = dimer_ask_int_price_ + max_ask_offset_int_;
    if(ask_order_ && (ask_order_->int_price_ >= current_order_ask_int_price_)) {
      current_order_ask_int_price_ = ask_order_->int_price_;
    } 
    sending_order_to_maxoffset_ = true;
    // DBGLOG_TIME_CLASS_FUNC << " Hit MAXDEPTH " << DBGLOG_ENDL_FLUSH;
  } else {
    if (price_int_diff_ == 0) {
      // Dime Size present at theo price itself.
      // So cant find a place to dime going to max offset
      current_order_ask_int_price_ = dimer_ask_int_price_ + max_ask_offset_int_;
      sending_order_to_maxoffset_ = true;
      // DBGLOG_TIME_CLASS_FUNC << "DIME SIZE AT TOP LEVEL" << DBGLOG_ENDL_FLUSH;
    } else {
      // Need to traverse sizes from current level -> top_level to find join size
      // This traversal is simple is join size > 0 but join size 0
      // should be handled differently
      int prev_level_int_price_ = secondary_smv_->ask_int_price(level_count);
      int t_level_ = 0;
      if (dime_two_ticks_ && ask_order_ && ask_order_->int_price_ >= dimer_ask_int_price_ &&
          prev_level_int_price_ - ask_order_->int_price_ <= 2) {
        current_order_ask_int_price_ = ask_order_->int_price_;
      } else {
        for (t_level_ = level_count - 1; t_level_ >= level_dimer_price_; t_level_--) {
          HFSAT::MarketUpdateInfoLevelStruct* ask_info_ = secondary_smv_->ask_info(t_level_);
          int cur_level_int_price_ = ask_info_->limit_int_price_;

          if ((prev_level_int_price_ - cur_level_int_price_) > 1) {
            current_order_ask_int_price_ = prev_level_int_price_ - 1;
            break;
          } else {
            if (ask_order_ && ask_order_->int_price_ == cur_level_int_price_) {
              current_order_ask_int_price_ = ask_order_->int_price_;
              break;
            } else {
              int size_at_level = ask_info_->limit_size_;
              if (size_at_level <= max_ask_join_size_) {
                current_order_ask_int_price_ = cur_level_int_price_;
                break;
              }
            }
          }
          prev_level_int_price_ = cur_level_int_price_;
        }
      }

      if (t_level_ < level_dimer_price_) {
        int t_price_int_diff_ = prev_level_int_price_ - dimer_ask_int_price_;
        if (dime_two_ticks_ && (!ask_order_ || ask_order_->int_price_ > prev_level_int_price_) &&
            prev_level_int_price_ - 2 >= dimer_ask_int_price_) {
          current_order_ask_int_price_ = prev_level_int_price_ - 2;
        } else if (dime_two_ticks_ && ask_order_ && ask_order_->int_price_ >= dimer_ask_int_price_ &&
                   prev_level_int_price_ - ask_order_->int_price_ <= 2) {
          current_order_ask_int_price_ = ask_order_->int_price_;
        } else if (t_price_int_diff_ > 0) {
          current_order_ask_int_price_ = prev_level_int_price_ - 1;
        } else {
          // Did not find a place to join
          // go to max offset
          current_order_ask_int_price_ = dimer_ask_int_price_ + max_ask_offset_int_;
          sending_order_to_maxoffset_ = true;
          // DBGLOG_TIME_CLASS_FUNC << "NO LEVEL FOUND TO JOIN" << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  if (!allow_orders_beyond_maxoffset_ && current_order_ask_int_price_ > dimer_ask_int_price_ + max_ask_offset_int_) {
    // DBGLOG_TIME_CLASS_FUNC << " In case when current order greater than max offset" << DBGLOG_ENDL_FLUSH;
    current_order_ask_int_price_ = dimer_ask_int_price_ + max_ask_offset_int_;
    sending_order_to_maxoffset_ = true;
  }

  // Sanity checks once above logic is done
  // i) Do NOT dime your own order for now (till we are able to get size ahead)
  // ii) If current and previous order at max offset; dont modify until theo change > update%
  // iii) drag tighten/widen logic
  //

  // DBGLOG_TIME_CLASS_FUNC << " sending to maxoff " << sending_order_to_maxoffset_ << " already at max " <<
  // ask_order_at_maxoffset_ << DBGLOG_ENDL_FLUSH;
  // MaxOffset Update logic for ordering moderation
  if (sending_order_to_maxoffset_ && ask_order_at_maxoffset_ && ask_order_) {
    // previous order was also at max offset
    if (ask_order_->int_price_ < dimer_ask_int_price_ ||
        current_order_ask_int_price_ - ask_order_->int_price_ >= max_ask_offset_update_int_ ||
        ask_order_->int_price_ - current_order_ask_int_price_ >= max_ask_offset_update_int_) {
      // modify order to current_order_ask_int_price
    } else {
      // Do NOT modify order as price within update range
      // DBGLOG_TIME_CLASS_FUNC << "ORDER WITHIN UPDATE, NOT UPDATING ORDER " << DBGLOG_ENDL_FLUSH;
      current_order_ask_int_price_ = ask_order_->int_price_;
    }
  }

  // DBGLOG_TIME_CLASS_FUNC << "current final px " << current_order_ask_int_price_ << DBGLOG_ENDL_FLUSH;
  // SendOrder at current_order_ask_int_price_
  if (sending_order_to_maxoffset_) {
    sending_order_to_maxoffset_ = false;
    if((unselective_if_maxoffset_) || (!ask_order_) || (ask_order_->int_price_ < dimer_ask_int_price_)) {
      if(cancel_on_maxoffset_) {
        CancelAsk();
      } else {
        SendAskOrder(current_order_ask_int_price_);
        sending_order_to_maxoffset_ = true;
      }
    } else {
      current_order_ask_int_price_ = ask_order_->int_price_;
    }
  } else {
    SendAskOrder(current_order_ask_int_price_);
  }

  if (!sending_order_to_maxoffset_) {
    ask_order_at_maxoffset_ = false;
  } else {
    ask_order_at_maxoffset_ = true;
  }
}

void Dimer::SendAskOrder(int current_order_ask_int_price_) {
  if (current_order_ask_int_price_ <= 0) return;
  int current_time = watch_.msecs_from_midnight();
  if ((current_order_ask_int_price_ < dimer_ask_int_price_) && (max_ask_offset_ >= 0)) {
    dbglogger_ << watch_.tv() << " " << sec_shortcode_ << " " << identifier_
               << " ERROR SENDASKORDER CURRENT ORDER PRICE SENT " << current_order_ask_int_price_
               << " LESS THAN DIMER PRICE " << dimer_ask_int_price_ << DBGLOG_ENDL_FLUSH;
  }

  if ((current_order_ask_int_price_ > secondary_smv_->upper_int_price_limit_) ||
      (current_order_ask_int_price_ < secondary_smv_->lower_int_price_limit_)) {
    if (ask_order_) {
      CancelAsk();
    }
    return;
  }

  if (ask_order_) {
    if (ask_order_->int_price_ >= dimer_ask_int_price_) {
      if ((ask_order_->int_price_ > current_order_ask_int_price_ &&
           current_time - last_modify_ask_order_time_ < drag_tighten_delay_) ||
          (ask_order_->int_price_ < current_order_ask_int_price_ &&
           current_time - last_modify_ask_order_time_ < drag_widen_delay_)) {
        // Order Tighten/Widen case
        return;
      }
    }
    if (theo_values_.last_traded_int_price_ != 0 && !passive_reduce_ && !eff_squareoff_ &&
        !disable_one_percent_order_limit_) {
      if (current_order_ask_int_price_ > price_upper_limit_ || current_order_ask_int_price_ < price_lower_limit_) {
        ask_order_at_maxoffset_ = false;
        if (dimer_ask_int_price_ > price_upper_limit_) {
          // Cancel ask order. will have to sit out
          /*dbglogger_ << watch_.tv() << " " << sec_shortcode_
                  << " cancelling ask " << dimer_ask_int_price_
                  << " ltp " << theo_values_.last_traded_int_price_ << DBGLOG_ENDL_FLUSH;*/
          CancelAsk();
          return;
        } else if (current_order_ask_int_price_ < price_lower_limit_) {
          current_order_ask_int_price_ = price_lower_limit_;
        } else {
          CancelAsk();
        }
      }
    }
    // either the order is partial exec; order still exists
    if (ask_order_->int_price_ != current_order_ask_int_price_ && ask_order_->canceled_ == false &&
        ask_order_->order_status_ == HFSAT::kORRType_Conf) {
      double current_order_ask_price_ = current_order_ask_int_price_ * tick_size_;
      bool is_priority_ = ((ask_order_->int_price_ - current_order_ask_int_price_) < 0);
      if(is_priority_ && disallow_priority_modify_) {
        if(ask_order_->int_price_ < dimer_ask_int_price_) {
          CancelAsk();
        }
        return;
      }
      bool is_modified_ =
          basic_om_->Modify(ask_order_, current_order_ask_price_, current_order_ask_int_price_, dimer_ask_size_,
                            ((use_reserve_msg_) && (is_priority_)));
      if (is_modified_) {
        /*dbglogger_ << watch_.tv() << " "
                << sec_shortcode_ << " " << identifier_
                << "SEND MODIFYORDER SELL " << current_order_ask_price_
                << " size: " << dimer_ask_size_ << " intpx: "
                << current_order_ask_int_price_ << " theo["
                << theo_values_.theo_bid_price_ << " X " << theo_values_.theo_ask_price_ << "]"
                << " mkt[" << secondary_smv_->market_update_info().bestbid_price_
                << " X " << secondary_smv_->market_update_info().bestask_price_ << "]"
                << " dimerintpx " << dimer_ask_int_price_ << DBGLOG_ENDL_FLUSH;*/
        previous_ask_price_ = current_order_ask_price_;
        previous_ask_int_price_ = current_order_ask_int_price_;
        last_modify_ask_order_time_ = current_time;
        order_count_++;
        num_modify_++;
        if(is_priority_) num_priority_modify_++;
      }
    }
  } else {
    // First order of the day
    if (current_time - last_new_ask_order_time_ < delay_between_orders_) return;
    if (theo_values_.last_traded_int_price_ != 0 && !passive_reduce_ && !eff_squareoff_ &&
        !disable_one_percent_order_limit_ &&
        (current_order_ask_int_price_ > price_upper_limit_ || current_order_ask_int_price_ < price_lower_limit_))
      return;
    double current_order_ask_price_ = current_order_ask_int_price_ * tick_size_;
    ask_order_ = basic_om_->SendTrade(current_order_ask_price_, current_order_ask_int_price_, dimer_ask_size_,
                                      HFSAT::kTradeTypeSell, 'B', HFSAT::kOrderDay, unique_exec_id_, 1, false);

    if (ask_order_) {
#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
dbglogger_.LogQueryOrder(order_log_buffer_,sec_shortcode_.c_str(), identifier_.c_str(), watch_.tv(), HFSAT::kTradeTypeSell, false, false, false, false,
                                        current_order_ask_price_, dimer_ask_size_, theo_values_.theo_bid_price_, theo_values_.theo_ask_price_,
                                        secondary_smv_->market_update_info().bestbid_price_, secondary_smv_->market_update_info().bestask_price_,
                                        -1, -1, reference_primary_bid_, reference_primary_ask_, secondary_smv_->market_update_info().bestbid_size_,
                                        ask_order_->client_assigned_order_sequence_, unique_exec_id_, -1, current_order_ask_int_price_, dimer_ask_int_price_, true);

#endif
      previous_ask_price_ = current_order_ask_price_;
      previous_ask_int_price_ = current_order_ask_int_price_;
      last_new_ask_order_time_ = current_time;
      order_count_++;
    }
  }
}
