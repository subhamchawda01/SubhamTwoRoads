#include "tradeengine/Executioner/ModifyEye.hpp"
#include "tradeengine/Utils/Parser.hpp"

ModifyEye::ModifyEye(std::string _exec_param_file, HFSAT::Watch& _watch_, HFSAT::DebugLogger& dbglogger_,
                     HFSAT::SecurityMarketView* _secondary_smv, HFSAT::BasicOrderManager* _basic_om, bool _livetrading_,
                     bool _is_modify_before_confirmation_, bool _is_cancellable_before_confirmation_,
                     TheoValues& theo_values)
    : BaseExecutioner(_exec_param_file, _watch_, dbglogger_, _secondary_smv, _basic_om, _livetrading_,
                      _is_modify_before_confirmation_, _is_cancellable_before_confirmation_, theo_values),
      bid_percentage_offset_(1),
      ask_percentage_offset_(1),
      bid_offset_(0),
      ask_offset_(0),
      quoter_bid_size_(0),
      quoter_ask_size_(0),
      bid_overruled_(false),
      ask_overruled_(false),
      overruled_leaway_percent_(0.1),
      overruled_leaway_offset_(0),
      delay_between_orders_(0),
      eye_bid_price_(-1),
      eye_ask_price_(-1),
      quoter_bid_price_(-1),
      quoter_ask_price_(-1),
      bid_order_(NULL),
      ask_order_(NULL),
      bid_tighten_update_int_offset_(0),
      bid_widen_update_int_offset_(0),
      ask_tighten_update_int_offset_(0),
      ask_widen_update_int_offset_(0),
      quoter_bid_order_offset_(0),
      quoter_ask_order_offset_(0),
      last_new_bid_order_time_(0),
      last_new_ask_order_time_(0),
      last_modify_bid_order_time_(0),
      last_modify_ask_order_time_(0) {
  LoadParams();
  bid_status_ = true;
  ask_status_ = true;
  strcpy(order_log_buffer_.buffer_data_.query_order_.identifier_, identifier_.c_str());
}

void ModifyEye::LoadParams() {
  identifier_ = Parser::GetString(exec_key_val_map_, "IDENTIFIER", "");
  bool status_ = Parser::GetBool(exec_key_val_map_, "STATUS", false);
  status_mask_ = (status_ == true ? (status_mask_ | CONFIG_STATUS_SET) : (status_mask_ & CONFIG_STATUS_UNSET));
  bid_percentage_offset_ = Parser::GetDouble(exec_key_val_map_, "BID_PERCENTAGE_OFFSET", 1) / 100;
  ask_percentage_offset_ = Parser::GetDouble(exec_key_val_map_, "ASK_PERCENTAGE_OFFSET", 1) / 100;
  int shoot_size = Parser::GetInt(exec_key_val_map_, "SHOOT_SIZE", 0, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);

//  DBGLOG_CLASS_FUNC_LINE_INFO << "EYESHOOT :" << secondary_smv_->shortcode() << " SHOOTSIZE " << shoot_size << DBGLOG_ENDL_FLUSH ;

  order_limit_ = Parser::GetInt(exec_key_val_map_, "SHOOT_LIMIT", 0);
  bad_ratio_limit_ = Parser::GetInt(exec_key_val_map_, "BAD_RATIO_LIMIT", 1000000);
  bid_overruled_ = Parser::GetBool(exec_key_val_map_, "BID_OVERRULED", false);
  ask_overruled_ = Parser::GetBool(exec_key_val_map_, "ASK_OVERRULED", false);
  overruled_leaway_percent_ = Parser::GetDouble(exec_key_val_map_, "OVERRULED_LEAWAY_PERCENT", 0.1) / 100;
  delay_between_orders_ = Parser::GetInt(exec_key_val_map_, "DELAY_BETWEEN_ORDERS", 0);
  shoot_only_on_sweep_ = Parser::GetBool(exec_key_val_map_, "SHOOT_ONLY_ON_SWEEP", false);
  bid_tighten_update_percent_ = Parser::GetDouble(exec_key_val_map_, "BID_TIGHTEN_UPDATE_PERCENT", 0) / 100;
  bid_widen_update_percent_ = Parser::GetDouble(exec_key_val_map_, "BID_WIDEN_UPDATE_PERCENT", 0) / 100;
  ask_tighten_update_percent_ = Parser::GetDouble(exec_key_val_map_, "ASK_TIGHTEN_UPDATE_PERCENT", 0) / 100;
  ask_widen_update_percent_ = Parser::GetDouble(exec_key_val_map_, "ASK_WIDEN_UPDATE_PERCENT", 0) / 100;
  quoter_bid_order_percent_ = Parser::GetDouble(exec_key_val_map_, "QUOTER_BID_ORDER_PERCENT", 0) / 100;
  quoter_ask_order_precent_ = Parser::GetDouble(exec_key_val_map_, "QUOTER_ASK_ORDER_PERCENT", 0) / 100;
  use_reserve_msg_ = Parser::GetBool(exec_key_val_map_, "USE_RESERVE_MSG", false);
  bid_size_ = std::max(
      HFSAT::MathUtils::GetFlooredMultipleOf(shoot_size * size_multiplier_ + 0.001, secondary_smv_->min_order_size_),
      secondary_smv_->min_order_size_);
  ask_size_ = std::max(
      HFSAT::MathUtils::GetFlooredMultipleOf(shoot_size * size_multiplier_ + 0.001, secondary_smv_->min_order_size_),
      secondary_smv_->min_order_size_);
}

