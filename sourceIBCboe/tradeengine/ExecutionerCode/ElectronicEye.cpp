#include "tradeengine/Executioner/ElectronicEye.hpp"
#include "tradeengine/Utils/Parser.hpp"

ElectronicEye::ElectronicEye(std::string _exec_param_file, HFSAT::Watch& _watch_, HFSAT::DebugLogger& dbglogger_,
                             HFSAT::SecurityMarketView* _secondary_smv, HFSAT::BasicOrderManager* _basic_om,
                             bool _livetrading_, bool _is_modify_before_confirmation_,
                             bool _is_cancellable_before_confirmation_, TheoValues& theo_values)
    : BaseExecutioner(_exec_param_file, _watch_, dbglogger_, _secondary_smv, _basic_om, _livetrading_,
                      _is_modify_before_confirmation_, _is_cancellable_before_confirmation_, theo_values),
      bid_percentage_offset_(1),
      ask_percentage_offset_(1),
      bid_offset_(0),
      ask_offset_(0),
      eye_shoot_bid_size_(0),
      eye_shoot_ask_size_(0),
      bid_overruled_(false),
      ask_overruled_(false),
      overruled_leaway_percent_(0.1),
      overruled_leaway_offset_(0),
      delay_between_orders_(0),
      eye_bid_price_(-1),
      eye_ask_price_(-1),
      bid_order_(NULL),
      ask_order_(NULL),
      target_position_(0),
      mirror_factor_(1),
      last_new_bid_order_time_(0),
      last_new_ask_order_time_(0),
      max_bid_orders_to_shoot_(1),
      max_ask_orders_to_shoot_(1),
      min_price_move_percent_(1000),
      min_price_move_(0),
      shoot_only_on_primary_move_(false),
      num_outstanding_bid_orders_(0),
      num_outstanding_ask_orders_(0),
      prev_market_bid_price_(0),
      prev_market_ask_price_(0) {
  LoadParams();
  bid_status_ = true;
  ask_status_ = true;
  strcpy(order_log_buffer_.buffer_data_.query_order_.identifier_, identifier_.c_str());
}

void ElectronicEye::LoadParams() {
  identifier_ = Parser::GetString(exec_key_val_map_, "IDENTIFIER", "");
  bool status_ = Parser::GetBool(exec_key_val_map_, "STATUS", false);
  status_mask_ = (status_ == true ? (status_mask_ | CONFIG_STATUS_SET) : (status_mask_ & CONFIG_STATUS_UNSET));
  bid_percentage_offset_ = Parser::GetDouble(exec_key_val_map_, "BID_PERCENTAGE_OFFSET", 1) / 100;
  ask_percentage_offset_ = Parser::GetDouble(exec_key_val_map_, "ASK_PERCENTAGE_OFFSET", 1) / 100;
  int shoot_size = Parser::GetInt(exec_key_val_map_, "SHOOT_SIZE", 0, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);

  DBGLOG_CLASS_FUNC_LINE_INFO << "EYESHOOT :" << secondary_smv_->shortcode() << " SHOOTSIZE " << shoot_size << DBGLOG_ENDL_FLUSH ;

  order_limit_ = Parser::GetInt(exec_key_val_map_, "SHOOT_LIMIT", 0);
  bad_ratio_limit_ = Parser::GetInt(exec_key_val_map_, "BAD_RATIO_LIMIT", 1000000);
  bid_overruled_ = Parser::GetBool(exec_key_val_map_, "BID_OVERRULED", false);
  ask_overruled_ = Parser::GetBool(exec_key_val_map_, "ASK_OVERRULED", false);
  mirror_factor_ = Parser::GetInt(exec_key_val_map_, "MIRROR_FACTOR", 1);
  overruled_leaway_percent_ = Parser::GetDouble(exec_key_val_map_, "OVERRULED_LEAWAY_PERCENT", 0.1) / 100;
  delay_between_orders_ = Parser::GetInt(exec_key_val_map_, "DELAY_BETWEEN_ORDERS", 0);
  shoot_only_on_sweep_ = Parser::GetBool(exec_key_val_map_, "SHOOT_ONLY_ON_SWEEP", false);
  max_bid_orders_to_shoot_ = Parser::GetInt(exec_key_val_map_, "MAX_ORDERS_TO_SHOOT", 1);
  min_price_move_percent_ = Parser::GetDouble(exec_key_val_map_, "MIN_PRICE_MOVE_PERCENT", 1000) / 100;
  shoot_only_on_primary_move_ = Parser::GetBool(exec_key_val_map_, "SHOOT_ONLY_ON_PRIMARY_MOVE", false);
  use_reserve_msg_ = Parser::GetBool(exec_key_val_map_, "USE_RESERVE_MSG", false);
  if (shoot_size % secondary_smv_->min_order_size_ != 0) {
    dbglogger_ << watch_.tv() << " ERROR: " << secondary_smv_->shortcode() << " WRONG SIZE BS: " << shoot_size
               << " AS: " << shoot_size << " LS: " << secondary_smv_->min_order_size_ << " UNSETTING CONFIG STATUS "
               << DBGLOG_ENDL_FLUSH;
    status_mask_ = status_mask_ & CONFIG_STATUS_UNSET;
  }
  bid_size_ = std::max(
      HFSAT::MathUtils::GetFlooredMultipleOf(shoot_size * size_multiplier_ + 0.001, secondary_smv_->min_order_size_),
      secondary_smv_->min_order_size_);
  ask_size_ = std::max(
      HFSAT::MathUtils::GetFlooredMultipleOf(shoot_size * size_multiplier_ + 0.001, secondary_smv_->min_order_size_),
      secondary_smv_->min_order_size_);
  eye_shoot_bid_size_ = bid_size_;
  eye_shoot_ask_size_ = ask_size_;
  max_ask_orders_to_shoot_ = max_bid_orders_to_shoot_;
}

