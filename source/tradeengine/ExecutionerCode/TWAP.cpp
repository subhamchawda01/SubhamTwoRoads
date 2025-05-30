#include "tradeengine/Executioner/TWAP.hpp"

TWAP::TWAP(HFSAT::Watch& _watch_, HFSAT::DebugLogger& dbglogger_, HFSAT::SecurityMarketView* _secondary_smv_,
           HFSAT::BasicOrderManager* _basic_om_, int _time_to_exec_in_secs_, int _start_time_mfm_, int _size_to_exec_,
           HFSAT::TradeType_t _buysell_, int _notional_to_place_)
    : shc_(_secondary_smv_->shortcode()),
      is_ready_(false),
      watch_(_watch_),
      dbglogger_(dbglogger_),
      secondary_smv_(_secondary_smv_),
      basic_om_(_basic_om_),
      time_to_exec_in_secs_(_time_to_exec_in_secs_),
      start_time_mfm_(_start_time_mfm_),
      window_width_(0),
      abs_pos_to_exec_(_size_to_exec_),
      buysell_(_buysell_),
      notional_to_place_(_notional_to_place_),
      position_(0),
      hedge_pos_exec_(NULL),
      hedge_multiplier_(-1),
      order_(NULL),
      current_index_(0),
      total_traded_value_(0),
      unique_exec_id_(0) {
  // 100 msec time period listener
  watch_.subscribe_TimePeriod(this);
  end_time_mfm_ = start_time_mfm_ + 1000 * time_to_exec_in_secs_;
}

void TWAP::InitializeVariables() {
  double price_ = (secondary_smv_->market_update_info_.bestbid_price_ != kInvalidPrice)
                      ? secondary_smv_->market_update_info().bestbid_price_
                      : secondary_smv_->market_update_info().bestask_price_;
  double quantity_to_place_ = notional_to_place_ / price_;
  order_size_ = HFSAT::MathUtils::GetFlooredMultipleOf(quantity_to_place_, secondary_smv_->min_order_size());
  order_size_ = std::max(order_size_, secondary_smv_->min_order_size());

  // If pos is not a multiple of min order size then make it the nearest one
  if ((abs_pos_to_exec_ % secondary_smv_->min_order_size()) != 0) {
    DBGLOG_TIME_CLASS_FUNC << "Shortcode: " << shc_ << "POS_TO_EXECUTE: " << abs_pos_to_exec_
                           << " is not a multiple of min order size: " << secondary_smv_->min_order_size()
                           << DBGLOG_ENDL_FLUSH;
    abs_pos_to_exec_ = HFSAT::MathUtils::GetFlooredMultipleOf(abs_pos_to_exec_, secondary_smv_->min_order_size());
  }

  num_buckets_ = std::abs(abs_pos_to_exec_ / order_size_);
  pos_vec_.resize(num_buckets_, order_size_);
  int size_remaning_ = (abs_pos_to_exec_ % order_size_);
  if (size_remaning_ != 0) {
    int size_remaning_to_execute =
        HFSAT::MathUtils::GetFlooredMultipleOf(size_remaning_, secondary_smv_->min_order_size());
    if (size_remaning_to_execute != 0) {
      num_buckets_++;
      pos_vec_.push_back(abs_pos_to_exec_ % order_size_);
    }
  }

  start_time_mfm_vec_.resize(num_buckets_, 0);
  end_time_mfm_vec_.resize(num_buckets_, 0);

  if (watch_.msecs_from_midnight() > start_time_mfm_) {
    start_time_mfm_ = watch_.msecs_from_midnight();
  }
  window_width_ = 0;
  if (num_buckets_ > 0) {
    window_width_ = (end_time_mfm_ - start_time_mfm_) / num_buckets_;
  }

  int prev_window_stop_mfm_ = start_time_mfm_;
  for (int i = 0; i < num_buckets_; i++) {
    start_time_mfm_vec_[i] = prev_window_stop_mfm_;
    end_time_mfm_vec_[i] = start_time_mfm_vec_[i] + window_width_;
    prev_window_stop_mfm_ = end_time_mfm_vec_[i];
  }
}