void ModifyEye::OnReady(const HFSAT::MarketUpdateInfo& _market_update_info_) {
  bid_offset_ = bid_percentage_offset_ * _market_update_info_.bestbid_price_;
  ask_offset_ = ask_percentage_offset_ * _market_update_info_.bestbid_price_;

  quoter_bid_order_offset_ = quoter_bid_order_percent_ * _market_update_info_.bestbid_price_;
  quoter_ask_order_offset_ = quoter_ask_order_precent_ * _market_update_info_.bestbid_price_;

  bid_tighten_update_int_offset_ = bid_tighten_update_percent_ * _market_update_info_.bestbid_int_price_;
  bid_widen_update_int_offset_ = -1 * bid_widen_update_percent_ * _market_update_info_.bestbid_int_price_;
  ask_tighten_update_int_offset_ = ask_tighten_update_percent_ * _market_update_info_.bestbid_int_price_;
  ask_widen_update_int_offset_ = -1 * ask_widen_update_percent_ * _market_update_info_.bestbid_int_price_;

  dbglogger_ << watch_.tv() << " MODIFYEYE Offsets: " << bid_offset_ << "|" << ask_offset_ << "|"
             << bid_tighten_update_int_offset_ << "|" << bid_widen_update_int_offset_ << "|"
             << ask_tighten_update_int_offset_ << "|" << ask_widen_update_int_offset_ << "\n";

  overruled_leaway_offset_ = overruled_leaway_percent_ * _market_update_info_.bestbid_price_;
}

void ModifyEye::OnTheoUpdate() {
  quoter_bid_price_ = theo_values_.theo_bid_price_ - quoter_bid_order_offset_;
  quoter_bid_int_price_ = int(quoter_bid_price_ * inverse_tick_size_ + DOUBLE_PRECISION);
  quoter_bid_price_ = quoter_bid_int_price_ * tick_size_;

  quoter_ask_price_ = theo_values_.theo_ask_price_ + quoter_ask_order_offset_;
  quoter_ask_int_price_ = int(std::ceil(quoter_ask_price_ * inverse_tick_size_ - DOUBLE_PRECISION));
  quoter_ask_price_ = quoter_ask_int_price_ * tick_size_;

  eye_bid_price_ = theo_values_.theo_bid_price_ - bid_offset_;
  eye_ask_price_ = theo_values_.theo_ask_price_ + ask_offset_;
  // DBGLOG_TIME_CLASS_FUNC << identifier_ << "theopx "<< quoter_bid_price_ << " X " << quoter_ask_price_ << "  " <<
  // theo_position << DBGLOG_ENDL_FLUSH;
  UpdateOrders();
}

