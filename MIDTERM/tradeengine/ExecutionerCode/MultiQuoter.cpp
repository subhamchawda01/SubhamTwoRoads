
#include "tradeengine/Executioner/MultiQuoter.hpp"
#include "tradeengine/Utils/Parser.hpp"

MultiQuoter::MultiQuoter(std::string _exec_param_file, HFSAT::Watch& _watch_, HFSAT::DebugLogger& dbglogger_,
                         HFSAT::SecurityMarketView* _secondary_smv, HFSAT::BasicOrderManager* _basic_om,
                         bool _livetrading_, bool _is_modify_before_confirmation_,
                         bool _is_cancellable_before_confirmation_, TheoValues& theo_values)
    : BaseExecutioner(_exec_param_file, _watch_, dbglogger_, _secondary_smv, _basic_om, _livetrading_,
                      _is_modify_before_confirmation_, _is_cancellable_before_confirmation_, theo_values),
      bid_percentage_offset_(1),
      ask_percentage_offset_(1),
      bid_offset_(0),
      ask_offset_(0),
      step_offset_(1),
      num_levels_to_place_(1),
      quoter_bid_size_(0),
      quoter_ask_size_(0),
      bid_tighten_update_percent_(0),
      bid_widen_update_percent_(0),
      ask_widen_update_percent_(0),
      bid_tighten_update_int_offset_(0),
      bid_widen_update_int_offset_(0),
      ask_tighten_update_int_offset_(0),
      ask_widen_update_int_offset_(0),
      bid_overruled_(false),
      ask_overruled_(false),
      overruled_leaway_percent_(0.1),
      overruled_leaway_offset_(0),
      delay_between_orders_(0),
      delay_between_modify_usecs_(0),
      quoter_bid_price_(-1),
      quoter_bid_int_price_(-1),
      quoter_ask_price_(-1),
      quoter_ask_int_price_(-1),
      previous_bid_price_(-1),
      previous_bid_int_price_(-1),
      previous_ask_price_(-1),
      previous_ask_int_price_(-1),
      bid_order_price_pair_vec_(),
      ask_order_price_pair_vec_(),
      cancel_on_sweep_(0),
      use_cancel_on_widen_(false),
      last_new_bid_order_time_(0),
      last_modify_bid_order_time_(0),
      last_new_ask_order_time_(0),
      last_modify_ask_order_time_(0) {
  LoadParams();
  bid_status_ = true;
  ask_status_ = true;
  strcpy(order_log_buffer_.buffer_data_.query_order_.identifier_, identifier_.c_str());
}