void TWAP::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if (!is_ready_) {
    if ((secondary_smv_->market_update_info_.bestbid_price_ != kInvalidPrice) ||
        (secondary_smv_->market_update_info_.bestask_price_ != kInvalidPrice)) {
      is_ready_ = true;
      InitializeVariables();
    }
  } else {
    // All orders executed
    if (current_index_ >= num_buckets_) return;

    if (watch_.msecs_from_midnight() > end_time_mfm_vec_[current_index_]) {
      if (((buysell_ == HFSAT::TradeType_t::kTradeTypeSell) &&
           (secondary_smv_->market_update_info_.bestbid_price_ == kInvalidPrice)) ||
          ((buysell_ == HFSAT::TradeType_t::kTradeTypeBuy) &&
           (secondary_smv_->market_update_info_.bestask_price_ == kInvalidPrice))) {
        PlacePassiveOrder();
      } else {
        if (order_ && order_->order_status_ != HFSAT::kORRType_Cxld) {
          basic_om_->Cancel(*order_);
        }
        PlaceAggressiveOrder();
      }
    } else if (watch_.msecs_from_midnight() > start_time_mfm_vec_[current_index_]) {
      PlacePassiveOrder();
    } else {
      return;
    }
  }
}

void TWAP::CheckOrderStatus(HFSAT::BaseOrder* _order_) {
  if (order_ && _order_ && order_->client_assigned_order_sequence_ == _order_->client_assigned_order_sequence_) {
    if ((_order_->order_status_ == HFSAT::kORRType_Exec || _order_->order_status_ == HFSAT::kORRType_Rejc ||
         _order_->order_status_ == HFSAT::kORRType_Cxld)) {
      order_ = NULL;
    }
  }
}

void TWAP::OnOrderReject(HFSAT::BaseOrder* _order_) {
  // DBGLOG_TIME_CLASS_FUNC << " TWAP: OrderReject callback for " << secondary_smv_->shortcode() << DBGLOG_ENDL_FLUSH;
  CheckOrderStatus(_order_);
  //	dbglogger_.DumpCurrentBuffer();
}

void TWAP::OnOrderChange(HFSAT::BaseOrder* _order_) {
  CheckOrderStatus(_order_);
  // dbglogger_.DumpCurrentBuffer();
}

void TWAP::PlaceAggressiveOrder() {
  if (order_ && (order_->order_status_ == HFSAT::kORRType_Exec || order_->order_status_ == HFSAT::kORRType_Rejc ||
                 order_->order_status_ == HFSAT::kORRType_Cxld)) {
    order_ = NULL;
  }

  if ((!order_) && (pos_vec_[current_index_] > 0)) {
    if (buysell_ == HFSAT::TradeType_t::kTradeTypeSell) {
      if (secondary_smv_->market_update_info_.bestbid_price_ != kInvalidPrice) {
        order_ =
            basic_om_->SendTrade(secondary_smv_->bestbid_price(), secondary_smv_->bestbid_int_price(),
                                 pos_vec_[current_index_], buysell_, 'B', HFSAT::kOrderIOC, unique_exec_id_, 1, false);
      }
    } else {
      if (secondary_smv_->market_update_info_.bestask_price_ != kInvalidPrice) {
        order_ =
            basic_om_->SendTrade(secondary_smv_->bestask_price(), secondary_smv_->bestask_int_price(),
                                 pos_vec_[current_index_], buysell_, 'B', HFSAT::kOrderIOC, unique_exec_id_, 1, false);
      }
    }

    if (order_) {
      dbglogger_ << watch_.tv() << " " << shc_ << " "
                 << "SEND IOC ORDER " << ((buysell_ == HFSAT::TradeType_t::kTradeTypeBuy) ? "BUY " : "SELL ")
                 << " size: " << pos_vec_[current_index_]
                 << " px: " << ((buysell_ == HFSAT::TradeType_t::kTradeTypeSell) ? secondary_smv_->bestbid_price()
                                                                                 : secondary_smv_->bestask_price())
                 << " mkt[" << secondary_smv_->market_update_info().bestbid_price_ << " X "
                 << secondary_smv_->market_update_info().bestask_price_ << "]" << DBGLOG_ENDL_FLUSH;
    }
  }
}