void ModifyEye::UpdateOrders() {
  if (aggressive_reduce_ || eff_squareoff_) {
    // To avoid cases like DRREDDY on 20171201 where we are constantly trying to close
    // positions aggressively and caught in a loop when position to close is exactly
    // half of min order size
    int target_pos_to_close_ =
        HFSAT::MathUtils::RoundOff((-1 * theo_values_.position_to_offset_) + 0.001, secondary_smv_->min_order_size_);
    if (target_pos_to_close_ > 0) {
      EnableBid();
      DisableAsk();
      quoter_bid_size_ = std::min(bid_size_, target_pos_to_close_);
      enable_ask_on_reload_ = true;
    } else if (target_pos_to_close_ < 0) {
      EnableAsk();
      DisableBid();
      quoter_ask_size_ = std::min(ask_size_, -1 * target_pos_to_close_);
      enable_bid_on_reload_ = true;
    } else {
      DisableBid();
      DisableAsk();
      enable_bid_on_reload_ = true;
      enable_ask_on_reload_ = true;
      return;
    }
  } else {
    quoter_bid_size_ = bid_size_;
    quoter_ask_size_ = ask_size_;
  }

  if (theo_values_.movement_indicator_ == pUPWARD) {
    UpdateBidOrders();
    UpdateAskOrders();
  } else {
    UpdateAskOrders();
    UpdateBidOrders();
  }

  if (order_count_ >= order_limit_) {
    if (status_mask_ & SHOOT_STATUS_SET) {
      DBGLOG_TIME_CLASS_FUNC << "SHOOT COUNT HIT LIMIT: " << order_limit_ << " DISABLING EYE " << DBGLOG_ENDL_FLUSH;
    }
    TurnOff(SHOOT_STATUS_UNSET);
  }
}

