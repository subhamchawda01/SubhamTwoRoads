#include "tradeengine/Executioner/Improver.hpp"
#include "tradeengine/Utils/Parser.hpp"

Improver::Improver(std::string _exec_param_file, HFSAT::Watch& _watch_, HFSAT::DebugLogger& dbglogger_,
                   HFSAT::SecurityMarketView* _secondary_smv, HFSAT::BasicOrderManager* _base_om, bool _livetrading_,
                   bool _is_modify_before_confirmation_, bool _is_cancellable_before_confirmation_,
                   TheoValues& theo_values)
    : BaseExecutioner(_exec_param_file, _watch_, dbglogger_, _secondary_smv, _base_om, _livetrading_,
                      _is_modify_before_confirmation_, _is_cancellable_before_confirmation_, theo_values),
      max_depth_(1),
      bid_percentage_offset_(1),
      ask_percentage_offset_(1),
      bid_offset_(0),
      ask_offset_(0),
      improver_bid_size_(0),
      improver_ask_size_(0),
      max_bid_join_size_(0),
      max_ask_join_size_(0),
      min_bid_dime_size_(0),
      min_ask_dime_size_(0),
      delay_between_orders_(0),
      drag_tighten_delay_(0),
      min_drag_tighten_delay_(0),
      max_drag_tighten_delay_(0),
      drag_widen_delay_(0),
      order_timeout_(0),
      improver_bid_price_(-1),
      improver_bid_int_price_(-1),
      improver_ask_price_(-1),
      improver_ask_int_price_(-1),
      previous_bid_price_(-1),
      previous_bid_int_price_(-1),
      previous_ask_price_(-1),
      previous_ask_int_price_(-1),
      last_new_bid_order_time_(0),
      last_modify_bid_order_time_(0),
      last_new_ask_order_time_(0),
      last_modify_ask_order_time_(0),
      price_upper_limit_(0),
      price_lower_limit_(0),
      cancel_on_sweep_(0),
      bid_order_(NULL),
      ask_order_(NULL),
      disable_one_percent_order_limit_(false) {
  LoadParams();
  bid_status_ = true;
  ask_status_ = true;
  basic_om_->AddThrottleNumberListeners(this);
}

void Improver::LoadParams() {
  identifier_ = Parser::GetString(exec_key_val_map_, "IDENTIFIER", "");
  bool status_ = Parser::GetBool(exec_key_val_map_, "STATUS", false);
  status_mask_ = (status_ == true ? (status_mask_ | CONFIG_STATUS_SET) : (status_mask_ & CONFIG_STATUS_UNSET));
  bid_percentage_offset_ = Parser::GetDouble(exec_key_val_map_, "BID_PERCENTAGE_OFFSET", 1) / 100;
  ask_percentage_offset_ = Parser::GetDouble(exec_key_val_map_, "ASK_PERCENTAGE_OFFSET", 1) / 100;
  bid_size_ = Parser::GetInt(exec_key_val_map_, "BID_SIZE", 0, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);
  ask_size_ = Parser::GetInt(exec_key_val_map_, "ASK_SIZE", 0, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);
  max_bid_join_size_ = Parser::GetInt(exec_key_val_map_, "MAX_BID_JOIN_SIZE", 0, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);
  max_ask_join_size_ = Parser::GetInt(exec_key_val_map_, "MAX_ASK_JOIN_SIZE", 0, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);
  min_bid_dime_size_ = Parser::GetInt(exec_key_val_map_, "MIN_BID_DIME_SIZE", 0, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);
  min_ask_dime_size_ = Parser::GetInt(exec_key_val_map_, "MIN_ASK_DIME_SIZE", 0, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);

  DBGLOG_CLASS_FUNC_LINE_INFO << "IMPROVER :" << secondary_smv_->shortcode() << " " << bid_size_ << " " << ask_size_ << " " << max_bid_join_size_ << " " << max_ask_join_size_ << " " << min_bid_dime_size_ << " " << min_ask_dime_size_ << DBGLOG_ENDL_FLUSH ;

  order_limit_ = Parser::GetInt(exec_key_val_map_, "ORDER_LIMIT", 0);
  bad_ratio_limit_ = Parser::GetInt(exec_key_val_map_, "BAD_RATIO_LIMIT", 1000000);
  delay_between_orders_ = Parser::GetInt(exec_key_val_map_, "DELAY_BETWEEN_ORDERS", 0);
  config_drag_tighten_delay_ = Parser::GetInt(exec_key_val_map_, "DRAG_TIGHTEN_DELAY", 0);
  drag_tighten_delay_ = config_drag_tighten_delay_;
  min_drag_tighten_delay_ = Parser::GetInt(exec_key_val_map_, "MIN_DRAG_TIGHTEN_DELAY", 0);
  max_drag_tighten_delay_ = Parser::GetInt(exec_key_val_map_, "MAX_DRAG_TIGHTEN_DELAY", 200);
  min_throttles_per_min_ = Parser::GetInt(exec_key_val_map_, "MIN_THROTTLE_PER_MIN", 1);
  max_throttles_per_min_ = Parser::GetInt(exec_key_val_map_, "MAX_THROTTLE_PER_MIN", 100);
  drag_widen_delay_ = Parser::GetInt(exec_key_val_map_, "DRAG_WIDEN_DELAY", 0);
  order_timeout_ = Parser::GetInt(exec_key_val_map_, "ORDER_TIMEOUT", 0);
  disable_one_percent_order_limit_ = Parser::GetBool(exec_key_val_map_, "DISABLE_ONE_PERCENT_ORDER_LIMIT", false);
  cancel_on_sweep_ = Parser::GetInt(exec_key_val_map_, "CANCEL_ON_SWEEP", 0);
  use_reserve_msg_ = Parser::GetBool(exec_key_val_map_, "USE_RESERVE_MSG", false);
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
  improver_bid_size_ = bid_size_;
  improver_ask_size_ = ask_size_;
}