void TWAP::PlacePassiveOrder() {
  if (order_ && (order_->order_status_ == HFSAT::kORRType_Exec || order_->order_status_ == HFSAT::kORRType_Rejc ||
                 order_->order_status_ == HFSAT::kORRType_Cxld)) {
    order_ = NULL;
  }

  if (pos_vec_[current_index_] == 0) return;

  if (order_) {
    // either the order is partial exec; order still exists
    if (order_->modified_ == false && order_->canceled_ == false && order_->order_status_ == HFSAT::kORRType_Conf) {
      if (buysell_ == HFSAT::TradeType_t::kTradeTypeBuy) {
        if ((secondary_smv_->market_update_info_.bestbid_price_ != kInvalidPrice) &&
            (order_->int_price_ < secondary_smv_->bestbid_int_price())) {
          basic_om_->Modify(order_, secondary_smv_->bestbid_price(), secondary_smv_->bestbid_int_price(),
                            pos_vec_[current_index_]);
          dbglogger_ << watch_.tv() << " " << shc_ << " "
                     << "SEND MODIFYORDER BUY "
                     << " size: " << pos_vec_[current_index_]
                     << " px: " << ((buysell_ == HFSAT::TradeType_t::kTradeTypeBuy) ? secondary_smv_->bestbid_price()
                                                                                    : secondary_smv_->bestask_price())
                     << " mkt[" << secondary_smv_->market_update_info().bestbid_price_ << " X "
                     << secondary_smv_->market_update_info().bestask_price_ << "]" << DBGLOG_ENDL_FLUSH;
        }
      } else {
        if ((secondary_smv_->market_update_info_.bestask_price_ != kInvalidPrice) &&
            (order_->int_price_ > secondary_smv_->bestask_int_price())) {  // kTradeTypeSell
          basic_om_->Modify(order_, secondary_smv_->bestask_price(), secondary_smv_->bestask_int_price(),
                            pos_vec_[current_index_]);
          dbglogger_ << watch_.tv() << " " << shc_ << " "
                     << "SEND MODIFYORDER SELL "
                     << " size: " << pos_vec_[current_index_]
                     << " px: " << ((buysell_ == HFSAT::TradeType_t::kTradeTypeBuy) ? secondary_smv_->bestbid_price()
                                                                                    : secondary_smv_->bestask_price())
                     << " mkt[" << secondary_smv_->market_update_info().bestbid_price_ << " X "
                     << secondary_smv_->market_update_info().bestask_price_ << "]" << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  } else {
    if (buysell_ == HFSAT::TradeType_t::kTradeTypeBuy) {
      if (secondary_smv_->market_update_info_.bestbid_price_ != kInvalidPrice) {
        order_ =
            basic_om_->SendTrade(secondary_smv_->bestbid_price(), secondary_smv_->bestbid_int_price(),
                                 pos_vec_[current_index_], buysell_, 'B', HFSAT::kOrderDay, unique_exec_id_, 1, false);
      }
    } else {
      if (secondary_smv_->market_update_info_.bestask_price_ != kInvalidPrice) {
        order_ =
            basic_om_->SendTrade(secondary_smv_->bestask_price(), secondary_smv_->bestask_int_price(),
                                 pos_vec_[current_index_], buysell_, 'B', HFSAT::kOrderDay, unique_exec_id_, 1, false);
      }
    }
    if (order_) {
      dbglogger_ << watch_.tv() << " " << shc_ << " "
                 << "SEND NEWORDER " << ((buysell_ == HFSAT::TradeType_t::kTradeTypeBuy) ? "BUY " : "SELL ")
                 << " size: " << pos_vec_[current_index_]
                 << " px: " << ((buysell_ == HFSAT::TradeType_t::kTradeTypeBuy) ? secondary_smv_->bestbid_price()
                                                                                : secondary_smv_->bestask_price())
                 << " mkt[" << secondary_smv_->market_update_info().bestbid_price_ << " X "
                 << secondary_smv_->market_update_info().bestask_price_ << "]" << DBGLOG_ENDL_FLUSH;
    }
  }
}

void TWAP::OnExec(const int _new_position_, const int _exec_quantity_, const HFSAT::TradeType_t _buysell_,
                  const double _price_, const int r_int_price_, const int _security_id_, const int _caos_) {
  pos_vec_[current_index_] -= _exec_quantity_;
  if (pos_vec_[current_index_] == 0) {
    current_index_++;
  }

  if (current_index_ <= num_buckets_ - 1 && pos_vec_[current_index_] < 0) {
    dbglogger_ << watch_.tv() << " " << shc_ << " WINDOW TARGET OVERSHOT curr_window: " << pos_vec_[current_index_]
               << DBGLOG_ENDL_FLUSH;
    if (current_index_ < num_buckets_ - 1 && pos_vec_[current_index_ + 1] + pos_vec_[current_index_] >= 0) {
      pos_vec_[current_index_ + 1] += pos_vec_[current_index_];
      pos_vec_[current_index_] = 0;
      current_index_++;
      if (pos_vec_[current_index_] == 0) {
        current_index_++;
      }
    } else {
      if (current_index_ < num_buckets_ - 1) {
        pos_vec_[current_index_ + 1] += pos_vec_[current_index_];
        pos_vec_[current_index_] = 0;
        current_index_++;
      }
      dbglogger_ << watch_.tv() << " " << shc_
                 << "ERROR: WINDOW TARGET unable to adjust in the next window size_remaning_: "
                 << pos_vec_[current_index_] << DBGLOG_ENDL_FLUSH;
      pos_vec_[current_index_] = 0;
    }
  }

  total_traded_value_ += _exec_quantity_ * _price_;
  position_ = _new_position_;

  if (hedge_pos_exec_ != NULL) {
    hedge_pos_exec_->AddBucketToExecute(position_ * hedge_multiplier_ - hedge_pos_exec_->GetPosToExec(), window_width_);
  }

  if (current_index_ == num_buckets_) {
    if (std::abs(_new_position_) != abs_pos_to_exec_) {
      dbglogger_ << watch_.tv() << " "
                 << "ERROR: " << shc_ << " Total size executed is different than given size to execute "
                 << DBGLOG_ENDL_FLUSH;
    }
  } else {
    if (std::abs(_new_position_) < order_size_ * current_index_) {
      dbglogger_ << watch_.tv() << " "
                 << "ERROR: " << shc_ << " Total size executed is less than sum of previous windows' order sizes"
                 << DBGLOG_ENDL_FLUSH;
    }
  }
}

void TWAP::AddBucketToExecute(int _pos_to_exec_, int _bucket_time_width_msecs_) {
  if (_pos_to_exec_ == 0) return;
  int size_to_exec_ = std::abs(_pos_to_exec_);
  abs_pos_to_exec_ += size_to_exec_;
  int num_buckets_to_add_ = size_to_exec_ / order_size_;

  int window_width_ = 0;
  if (num_buckets_to_add_ > 0) {
    window_width_ = _bucket_time_width_msecs_ / num_buckets_to_add_;
  }

  int prev_window_stop_mfm_ = watch_.msecs_from_midnight();
  for (int i = 0; i < num_buckets_to_add_; i++) {
    pos_vec_.push_back(order_size_);
    start_time_mfm_vec_.push_back(prev_window_stop_mfm_);
    if (end_time_mfm_ < (prev_window_stop_mfm_ + window_width_)) {
      end_time_mfm_vec_.push_back(end_time_mfm_);
    } else {
      end_time_mfm_vec_.push_back(prev_window_stop_mfm_ + window_width_);
    }
    prev_window_stop_mfm_ = end_time_mfm_vec_[i];
  }

  int size_remaning_ = (size_to_exec_ % order_size_);
  if (size_remaning_ != 0) {
    int size_remaning_to_execute =
        HFSAT::MathUtils::GetFlooredMultipleOf(size_remaning_, secondary_smv_->min_order_size());
    if (size_remaning_to_execute != 0) {
      num_buckets_to_add_++;
      pos_vec_.push_back(size_to_exec_ % order_size_);
      start_time_mfm_vec_.push_back(prev_window_stop_mfm_);
      if (end_time_mfm_ < (prev_window_stop_mfm_ + window_width_)) {
        end_time_mfm_vec_.push_back(end_time_mfm_);
      } else {
        end_time_mfm_vec_.push_back(prev_window_stop_mfm_ + window_width_);
      }
    }
  }

  if (num_buckets_ != 0) {
    for (int i = current_index_; i < num_buckets_; i++) {
      end_time_mfm_vec_[i] = watch_.msecs_from_midnight();
    }
  }
  num_buckets_ += num_buckets_to_add_;
}