void ModifyEye::UpdateBidOrders() {
  if (!theo_values_.is_valid_ || (!bid_status_) || (status_mask_ != BIT_SET_ALL)) {
    CancelBid();
    return;
  }
  if (quoter_bid_price_ <= 0 || quoter_ask_price_ <= 0) return;

  if (theo_values_.is_big_trade_ == -1) return;
  int current_time = watch_.msecs_from_midnight();
  eye_bid_int_price_ = int(eye_bid_price_ * inverse_tick_size_ + DOUBLE_PRECISION);

  if (bid_order_) {
    if (bid_order_->modified_ == true || bid_order_->canceled_ == true ||
        bid_order_->order_status_ != HFSAT::kORRType_Conf) {
      return;
    }

    if ((!(shoot_only_on_sweep_) || theo_values_.sweep_mode_active_ > 0) &&
        (bid_order_->int_price_ != eye_bid_int_price_) &&
        (eye_bid_price_ > 0 && eye_bid_int_price_ >= secondary_smv_->market_update_info().bestask_int_price_)) {
      int current_time = watch_.msecs_from_midnight();
      if (eye_bid_int_price_ > secondary_smv_->upper_int_price_limit_) {
        eye_bid_int_price_ = secondary_smv_->upper_int_price_limit_;
      }
      eye_bid_price_ = eye_bid_int_price_ * tick_size_;
      bool can_modify_ = basic_om_->Modify(bid_order_, eye_bid_price_, eye_bid_int_price_, bid_size_, true);
      last_modify_bid_order_time_ = current_time;
      if (can_modify_) {
        if (livetrading_) {
          order_count_++;
        }
#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
	dbglogger_.LogQueryOrder(order_log_buffer_, sec_shortcode_.c_str(), identifier_.c_str(), watch_.tv(), HFSAT::kTradeTypeSell, true, false, true, false
						eye_bid_price_, bid_size_, eye_bid_price_, eye_ask_price_,
						secondary_smv_->market_update_info().bestbid_price_,
						secondary_smv_->market_update_info().bestask_price_, 
						theo_values_.primary_best_bid_price_, theo_values_.primary_best_ask_price_,
						theo_values_.reference_primary_bid_, theo_values_.reference_primary_ask_, secondary_smv_->market_update_info().bestask_size_,
						bid_order_->client_assigned_order_sequence_, unique_exec_id_, bid_order_->price_);
#endif
      }
    } else {  // Modify Quoter order if possible
      if ((quoter_bid_int_price_ > secondary_smv_->upper_int_price_limit_) ||
          (quoter_bid_int_price_ < secondary_smv_->lower_int_price_limit_)) {
        CancelBid();
        return;
      }
      int price_int_diff_ = quoter_bid_int_price_ - bid_order_->int_price_;
      if ((price_int_diff_ != 0) &&
          (((price_int_diff_ > 0) ? ((double)price_int_diff_ >= bid_tighten_update_int_offset_)
                                  : (price_int_diff_ <= bid_widen_update_int_offset_)) ||
           (theo_values_.theo_bid_price_ + DOUBLE_PRECISION < bid_order_->price_))) {
        basic_om_->Modify(bid_order_, quoter_bid_price_, quoter_bid_int_price_, quoter_bid_size_, false);
        last_modify_bid_order_time_ = current_time;
        order_count_++;
        /*dbglogger_ << watch_.tv() << " "
          << sec_shortcode_ << " " << identifier_
          << "SEND MODIFYORDER BUY " << quoter_bid_price_
          << " size: " << quoter_bid_size_ << " intpx: "
          << quoter_bid_int_price_ << " theo["
          << theo_values_.theo_bid_price_ << " X " << theo_values_.theo_ask_price_ << "]"
          << " mkt[" << secondary_smv_->market_update_info().bestbid_price_
          << " X " << secondary_smv_->market_update_info().bestask_price_ << "]"
          << DBGLOG_ENDL_FLUSH;*/
      }
    }
  } else {
    if (current_time - last_new_bid_order_time_ < delay_between_orders_) return;
    if ((!(shoot_only_on_sweep_) || theo_values_.sweep_mode_active_ > 0) && eye_bid_price_ > 0 &&
        eye_bid_int_price_ >= secondary_smv_->market_update_info().bestask_int_price_) {
      if (eye_bid_int_price_ > secondary_smv_->upper_int_price_limit_) {
        eye_bid_int_price_ = secondary_smv_->upper_int_price_limit_;
      }
      eye_bid_price_ = eye_bid_int_price_ * tick_size_;
      bid_order_ = basic_om_->SendTrade(eye_bid_price_, eye_bid_int_price_, bid_size_, HFSAT::kTradeTypeBuy, 'B',
                                        HFSAT::kOrderIOC, unique_exec_id_, 1, false);
      if (bid_order_) {
#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
	dbglogger_.LogQueryOrder(order_log_buffer_, sec_shortcode_.c_str(), identifier_.c_str(), watch_.tv(), HFSAT::kTradeTypeSell, true, false, false, true
                                                eye_bid_price_, bid_size_, eye_bid_price_, eye_ask_price_,
                                                secondary_smv_->market_update_info().bestbid_price_,
                                                secondary_smv_->market_update_info().bestask_price_, 
                                                theo_values_.primary_best_bid_price_, theo_values_.primary_best_ask_price_,
                                                theo_values_.reference_primary_bid_, theo_values_.reference_primary_ask_, secondary_smv_->market_update_info().bestask_size_,
                                                bid_order_->client_assigned_order_sequence_, unique_exec_id_, bid_order_->price_);
#endif
        if (livetrading_) {
          order_count_++;
        }
        last_new_bid_order_time_ = current_time;
      }
    } else {
      if ((quoter_bid_int_price_ > secondary_smv_->upper_int_price_limit_) ||
          (quoter_bid_int_price_ < secondary_smv_->lower_int_price_limit_)) {
        return;
      }
      bid_order_ = basic_om_->SendTrade(quoter_bid_price_, quoter_bid_int_price_, quoter_bid_size_,
                                        HFSAT::kTradeTypeBuy, 'B', HFSAT::kOrderDay, unique_exec_id_, 1, false);
      if (bid_order_) {
#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
	dbglogger_.LogQueryOrder(order_log_buffer_, sec_shortcode_.c_str(), identifier_.c_str(), watch_.tv(), HFSAT::kTradeTypeSell, false, false, false, false
                                                quote_bid_price_, quote_bid_size_, theo_values_.theo_bid_price_, theo_values_.theo_ask_price_,
                                                secondary_smv_->market_update_info().bestbid_price_,
                                                secondary_smv_->market_update_info().bestask_price_, 
                                                theo_values_.primary_best_bid_price_, theo_values_.primary_best_ask_price_,
                                                reference_primary_bid_, reference_primary_ask_, secondary_smv_->market_update_info().bestask_size_,
                                                bid_order_->client_assigned_order_sequence_, unique_exec_id_, -1, quoter_bid_int_price_);
#endif
        last_new_bid_order_time_ = current_time;
        order_count_++;
      }
    }
  }
}