void Improver::OnReady(const HFSAT::MarketUpdateInfo& _market_update_info_) {
  bid_offset_ = bid_percentage_offset_ * _market_update_info_.bestbid_price_;
  ask_offset_ = ask_percentage_offset_ * _market_update_info_.bestbid_price_;

  dbglogger_ << watch_.tv() << "IMPROVER Offsets: " << bid_offset_ << "|" << ask_offset_ << "\n";
}

void Improver::OnTheoUpdate() {
  improver_bid_price_ = theo_values_.theo_bid_price_ - bid_offset_;
  improver_bid_int_price_ = int(improver_bid_price_ * inverse_tick_size_ + DOUBLE_PRECISION);
  improver_bid_price_ = improver_bid_int_price_ * tick_size_;

  improver_ask_price_ = theo_values_.theo_ask_price_ + ask_offset_;
  improver_ask_int_price_ = int(std::ceil(improver_ask_price_ * inverse_tick_size_ - DOUBLE_PRECISION));
  improver_ask_price_ = improver_ask_int_price_ * tick_size_;

  CheckOrderStatus(bid_order_);
  CheckOrderStatus(ask_order_);
  UpdateOrders();
}

void Improver::CheckOrderStatus(HFSAT::BaseOrder* _order_) {
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
  //   else{
  //   	dbglogger_ << watch_.tv() << " "
  // << sec_shortcode_ << " " << identifier_
  // <<" Invalid order_update " << DBGLOG_ENDL_FLUSH;
  //   }
}

void Improver::OnOrderReject(HFSAT::BaseOrder* _order_) {
  // DBGLOG_TIME_CLASS_FUNC << " IMPROVER: OrderReject callback for " << secondary_smv_->shortcode() <<
  // DBGLOG_ENDL_FLUSH;
  CheckOrderStatus(_order_);
  // dbglogger_.DumpCurrentBuffer();
}

void Improver::OnOrderChange(HFSAT::BaseOrder* _order_) {
  CheckOrderStatus(_order_);
  UpdateOrders();
  dbglogger_.DumpCurrentBuffer();
}

