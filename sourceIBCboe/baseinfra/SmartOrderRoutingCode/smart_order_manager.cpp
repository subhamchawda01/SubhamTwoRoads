/**
    \file SmartOrderRoutingCode/smart_order_manager.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "baseinfra/SmartOrderRouting/smart_order_manager.hpp"

namespace HFSAT {

SmartOrderManager::SmartOrderManager(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                     SecurityNameIndexer& _sec_name_indexer_, BaseTrader& _base_trader_,
                                     SecurityMarketView& t_dep_market_view_, int runtime_id_, const bool r_livetrading_,
                                     int _first_client_assigned_order_sequence_)
    : BaseOrderManager(t_dbglogger_, r_watch_, _sec_name_indexer_, _base_trader_, t_dep_market_view_.shortcode(),
                       t_dep_market_view_.security_id(), t_dep_market_view_.secname(),
                       t_dep_market_view_.min_price_increment(), _first_client_assigned_order_sequence_),
      dep_market_view_(t_dep_market_view_),
      livetrading_(r_livetrading_),
      p_base_pnl_(NULL),
      num_whole_trades_(0),
      last_update_was_quote_(true),
      this_update_is_trade_(false) {
  if (queue_sizes_needed_) {
    dep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);  // only L1 queue positions computed
  }
}

void SmartOrderManager::ComputeQueueSizes() {
  queue_sizes_needed_ = true;
  dep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);  // only L1 queue positions computed
}

void SmartOrderManager::UpdateBidQueuePosition(int t_int_price_) {
  if (queue_sizes_needed_) {
    if (t_int_price_ > dep_market_view_.market_update_info_.bestbid_int_price_ || t_int_price_ == kInvalidIntPrice) {
      return;
    }

    int bid_index_ = GetBidIndex(t_int_price_);

    if (bid_index_ >= 0 && bid_index_ < ORDER_MANAGER_INT_PRICE_RANGE && order_vec_top_bid_index_ != -1) {
      std::vector<BaseOrder*>& _thisbidrow_ = bid_order_vec_[bid_index_];

      int new_size_;
      int new_ordercount_;

      if (t_int_price_ == dep_market_view_.market_update_info_.bestbid_int_price_) {
        new_size_ = dep_market_view_.market_update_info_.bestbid_size_;
        new_ordercount_ = dep_market_view_.market_update_info_.bestbid_ordercount_;
      } else {
        new_size_ = dep_market_view_.bid_size_at_int_price(t_int_price_);
        new_ordercount_ = dep_market_view_.bid_order_at_int_price(t_int_price_);
      }

      for (auto i = 0u; i < _thisbidrow_.size(); i++) {
        if (_thisbidrow_[i] == NULL) continue;

        int prev_size_ = _thisbidrow_[i]->queue_size_ahead_ + _thisbidrow_[i]->queue_size_behind_;
        int prev_ordercount_ = _thisbidrow_[i]->queue_orders_ahead_ + _thisbidrow_[i]->queue_orders_behind_;

        if (new_size_ < prev_size_) {
          if (this_update_is_trade_) {
            // If this is the trade update, remove the size difference from qa only
            _thisbidrow_[i]->queue_size_ahead_ -= (prev_size_ - new_size_);

            if (_thisbidrow_[i]->queue_size_ahead_ < 0) {
              _thisbidrow_[i]->queue_size_ahead_ = 0;
            }
          } else {
            _thisbidrow_[i]->queue_size_ahead_ -=
                (int)round(_thisbidrow_[i]->queue_size_ahead_ * (prev_size_ - new_size_) / prev_size_);
          }
        }

        if (new_ordercount_ < prev_ordercount_) {
          if (prev_ordercount_ <= 0) {
            // TODO - debug situation for NSE
            _thisbidrow_[i]->queue_orders_ahead_ = 0;
          } else if (last_update_was_quote_) {
            _thisbidrow_[i]->queue_orders_ahead_ -= (int)round(_thisbidrow_[i]->queue_orders_ahead_ *
                                                               (prev_ordercount_ - new_ordercount_) / prev_ordercount_);
          } else {
            // If the last update was trade, remove the order count difference from qa only
            _thisbidrow_[i]->queue_orders_ahead_ -= (prev_ordercount_ - new_ordercount_);

            if (_thisbidrow_[i]->queue_orders_ahead_ < 0) {
              _thisbidrow_[i]->queue_orders_ahead_ = 0;
            }
          }
        }

        if (_thisbidrow_[i]->queue_size_ahead_ > new_size_) {
          _thisbidrow_[i]->queue_size_ahead_ = new_size_;
        }

        if (_thisbidrow_[i]->queue_orders_ahead_ > new_ordercount_) {
          _thisbidrow_[i]->queue_orders_ahead_ = new_ordercount_;
        }

        _thisbidrow_[i]->queue_size_behind_ = new_size_ - _thisbidrow_[i]->queue_size_ahead_;
        _thisbidrow_[i]->queue_orders_behind_ = new_ordercount_ - _thisbidrow_[i]->queue_orders_ahead_;
        _thisbidrow_[i]->num_events_seen_++;
      }
    }
  }
}

void SmartOrderManager::UpdateAskQueuePosition(int t_int_price_) {
  if (queue_sizes_needed_) {
    if (t_int_price_ < dep_market_view_.market_update_info_.bestask_int_price_ || t_int_price_ == kInvalidIntPrice) {
      return;
    }

    int ask_index_ = GetAskIndex(t_int_price_);

    int new_size_;
    int new_ordercount_;

    if (t_int_price_ == dep_market_view_.market_update_info_.bestask_int_price_) {
      new_size_ = dep_market_view_.market_update_info_.bestask_size_;
      new_ordercount_ = dep_market_view_.market_update_info_.bestask_ordercount_;
    } else {
      new_size_ = dep_market_view_.ask_size_at_int_price(t_int_price_);
      new_ordercount_ = dep_market_view_.ask_order_at_int_price(t_int_price_);
    }

    if (ask_index_ >= 0 && ask_index_ < ORDER_MANAGER_INT_PRICE_RANGE && order_vec_top_bid_index_ != -1) {
      std::vector<BaseOrder*>& _thisaskrow_ = ask_order_vec_[ask_index_];
      for (auto i = 0u; i < _thisaskrow_.size(); i++) {
        if (_thisaskrow_[i] == NULL) continue;

        int prev_size_ = _thisaskrow_[i]->queue_size_ahead_ + _thisaskrow_[i]->queue_size_behind_;
        int prev_ordercount_ = _thisaskrow_[i]->queue_orders_ahead_ + _thisaskrow_[i]->queue_orders_behind_;

        if (new_size_ < prev_size_) {
          if (this_update_is_trade_) {
            // If this is the trade update, remove the size difference from qa only
            _thisaskrow_[i]->queue_size_ahead_ -= (prev_size_ - new_size_);

            if (_thisaskrow_[i]->queue_size_ahead_ < 0) {
              _thisaskrow_[i]->queue_size_ahead_ = 0;
            }
          } else {
            _thisaskrow_[i]->queue_size_ahead_ -=
                (int)round(_thisaskrow_[i]->queue_size_ahead_ * (prev_size_ - new_size_) / prev_size_);
          }
        }

        if (new_ordercount_ < prev_ordercount_) {
          if (prev_ordercount_ <= 0) {
            // TODO - debug
            _thisaskrow_[i]->queue_orders_ahead_ = 0;
          } else if (last_update_was_quote_) {
            _thisaskrow_[i]->queue_orders_ahead_ -= (int)round(_thisaskrow_[i]->queue_orders_ahead_ *
                                                               (prev_ordercount_ - new_ordercount_) / prev_ordercount_);
          } else {
            // If the last update was trade, remove the order count difference from qa only
            _thisaskrow_[i]->queue_orders_ahead_ -= (prev_ordercount_ - new_ordercount_);

            if (_thisaskrow_[i]->queue_orders_ahead_ < 0) {
              _thisaskrow_[i]->queue_orders_ahead_ = 0;
            }
          }
        }

        if (_thisaskrow_[i]->queue_size_ahead_ > new_size_) {
          _thisaskrow_[i]->queue_size_ahead_ = new_size_;
        }

        if (_thisaskrow_[i]->queue_orders_ahead_ > new_ordercount_) {
          _thisaskrow_[i]->queue_orders_ahead_ = new_ordercount_;
        }

        _thisaskrow_[i]->queue_size_behind_ = new_size_ - _thisaskrow_[i]->queue_size_ahead_;
        _thisaskrow_[i]->queue_orders_behind_ = new_ordercount_ - _thisaskrow_[i]->queue_orders_ahead_;
        _thisaskrow_[i]->num_events_seen_++;
      }
    }
  }
}

void SmartOrderManager::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (queue_sizes_needed_) {
    // Update queue pos for first 3 int levels
    UpdateBidQueuePosition(_market_update_info_.bestbid_int_price_);
    UpdateAskQueuePosition(_market_update_info_.bestask_int_price_);

    UpdateBidQueuePosition(_market_update_info_.bestbid_int_price_ - 1);
    UpdateBidQueuePosition(_market_update_info_.bestbid_int_price_ - 2);
    UpdateAskQueuePosition(_market_update_info_.bestask_int_price_ + 1);
    UpdateAskQueuePosition(_market_update_info_.bestask_int_price_ + 2);
  }

  if (!last_update_was_quote_) {
    last_update_was_quote_ = true;
  }
}

void SmartOrderManager::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                     const MarketUpdateInfo& _market_update_info_) {
  if (queue_sizes_needed_) {
    this_update_is_trade_ = true;
    UpdateBidQueuePosition(_market_update_info_.bestbid_int_price_);
    UpdateAskQueuePosition(_market_update_info_.bestask_int_price_);
    this_update_is_trade_ = false;

    UpdateBidQueuePosition(_market_update_info_.bestbid_int_price_ - 1);
    UpdateBidQueuePosition(_market_update_info_.bestbid_int_price_ - 2);
    UpdateAskQueuePosition(_market_update_info_.bestask_int_price_ + 1);
    UpdateAskQueuePosition(_market_update_info_.bestask_int_price_ + 2);
  }

  if (last_update_was_quote_) {
    // num_whole_trades_ ++;
    last_update_was_quote_ = false;
  }
}

void SmartOrderManager::InitialQueueBid(BaseOrder* p_newly_confirmed_bid_order_) {
  p_newly_confirmed_bid_order_->Enqueue(
      dep_market_view_.bid_size_at_int_price(p_newly_confirmed_bid_order_->int_price_),
      dep_market_view_.bid_order_at_int_price(p_newly_confirmed_bid_order_->int_price_));
}

void SmartOrderManager::InitialQueueAsk(BaseOrder* p_newly_confirmed_ask_order_) {
  p_newly_confirmed_ask_order_->Enqueue(
      dep_market_view_.ask_size_at_int_price(p_newly_confirmed_ask_order_->int_price_),
      dep_market_view_.ask_order_at_int_price(p_newly_confirmed_ask_order_->int_price_));
}
}