void ModifyEye::UpdateAskOrders() {
  if (!theo_values_.is_valid_ || (!ask_status_) || (status_mask_ != BIT_SET_ALL)) {
    CancelAsk();
    return;
  }
  if (quoter_bid_price_ <= 0 || quoter_ask_price_ <= 0) return;

  if (theo_values_.is_big_trade_ == 1) return;

  int current_time = watch_.msecs_from_midnight();
  eye_ask_int_price_ = int(ceil(eye_ask_price_ * inverse_tick_size_ - DOUBLE_PRECISION));

  if (ask_order_) {
    if (ask_order_->modified_ == true || ask_order_->canceled_ == true ||
        ask_order_->order_status_ != HFSAT::kORRType_Conf) {
      return;
    }

    if ((!(shoot_only_on_sweep_) || theo_values_.sweep_mode_active_ < 0) &&
        (ask_order_->int_price_ != eye_ask_int_price_) &&
        (eye_ask_price_ > 0 && eye_ask_int_price_ <= secondary_smv_->market_update_info().bestbid_int_price_)) {
      int current_time = watch_.msecs_from_midnight();
      if (eye_ask_int_price_ < secondary_smv_->lower_int_price_limit_) {
        eye_ask_int_price_ = secondary_smv_->lower_int_price_limit_;
      }
      eye_ask_price_ = eye_ask_int_price_ * tick_size_;
      bool can_modify_ = basic_om_->Modify(ask_order_, eye_ask_price_, eye_ask_int_price_, ask_size_, true);
      last_modify_ask_order_time_ = current_time;
      if (can_modify_) {
        if (livetrading_) {
          order_count_++;
        }
#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
	dbglogger_.LogQueryOrder(order_log_buffer_, sec_shortcode_.c_str(), identifier_.c_str(), watch_.tv(), HFSAT::kTradeTypeSell, true, false, true, false
						eye_ask_price_, ask_size_, eye_bid_price_, eye_ask_price_,
						secondary_smv_->market_update_info().bestbid_price_,
						secondary_smv_->market_update_info().bestask_price_, 
						theo_values_.primary_best_bid_price_, theo_values_.primary_best_ask_price_, 
						theo_values_.reference_primary_bid_, theo_values_.reference_primary_ask_, secondary_smv_->market_update_info().bestask_size_,
						ask_order_->client_assigned_order_sequence_, unique_exec_id_, ask_order_->price_);
#endif
      }
    } else {  // Modify Quoter order if possible
      if ((quoter_ask_int_price_ > secondary_smv_->upper_int_price_limit_) ||
          (quoter_ask_int_price_ < secondary_smv_->lower_int_price_limit_)) {
        CancelAsk();
        return;
      }
      int price_int_diff_ = ask_order_->int_price_ - quoter_ask_int_price_;
      if ((price_int_diff_ != 0) &&
          (((price_int_diff_ > 0) ? ((double)price_int_diff_ >= ask_tighten_update_int_offset_)
                                  : (price_int_diff_ <= ask_widen_update_int_offset_)) ||
           (theo_values_.theo_ask_price_ - DOUBLE_PRECISION > ask_order_->price_))) {
        basic_om_->Modify(ask_order_, quoter_ask_price_, quoter_ask_int_price_, quoter_ask_size_, false);
        last_modify_ask_order_time_ = current_time;
        order_count_++;
        /*dbglogger_ << watch_.tv() << " "
          << sec_shortcode_ << " " << identifier_
          << "SEND MODIFYORDER SELL " << quoter_ask_price_
          << " size: " << quoter_ask_size_ << " intpx: "
          << quoter_ask_int_price_ << " theo["
          << theo_values_.theo_bid_price_ << " X " << theo_values_.theo_ask_price_ << "]"
          << " mkt[" << secondary_smv_->market_update_info().bestbid_price_
          << " X " << secondary_smv_->market_update_info().bestask_price_ << "]"
          << DBGLOG_ENDL_FLUSH;*/
      }
    }
  } else {
    if (current_time - last_new_ask_order_time_ < delay_between_orders_) return;
    if ((!(shoot_only_on_sweep_) || theo_values_.sweep_mode_active_ < 0) && eye_ask_price_ > 0 &&
        eye_ask_int_price_ <= secondary_smv_->market_update_info().bestbid_int_price_) {
      if (eye_ask_int_price_ < secondary_smv_->lower_int_price_limit_) {
        eye_ask_int_price_ = secondary_smv_->lower_int_price_limit_;
      }
      eye_ask_price_ = eye_ask_int_price_ * tick_size_;
      ask_order_ = basic_om_->SendTrade(eye_ask_price_, eye_ask_int_price_, ask_size_, HFSAT::kTradeTypeSell, 'B',
                                        HFSAT::kOrderIOC, unique_exec_id_, 1, false);
      if (ask_order_) {
#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
	dbglogger_.LogQueryOrder(order_log_buffer_, sec_shortcode_.c_str(), identifier_.c_str(), watch_.tv(), HFSAT::kTradeTypeSell, true, false, false, true
						eye_ask_price_, ask_size_, eye_bid_price_, eye_ask_price_,
						secondary_smv_->market_update_info().bestbid_price_,
						secondary_smv_->market_update_info().bestask_price_, theo_values_.primary_best_bid_price_, theo_values_.primary_best_ask_price_,
						theo_values_.reference_primary_bid_, theo_values_.reference_primary_ask_, secondary_smv_->market_update_info().bestask_size_,
						ask_order_->client_assigned_order_sequence_, unique_exec_id_, ask_order_->price_);
#endif
        if (livetrading_) {
          order_count_++;
        }
        last_new_ask_order_time_ = current_time;
      }
    } else {
      if ((quoter_ask_int_price_ > secondary_smv_->upper_int_price_limit_) ||
          (quoter_ask_int_price_ < secondary_smv_->lower_int_price_limit_)) {
        return;
      }
      ask_order_ = basic_om_->SendTrade(quoter_ask_price_, quoter_ask_int_price_, quoter_ask_size_,
                                        HFSAT::kTradeTypeSell, 'B', HFSAT::kOrderDay, unique_exec_id_, 1, false);
      if (ask_order_) {
#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
	dbglogger_.LogQueryOrder(order_log_buffer_, watch_.tv(), HFSAT::kTradeTypeSell, false, false, false, false
						quoter_ask_price_, quoter_ask_size_, theo_values_.theo_bid_price_, theo_values_.theo_ask_price_,
						secondary_smv_->market_update_info().bestbid_price_,
						secondary_smv_->market_update_info().bestask_price_, -1, -1, reference_primary_bid_,
						reference_primary_ask_, secondary_smv_->market_update_info().bestbid_size_,
						ask_order_->client_assigned_order_sequence_, unique_exec_id_, -1, quoter_ask_int_price_);
#endif
        last_new_ask_order_time_ = current_time;
        order_count_++;
      }
    }
  }
}

