#include "tradeengine/Executioner/Quoter.hpp"
#include "tradeengine/Utils/Parser.hpp"

Quoter::Quoter(std::string _exec_param_file, HFSAT::Watch& _watch_, HFSAT::DebugLogger& dbglogger_,
               HFSAT::SecurityMarketView* _secondary_smv, HFSAT::BasicOrderManager* _basic_om, bool _livetrading_,
               bool _is_modify_before_confirmation_, bool _is_cancellable_before_confirmation_, TheoValues& theo_values)
    : BaseExecutioner(_exec_param_file, _watch_, dbglogger_, _secondary_smv, _basic_om, _livetrading_,
                      _is_modify_before_confirmation_, _is_cancellable_before_confirmation_, theo_values),
      bid_percentage_offset_(1),
      ask_percentage_offset_(1),
      bid_offset_(0),
      ask_offset_(0),
      quoter_bid_size_(0),
      quoter_ask_size_(0),
      bid_tighten_update_percent_(0),
      bid_widen_update_percent_(0),
      ask_tighten_update_percent_(0),
      ask_widen_update_percent_(0),
      bid_tighten_update_int_offset_(0),
      bid_widen_update_int_offset_(0),
      ask_tighten_update_int_offset_(0),
      ask_widen_update_int_offset_(0),
      bid_overruled_(false),
      ask_overruled_(false),
      overruled_leaway_percent_(0.1),
      overruled_leaway_offset_(0),
      delay_between_modify_usecs_(0),
      delay_between_orders_(0),
      quoter_bid_price_(-1),
      quoter_bid_int_price_(-1),
      quoter_ask_price_(-1),
      quoter_ask_int_price_(-1),
      previous_bid_price_(-1),
      previous_bid_int_price_(-1),
      previous_ask_price_(-1),
      previous_ask_int_price_(-1),
      bid_order_(NULL),
      ask_order_(NULL),
      last_new_bid_order_time_(0),
      last_modify_bid_order_time_(0),
      last_new_ask_order_time_(0),
      last_modify_ask_order_time_(0),
      use_cancel_on_widen_(false),
      use_selective_quoter_(false),
      selective_bid_threshold_percent_(0),
      selective_ask_threshold_percent_(0),
      selective_bid_threshold_(0),
      selective_ask_threshold_(0),
      turnoff_improve_orders_(false) {
  LoadParams();
  bid_status_ = true;
  ask_status_ = true;
  strcpy(order_log_buffer_.buffer_data_.query_order_.identifier_, identifier_.c_str());
}