void Improver::UpdateOrders() {
  if (passive_reduce_ || eff_squareoff_) {
    // To avoid cases like DRREDDY on 20171201 where we are constantly trying to close
    // positions aggressively and caught in a loop when position to close is exactly
    // half of min order size
    int target_pos_to_close_ =
        HFSAT::MathUtils::RoundOff((-1 * theo_values_.position_to_offset_) + 0.001, secondary_smv_->min_order_size_);
    if (target_pos_to_close_ > 0) {
      EnableBid();
      DisableAsk();
      improver_bid_size_ = std::min(bid_size_, target_pos_to_close_);
      enable_ask_on_reload_ = true;
    } else if (target_pos_to_close_ < 0) {
      EnableAsk();
      DisableBid();
      improver_ask_size_ = std::min(ask_size_, -1 * target_pos_to_close_);
      enable_bid_on_reload_ = true;
    } else {
      DisableBid();
      DisableAsk();
      enable_bid_on_reload_ = true;
      enable_ask_on_reload_ = true;
      return;
    }
  } else {
    improver_bid_size_ = bid_size_;
    improver_ask_size_ = ask_size_;
  }

  if (!theo_values_.is_valid_ || secondary_smv_->market_update_info().bestbid_price_ < 0 ||
      secondary_smv_->market_update_info().bestask_price_ < 0) {
    CancelBid();
    CancelAsk();
    return;
  }

  price_upper_limit_ = 1.0099 * theo_values_.last_traded_int_price_;
  price_lower_limit_ = 0.9901 * theo_values_.last_traded_int_price_;
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
      DBGLOG_TIME_CLASS_FUNC << "ORDER COUNT HIT LIMIT: " << order_limit_ << " DISABLING IMPROVER "
                             << DBGLOG_ENDL_FLUSH;
    }
    TurnOff(SHOOT_STATUS_UNSET);
  }
}

void Improver::DisableBid() {
  bid_status_ = false;
  enable_bid_on_reload_ = false;
  CancelBid();
}

void Improver::DisableAsk() {
  ask_status_ = false;
  enable_ask_on_reload_ = false;
  CancelAsk();
}

void Improver::CancelBid() {
  if (bid_order_ &&
      (bid_order_->order_status_ == HFSAT::kORRType_Exec || bid_order_->order_status_ == HFSAT::kORRType_Rejc ||
       bid_order_->order_status_ == HFSAT::kORRType_Cxld)) {
    bid_order_ = NULL;
    previous_bid_price_ = -1;
    previous_bid_int_price_ = -1;
  }
  if (bid_order_ && bid_order_->order_status_ != HFSAT::kORRType_Cxld) basic_om_->Cancel(*bid_order_);
}

void Improver::CancelAsk() {
  if (ask_order_ &&
      (ask_order_->order_status_ == HFSAT::kORRType_Exec || ask_order_->order_status_ == HFSAT::kORRType_Rejc ||
       ask_order_->order_status_ == HFSAT::kORRType_Cxld)) {
    ask_order_ = NULL;
    previous_ask_price_ = -1;
    previous_ask_int_price_ = -1;
  }
  if (ask_order_ && ask_order_->order_status_ != HFSAT::kORRType_Cxld) basic_om_->Cancel(*ask_order_);
}