void ModifyEye::DisableBid() {
  bid_status_ = false;
  enable_bid_on_reload_ = false;
  CancelBid();
}

void ModifyEye::DisableAsk() {
  ask_status_ = false;
  enable_ask_on_reload_ = false;
  CancelAsk();
}

void ModifyEye::CancelBid() {
  if (bid_order_ &&
      (bid_order_->order_status_ == HFSAT::kORRType_Exec || bid_order_->order_status_ == HFSAT::kORRType_Rejc ||
       bid_order_->order_status_ == HFSAT::kORRType_Cxld ||
       (bid_order_->is_ioc_ && bid_order_->order_status_ == HFSAT::kORRType_Conf))) {
    bid_order_ = NULL;
  }
  if (bid_order_ && bid_order_->order_status_ != HFSAT::kORRType_Cxld) basic_om_->Cancel(*bid_order_);
}

void ModifyEye::CancelAsk() {
  if (ask_order_ &&
      (ask_order_->order_status_ == HFSAT::kORRType_Exec || ask_order_->order_status_ == HFSAT::kORRType_Rejc ||
       ask_order_->order_status_ == HFSAT::kORRType_Cxld ||
       (ask_order_->is_ioc_ && ask_order_->order_status_ == HFSAT::kORRType_Conf))) {
    ask_order_ = NULL;
  }
  if (ask_order_ && ask_order_->order_status_ != HFSAT::kORRType_Cxld) basic_om_->Cancel(*ask_order_);
}