void Quoter::LoadParams() {
  identifier_ = Parser::GetString(exec_key_val_map_, "IDENTIFIER", "");
  bool status_ = Parser::GetBool(exec_key_val_map_, "STATUS", false);
  status_mask_ = (status_ == true ? (status_mask_ | CONFIG_STATUS_SET) : (status_mask_ & CONFIG_STATUS_UNSET));
  bid_percentage_offset_ = Parser::GetDouble(exec_key_val_map_, "BID_PERCENTAGE_OFFSET", 1) / 100;
  ask_percentage_offset_ = Parser::GetDouble(exec_key_val_map_, "ASK_PERCENTAGE_OFFSET", 1) / 100;
  bid_size_ = Parser::GetInt(exec_key_val_map_, "BID_SIZE", 0, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);
  ask_size_ = Parser::GetInt(exec_key_val_map_, "ASK_SIZE", 0, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);

  DBGLOG_CLASS_FUNC_LINE_INFO << "QUOTER :" << secondary_smv_->shortcode() << " " << bid_size_ << " " << ask_size_ << DBGLOG_ENDL_FLUSH ; 

  bid_tighten_update_percent_ = Parser::GetDouble(exec_key_val_map_, "BID_TIGHTEN_UPDATE_PERCENT", 0) / 100;
  bid_widen_update_percent_ = Parser::GetDouble(exec_key_val_map_, "BID_WIDEN_UPDATE_PERCENT", 0) / 100;
  ask_tighten_update_percent_ = Parser::GetDouble(exec_key_val_map_, "ASK_TIGHTEN_UPDATE_PERCENT", 0) / 100;
  ask_widen_update_percent_ = Parser::GetDouble(exec_key_val_map_, "ASK_WIDEN_UPDATE_PERCENT", 0) / 100;
  order_limit_ = Parser::GetInt(exec_key_val_map_, "ORDER_LIMIT", 0);
  bad_ratio_limit_ = Parser::GetInt(exec_key_val_map_, "BAD_RATIO_LIMIT", 1000000);
  bid_overruled_ = Parser::GetBool(exec_key_val_map_, "BID_OVERRULED", false);
  ask_overruled_ = Parser::GetBool(exec_key_val_map_, "ASK_OVERRULED", false);
  overruled_leaway_percent_ = Parser::GetDouble(exec_key_val_map_, "OVERRULED_LEAWAY_PERCENT", 0.1) / 100;
  delay_between_modify_usecs_ = Parser::GetInt(exec_key_val_map_, "DELAY_BETWEEN_MODIFY_USECS", 0);
  delay_between_orders_ = Parser::GetInt(exec_key_val_map_, "DELAY_BETWEEN_ORDERS", 0);
  cancel_on_sweep_ = Parser::GetInt(exec_key_val_map_, "CANCEL_ON_SWEEP", 0);
  use_cancel_on_widen_ = Parser::GetBool(exec_key_val_map_, "USE_CANCEL_ON_WIDEN", false);
  use_reserve_msg_ = Parser::GetBool(exec_key_val_map_, "USE_RESERVE_MSG", false);

  use_selective_quoter_ = Parser::GetBool(exec_key_val_map_, "USE_SELECTIVE_QUOTER", false);
  selective_bid_threshold_percent_ = Parser::GetDouble(exec_key_val_map_, "SELECTIVE_BID_THRESHOLD_PERCENT", 1) / 100;
  selective_ask_threshold_percent_ = Parser::GetDouble(exec_key_val_map_, "SELECTIVE_ASK_THRESHOLD_PERCENT", 1) / 100;
  turnoff_improve_orders_ = Parser::GetBool(exec_key_val_map_, "TURNOFF_IMPROVE_ORDERS", false);

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
  quoter_bid_size_ = bid_size_;
  quoter_ask_size_ = ask_size_;
}

void Quoter::OnReady(const HFSAT::MarketUpdateInfo& _market_update_info_) {
  bid_offset_ = bid_percentage_offset_ * _market_update_info_.bestbid_price_;
  ask_offset_ = ask_percentage_offset_ * _market_update_info_.bestbid_price_;

  bid_tighten_update_int_offset_ = bid_tighten_update_percent_ * _market_update_info_.bestbid_int_price_;
  bid_widen_update_int_offset_ = -1 * bid_widen_update_percent_ * _market_update_info_.bestbid_int_price_;
  ask_tighten_update_int_offset_ = ask_tighten_update_percent_ * _market_update_info_.bestbid_int_price_;
  ask_widen_update_int_offset_ = -1 * ask_widen_update_percent_ * _market_update_info_.bestbid_int_price_;

  selective_bid_threshold_ = selective_bid_threshold_percent_ * _market_update_info_.bestbid_price_;
  selective_ask_threshold_ = selective_ask_threshold_percent_ * _market_update_info_.bestbid_price_;

  overruled_leaway_offset_ = overruled_leaway_percent_ * _market_update_info_.bestbid_price_;

  dbglogger_ << watch_.tv() << " QUOTER Offsets: " << bid_offset_ << "|" << ask_offset_ << "|"
             << bid_tighten_update_int_offset_ << "|" << bid_widen_update_int_offset_ << "|"
             << ask_tighten_update_int_offset_ << "|" << ask_widen_update_int_offset_ << "\n";
}