void Improver::UpdateBidOrders() {
  if ((!bid_status_) || (status_mask_ != BIT_SET_ALL)) {
    CancelBid();
    return;
  }
  if (improver_bid_price_ <= 0 || improver_ask_price_ <= 0) return;

  if (bid_order_ && ((bid_order_->canceled_) || ((bid_order_->modified_) && (!is_modify_before_confirmation_)))) {
    // Cant do anything with the order till order modify sent
    return;
  }

  // DBGLOG_TIME_CLASS_FUNC << "UpdateBid bidpx" << improver_bid_price_ << " askpx " << improver_ask_price_ <<
  // DBGLOG_ENDL_FLUSH;
  bool already_at_good_price_ = false;
  bool cancel_order_ = false;
  bool dime_l0_ = false;
  bool dime_l1_ = false;
  bool dime_l2_ = false;

  HFSAT::MarketUpdateInfoLevelStruct* l0_info_ = secondary_smv_->bid_info(0);
  HFSAT::MarketUpdateInfoLevelStruct* l1_info_ = secondary_smv_->bid_info(1);
  HFSAT::MarketUpdateInfoLevelStruct* l2_info_ = secondary_smv_->bid_info(2);
  if (l0_info_ == NULL || l1_info_ == NULL) {
    cancel_order_ = true;
  } else if (l0_info_->limit_int_price_ > improver_bid_int_price_) {
    if (l1_info_->limit_int_price_ <= improver_bid_int_price_ && bid_order_ &&
        bid_order_->int_price_ == l1_info_->limit_int_price_) {
      if (l1_info_->limit_size_ - bid_order_->size_remaining_ >= min_bid_dime_size_) {
        already_at_good_price_ = true;
      } else if (l2_info_ != NULL && l2_info_->limit_size_ >= min_bid_dime_size_) {
        dime_l2_ = true;
      } else {
        cancel_order_ = true;
      }
    } else if (l1_info_->limit_int_price_ < improver_bid_int_price_ && l1_info_->limit_size_ >= min_bid_dime_size_) {
      dime_l1_ = true;
    } else {
      cancel_order_ = true;
    }
  } else {
    if (bid_order_ && bid_order_->int_price_ == l0_info_->limit_int_price_) {
      if (l0_info_->limit_size_ - bid_order_->size_remaining_ >= min_bid_dime_size_) {
        already_at_good_price_ = true;
      } else if (l1_info_->limit_size_ >= min_bid_dime_size_) {
        dime_l1_ = true;
      } else {
        cancel_order_ = true;
      }
    } else {
      if (l0_info_->limit_size_ >= min_bid_dime_size_) {
        dime_l0_ = true;
      } else if (bid_order_ && bid_order_->int_price_ == l1_info_->limit_int_price_) {
        if (l1_info_->limit_size_ - bid_order_->size_remaining_ >= min_bid_dime_size_) {
          already_at_good_price_ = true;
        } else if (l2_info_ != NULL && l2_info_->limit_size_ >= min_bid_dime_size_) {
          dime_l2_ = true;
        } else {
          cancel_order_ = true;
        }
      } else if (l1_info_->limit_size_ >= min_bid_dime_size_) {
        dime_l1_ = true;
      } else {
        cancel_order_ = true;
      }
    }
  }

  int current_order_bid_int_price_ = improver_bid_int_price_;

  // TODO Check while creating new level if (join sizing > max join && my order not at this level) then cancel
  if (bid_order_ && already_at_good_price_) {
    current_order_bid_int_price_ = bid_order_->int_price_;
  } else if (dime_l0_) {
    current_order_bid_int_price_ = l0_info_->limit_int_price_ + 1;
  } else if (dime_l1_) {
    current_order_bid_int_price_ = l1_info_->limit_int_price_ + 1;
    if (l0_info_->limit_int_price_ == current_order_bid_int_price_ &&
        (!bid_order_ || (bid_order_ && bid_order_->int_price_ != current_order_bid_int_price_)) &&
        l0_info_->limit_size_ > max_bid_join_size_) {
      cancel_order_ = true;
    }
  } else if (dime_l2_) {
    if (l2_info_ == NULL) {
      cancel_order_ = true;
    } else {
      current_order_bid_int_price_ = l2_info_->limit_int_price_ + 1;
      if (l1_info_->limit_int_price_ == current_order_bid_int_price_ &&
          (!bid_order_ || (bid_order_ && bid_order_->int_price_ != current_order_bid_int_price_)) &&
          l1_info_->limit_size_ > max_bid_join_size_) {
        cancel_order_ = true;
      }
    }
  }
  if (cancel_order_ || current_order_bid_int_price_ > improver_bid_int_price_) {
    CancelBid();
  } else {
    SendBidOrder(current_order_bid_int_price_);
  }
}