void ModifyEye::CheckOrderStatus(HFSAT::BaseOrder* _order_) {
  if (_order_ && bid_order_ &&
      _order_->client_assigned_order_sequence_ == bid_order_->client_assigned_order_sequence_) {
    if ((_order_->order_status_ == HFSAT::kORRType_Exec || _order_->order_status_ == HFSAT::kORRType_Rejc ||
         _order_->order_status_ == HFSAT::kORRType_Cxld ||
         (_order_->is_ioc_ && _order_->order_status_ == HFSAT::kORRType_Conf))) {
      bid_order_ = NULL;
    }
  } else if (_order_ && ask_order_ &&
             _order_->client_assigned_order_sequence_ == ask_order_->client_assigned_order_sequence_) {
    if ((_order_->order_status_ == HFSAT::kORRType_Exec || _order_->order_status_ == HFSAT::kORRType_Rejc ||
         _order_->order_status_ == HFSAT::kORRType_Cxld ||
         (_order_->is_ioc_ && _order_->order_status_ == HFSAT::kORRType_Conf))) {
      ask_order_ = NULL;
    }
  }
  // else{
  // 	dbglogger_ << watch_.tv() << " "
  // 	<< sec_shortcode_ << " " << identifier_
  // 	<<" Invalid order_update " << DBGLOG_ENDL_FLUSH;
  // }
}

void ModifyEye::OnOrderReject(HFSAT::BaseOrder* _order_) {
  // DBGLOG_TIME_CLASS_FUNC << " MODIFYEYE: OrderReject callback for " << secondary_smv_->shortcode() <<
  // DBGLOG_ENDL_FLUSH;
  CheckOrderStatus(_order_);
  // dbglogger_.DumpCurrentBuffer();
}

void ModifyEye::OnOrderChange(HFSAT::BaseOrder* _order_) {
  CheckOrderStatus(_order_);
  UpdateOrders();
  dbglogger_.DumpCurrentBuffer();
}