void Quoter::OnTheoUpdate() {
  quoter_bid_price_ = theo_values_.theo_bid_price_ - bid_offset_;
  quoter_bid_int_price_ = int(quoter_bid_price_ * inverse_tick_size_ + DOUBLE_PRECISION);
  quoter_bid_price_ = quoter_bid_int_price_ * tick_size_;

  quoter_ask_price_ = theo_values_.theo_ask_price_ + ask_offset_;
  quoter_ask_int_price_ = int(std::ceil(quoter_ask_price_ * inverse_tick_size_ - DOUBLE_PRECISION));
  quoter_ask_price_ = quoter_ask_int_price_ * tick_size_;

  // DBGLOG_TIME_CLASS_FUNC << identifier_ << "theopx "<< quoter_bid_price_ << " X " << quoter_ask_price_ << "  " <<
  // theo_position << DBGLOG_ENDL_FLUSH;
  UpdateOrders();
}

void Quoter::CheckOrderStatus(HFSAT::BaseOrder* _order_) {
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
      ask_order_ = NULL;
      previous_ask_price_ = -1;
      previous_ask_int_price_ = -1;
    }
  }
  // else{
  // 	dbglogger_ << watch_.tv() << " "
  // 	<< identifier_ <<" Invalid order_update " << DBGLOG_ENDL_FLUSH;
  // }
}

void Quoter::OnOrderReject(HFSAT::BaseOrder* _order_) {
  // DBGLOG_TIME_CLASS_FUNC << " QUOTER: OrderReject callback for " << secondary_smv_->shortcode() << DBGLOG_ENDL_FLUSH;
  CheckOrderStatus(_order_);
  // dbglogger_.DumpCurrentBuffer();
}

void Quoter::OnOrderChange(HFSAT::BaseOrder* _order_) {
  CheckOrderStatus(_order_);
  UpdateOrders();
  dbglogger_.DumpCurrentBuffer();
}

void Quoter::UpdateOrders() {
  if (passive_reduce_ || eff_squareoff_) {
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
      DBGLOG_TIME_CLASS_FUNC << "ORDER COUNT HIT LIMIT: " << order_limit_ << " DISABLING QUOTER " << DBGLOG_ENDL_FLUSH;
    }
    TurnOff(SHOOT_STATUS_UNSET);
  }
}
void Quoter::DisableBid() {
  bid_status_ = false;
  enable_bid_on_reload_ = false;
  CancelBid();
}

void Quoter::DisableAsk() {
  ask_status_ = false;
  enable_ask_on_reload_ = false;
  CancelAsk();
}
void Quoter::CancelBid() {
  if (bid_order_ &&
      (bid_order_->order_status_ == HFSAT::kORRType_Exec || bid_order_->order_status_ == HFSAT::kORRType_Rejc ||
       bid_order_->order_status_ == HFSAT::kORRType_Cxld)) {
    bid_order_ = NULL;
    previous_bid_price_ = -1;
    previous_bid_int_price_ = -1;
  }
  if (bid_order_ && bid_order_->order_status_ != HFSAT::kORRType_Cxld) basic_om_->Cancel(*bid_order_);
}

void Quoter::CancelAsk() {
  if (ask_order_ &&
      (ask_order_->order_status_ == HFSAT::kORRType_Exec || ask_order_->order_status_ == HFSAT::kORRType_Rejc ||
       ask_order_->order_status_ == HFSAT::kORRType_Cxld)) {
    ask_order_ = NULL;
    previous_ask_price_ = -1;
    previous_ask_int_price_ = -1;
  }
  if (ask_order_ && ask_order_->order_status_ != HFSAT::kORRType_Cxld) basic_om_->Cancel(*ask_order_);
}