void ElectronicEye::OnReady(const HFSAT::MarketUpdateInfo& _market_update_info_) {
  bid_offset_ = bid_percentage_offset_ * _market_update_info_.bestbid_price_;
  ask_offset_ = ask_percentage_offset_ * _market_update_info_.bestbid_price_;
  overruled_leaway_offset_ = overruled_leaway_percent_ * _market_update_info_.bestbid_price_;
  min_price_move_ = min_price_move_percent_ * _market_update_info_.bestbid_price_;
}

void ElectronicEye::OnTheoUpdate() {
  eye_bid_price_ = theo_values_.theo_bid_price_ - bid_offset_;
  eye_ask_price_ = theo_values_.theo_ask_price_ + ask_offset_;
  target_position_ = -1 * theo_values_.position_to_offset_;

  // It will only place order if default shoot is on
  // or sweep mode is active
  if ((shoot_only_on_sweep_) && (theo_values_.sweep_mode_active_ == 0)) {
    return;
  }
  // dbglogger_ << watch_.tv() << " ontheoupdate " << theo_values_.primary_best_bid_price_ << " "
  // 	<< theo_values_.primary_best_ask_price_ << DBGLOG_ENDL_FLUSH;
  UpdateOrders();
}

void ElectronicEye::UpdateOrders() {
  if (eff_squareoff_) {
    DisableBid();
    DisableAsk();
    enable_bid_on_reload_ = true;
    enable_ask_on_reload_ = true;
    return;
  }
  if (aggressive_reduce_) {
    int target_pos_to_close_ = HFSAT::MathUtils::RoundOff(target_position_ + 0.001, secondary_smv_->min_order_size_);
    if (target_pos_to_close_ > 0) {
      EnableBid();
      DisableAsk();
      eye_shoot_bid_size_ = std::min(bid_size_, target_pos_to_close_);
      enable_ask_on_reload_ = true;
    } else if (target_pos_to_close_ < 0) {
      EnableAsk();
      DisableBid();
      eye_shoot_ask_size_ = std::min(ask_size_, -1 * target_pos_to_close_);
      enable_bid_on_reload_ = true;
    } else {
      DisableBid();
      DisableAsk();
      enable_bid_on_reload_ = true;
      enable_ask_on_reload_ = true;
      return;
    }
  } else {
    eye_shoot_bid_size_ = bid_size_;
    eye_shoot_ask_size_ = ask_size_;
  }

  if (shoot_only_on_primary_move_ && !theo_values_.is_primary_update_) {
    return;
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

void ElectronicEye::UpdateBidOrders() {
  eye_bid_int_price_ = int(eye_bid_price_ * inverse_tick_size_ + DOUBLE_PRECISION);
  // If theo_values_.sweep_mode_active_ is 0 here it means that shoot_only_on_sweep_  is set to false,
  // otherwise it would have returned before

//  DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " " << sec_shortcode_ << " " << identifier_ << " sw " << theo_values_.sweep_mode_active_ << " status" << status_mask_ << " bs " << bid_status_ << " theo " << theo_values_.is_valid_ << " eye_bid " << eye_bid_price_ << " seco " << secondary_smv_->market_update_info().bestask_int_price_ << " eye_bid_int_price_ " << eye_bid_int_price_ << " market_ask_price " << theo_values_.primary_best_ask_price_ << " out " << num_outstanding_bid_orders_ << " max " << max_bid_orders_to_shoot_ << " theo_values_.primary_best_ask_price_ " << theo_values_.primary_best_ask_price_ << " prev_market_ask_price_ " << prev_market_ask_price_ << " min_price_move_ " << min_price_move_ << " eye_bid " << eye_bid_int_price_ * tick_size_ << DBGLOG_ENDL_FLUSH ;

//  DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " " << sec_shortcode_ << " " << identifier_ << " bid_orders_ " << num_outstanding_bid_orders_ << " max_shoot_ " << max_bid_orders_to_shoot_ << "EYE " << eye_shoot_bid_size_ << " " << eye_bid_price_ << " X "  << eye_ask_price_ << " " << eye_shoot_ask_size_ << "SELF " << secondary_smv_->market_update_info().bestbid_price_ << " X "  << secondary_smv_->market_update_info().bestask_price_ << "PRI " << theo_values_.primary_best_bid_price_ << " X "  << theo_values_.primary_best_ask_price_ << "REF " << theo_values_.reference_primary_bid_ << " X " << theo_values_.reference_primary_ask_ << DBGLOG_ENDL_FLUSH ;

  if ((theo_values_.sweep_mode_active_ >= 0) && (status_mask_ == BIT_SET_ALL) && bid_status_ &&
      theo_values_.is_valid_ && eye_bid_price_ > 0 && secondary_smv_->market_update_info().bestask_int_price_ > 0 &&
      eye_bid_int_price_ >= secondary_smv_->market_update_info().bestask_int_price_) {

//    DBGLOG_TIME_CLASS_FUNC << " CONDITION 1 CLEAR " << DBGLOG_ENDL_FLUSH ;

    double market_ask_price_ = theo_values_.primary_best_ask_price_;
    // dbglogger_ << watch_.tv() << " price_move_ " << prev_market_ask_price_ << " "
    // << market_ask_price_ << " "<<min_price_move_ << " " <<max_bid_orders_to_shoot_ << DBGLOG_ENDL_FLUSH;
    if (num_outstanding_bid_orders_ < max_bid_orders_to_shoot_ &&
        (num_outstanding_bid_orders_ == 0 || (prev_market_ask_price_ && market_ask_price_ && min_price_move_ &&
                                              (market_ask_price_ - prev_market_ask_price_ > min_price_move_)))) {
      int current_time = watch_.msecs_from_midnight();

//      DBGLOG_TIME_CLASS_FUNC << " CONDITION 2 CLEAR " << DBGLOG_ENDL_FLUSH ;

      if (current_time - last_new_bid_order_time_ < delay_between_orders_) return;

      if (eye_bid_int_price_ > secondary_smv_->upper_int_price_limit_) {
        eye_bid_int_price_ = secondary_smv_->upper_int_price_limit_;
      }

      eye_bid_price_ = eye_bid_int_price_ * tick_size_;
      // HFSAT::BaseOrder* new_bid_order_;
      bid_order_ = basic_om_->SendTrade(eye_bid_price_, eye_bid_int_price_, eye_shoot_bid_size_, HFSAT::kTradeTypeBuy,
                                        'B', HFSAT::kOrderIOC, unique_exec_id_, mirror_factor_, false);

//      DBGLOG_TIME_CLASS_FUNC << " EYE SEND TRADE ON BID : " << eye_bid_price_ << " " << eye_bid_int_price_ << " " << eye_shoot_bid_size_ << DBGLOG_ENDL_FLUSH ;

//      dbglogger_ << watch_.tv() << " " << sec_shortcode_ << " " << identifier_ << " bid_orders_ " << num_outstanding_bid_orders_ << " max_shoot_ " << max_bid_orders_to_shoot_ << "EYE " << eye_shoot_bid_size_ << " " << eye_bid_price_ << " X "  << eye_ask_price_ << " " << eye_shoot_ask_size_ << "SELF " << secondary_smv_->market_update_info().bestbid_price_ << " X "  << secondary_smv_->market_update_info().bestask_price_ << "PRI " << theo_values_.primary_best_bid_price_ << " X "  << theo_values_.primary_best_ask_price_ << "REF " << theo_values_.reference_primary_bid_ << " X " << theo_values_.reference_primary_ask_ << DBGLOG_ENDL_FLUSH ;
      // if(new_bid_order_){
      // 	bid_order_ = new_bid_order_;
      // }
      last_new_bid_order_time_ = current_time;
      if (bid_order_) {
        if (livetrading_) {
          order_count_++;
        }
        num_outstanding_bid_orders_++;
        prev_market_ask_price_ = market_ask_price_;

#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
        if (num_outstanding_bid_orders_ > 1) {
          dbglogger_ << watch_.tv() << " " << sec_shortcode_ << " " << identifier_ << " bid_orders_ "
                     << num_outstanding_bid_orders_ << " max_shoot_ " << max_bid_orders_to_shoot_ << DBGLOG_ENDL_FLUSH;
	  dbglogger_.LogQueryOrder(order_log_buffer_, sec_shortcode_.c_str(), identifier_.c_str(), watch_.tv(), HFSAT::kTradeTypeBuy, true, true, false, false,
							eye_bid_price_, eye_shoot_bid_size_, eye_bid_price_, eye_ask_price_,
							secondary_smv_->market_update_info().bestbid_price_,
							secondary_smv_->market_update_info().bestask_price_,
							theo_values_.primary_best_bid_price_, theo_values_.primary_best_ask_price_,
							theo_values_.reference_primary_bid_, theo_values_.reference_primary_ask_, secondary_smv_->market_update_info().bestask_size_,
							bid_order_->client_assigned_order_sequence_, unique_exec_id_, -1);
        } else {
	  dbglogger_.LogQueryOrder(order_log_buffer_, sec_shortcode_.c_str(), identifier_.c_str(), watch_.tv(), HFSAT::kTradeTypeBuy, true, false, false, false,
							eye_bid_price_, eye_shoot_bid_size_, eye_bid_price_, eye_ask_price_,
							secondary_smv_->market_update_info().bestbid_int_price_,
							secondary_smv_->market_update_info().bestask_int_price_, 
							theo_values_.primary_best_bid_price_, theo_values_.primary_best_ask_price_,
							theo_values_.reference_primary_bid_, theo_values_.reference_primary_ask_, secondary_smv_->market_update_info().bestask_size_,
							bid_order_->client_assigned_order_sequence_, unique_exec_id_, -1);
        }
#endif
      }
    }
  }
}

void ElectronicEye::UpdateAskOrders() {
  eye_ask_int_price_ = int(std::ceil(eye_ask_price_ * inverse_tick_size_ - DOUBLE_PRECISION));

//  DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " " << sec_shortcode_ << " " << identifier_ << " sw " << theo_values_.sweep_mode_active_ << " status" << status_mask_ << " as " << ask_status_ << " theo " << theo_values_.is_valid_ << " eye_ask " << eye_ask_price_ << " seco " << secondary_smv_->market_update_info().bestbid_int_price_ << " eye_ask_int_price_ " << eye_ask_int_price_ << " market_bid_price " << theo_values_.primary_best_bid_price_ << " out " << num_outstanding_bid_orders_ << " max " << max_ask_orders_to_shoot_ << " theo_values_.primary_best_bid_price_ " << theo_values_.primary_best_bid_price_ << " prev_market_bid_price_ " << prev_market_bid_price_ << " min_price_move_ " << min_price_move_ << " eye_ask " << eye_ask_int_price_ * tick_size_ << DBGLOG_ENDL_FLUSH ;

//  DBGLOG_TIME_CLASS_FUNC << watch_.tv() << " " << sec_shortcode_ << " " << identifier_ << " bid_orders_ " << num_outstanding_bid_orders_ << " max_shoot_ " << max_bid_orders_to_shoot_ << "EYE " << eye_shoot_bid_size_ << " " << eye_bid_price_ << " X "  << eye_ask_price_ << " " << eye_shoot_ask_size_ << "SELF " << secondary_smv_->market_update_info().bestbid_price_ << " X "  << secondary_smv_->market_update_info().bestask_price_ << "PRI " << theo_values_.primary_best_bid_price_ << " X "  << theo_values_.primary_best_ask_price_ << "REF " << theo_values_.reference_primary_bid_ << " X " << theo_values_.reference_primary_ask_ << DBGLOG_ENDL_FLUSH ;

  // If theo_values_.sweep_mode_active_ is 0 here it means that shoot_only_on_sweep_  is set to false,
  // otherwise it would have returned before
  if ((theo_values_.sweep_mode_active_ <= 0) && (status_mask_ == BIT_SET_ALL) && ask_status_ &&
      theo_values_.is_valid_ && eye_ask_price_ > 0 && secondary_smv_->market_update_info().bestbid_int_price_ > 0 &&
      eye_ask_int_price_ <= secondary_smv_->market_update_info().bestbid_int_price_) {

//    DBGLOG_TIME_CLASS_FUNC << " CONDITION 1 CLEAR " << DBGLOG_ENDL_FLUSH ;

    double market_bid_price_ = theo_values_.primary_best_bid_price_;
    // dbglogger_ << watch_.tv() << " " << prev_market_bid_price_ << " "
    // << market_bid_price_ << " "<<min_price_move_ << " " <<max_ask_orders_to_shoot_ << DBGLOG_ENDL_FLUSH;
    if (num_outstanding_ask_orders_ < max_ask_orders_to_shoot_ &&
        (num_outstanding_ask_orders_ == 0 || (prev_market_bid_price_ && market_bid_price_ && min_price_move_ &&
                                              (prev_market_bid_price_ - market_bid_price_ > min_price_move_)))) {
      int current_time = watch_.msecs_from_midnight();
      if (current_time - last_new_ask_order_time_ < delay_between_orders_) return;

//      DBGLOG_TIME_CLASS_FUNC << " CONDITION 2 CLEAR " << DBGLOG_ENDL_FLUSH ;

      if (eye_ask_int_price_ < secondary_smv_->lower_int_price_limit_) {
        eye_ask_int_price_ = secondary_smv_->lower_int_price_limit_;
      }

      eye_ask_price_ = eye_ask_int_price_ * tick_size_;
      // HFSAT::BaseOrder* new_ask_order_;
      ask_order_ = basic_om_->SendTrade(eye_ask_price_, eye_ask_int_price_, eye_shoot_ask_size_, HFSAT::kTradeTypeSell,
                                        'B', HFSAT::kOrderIOC, unique_exec_id_, mirror_factor_, false);

//      DBGLOG_TIME_CLASS_FUNC << " EYE SEND TRADE ON ASK : " << eye_ask_price_ << " " << eye_ask_int_price_ << " " << eye_shoot_ask_size_ << DBGLOG_ENDL_FLUSH ;

//      dbglogger_ << watch_.tv() << " " << sec_shortcode_ << " " << identifier_ << " bid_orders_ " << num_outstanding_bid_orders_ << " max_shoot_ " << max_bid_orders_to_shoot_ << "EYE " << eye_shoot_bid_size_ << " " << eye_bid_price_ << " X "  << eye_ask_price_ << " " << eye_shoot_ask_size_ << "SELF " << secondary_smv_->market_update_info().bestbid_price_ << " X "  << secondary_smv_->market_update_info().bestask_price_ << "PRI " << theo_values_.primary_best_bid_price_ << " X "  << theo_values_.primary_best_ask_price_ << "REF " << theo_values_.reference_primary_bid_ << " X " << theo_values_.reference_primary_ask_ << DBGLOG_ENDL_FLUSH ;

      // if(new_ask_order_){
      // 	ask_order_ = new_ask_order_;
      // }
      last_new_ask_order_time_ = current_time;
      if (ask_order_) {
        if (livetrading_) {
          order_count_++;
        }
        num_outstanding_ask_orders_++;
        prev_market_bid_price_ = market_bid_price_;
#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
        if (num_outstanding_ask_orders_ > 1) {
          dbglogger_ << watch_.tv() << " " << sec_shortcode_ << " " << identifier_ << " ask_orders_ "
                     << num_outstanding_ask_orders_ << " max_shoot_ " << max_ask_orders_to_shoot_ << DBGLOG_ENDL_FLUSH;
	  dbglogger_.LogQueryOrder(order_log_buffer_, sec_shortcode_.c_str(), identifier_.c_str(), watch_.tv(), HFSAT::kTradeTypeSell, true, true, false, false
							eye_ask_price_, eye_shoot_ask_size_, eye_bid_price_, eye_ask_price_,
							secondary_smv_->market_update_info().bestbid_price_,
							secondary_smv_->market_update_info().bestask_price_, 
                                                        theo_values_.primary_best_bid_price_, theo_values_.primary_best_ask_price_,
                                                        theo_values_.reference_primary_bid_, theo_values_.reference_primary_ask_,
							secondary_smv_->market_update_info().bestbid_size_,
							ask_order_->client_assigned_order_sequence_, unique_exec_id_, -1);

        } else {
	  dbglogger_.LogQueryOrder(order_log_buffer_, sec_shortcode_.c_str(), identifier_.c_str(), watch_.tv(), HFSAT::kTradeTypeSell, true, false, false, false
							eye_ask_price_, eye_shoot_ask_size_, eye_bid_price_, eye_ask_price_,
							secondary_smv_->market_update_info().bestbid_price_,
							secondary_smv_->market_update_info().bestask_price_,
                                                        theo_values_.primary_best_bid_price_, theo_values_.primary_best_ask_price_,
                                                        theo_values_.reference_primary_bid_, theo_values_.reference_primary_ask_,
							secondary_smv_->market_update_info().bestbid_size_,
							ask_order_->client_assigned_order_sequence_, unique_exec_id_, -1);
        }
#endif
      }
    }
  }
}

void ElectronicEye::DisableBid() {
  bid_status_ = false;
  enable_bid_on_reload_ = false;
}

void ElectronicEye::DisableAsk() {
  ask_status_ = false;
  enable_ask_on_reload_ = false;
}

void ElectronicEye::CheckOrderStatus(HFSAT::BaseOrder* _order_) {
  if (_order_ && (_order_->order_status_ == HFSAT::kORRType_Exec || _order_->order_status_ == HFSAT::kORRType_Rejc ||
                  _order_->order_status_ == HFSAT::kORRType_Cxld)) {
    // Order has lived its life. Time to destroy
    if (_order_->buysell_ == HFSAT::kTradeTypeBuy) {
      num_outstanding_bid_orders_--;
      if (bid_order_ && _order_->client_assigned_order_sequence_ == bid_order_->client_assigned_order_sequence_) {
        bid_order_ = NULL;
      }
    } else {
      num_outstanding_ask_orders_--;
      if (ask_order_ && _order_->client_assigned_order_sequence_ == ask_order_->client_assigned_order_sequence_) {
        ask_order_ = NULL;
      }
    }
    // _order_ = NULL;
  }
}

void ElectronicEye::OnOrderReject(HFSAT::BaseOrder* _order_) {
  // DBGLOG_TIME_CLASS_FUNC << " EYE: OrderReject callback for " << secondary_smv_->shortcode() << DBGLOG_ENDL_FLUSH;
  CheckOrderStatus(_order_);
}

void ElectronicEye::OnOrderChange(HFSAT::BaseOrder* _order_) {
  CheckOrderStatus(_order_);
  // It will only place order if default shoot is on
  // or sweep mode is active
  if ((shoot_only_on_sweep_) && (theo_values_.sweep_mode_active_ == 0)) {
    return;
  }
  UpdateOrders();
  dbglogger_.DumpCurrentBuffer();
}