void MultiQuoter::LoadParams() {
  identifier_ = Parser::GetString(exec_key_val_map_, "IDENTIFIER", "");
  bool status_ = Parser::GetBool(exec_key_val_map_, "STATUS", false);
  status_mask_ = (status_ == true ? (status_mask_ | CONFIG_STATUS_SET) : (status_mask_ & CONFIG_STATUS_UNSET));
  bid_percentage_offset_ = Parser::GetDouble(exec_key_val_map_, "BID_PERCENTAGE_OFFSET", 1) / 100;
  ask_percentage_offset_ = Parser::GetDouble(exec_key_val_map_, "ASK_PERCENTAGE_OFFSET", 1) / 100;
  step_offset_ = std::max(1, Parser::GetInt(exec_key_val_map_, "STEP_OFFSET", 1));
  num_levels_to_place_ = Parser::GetDouble(exec_key_val_map_, "NUM_LEVELS_TO_PLACE", 1);
  bid_size_ = Parser::GetInt(exec_key_val_map_, "BID_SIZE", 0, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);
  ask_size_ = Parser::GetInt(exec_key_val_map_, "ASK_SIZE", 0, DEP_ONE_LOT_SIZE_, INDEP_ONE_LOT_SIZE_);

  DBGLOG_CLASS_FUNC_LINE_INFO << "MQ :" << secondary_smv_->shortcode() << " " << bid_size_ << " " << ask_size_ << DBGLOG_ENDL_FLUSH ;

  bid_tighten_update_percent_ = Parser::GetDouble(exec_key_val_map_, "BID_TIGHTEN_UPDATE_PERCENT", 0) / 100;
  bid_widen_update_percent_ = Parser::GetDouble(exec_key_val_map_, "BID_WIDEN_UPDATE_PERCENT", 0) / 100;
  ask_tighten_update_percent_ = Parser::GetDouble(exec_key_val_map_, "ASK_TIGHTEN_UPDATE_PERCENT", 0) / 100;
  ask_widen_update_percent_ = Parser::GetDouble(exec_key_val_map_, "ASK_WIDEN_UPDATE_PERCENT", 0) / 100;
  order_limit_ = Parser::GetInt(exec_key_val_map_, "ORDER_LIMIT", 0);
  bad_ratio_limit_ = Parser::GetInt(exec_key_val_map_, "BAD_RATIO_LIMIT", 1000000);
  bid_overruled_ = Parser::GetBool(exec_key_val_map_, "BID_OVERRULED", false);
  ask_overruled_ = Parser::GetBool(exec_key_val_map_, "ASK_OVERRULED", false);
  overruled_leaway_percent_ = Parser::GetDouble(exec_key_val_map_, "OVERRULED_LEAWAY_PERCENT", 0.1) / 100;
  delay_between_orders_ = Parser::GetInt(exec_key_val_map_, "DELAY_BETWEEN_ORDERS", 0);
  delay_between_modify_usecs_ = Parser::GetInt(exec_key_val_map_, "DELAY_BETWEEN_MODIFY_USECS", 0);
  cancel_on_sweep_ = Parser::GetInt(exec_key_val_map_, "CANCEL_ON_SWEEP", 0);
  use_cancel_on_widen_ = Parser::GetBool(exec_key_val_map_, "USE_CANCEL_ON_WIDEN", false);
  use_reserve_msg_ = Parser::GetBool(exec_key_val_map_, "USE_RESERVE_MSG", false);

  bid_order_price_pair_vec_.resize(num_levels_to_place_, std::make_pair((HFSAT::BaseOrder*)NULL, -1000));
  ask_order_price_pair_vec_.resize(num_levels_to_place_, std::make_pair((HFSAT::BaseOrder*)NULL, -1000));

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

void MultiQuoter::OnReady(const HFSAT::MarketUpdateInfo& _market_update_info_) {
  bid_offset_ = bid_percentage_offset_ * _market_update_info_.bestbid_price_;
  ask_offset_ = ask_percentage_offset_ * _market_update_info_.bestbid_price_;

  bid_tighten_update_int_offset_ = bid_tighten_update_percent_ * _market_update_info_.bestbid_int_price_;
  bid_widen_update_int_offset_ = bid_widen_update_percent_ * _market_update_info_.bestbid_int_price_;
  ask_tighten_update_int_offset_ = ask_tighten_update_percent_ * _market_update_info_.bestbid_int_price_;
  ask_widen_update_int_offset_ = ask_widen_update_percent_ * _market_update_info_.bestbid_int_price_;

  overruled_leaway_offset_ = overruled_leaway_percent_ * _market_update_info_.bestbid_price_;

  dbglogger_ << watch_.tv() << " QUOTER Offsets: " << bid_offset_ << "|" << ask_offset_ << "|"
             << bid_tighten_update_int_offset_ << "|" << bid_widen_update_int_offset_ << "|"
             << ask_tighten_update_int_offset_ << "|" << ask_widen_update_int_offset_ << "|" << step_offset_ << "\n";
}

void MultiQuoter::OnTheoUpdate() {
  quoter_bid_price_ = theo_values_.theo_bid_price_ - bid_offset_;
  quoter_bid_int_price_ = int(quoter_bid_price_ * inverse_tick_size_ + DOUBLE_PRECISION);
  quoter_bid_price_ = quoter_bid_int_price_ * tick_size_;

  quoter_ask_price_ = theo_values_.theo_ask_price_ + ask_offset_;
  quoter_ask_int_price_ = int(std::ceil(quoter_ask_price_ * inverse_tick_size_ - DOUBLE_PRECISION));
  quoter_ask_price_ = quoter_ask_int_price_ * tick_size_;

  // DBGLOG_TIME_CLASS_FUNC << identifier_ << "theopx "<< quoter_bid_price_ << " X " << quoter_ask_price_ << "  " <<
  // theo_position << DBGLOG_ENDL_FLUSH;
  UpdateOrders();
  // basic_om_->LogFullStatus();
}

void MultiQuoter::CheckOrderStatus(HFSAT::BaseOrder* _order_) {
  // bool order_found_ =false;
  if (_order_ && _order_->buysell_ == HFSAT::kTradeTypeBuy) {
    for (auto& bid_order_price_ : bid_order_price_pair_vec_) {
      HFSAT::BaseOrder* bid_order_ = bid_order_price_.first;
      if (bid_order_ && bid_order_->client_assigned_order_sequence_ == _order_->client_assigned_order_sequence_) {
        // order_found_ = true;
        if ((_order_->order_status_ == HFSAT::kORRType_Exec || _order_->order_status_ == HFSAT::kORRType_Rejc ||
             _order_->order_status_ == HFSAT::kORRType_Cxld)) {
          bid_order_ = NULL;
          bid_order_price_.first = NULL;
        }
        break;
      }
    }
  }

  else if (_order_) {
    for (auto& ask_order_price_ : ask_order_price_pair_vec_) {
      HFSAT::BaseOrder* ask_order_ = ask_order_price_.first;
      if (ask_order_ && ask_order_->client_assigned_order_sequence_ == _order_->client_assigned_order_sequence_) {
        // order_found_ = true;
        if ((_order_->order_status_ == HFSAT::kORRType_Exec || _order_->order_status_ == HFSAT::kORRType_Rejc ||
             _order_->order_status_ == HFSAT::kORRType_Cxld)) {
          ask_order_ = NULL;
          ask_order_price_.first = NULL;
        }
        break;
      }
    }
  }

  // if(!order_found_){
  // 	dbglogger_ << watch_.tv() << " "
  // 	<< sec_shortcode_ << " " << identifier_
  // 	<<" Invalid order_update " << DBGLOG_ENDL_FLUSH;
  // }
}

void MultiQuoter::OnOrderReject(HFSAT::BaseOrder* _order_) {
  // DBGLOG_TIME_CLASS_FUNC << " MULTIQUOTER: OrderReject callback for " << secondary_smv_->shortcode() <<
  // DBGLOG_ENDL_FLUSH;
  CheckOrderStatus(_order_);
  // dbglogger_.DumpCurrentBuffer();
}

void MultiQuoter::OnOrderChange(HFSAT::BaseOrder* _order_) {
  CheckOrderStatus(_order_);
  UpdateOrders();
  dbglogger_.DumpCurrentBuffer();
}

void MultiQuoter::UpdateOrders() {
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

  if (cancel_on_sweep_ != 0 && theo_values_.sweep_mode_active_ != 0) {
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

void MultiQuoter::DisableBid() {
  bid_status_ = false;
  enable_bid_on_reload_ = false;
  CancelBid();
}

void MultiQuoter::DisableAsk() {
  ask_status_ = false;
  enable_ask_on_reload_ = false;
  CancelAsk();
}

void MultiQuoter::CancelBid() {
  for (auto& bid_order_price_ : bid_order_price_pair_vec_) {
    HFSAT::BaseOrder* bid_order_ = bid_order_price_.first;
    if (bid_order_ &&
        (bid_order_->order_status_ == HFSAT::kORRType_Exec || bid_order_->order_status_ == HFSAT::kORRType_Rejc ||
         bid_order_->order_status_ == HFSAT::kORRType_Cxld)) {
      bid_order_ = NULL;
      bid_order_price_.first = NULL;
    }
    if (bid_order_ && bid_order_->order_status_ != HFSAT::kORRType_Cxld) {
      basic_om_->Cancel(*bid_order_);
      // dbglogger_ << watch_.tv() << " Sending Bid Cancel " << DBGLOG_ENDL_FLUSH;
    }
  }
}

void MultiQuoter::CancelAsk() {
  for (auto& ask_order_price_ : ask_order_price_pair_vec_) {
    HFSAT::BaseOrder* ask_order_ = ask_order_price_.first;
    if (ask_order_ &&
        (ask_order_->order_status_ == HFSAT::kORRType_Exec || ask_order_->order_status_ == HFSAT::kORRType_Rejc ||
         ask_order_->order_status_ == HFSAT::kORRType_Cxld)) {
      ask_order_ = NULL;
      ask_order_price_.first = NULL;
    }
    if (ask_order_ && ask_order_->order_status_ != HFSAT::kORRType_Cxld) {
      basic_om_->Cancel(*ask_order_);
      // dbglogger_ << watch_.tv() << " Sending Ask Cancel " << DBGLOG_ENDL_FLUSH;
    }
  }
}

void MultiQuoter::UpdateBidOrders() {
  if (!theo_values_.is_valid_ || (!bid_status_) || (status_mask_ != BIT_SET_ALL)) {
    CancelBid();
    return;
  }

  if (quoter_bid_price_ <= 0 || quoter_ask_price_ <= 0) return;

  // First Order of day
  if (previous_bid_int_price_ == -1) {
    // Price of last order
    bid_order_price_pair_vec_.back().second = quoter_bid_int_price_;
  }

  // Sending Top invalid orders to the back of the queue
  int num_null_ordes_from_top_ = 0;
  HFSAT::BaseOrder* top_bid_order_ = bid_order_price_pair_vec_.front().first;
  previous_bid_int_price_ = bid_order_price_pair_vec_.front().second;
  int price_to_place_ = bid_order_price_pair_vec_.back().second - step_offset_;
  if ((!(top_bid_order_) || (top_bid_order_->order_status_ == HFSAT::kORRType_Exec ||
                             top_bid_order_->order_status_ == HFSAT::kORRType_Rejc ||
                             top_bid_order_->order_status_ == HFSAT::kORRType_Cxld))) {
    top_bid_order_ = NULL;
  }
  while (top_bid_order_ == NULL) {
    top_bid_order_ = NULL;
    num_null_ordes_from_top_++;
    // This is the case when all orders are null, then dont send final order back
    // just break from the loop
    if (num_null_ordes_from_top_ == num_levels_to_place_) {
      break;
    }

    bid_order_price_pair_vec_.pop_front();
    bid_order_price_pair_vec_.push_back(std::make_pair(top_bid_order_, price_to_place_));
    top_bid_order_ = bid_order_price_pair_vec_.front().first;
    previous_bid_int_price_ = bid_order_price_pair_vec_.front().second;
    price_to_place_ -= step_offset_;
    if ((!(top_bid_order_) || (top_bid_order_->order_status_ == HFSAT::kORRType_Exec ||
                               top_bid_order_->order_status_ == HFSAT::kORRType_Rejc ||
                               top_bid_order_->order_status_ == HFSAT::kORRType_Cxld))) {
      top_bid_order_ = NULL;
    }
  }

  // Checking for widen scenario
  int price_int_diff_ = quoter_bid_int_price_ - previous_bid_int_price_;

  if (((price_int_diff_ != 0) && (-1.0 * price_int_diff_ >= bid_widen_update_int_offset_)) ||  // Widen scenario
      (int(theo_values_.theo_bid_price_ * inverse_tick_size_) < previous_bid_int_price_)) {    // Theo Violation
    std::pair<HFSAT::BaseOrder*, int> current_bid_order_price_pair_ = bid_order_price_pair_vec_.front();
    int current_order_price_ = current_bid_order_price_pair_.second;
    HFSAT::BaseOrder* bid_order_ = current_bid_order_price_pair_.first;
    int price_to_place_ = bid_order_price_pair_vec_.back().second - step_offset_;

    if (bid_order_price_pair_vec_.back().second > quoter_bid_int_price_) {
      price_to_place_ = quoter_bid_int_price_;
    }

    while ((current_order_price_ > quoter_bid_int_price_)) {
      bid_order_price_pair_vec_.pop_front();
      bid_order_price_pair_vec_.push_back(std::make_pair(bid_order_, price_to_place_));
      current_bid_order_price_pair_ = bid_order_price_pair_vec_.front();
      current_order_price_ = current_bid_order_price_pair_.second;
      bid_order_ = current_bid_order_price_pair_.first;
      price_to_place_ -= step_offset_;
    }
    previous_bid_int_price_ = current_order_price_;
  }
  price_int_diff_ = quoter_bid_int_price_ - previous_bid_int_price_;

  // Checking for tighten scenario ; One thing to note here is that
  // both widen and tighten can exist simultaneously
  if ((price_int_diff_ != 0) && ((double)price_int_diff_ >= bid_tighten_update_int_offset_) &&
      (price_int_diff_ >= step_offset_)) {  // Tighten scenario
    int num_orders_ = std::min((int)(price_int_diff_ / step_offset_), num_levels_to_place_);
    while (num_orders_ > 0) {
      HFSAT::BaseOrder* bid_order_ = bid_order_price_pair_vec_.back().first;
      bid_order_price_pair_vec_.pop_back();
      bid_order_price_pair_vec_.push_front(
          std::make_pair(bid_order_, quoter_bid_int_price_ - step_offset_ * (num_orders_ - 1)));
      num_orders_--;
    }
    previous_bid_int_price_ = quoter_bid_int_price_;
  }

  int64_t current_time_usecs = watch_.usecs_from_midnight();
  for (auto it = bid_order_price_pair_vec_.begin(); it != bid_order_price_pair_vec_.end(); it++) {
    HFSAT::BaseOrder* bid_order_ = (*it).first;
    int int_price_to_place_ = (*it).second;
    if (!(bid_order_) ||
        (bid_order_->order_status_ == HFSAT::kORRType_Exec || bid_order_->order_status_ == HFSAT::kORRType_Rejc ||
         bid_order_->order_status_ == HFSAT::kORRType_Cxld)) {
      bid_order_ = NULL;
    }

    int current_time = watch_.msecs_from_midnight();

    if ((int_price_to_place_ > secondary_smv_->upper_int_price_limit_) ||
        (int_price_to_place_ < secondary_smv_->lower_int_price_limit_)) {
      if ((bid_order_) && (bid_order_->canceled_ == false) && (bid_order_->modified_ == false)) {
        basic_om_->Cancel(*bid_order_);
        order_count_++;
      }
      continue;
    }

    if (bid_order_) {
      // either the order is partial exec; order still exists

      int t_price_int_diff_ = int_price_to_place_ - bid_order_->int_price_;
      if (bid_order_->num_open_modified_orders_) {
        // In case of Modify reject, num_open_modified_orders_
        // can be non zero but modified_new_int_price_ is not known
        if (bid_order_->modified_) {
          t_price_int_diff_ = int_price_to_place_ - bid_order_->modified_new_int_price_;
        }
        if ((!is_modify_before_confirmation_) ||
            (current_time_usecs - last_modify_bid_order_time_ < (int64_t)delay_between_modify_usecs_)) {
          if (is_cancellable_before_confirmation_ && use_cancel_on_widen_) {
            if ((t_price_int_diff_ < 0) && (bid_order_->canceled_ == false)) {
              basic_om_->Cancel(*bid_order_);
              order_count_++;
            }
          }
        }
        continue;
      }
      if (t_price_int_diff_ != 0 && bid_order_->canceled_ == false &&
          bid_order_->order_status_ == HFSAT::kORRType_Conf) {
        if ((use_cancel_on_widen_) && (t_price_int_diff_ < 0)) {
          if (!bid_order_->num_open_modified_orders_ || (is_cancellable_before_confirmation_)) {
            basic_om_->Cancel(*bid_order_);
            order_count_++;
          }
        } else {
          bool is_modified_ = basic_om_->Modify(bid_order_, int_price_to_place_ * tick_size_, int_price_to_place_,
                                                quoter_bid_size_, ((use_reserve_msg_) && (t_price_int_diff_ < 0)));
          if (is_modified_) {
            last_modify_bid_order_time_ = current_time_usecs;
            order_count_++;
            /*dbglogger_ << watch_.tv() << " "
                            << sec_shortcode_ << " " << identifier_
                            << "SEND MODIFYORDER BUY " << int_price_to_place_*tick_size_
                            << " size: " << quoter_bid_size_ << " intpx: "
                            << int_price_to_place_ << " theo["
                            << theo_values_.theo_bid_price_ << " X " << theo_values_.theo_ask_price_ << "]"
                            << " mkt[" << secondary_smv_->market_update_info().bestbid_price_
                            << " X " << secondary_smv_->market_update_info().bestask_price_ << "]"
                            << DBGLOG_ENDL_FLUSH;*/
          }
        }
      }
    } else {
      if (current_time - last_new_bid_order_time_ < delay_between_orders_) continue;
      bid_order_ = basic_om_->SendTrade(int_price_to_place_ * tick_size_, int_price_to_place_, quoter_bid_size_,
                                        HFSAT::kTradeTypeBuy, 'B', HFSAT::kOrderDay, unique_exec_id_, 1, false);
      if (bid_order_) {
#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
	dbglogger_.LogQueryOrder(order_log_buffer_, sec_shortcode_.c_str(), identifier_.c_str(), watch_.tv(), HFSAT::kTradeTypeBuy, false, false, false, false
						int_price_to_place_ * tick_size_, quoter_bid_size_, theo_values_.theo_bid_price_, theo_values_.theo_ask_price_,
						secondary_smv_->market_update_info().bestbid_price_,
						secondary_smv_->market_update_info().bestask_price_, -1, -1, reference_primary_bid_,
						reference_primary_ask_, secondary_smv_->market_update_info().bestask_size_,
						bid_order_->client_assigned_order_sequence_, unique_exec_id_, -1, quoter_bid_size_);
#endif
        last_new_bid_order_time_ = current_time;
        order_count_++;
      }
      (*it).first = bid_order_;
    }
  }
  previous_bid_price_ = previous_bid_int_price_ * tick_size_;
}

void MultiQuoter::UpdateAskOrders() {
  if (!theo_values_.is_valid_ || (!ask_status_) || (status_mask_ != BIT_SET_ALL)) {
    CancelAsk();
    return;
  }

  if (quoter_bid_price_ <= 0 || quoter_ask_price_ <= 0) return;

  // First Order of day
  if (previous_ask_int_price_ == -1) {
    // Price of last order
    ask_order_price_pair_vec_.back().second = quoter_ask_int_price_;
  }

  // Sending Top invalid orders to the back of the queue
  int num_null_ordes_from_top_ = 0;
  HFSAT::BaseOrder* top_ask_order_ = ask_order_price_pair_vec_.front().first;
  previous_ask_int_price_ = ask_order_price_pair_vec_.front().second;
  int price_to_place_ = ask_order_price_pair_vec_.back().second + step_offset_;
  if ((!(top_ask_order_) || (top_ask_order_->order_status_ == HFSAT::kORRType_Exec ||
                             top_ask_order_->order_status_ == HFSAT::kORRType_Rejc ||
                             top_ask_order_->order_status_ == HFSAT::kORRType_Cxld))) {
    top_ask_order_ = NULL;
  }
  while (top_ask_order_ == NULL) {
    top_ask_order_ = NULL;
    num_null_ordes_from_top_++;

    // This is the case when all orders are null, then dont send final order back
    // just break from the loop
    if (num_null_ordes_from_top_ == num_levels_to_place_) {
      break;
    }

    ask_order_price_pair_vec_.pop_front();
    ask_order_price_pair_vec_.push_back(std::make_pair(top_ask_order_, price_to_place_));
    top_ask_order_ = ask_order_price_pair_vec_.front().first;
    previous_ask_int_price_ = ask_order_price_pair_vec_.front().second;
    price_to_place_ += step_offset_;
    if ((!(top_ask_order_) || (top_ask_order_->order_status_ == HFSAT::kORRType_Exec ||
                               top_ask_order_->order_status_ == HFSAT::kORRType_Rejc ||
                               top_ask_order_->order_status_ == HFSAT::kORRType_Cxld))) {
      top_ask_order_ = NULL;
    }
  }

  // Checking for widen scenario
  int price_int_diff_ = previous_ask_int_price_ - quoter_ask_int_price_;

  if (((price_int_diff_ != 0) && (-1.0 * price_int_diff_ >= ask_widen_update_int_offset_)) ||  // Widen scenario
      (int(theo_values_.theo_ask_price_ * inverse_tick_size_) > previous_ask_int_price_)) {    // Theo Violation
    std::pair<HFSAT::BaseOrder*, int> current_ask_order_price_pair_ = ask_order_price_pair_vec_.front();
    int current_order_price_ = current_ask_order_price_pair_.second;
    HFSAT::BaseOrder* ask_order_ = current_ask_order_price_pair_.first;
    int price_to_place_ = ask_order_price_pair_vec_.back().second + step_offset_;

    if (ask_order_price_pair_vec_.back().second < quoter_ask_int_price_) {
      price_to_place_ = quoter_ask_int_price_;
    }

    while (current_order_price_ < quoter_ask_int_price_) {
      ask_order_price_pair_vec_.pop_front();
      ask_order_price_pair_vec_.push_back(std::make_pair(ask_order_, price_to_place_));
      current_ask_order_price_pair_ = ask_order_price_pair_vec_.front();
      current_order_price_ = current_ask_order_price_pair_.second;
      ask_order_ = current_ask_order_price_pair_.first;
      price_to_place_ += step_offset_;
    }
    previous_ask_int_price_ = current_order_price_;
  }

  price_int_diff_ = previous_ask_int_price_ - quoter_ask_int_price_;

  // Checking for tighten scenario ; One thing to note here is that
  // both widen and tighten can exist simultaneously
  if ((price_int_diff_ != 0) && (price_int_diff_ >= ask_tighten_update_int_offset_) &&
      (price_int_diff_ >= step_offset_)) {  // Tighten scenario
    int num_orders_ = std::min((int)(price_int_diff_ / step_offset_), num_levels_to_place_);
    while (num_orders_ > 0) {
      HFSAT::BaseOrder* ask_order_ = ask_order_price_pair_vec_.back().first;
      ask_order_price_pair_vec_.pop_back();
      ask_order_price_pair_vec_.push_front(
          std::make_pair(ask_order_, quoter_ask_int_price_ + step_offset_ * (num_orders_ - 1)));
      num_orders_--;
    }
    previous_ask_int_price_ = quoter_ask_int_price_;
  }

  int64_t current_time_usecs = watch_.usecs_from_midnight();
  for (auto it = ask_order_price_pair_vec_.begin(); it != ask_order_price_pair_vec_.end(); it++) {
    HFSAT::BaseOrder* ask_order_ = (*it).first;
    int int_price_to_place_ = (*it).second;
    if (!(ask_order_) ||
        (ask_order_->order_status_ == HFSAT::kORRType_Exec || ask_order_->order_status_ == HFSAT::kORRType_Rejc ||
         ask_order_->order_status_ == HFSAT::kORRType_Cxld)) {
      ask_order_ = NULL;
    }

    int current_time = watch_.msecs_from_midnight();

    if ((int_price_to_place_ > secondary_smv_->upper_int_price_limit_) ||
        (int_price_to_place_ < secondary_smv_->lower_int_price_limit_)) {
      if ((ask_order_) && (ask_order_->canceled_ == false) && (ask_order_->modified_ == false)) {
        basic_om_->Cancel(*ask_order_);
        order_count_++;
      }
      continue;
    }

    if (ask_order_) {
      // either the order is partial exec; order still exists

      int t_price_int_diff_ = ask_order_->int_price_ - int_price_to_place_;
      if (ask_order_->num_open_modified_orders_) {
        // In case of Modify reject, num_open_modified_orders_
        // can be non zero but modified_new_int_price_ is not known
        if (ask_order_->modified_) {
          t_price_int_diff_ = ask_order_->modified_new_int_price_ - int_price_to_place_;
        }
        if ((!is_modify_before_confirmation_) ||
            (current_time_usecs - last_modify_ask_order_time_ < (int64_t)delay_between_modify_usecs_)) {
          if (is_cancellable_before_confirmation_ && use_cancel_on_widen_) {
            if ((t_price_int_diff_ < 0) && (ask_order_->canceled_ == false)) {
              basic_om_->Cancel(*ask_order_);
              order_count_++;
            }
          }
          continue;
        }
      }

      if (t_price_int_diff_ != 0 && ask_order_->canceled_ == false &&
          ask_order_->order_status_ == HFSAT::kORRType_Conf) {
        if ((use_cancel_on_widen_) && (t_price_int_diff_ < 0)) {
          if (!ask_order_->num_open_modified_orders_ || (is_cancellable_before_confirmation_)) {
            basic_om_->Cancel(*ask_order_);
            order_count_++;
          }
        } else {
          bool is_modified_ = basic_om_->Modify(ask_order_, int_price_to_place_ * tick_size_, int_price_to_place_,
                                                quoter_ask_size_, ((use_reserve_msg_) && (t_price_int_diff_ < 0)));
          if (is_modified_) {
            last_modify_ask_order_time_ = current_time_usecs;
            order_count_++;
            /*dbglogger_ << watch_.tv() << " "
                            << sec_shortcode_ << " " << identifier_
                            << "SEND MODIFYORDER SELL " << int_price_to_place_*tick_size_
                            << " size: " << quoter_ask_size_ << " intpx: "
                            << int_price_to_place_ << " theo["
                            << theo_values_.theo_bid_price_ << " X " << theo_values_.theo_ask_price_ << "]"
                            << " mkt[" << secondary_smv_->market_update_info().bestbid_price_
                            << " X " << secondary_smv_->market_update_info().bestask_price_ << "]"
                            << DBGLOG_ENDL_FLUSH;*/
          }
        }
      }
    } else {
      if (current_time - last_new_ask_order_time_ < delay_between_orders_) continue;
      ask_order_ = basic_om_->SendTrade(int_price_to_place_ * tick_size_, int_price_to_place_, quoter_ask_size_,
                                        HFSAT::kTradeTypeSell, 'B', HFSAT::kOrderDay, unique_exec_id_, 1, false);
      if (ask_order_) {
#ifdef _DBGLOGGER_TRADE_ENGINE_INFO_
	dbglogger_.LogQueryOrder(order_log_buffer_, sec_shortcode_.c_str(), identifier_.c_str(), watch_.tv(), HFSAT::kTradeTypeSell, false, false, false, false
						int_price_to_place_ * tick_size_, quoter_ask_size_, theo_values_.theo_bid_price_, theo_values_.theo_ask_price_,
						secondary_smv_->market_update_info().bestbid_price_,
						secondary_smv_->market_update_info().bestask_price_, -1, -1, reference_primary_bid_,
						reference_primary_ask_, secondary_smv_->market_update_info().bestbid_size_,
						ask_order_->client_assigned_order_sequence_, unique_exec_id_, -1, int_price_to_place_);
#endif
        last_new_ask_order_time_ = current_time;
        order_count_++;
      }
      (*it).first = ask_order_;
    }
  }
  previous_ask_price_ = previous_ask_int_price_ * tick_size_;
}

// sendTrade some accounting functions can be moved below basetrade->sendTrade
// unsequenced_bids_ is a vector!! on every sequence it traverses the vector!
// fast_price_convertor not so fast? division instead of multiplication