void Quoter::UpdateBidOrders() {
  if (!theo_values_.is_valid_ || (!bid_status_) || (status_mask_ != BIT_SET_ALL)) {
    CancelBid();
    return;
  }
  if (quoter_bid_price_ <= 0 || quoter_ask_price_ <= 0) return;
  int current_time = watch_.msecs_from_midnight();
  int64_t current_time_usecs = watch_.usecs_from_midnight();

  if ((quoter_bid_int_price_ > secondary_smv_->upper_int_price_limit_) ||
      (quoter_bid_int_price_ < secondary_smv_->lower_int_price_limit_)) {
    if (bid_order_) {
      CancelBid();
    }
    return;
  }

  if (use_selective_quoter_) {
    if ((secondary_smv_->market_update_info_.bestbid_price_ != kInvalidPrice) &&
        (secondary_smv_->market_update_info_.bestbid_price_ - selective_bid_threshold_ > quoter_bid_price_)) {
      if (bid_order_) {
        if (!(bid_order_->canceled_) &&
            (!((bid_order_->order_status_ == HFSAT::kORRType_Seqd) || (bid_order_->num_open_modified_orders_)) ||
             (is_cancellable_before_confirmation_)) &&
            (theo_values_.theo_bid_price_ + DOUBLE_PRECISION < bid_order_->price_)) {
          CancelBid();
        }
      }
      return;
    } else {
      if (turnoff_improve_orders_) {
        quoter_bid_int_price_ = (quoter_bid_price_ < secondary_smv_->market_update_info_.bestbid_price_)
                                    ? quoter_bid_int_price_
                                    : secondary_smv_->market_update_info_.bestbid_int_price_;
      } else {
        quoter_bid_int_price_ = quoter_bid_int_price_;
      }
      quoter_bid_price_ = quoter_bid_int_price_ * tick_size_;
    }
  }

  if (bid_order_) {
    // either the order is partial exec; order still exists
    int price_int_diff_ = quoter_bid_int_price_ - bid_order_->int_price_;
    if ((bid_order_->num_open_modified_orders_) || (bid_order_->order_status_ == HFSAT::kORRType_Seqd)) {
      // In case of Modify reject, num_open_modified_orders_
      // can be non zero but modified_new_int_price_ is not known
      if (bid_order_->modified_) {
        price_int_diff_ = quoter_bid_int_price_ - bid_order_->modified_new_int_price_;
      }
      if ((!is_modify_before_confirmation_) ||
          (current_time_usecs - last_modify_bid_order_time_ < (int64_t)delay_between_modify_usecs_)) {
        if (is_cancellable_before_confirmation_ && use_cancel_on_widen_) {
          if ((price_int_diff_ != 0) && (bid_order_->canceled_ == false) &&
              (price_int_diff_ <= bid_widen_update_int_offset_ ||
               (theo_values_.theo_bid_price_ + DOUBLE_PRECISION < bid_order_->price_))) {
            CancelBid();
            order_count_++;
          }
        }
        return;
      }
    }
    if (price_int_diff_ != 0 && bid_order_->canceled_ == false && bid_order_->order_status_ == HFSAT::kORRType_Conf) {
      if (use_cancel_on_widen_) {
        if (price_int_diff_ <= bid_widen_update_int_offset_ ||
            (theo_values_.theo_bid_price_ + DOUBLE_PRECISION < bid_order_->price_)) {
          // Cancel on widen
          if (!bid_order_->num_open_modified_orders_ || (is_cancellable_before_confirmation_)) {
            CancelBid();
            order_count_++;
          }
        } else if (price_int_diff_ >= bid_tighten_update_int_offset_) {
          // Send Modify on tighten
          bool is_modified =
              basic_om_->Modify(bid_order_, quoter_bid_price_, quoter_bid_int_price_, quoter_bid_size_, false);
          if (is_modified) {
            previous_bid_price_ = quoter_bid_price_;
            previous_bid_int_price_ = quoter_bid_int_price_;
            last_modify_bid_order_time_ = current_time_usecs;
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
        if (((price_int_diff_ > 0) ? ((double)price_int_diff_ >= bid_tighten_update_int_offset_)
                                   : (price_int_diff_ <= bid_widen_update_int_offset_)) ||
            (theo_values_.theo_bid_price_ + DOUBLE_PRECISION < bid_order_->price_)) {
          bool is_modified = basic_om_->Modify(bid_order_, quoter_bid_price_, quoter_bid_int_price_, quoter_bid_size_,
                                               ((use_reserve_msg_) && (price_int_diff_ < 0)));
          if (is_modified) {
            previous_bid_price_ = quoter_bid_price_;
            previous_bid_int_price_ = quoter_bid_int_price_;
            last_modify_bid_order_time_ = current_time_usecs;
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
      }
    }
  } else {
    if (current_time - last_new_bid_order_time_ < delay_between_orders_) return;
    bid_order_ = basic_om_->SendTrade(quoter_bid_price_, quoter_bid_int_price_, quoter_bid_size_, HFSAT::kTradeTypeBuy,
                                      'B', HFSAT::kOrderDay, unique_exec_id_, 1, false);
    if (bid_order_) {
#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
      dbglogger_.LogQueryOrder(order_log_buffer_, sec_shortcode_.c_str(), identifier_.c_str(), watch_.tv(), HFSAT::kTradeTypeBuy, false, false, false, false
					quoter_bid_price_, quoter_bid_size_, theo_values_.theo_bid_price_, theo_values_.theo_ask_price_,
					secondary_smv_->market_update_info().bestbid_price_,
					secondary_smv_->market_update_info().bestask_price_, -1, -1, reference_primary_bid_,
					reference_primary_ask_, secondary_smv_->market_update_info().bestask_size_,
					bid_order_->client_assigned_order_sequence_, unique_exec_id_, -1, quoter_bid_int_price_);
#endif
      previous_bid_price_ = quoter_bid_price_;
      previous_bid_int_price_ = quoter_bid_int_price_;
      last_new_bid_order_time_ = current_time;
      order_count_++;
    }
  }
}

void Quoter::UpdateAskOrders() {
  if (!theo_values_.is_valid_ || (!ask_status_) || (status_mask_ != BIT_SET_ALL)) {
    CancelAsk();
    return;
  }
  if (quoter_bid_price_ <= 0 || quoter_ask_price_ <= 0) return;
  int current_time = watch_.msecs_from_midnight();
  int64_t current_time_usecs = watch_.usecs_from_midnight();

  if ((quoter_ask_int_price_ > secondary_smv_->upper_int_price_limit_) ||
      (quoter_ask_int_price_ < secondary_smv_->lower_int_price_limit_)) {
    if (ask_order_) {
      CancelAsk();
    }
    return;
  }

  if (use_selective_quoter_) {
    if ((secondary_smv_->market_update_info_.bestask_price_ != kInvalidPrice) &&
        (secondary_smv_->market_update_info_.bestask_price_ + selective_ask_threshold_ < quoter_ask_price_)) {
      if (ask_order_) {
        if (!(ask_order_->canceled_) &&
            (!((ask_order_->order_status_ == HFSAT::kORRType_Seqd) && (ask_order_->num_open_modified_orders_)) ||
             (is_cancellable_before_confirmation_)) &&
            (theo_values_.theo_ask_price_ - DOUBLE_PRECISION > ask_order_->price_)) {
          CancelAsk();
        }
      }
      return;
    } else {
      if (turnoff_improve_orders_) {
        quoter_ask_int_price_ = (quoter_ask_price_ > secondary_smv_->market_update_info_.bestask_price_)
                                    ? quoter_ask_int_price_
                                    : secondary_smv_->market_update_info_.bestask_int_price_;
      } else {
        quoter_ask_int_price_ = quoter_ask_int_price_;
      }
      quoter_ask_price_ = quoter_ask_int_price_ * tick_size_;
    }
  }

  if (ask_order_) {
    // either the order is partial exec; order still exists
    int price_int_diff_ = ask_order_->int_price_ - quoter_ask_int_price_;
    if ((ask_order_->num_open_modified_orders_) || (ask_order_->order_status_ == HFSAT::kORRType_Seqd)) {
      // In case of Modify reject, num_open_modified_orders_
      // can be non zero but modified_new_int_price_ is not known
      if (ask_order_->modified_) {
        price_int_diff_ = ask_order_->modified_new_int_price_ - quoter_ask_int_price_;
      }
      if ((!is_modify_before_confirmation_) ||
          (current_time_usecs - last_modify_ask_order_time_ < (int64_t)delay_between_modify_usecs_)) {
        if (is_cancellable_before_confirmation_ && use_cancel_on_widen_) {
          if ((price_int_diff_ != 0) && (ask_order_->canceled_ == false) &&
              (price_int_diff_ <= ask_widen_update_int_offset_ ||
               (theo_values_.theo_ask_price_ - DOUBLE_PRECISION > ask_order_->price_))) {
            CancelAsk();
            order_count_++;
          }
        }
        return;
      }
    }
    if (price_int_diff_ != 0 && ask_order_->canceled_ == false && ask_order_->order_status_ == HFSAT::kORRType_Conf) {
      if (use_cancel_on_widen_) {
        if (price_int_diff_ <= ask_widen_update_int_offset_ ||
            (theo_values_.theo_ask_price_ - DOUBLE_PRECISION > ask_order_->price_)) {
          // Cancel on widen
          if (!ask_order_->num_open_modified_orders_ || (is_cancellable_before_confirmation_)) {
            CancelAsk();
            order_count_++;
          }
        } else if (price_int_diff_ >= ask_tighten_update_int_offset_) {
          // Send Modify for tighten
          bool is_modified =
              basic_om_->Modify(ask_order_, quoter_ask_price_, quoter_ask_int_price_, quoter_ask_size_, false);
          if (is_modified) {
            previous_ask_price_ = quoter_ask_price_;
            previous_ask_int_price_ = quoter_ask_int_price_;
            last_modify_ask_order_time_ = current_time_usecs;
            order_count_++;
          }
        }
      } else {
        if (((price_int_diff_ > 0) ? ((double)price_int_diff_ >= ask_tighten_update_int_offset_)
                                   : (price_int_diff_ <= ask_widen_update_int_offset_)) ||
            (theo_values_.theo_ask_price_ - DOUBLE_PRECISION > ask_order_->price_)) {
          bool is_modified = basic_om_->Modify(ask_order_, quoter_ask_price_, quoter_ask_int_price_, quoter_ask_size_,
                                               ((use_reserve_msg_) && (price_int_diff_ < 0)));
          if (is_modified) {
            previous_ask_price_ = quoter_ask_price_;
            previous_ask_int_price_ = quoter_ask_int_price_;
            last_modify_ask_order_time_ = current_time_usecs;
            order_count_++;
            /*dbglogger_ << watch_.tv() << " "
              << sec_shortcode_ << " " << identifier_
              << "SEND MODIFYORDER ASK " << quoter_ask_price_
              << " size: " << quoter_ask_size_ << " intpx: "
              << quoter_ask_int_price_ << " theo["
              << theo_values_.theo_bid_price_ << " X " << theo_values_.theo_ask_price_ << "]"
              << " mkt[" << secondary_smv_->market_update_info().bestbid_price_
              << " X " << secondary_smv_->market_update_info().bestask_price_ << "]"
              << DBGLOG_ENDL_FLUSH;*/
          }
        }
      }
    }
  } else {
    if (current_time - last_new_ask_order_time_ < delay_between_orders_) return;
    ask_order_ = basic_om_->SendTrade(quoter_ask_price_, quoter_ask_int_price_, quoter_ask_size_, HFSAT::kTradeTypeSell,
                                      'B', HFSAT::kOrderDay, unique_exec_id_, 1, false);
    if (ask_order_) {
#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
      dbglogger_.LogQueryOrder(order_log_buffer_, sec_shortcode_.c_str(), identifier_.c_str(), watch_.tv(), HFSAT::kTradeTypeSell, false, false, false, false
					quoter_ask_price_, quoter_ask_size_, theo_values_.theo_bid_price_, theo_values_.theo_ask_price_,
					secondary_smv_->market_update_info().bestbid_price_,
					secondary_smv_->market_update_info().bestask_price_, -1, -1, reference_primary_bid_,
					reference_primary_ask_, secondary_smv_->market_update_info().bestbid_size_,
					ask_order_->client_assigned_order_sequence_, unique_exec_id_, -1, quoter_ask_int_price_);
#endif
      previous_ask_price_ = quoter_ask_price_;
      previous_ask_int_price_ = quoter_ask_int_price_;
      last_new_ask_order_time_ = current_time;
      order_count_++;
    }
  }
}

// sendTrade some accounting functions can be moved below basetrade->sendTrade
// unsequenced_bids_ is a vector!! on every sequence it traverses the vector!
// fast_price_convertor not so fast? division instead of multiplication