void Improver::SendBidOrder(int current_order_bid_int_price_) {
  if (current_order_bid_int_price_ <= 0) return;
  int current_time = watch_.msecs_from_midnight();
  if (current_order_bid_int_price_ > improver_bid_int_price_) {
    dbglogger_ << watch_.tv() << " " << sec_shortcode_ << " " << identifier_
               << " ERROR SENDBIDORDER CURRENT ORDER PRICE SENT " << current_order_bid_int_price_
               << " GREATER THAN IMPROVER PRICE " << improver_bid_int_price_ << DBGLOG_ENDL_FLUSH;
  }
  if (bid_order_) {
    // either the order is partial exec; order still exists
    if (bid_order_->int_price_ <= improver_bid_int_price_) {
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
        if (improver_bid_int_price_ < price_lower_limit_) {
          /*dbglogger_ << watch_.tv() << " " << sec_shortcode_
                  << " cancelling bid " << improver_bid_int_price_
                  << " ltp " << theo_values_.last_traded_int_price_ << DBGLOG_ENDL_FLUSH;*/
          CancelBid();
          return;
        } else if (improver_bid_int_price_ >= price_upper_limit_) {
          current_order_bid_int_price_ = price_upper_limit_;
        } else {
          current_order_bid_int_price_ = improver_bid_int_price_;
        }
      }
    }

    if (bid_order_->int_price_ != current_order_bid_int_price_ && bid_order_->canceled_ == false &&
        bid_order_->order_status_ == HFSAT::kORRType_Conf) {
      double current_order_bid_price_ = current_order_bid_int_price_ * tick_size_;
      bool is_modified_ =
          basic_om_->Modify(bid_order_, current_order_bid_price_, current_order_bid_int_price_, improver_bid_size_,
                            ((use_reserve_msg_) && ((bid_order_->int_price_ - current_order_bid_int_price_) > 0)));
      if (is_modified_) {
        /*dbglogger_ << watch_.tv() << " "
                << sec_shortcode_ << " " << identifier_
                << "SEND MODIFYORDER BUY " << current_order_bid_price_
                << " size: " << improver_bid_size_ << " intpx: "
                << current_order_bid_int_price_ << " theo["
                << theo_values_.theo_bid_price_ << " X " << theo_values_.theo_ask_price_ << "]"
                << " mkt[" << secondary_smv_->market_update_info().bestbid_price_
                << " X " << secondary_smv_->market_update_info().bestask_price_ << "]"
                << " improverintpx " << improver_bid_int_price_ << DBGLOG_ENDL_FLUSH;*/
        previous_bid_price_ = current_order_bid_price_;
        previous_bid_int_price_ = current_order_bid_int_price_;
        last_modify_bid_order_time_ = current_time;
        order_count_++;
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
    bid_order_ = basic_om_->SendTrade(current_order_bid_price_, current_order_bid_int_price_, improver_bid_size_,
                                      HFSAT::kTradeTypeBuy, 'B', HFSAT::kOrderDay, unique_exec_id_, 1, false);

    if (bid_order_) {
#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
      dbglogger_ << watch_.tv() << " " << sec_shortcode_ << " " << identifier_ << "SEND NEWORDER BUY "
                 << current_order_bid_price_ << " size: " << improver_bid_size_
                 << " intpx: " << current_order_bid_int_price_ << " theo[" << theo_values_.theo_bid_price_ << " X "
                 << theo_values_.theo_ask_price_ << "]"
                 << " mkt[" << secondary_smv_->market_update_info().bestbid_price_ << " X "
                 << secondary_smv_->market_update_info().bestask_price_ << "]"
                 << " improverintpx " << improver_bid_int_price_
                 << " CAOS: " << bid_order_->client_assigned_order_sequence_ << DBGLOG_ENDL_FLUSH;
#endif
      order_count_++;
      previous_bid_price_ = current_order_bid_price_;
      previous_bid_int_price_ = current_order_bid_int_price_;
      last_new_bid_order_time_ = current_time;
    }
  }
}

void Improver::UpdateAskOrders() {
  if ((!ask_status_) || (status_mask_ != BIT_SET_ALL)) {
    CancelAsk();
    return;
  }
  if (improver_bid_price_ <= 0 || improver_ask_price_ <= 0) return;

  if (ask_order_ && ((ask_order_->canceled_) || ((ask_order_->modified_) && (!is_modify_before_confirmation_)))) {
    // Cant do anything with the order till order modify sent
    return;
  }

  // DBGLOG_TIME_CLASS_FUNC << "UpdateAsk bidpx" << improver_bid_price_ << " askpx " << improver_ask_price_ <<
  // DBGLOG_ENDL_FLUSH;
  bool already_at_good_price_ = false;
  bool cancel_order_ = false;
  bool dime_l0_ = false;
  bool dime_l1_ = false;
  bool dime_l2_ = false;

  HFSAT::MarketUpdateInfoLevelStruct* l0_info_ = secondary_smv_->ask_info(0);
  HFSAT::MarketUpdateInfoLevelStruct* l1_info_ = secondary_smv_->ask_info(1);
  HFSAT::MarketUpdateInfoLevelStruct* l2_info_ = secondary_smv_->ask_info(2);
  if (l0_info_ == NULL || l1_info_ == NULL) {
    cancel_order_ = true;
  } else if (l0_info_->limit_int_price_ < improver_ask_int_price_) {
    if (l1_info_->limit_int_price_ >= improver_ask_int_price_ && ask_order_ &&
        ask_order_->int_price_ == l1_info_->limit_int_price_) {
      if (l1_info_->limit_size_ - ask_order_->size_remaining_ >= min_ask_dime_size_) {
        already_at_good_price_ = true;
      } else if (l2_info_ != NULL && l2_info_->limit_size_ >= min_ask_dime_size_) {
        dime_l2_ = true;
      } else {
        cancel_order_ = true;
      }
    } else if (l1_info_->limit_int_price_ > improver_ask_int_price_ && l1_info_->limit_size_ >= min_ask_dime_size_) {
      dime_l1_ = true;
    } else {
      cancel_order_ = true;
    }
  } else {
    if (ask_order_ && ask_order_->int_price_ == l0_info_->limit_int_price_) {
      if (l0_info_->limit_size_ - ask_order_->size_remaining_ >= min_ask_dime_size_) {
        already_at_good_price_ = true;
      } else if (l1_info_->limit_size_ >= min_ask_dime_size_) {
        dime_l1_ = true;
      } else {
        cancel_order_ = true;
      }
    } else {
      if (l0_info_->limit_size_ >= min_ask_dime_size_) {
        dime_l0_ = true;
      } else if (ask_order_ && ask_order_->int_price_ == l1_info_->limit_int_price_) {
        if (l1_info_->limit_size_ - ask_order_->size_remaining_ >= min_ask_dime_size_) {
          already_at_good_price_ = true;
        } else if (l2_info_ != NULL && l2_info_->limit_size_ >= min_ask_dime_size_) {
          dime_l2_ = true;
        } else {
          cancel_order_ = true;
        }
      } else if (l1_info_->limit_size_ >= min_ask_dime_size_) {
        dime_l1_ = true;
      } else {
        cancel_order_ = true;
      }
    }
  }

  int current_order_ask_int_price_ = improver_ask_int_price_;

  // TODO Check while creating new level if (join sizing > max join && my order not at this level) then cancel
  if (ask_order_ && already_at_good_price_) {
    current_order_ask_int_price_ = ask_order_->int_price_;
  } else if (dime_l0_) {
    current_order_ask_int_price_ = l0_info_->limit_int_price_ - 1;
  } else if (dime_l1_) {
    current_order_ask_int_price_ = l1_info_->limit_int_price_ - 1;
    if (l0_info_->limit_int_price_ == current_order_ask_int_price_ &&
        (!ask_order_ || (ask_order_ && ask_order_->int_price_ != current_order_ask_int_price_)) &&
        l0_info_->limit_size_ > max_ask_join_size_) {
      cancel_order_ = true;
    }
  } else if (dime_l2_) {
    if (l2_info_ == NULL) {
      cancel_order_ = true;
    } else {
      current_order_ask_int_price_ = l2_info_->limit_int_price_ - 1;
      if (l1_info_->limit_int_price_ == current_order_ask_int_price_ &&
          (!ask_order_ || (ask_order_ && ask_order_->int_price_ != current_order_ask_int_price_)) &&
          l1_info_->limit_size_ > max_ask_join_size_) {
        cancel_order_ = true;
      }
    }
  }
  if (cancel_order_ || current_order_ask_int_price_ < improver_ask_int_price_) {
    CancelAsk();
  } else {
    SendAskOrder(current_order_ask_int_price_);
  }
}

void Improver::SendAskOrder(int current_order_ask_int_price_) {
  if (current_order_ask_int_price_ <= 0) return;
  int current_time = watch_.msecs_from_midnight();
  if (current_order_ask_int_price_ < improver_ask_int_price_) {
    dbglogger_ << watch_.tv() << " " << sec_shortcode_ << " " << identifier_
               << " ERROR SENDASKORDER CURRENT ORDER PRICE SENT " << current_order_ask_int_price_
               << " LESS THAN IMPROVER PRICE " << improver_ask_int_price_ << DBGLOG_ENDL_FLUSH;
  }
  if (ask_order_) {
    if (ask_order_->int_price_ >= improver_ask_int_price_) {
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
        if (improver_ask_int_price_ > price_upper_limit_) {
          // Cancel ask order. will have to sit out
          /*dbglogger_ << watch_.tv() << " " << sec_shortcode_
                  << " cancelling ask " << improver_ask_int_price_
                  << " ltp " << theo_values_.last_traded_int_price_ << DBGLOG_ENDL_FLUSH;*/
          CancelAsk();
          return;
        } else if (improver_ask_int_price_ <= price_lower_limit_) {
          current_order_ask_int_price_ = price_lower_limit_;
        } else {
          current_order_ask_int_price_ = improver_ask_int_price_;
        }
      }
    }
    // either the order is partial exec; order still exists
    if (ask_order_->int_price_ != current_order_ask_int_price_ && ask_order_->canceled_ == false &&
        ask_order_->order_status_ == HFSAT::kORRType_Conf) {
      double current_order_ask_price_ = current_order_ask_int_price_ * tick_size_;
      bool is_modified_ =
          basic_om_->Modify(ask_order_, current_order_ask_price_, current_order_ask_int_price_, improver_ask_size_,
                            ((use_reserve_msg_) && ((ask_order_->int_price_ - current_order_ask_int_price_) < 0)));
      if (is_modified_) {
        /*dbglogger_ << watch_.tv() << " "
                << sec_shortcode_ << " " << identifier_
                << "SEND MODIFYORDER SELL " << current_order_ask_price_
                << " size: " << improver_ask_size_ << " intpx: "
                << current_order_ask_int_price_ << " theo["
                << theo_values_.theo_bid_price_ << " X " << theo_values_.theo_ask_price_ << "]"
                << " mkt[" << secondary_smv_->market_update_info().bestbid_price_
                << " X " << secondary_smv_->market_update_info().bestask_price_ << "]"
                << " improverintpx " << improver_ask_int_price_ << DBGLOG_ENDL_FLUSH;*/
        previous_ask_price_ = current_order_ask_price_;
        previous_ask_int_price_ = current_order_ask_int_price_;
        last_modify_ask_order_time_ = current_time;
        order_count_++;
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
    ask_order_ = basic_om_->SendTrade(current_order_ask_price_, current_order_ask_int_price_, improver_ask_size_,
                                      HFSAT::kTradeTypeSell, 'B', HFSAT::kOrderDay, unique_exec_id_, 1, false);

    if (ask_order_) {
#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
      dbglogger_ << watch_.tv() << " " << sec_shortcode_ << " " << identifier_ << "SEND NEWORDER SELL "
                 << current_order_ask_price_ << " size: " << improver_ask_size_
                 << " intpx: " << current_order_ask_int_price_ << " theo[" << theo_values_.theo_bid_price_ << " X "
                 << theo_values_.theo_ask_price_ << "]"
                 << " mkt[" << secondary_smv_->market_update_info().bestbid_price_ << " X "
                 << secondary_smv_->market_update_info().bestask_price_ << "]"
                 << " improverintpx " << improver_ask_int_price_
                 << " CAOS: " << ask_order_->client_assigned_order_sequence_ << DBGLOG_ENDL_FLUSH;
#endif
      previous_ask_price_ = current_order_ask_price_;
      previous_ask_int_price_ = current_order_ask_int_price_;
      last_new_ask_order_time_ = current_time;
      order_count_++;
    }
  }
}
